#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.auto-release.h>
#include <rococo.strings.h>
#include <D3DCompiler.h>
#include <vector>
#include <rococo.renderer.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include "rococo.dx11.h"

#if _DEBUG
# define SHADER_COMPILE_DEBUG_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION
#else
# define SHADER_COMPILE_DEBUG_FLAGS 0
#endif

using namespace Rococo;
using namespace Rococo::Graphics;

namespace
{
	struct IncludeResolver : ID3DInclude
	{
		IInstallation& installation;
		cstr root;

		char lastError[1024] = "";
		HRESULT hr = S_OK;

		WideFilePath fullPath;

		AutoFree<IExpandingBuffer> scratch = CreateExpandingBuffer(128_kilobytes);

		IncludeResolver(IInstallation& ref_installation) :
			installation(ref_installation) {}

		STDMETHOD(Open)(D3D_INCLUDE_TYPE type,
			cstr pFileName,
			LPCVOID pParentData,
			LPCVOID* ppData,
			UINT* pBytes)
		{
			try
			{
				struct CLOSURE : IEventCallback<const fstring>
				{
					LPCVOID* ppData;
					UINT* pBytes;

					void OnEvent(const fstring& filedata) override
					{
						auto* s = new char[filedata.length];
						memcpy(s, filedata.buffer, filedata.length);
						*ppData = s;
						*pBytes = filedata.length;
					}
				} onLoad;

				onLoad.ppData = ppData;
				onLoad.pBytes = pBytes;

				installation.LoadResource(pFileName, *scratch, 1_megabytes);
			}
			catch (IException& ex)
			{
				char errCodeBuf[1024];
				Rococo::OS::FormatErrorMessage(errCodeBuf, sizeof errCodeBuf, ex.ErrorCode());
				SafeFormat(lastError, "\n%ls [%s]:\n\t 0x%X (%d)\n\t%s\n", fullPath.buf, ex.Message(), ex.ErrorCode(), ex.ErrorCode(), errCodeBuf);
				hr = ex.ErrorCode();
			}
			return S_OK;
		}

		STDMETHOD(Close)(LPCVOID pData)
		{
			delete[] pData;
			return S_OK;
		}
	};

	D3D_SHADER_MACRO Shader_Macros[] = { {"MAXIMUM_JELLYBABIES", "50"}, {NULL,NULL} };

	HRESULT CompileGenericShaderFromString(const fstring& srcCode, cstr target, cstr resourceName, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob, ID3DInclude& includer)
	{
		UINT flags = SHADER_COMPILE_DEBUG_FLAGS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
		UINT fxFlags = 0;
		HRESULT hr = D3DCompile2(
			srcCode.buffer,
			srcCode.length,
			resourceName,
			NULL, /* macros = none. Consider generating macros automatically from C++ definitions */
			&includer,
			"main",
			target,
			flags, fxFlags, 0, NULL, 0, pShaderBlob, pErrorBlob);
		return hr;
	}

	HRESULT CompilePixelShaderFromString(const fstring& srcCode, cstr resourceName, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob, ID3DInclude& includer)
	{
		return CompileGenericShaderFromString(srcCode, "ps_5_0", resourceName, pShaderBlob, pErrorBlob, includer);
	}

	HRESULT CompileVertexShaderFromString(const fstring& srcCode, cstr resourceName, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob, ID3DInclude& includer)
	{
		return CompileGenericShaderFromString(srcCode, "vs_5_0", resourceName, pShaderBlob, pErrorBlob, includer);
	}
}

enum class ShaderType : uint32
{
	NONE,
	VERTEX,
	PIXEL
};

struct ShaderId
{
	uint32 index : 16;
	uint32 unused : 12;
	ShaderType type : 4;
	operator uint32() { return *reinterpret_cast<uint32*>(this); }
};

union U64ShaderId
{
	uint64 u64Value;
	struct
	{
		uint32 zero;
		ShaderId id;
	} uValue;
};

static_assert(sizeof U64ShaderId == sizeof uint64);

struct ShaderItem
{
	char errMsg[1024] = "";
	ID3DBlob* shaderBlob = nullptr;
	ShaderType type = ShaderType::NONE;
	HString resourceName;
	HRESULT hr = E_PENDING;
};

using namespace Rococo::OS;

namespace ANON
{
	class ShaderCache : public IShaderCache, private IThreadJob
	{
	private:
		std::vector<ShaderItem> shaders;
		std::vector<ShaderId> inputQueue;
		std::vector<ShaderId> errorQueue;
		IInstallation& installation;
		AutoFree<ICriticalSection> sync;

		enum { MAX_SHADERS = 65534 };

		AutoFree<IThreadSupervisor> thread;

		volatile bool isRunning = true;

		IncludeResolver includer; // only used on the shader thread

		void LoadShader_OnThread(ShaderId id, cstr resourceName, const wchar_t* filename)
		{
			includer.root = resourceName;

			HRESULT hr;
			char error[2048] = "";

			AutoRelease<ID3DBlob> shaderBlob;
			AutoRelease<ID3DBlob> errorBlob;

			try
			{
				AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
				installation.LoadResource(resourceName, *buffer, 1_megabytes);

				fstring s = to_fstring((cstr) buffer->GetData());

				switch (id.type)
				{
				case ShaderType::PIXEL:
					hr = CompilePixelShaderFromString(s, resourceName, &shaderBlob, &errorBlob, includer);
					break;
				case ShaderType::VERTEX:
					hr = CompileVertexShaderFromString(s, resourceName, &shaderBlob, &errorBlob, includer);
					break;
				default:
					hr = E_NOTIMPL;
				}
			}
			catch (IException& ex)
			{
				hr = ex.ErrorCode();
				SafeFormat(error, "Error loading %ls: %s", filename, ex.Message());
			}

			Lock lock(sync);

			auto& s = shaders[id.index - 1];

			if (s.shaderBlob)
			{
				s.shaderBlob->Release();
			}

			if (*error || includer.hr != S_OK)
			{
				cstr mainErr = *error ? error : "";
				SafeFormat(s.errMsg, "%s%s", includer.lastError, mainErr);
			}

			s.shaderBlob = shaderBlob;
			if (s.shaderBlob) s.shaderBlob->AddRef();
			s.hr = hr;

			if (FAILED(s.hr) || FAILED(includer.hr))
			{
				errorQueue.push_back(id);
			}
		}

		uint32 RunThread(IThreadControl& tc)
		{
			while (isRunning)
			{
				tc.SleepUntilAysncEvent(1000);

				while (isRunning)
				{
					sync->Lock();

					if (inputQueue.empty())
					{
						sync->Unlock();
						break;
					}

					ShaderId nextId = inputQueue.back();
					inputQueue.pop_back();

					cstr shaderName = shaders[nextId.index - 1].resourceName;

					U8FilePath resourceName;
					Format(resourceName, "%s", shaderName);
					WideFilePath shaderPath;
					installation.ConvertPingPathToSysPath(shaderName, shaderPath);

					sync->Unlock();

					LoadShader_OnThread(nextId, resourceName, shaderPath);
				}
			}

			return 0;
		}
	public:
		ShaderCache(IInstallation& ref_installation) : installation(ref_installation), includer(installation)
		{
			thread = Rococo::OS::CreateRococoThread(this, 0);
			sync = thread->CreateCriticalSection();
			thread->Resume();
		}

		virtual ~ShaderCache()
		{
			isRunning = false;
			Rococo::OS::WakeUp(*thread);
			thread = nullptr;

			for (auto& s : shaders)
			{
				if (s.shaderBlob)
				{
					s.shaderBlob->Release();
				}
			}
		}

		void Free() override
		{
			delete this;
		}

		ShaderId AddGenericShader(ShaderType type, const char* resourceName)
		{
			// Assume [resourceName] has been validated at a higher level in the API

			for (uint32 i = 0; i < shaders.size(); i++)
			{
				auto& s = shaders[i];
				if (Eq(s.resourceName, resourceName))
				{
					ShaderId id;
					id.index = i + 1;
					id.type = s.type;
					id.unused = 0;
					return id;
				}
			}

			if (shaders.size() == MAX_SHADERS)
			{
				Throw(0, "MaxShaders reached. It requires improvement in the AddGenericShader algorithm to fix");
			}

			ShaderId id;

			{
				Lock lock(sync);

				shaders.push_back(ShaderItem());
				auto& s = shaders.back();
				s.resourceName = resourceName;
				s.type = type;

				id.index = shaders.size();
				id.type = s.type;
				id.unused = 0;

				inputQueue.push_back(id);
			}

			Rococo::OS::WakeUp(*thread);

			return id;
		}

		ID_PIXEL_SHADER AddPixelShader(const char* resourceName) override
		{
			U64ShaderId u64Id;
			u64Id.uValue.zero = 0;
			u64Id.uValue.id = AddGenericShader(ShaderType::PIXEL, resourceName);
			return ID_PIXEL_SHADER(u64Id.u64Value);
		}

		ID_VERTEX_SHADER AddVertexShader(const char* resourceName) override
		{
			U64ShaderId u64Id;
			u64Id.uValue.zero = 0;
			u64Id.uValue.id = AddGenericShader(ShaderType::VERTEX, resourceName);
			return ID_VERTEX_SHADER(u64Id.u64Value);
		}

		ShaderId ResourceNameToId(const char* resourceName)
		{
			ShaderId id;
			id.index = 0;

			for (uint32 i = 0; i < shaders.size(); ++i)
			{
				const auto& s = shaders[i];

				if (Eq(s.resourceName, resourceName))
				{
					id.type = s.type;
					id.index = i + 1;
					id.unused = 0;
					return id;
				}
			}

			Throw(0, "%s: unknown shader %s", __FUNCTION__, resourceName);
		}

		void ReloadShader(const char* resourceName) override
		{
			ShaderId id = ResourceNameToId(resourceName);

			if (id.index != 0)
			{
				Lock lock(sync);
				inputQueue.push_back(id);
			}
		}

		bool TryGrabAndPopNextError(IShaderViewGrabber& grabber) override
		{
			Lock lock(sync);

			if (!errorQueue.empty())
			{
				ShaderId id = errorQueue.back();
				errorQueue.pop_back();

				uint32 index = id.index - 1;
				auto& s = shaders[index];

				ShaderView sv;
				sv.blob = nullptr;
				sv.blobCapacity = 0;
				sv.errorString = s.errMsg;
				sv.hr = s.hr;
				sv.resourceName = s.resourceName;
				grabber.OnGrab(sv);
				return true;
			}
			else
			{
				return false;
			}
		}

		uint32 InputQueueLength() override
		{
			volatile uint32 len;
			sync->Lock();
			len = (uint32)inputQueue.size();
			sync->Unlock();
			return len;
		}

		void GrabBadId(IShaderViewGrabber& grabber)
		{
			ShaderView sv;
			sv.blob = nullptr;
			sv.blobCapacity = 0;
			sv.errorString = "Bad shader id";
			sv.hr = E_INVALIDARG;
			sv.resourceName = "<bad-id>";
			grabber.OnGrab(sv);
		}

		void GrabShaderObject(uint64 idValue64, IShaderViewGrabber& grabber)
		{
			U64ShaderId u64Id;
			u64Id.u64Value = idValue64;

			if (u64Id.uValue.zero != 0)
			{
				GrabBadId(grabber);
			}
			else
			{
				ShaderId id = u64Id.uValue.id;
				uint32 index = id.index - 1;
				if (index >= shaders.size())
				{
					GrabBadId(grabber);
				}
				else
				{
					const auto& s = shaders[index];
					if (s.type != id.type)
					{
						GrabBadId(grabber);
					}
					else
					{
						Lock lock(sync);

						ShaderView sv;
						sv.blob = s.shaderBlob != nullptr ? s.shaderBlob->GetBufferPointer() : nullptr;
						sv.blobCapacity = s.shaderBlob != nullptr ? s.shaderBlob->GetBufferSize() : 0;
						sv.errorString = s.errMsg[0] != 0 ? s.errMsg : nullptr;
						sv.hr = s.hr;
						sv.resourceName = s.resourceName;
						grabber.OnGrab(sv);
					}
				}
			}
		}

		void GrabShaderObject(ID_PIXEL_SHADER pxId, IShaderViewGrabber& grabber) override
		{
			GrabShaderObject(pxId.value, grabber);
		}

		void GrabShaderObject(ID_VERTEX_SHADER vxId, IShaderViewGrabber& grabber) override
		{
			GrabShaderObject(vxId.value, grabber);
		}

		void GrabShaderObject(const char* resourceName, IShaderViewGrabber& grabber) override
		{
			U64ShaderId u64Id;
			ShaderId id = ResourceNameToId(resourceName);
			u64Id.uValue.id = id;
			u64Id.uValue.zero = 0;
			GrabShaderObject(u64Id.u64Value, grabber);
		}
	};
}

namespace Rococo::Graphics
{
	IShaderCache* CreateShaderCache(IInstallation& installation)
	{
		return new ANON::ShaderCache(installation);
	}
}
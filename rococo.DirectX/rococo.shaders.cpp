#include <rococo.DirectX.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.strings.h>
#include <rococo.auto-release.h>

#include <d3dcompiler.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

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
	IExpandingBuffer* shaderBlob = nullptr;
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
		IShaderCompiler& compiler;
		AutoFree<ICriticalSection> sync;

		enum { MAX_SHADERS = 65534 };

		AutoFree<IThreadSupervisor> thread;

		volatile bool isRunning = true;

		void LoadShader_OnThread(ShaderId id, cstr resourceName)
		{
			IExpandingBuffer* shaderBinary = nullptr;

			HRESULT hr = S_OK;
			char error[4096] = "";

			try
			{	
				AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(0);
				installation.LoadResource(resourceName, *buffer, 1_megabytes);

				fstring srcCode{ (cstr) buffer->GetData(), (int32) buffer->Length() };
				shaderBinary = compiler.Compile(id.type, srcCode, resourceName);
			}
			catch (IException& ex)
			{
				hr = ex.ErrorCode();
				SafeFormat(error, "Error loading %s:\n\t%s", resourceName, ex.Message());
			}

			Lock lock(sync);

			auto& s = shaders[id.index - 1];

			if (s.shaderBlob)
			{
				s.shaderBlob->Free();
			}

			s.hr = hr;
			s.shaderBlob = shaderBinary;

			if (*error || hr != S_OK)
			{
				SafeFormat(s.errMsg, "%s", error);
				if (hr == S_OK) s.hr = E_FAIL;
			}

			if FAILED(s.hr)
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

					U8FilePath resourceName; // This will be valid after the unlock section
					Format(resourceName, "%s", shaders[nextId.index - 1].resourceName.c_str());

					sync->Unlock();

					LoadShader_OnThread(nextId, resourceName);
				}
			}

			return 0;
		}
	public:
		ShaderCache(IShaderCompiler& ref_compiler, IInstallation& ref_installation) : 
			compiler(ref_compiler), installation(ref_installation)
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
					s.shaderBlob->Free();
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

		ShaderId SysNameToId(const wchar_t* sysName)
		{
			ShaderId id;
			id.index = 0;

			for (uint32 i = 0; i < shaders.size(); ++i)
			{
				const auto& s = shaders[i];

				WideFilePath sysPath;
				installation.ConvertPingPathToSysPath(s.resourceName, sysPath);

				if (Eq(sysName, sysPath))
				{
					id.type = s.type;
					id.index = i + 1;
					id.unused = 0;
					return id;
				}
			}

			return ShaderId{ 0 };
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

		bool IsAnIncludeFile(const wchar_t* sysPath)
		{
			if (EndsWith(sysPath, L"mplat.api.hlsl")) return true;
			if (EndsWith(sysPath, L"mplat.types.hlsl")) return true;
			return false;
		}

		void ReloadShader(const wchar_t* sysPath)
		{
			ShaderId id = SysNameToId(sysPath);

			if (id.index != 0)
			{
				Lock lock(sync);
				inputQueue.push_back(id);
			}
			else
			{
				if (IsAnIncludeFile(sysPath))
				{
					Lock lock(sync);
					for (int i = 0; i < shaders.size(); ++i)
					{
						ShaderId id;
						id.type = shaders[i].type;
						id.index = i + 1;
						id.unused = 0;
						inputQueue.push_back(id);
					}
				}
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
				sv.id = id;
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
						sv.blob = s.shaderBlob != nullptr ? s.shaderBlob->GetData() : nullptr;
						sv.blobCapacity = s.shaderBlob != nullptr ? s.shaderBlob->Length() : 0;
						sv.errorString = s.errMsg;
						sv.hr = s.hr;
						sv.resourceName = s.resourceName;
						sv.id = id;
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

		void GrabShaderObject(ShaderId id, IShaderViewGrabber& grabber) override
		{
			U64ShaderId u64Id;
			u64Id.uValue.id = id;
			u64Id.uValue.zero = 0;
			GrabShaderObject(u64Id.u64Value, grabber);
		}
	};
}

namespace Rococo::Graphics
{
	IShaderCache* CreateShaderCache(IShaderCompiler& compiler, IInstallation& installation)
	{
		return new ANON::ShaderCache(compiler, installation);
	}
}
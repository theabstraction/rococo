#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "rococo.dx12.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <rococo.auto-release.h>
#include "rococo.dx12.helpers.inl"
#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing
#include <rococo.strings.h>
#include <D3DCompiler.h>
#include <vector>
#include <rococo.renderer.h>
#include <rococo.os.h>
#include <rococo.io.h>

#if _DEBUG
# define SHADER_COMPILE_DEBUG_FLAGS D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
# define SHADER_COMPILE_DEBUG_FLAGS 0
#endif

using namespace Rococo;
using namespace Rococo::Graphics;

namespace
{
	HRESULT CompilePixelShaderFromSource(const wchar_t* sourceFile, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob)
	{
		UINT compileFlags = SHADER_COMPILE_DEBUG_FLAGS;
		HRESULT hr = D3DCompileFromFile(sourceFile, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pShaderBlob, pErrorBlob);
		return hr;
	}

	HRESULT CompileVertexShaderFromSource(const wchar_t* sourceFile, ID3DBlob** pShaderBlob, ID3DBlob** pErrorBlob)
	{
		UINT compileFlags = SHADER_COMPILE_DEBUG_FLAGS;
		HRESULT hr = D3DCompileFromFile(sourceFile, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, pShaderBlob, pErrorBlob);
		return hr;
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
	ID3DBlob* errorBlob = nullptr;
	ID3DBlob* shaderBlob = nullptr;
	ShaderType type = ShaderType::NONE;
	HString resourceName;
	HRESULT hr = 0;
};

struct ShaderView
{
	cstr resourceName;
	HRESULT hr;
	cstr errorString;
	const void* blob;
	size_t blobCapacity;
	size_t errorLength;
};

ROCOCOAPI IShaderViewGrabber
{
	// N.B the shader thread is locked until OnGrab returns
	// and the contents may change, so copy what you need and do not block within the method
	virtual void OnGrab(const ShaderView & view) = 0;
};

ROCOCOAPI IShaderCache
{
	virtual	ID_PIXEL_SHADER AddPixelShader(const char* resourceName) = 0;
	virtual ID_VERTEX_SHADER AddVertexShader(const char* resourceName) = 0;
	virtual void GrabShaderObject(ID_PIXEL_SHADER pxId, IShaderViewGrabber& grabber) = 0;
	virtual void GrabShaderObject(ID_VERTEX_SHADER vxId, IShaderViewGrabber& grabber) = 0;
	virtual void ReloadShader(const char* resourceName) = 0;
	virtual uint32 InputQueueLength() = 0;
	virtual bool TryGrabAndPopNextError(IShaderViewGrabber& grabber) = 0;
	virtual void Free() = 0;
};

using namespace Rococo::OS;

class ShaderCache: public IShaderCache, private IThreadJob
{
private:
	std::vector<ShaderItem> shaders;
	std::vector<ShaderId> inputQueue;
	std::vector<ShaderId> errorQueue;
	IDX12ResourceResolver& resolver;
	AutoFree<ICriticalSection> sync;

	enum { MAX_SHADERS = 65534 };

	AutoFree<IThreadSupervisor> thread;
	
	volatile bool isRunning = true;

	void LoadShader_OnThread(ShaderId id, const wchar_t* filename)
	{
		struct CLOSURE : IEventCallback<const fstring>
		{
			const wchar_t* filename;
			ShaderId id;
			HRESULT hr = NO_ERROR;
			AutoRelease<ID3DBlob> shaderBlob;
			AutoRelease<ID3DBlob> errorBlob;
			void OnEvent(const fstring& s)
			{
				switch (id.type)
				{
				case ShaderType::PIXEL:
					hr = CompilePixelShaderFromSource(filename, &shaderBlob, &errorBlob);
					break;
				case ShaderType::VERTEX:
					hr = CompileVertexShaderFromSource(filename, &shaderBlob, &errorBlob);
					break;
				default:
					hr = E_NOTIMPL;
				}
			}
		} onLoad;
		onLoad.filename = filename;
		onLoad.id = id;

		try
		{
			resolver.LoadResource_FreeThreaded(filename, onLoad);
		}
		catch (IException& ex)
		{
			onLoad.hr = ex.ErrorCode();
			enum { ERROR_CAPACITY = 1024 };
			HRESULT errHr = D3DCreateBlob(ERROR_CAPACITY, &onLoad.errorBlob);
			if FAILED(errHr)
			{
				// If this happens we are probably f*cked
				Throw(errHr, "Error creating error blob for shader %ls", filename);
			}

			SafeFormat((char*)onLoad.errorBlob->GetBufferPointer(), onLoad.errorBlob->GetBufferSize(), "Error loading %ls: %s", filename, ex.Message());
		}

		Lock lock(sync);

		auto& s = shaders[id.index - 1];

		if (s.errorBlob)
		{
			s.errorBlob->Release();
		}

		if (s.shaderBlob)
		{
			s.shaderBlob->Release();
		}

		s.errorBlob = onLoad.errorBlob;
		if (s.errorBlob) s.errorBlob->AddRef();

		s.shaderBlob = onLoad.shaderBlob;
		if (s.shaderBlob) s.shaderBlob->AddRef();
		s.hr = onLoad.hr;

		if (FAILED(s.hr) || s.errorBlob != nullptr)
		{
			errorQueue.push_back(id);
		}
	}

	uint32 RunThread(IThreadControl& tc)
	{
		while (isRunning)
		{
			SleepEx(1000, TRUE);

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

				WideFilePath shaderPath;
				resolver.ConvertShortnameToPath(shaderName, shaderPath.buf, shaderPath.CAPACITY);

				sync->Unlock();

				LoadShader_OnThread(nextId, shaderPath);
			}
		}

		return 0;
	}
public:
	ShaderCache(IDX12ResourceResolver& ref_resolver): resolver(ref_resolver)
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
			if (s.errorBlob)
			{
				s.errorBlob->Release();
			}

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
		u64Id.uValue.id = AddGenericShader(ShaderType::PIXEL, resourceName);
		return ID_VERTEX_SHADER(u64Id.u64Value);
	}

	void ReloadShader(const char* resourceName) override
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
				break;
			}
		}

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
			sv.errorString = s.errorBlob != nullptr ? (const char*)s.errorBlob->GetBufferPointer() : nullptr;
			sv.errorLength = s.errorBlob != nullptr ? s.errorBlob->GetBufferSize() : 0;
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
		len = inputQueue.size();
		sync->Unlock();
		return len;
	}

	void GrabBadId(IShaderViewGrabber& grabber)
	{
		auto err = "Bad shader id"_fstring;

		ShaderView sv;
		sv.blob = nullptr;
		sv.blobCapacity = 0;
		sv.errorLength = err.length;
		sv.errorString = err.buffer;
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
					sv.errorString = s.errorBlob != nullptr ? (const char*) s.errorBlob->GetBufferPointer() : nullptr;
					sv.errorLength = s.errorBlob != nullptr ? s.errorBlob->GetBufferSize() : 0;
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
};

IShaderCache* CreateShaderCache(IDX12ResourceResolver& resolver)
{
	return new ShaderCache(resolver);
}
#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>

#include <dxgi1_6.h>

using namespace Rococo;
using namespace Rococo::Graphics;

#ifdef _DEBUG
# define FACTORY_DEBUG_FLAGS DXGI_CREATE_FACTORY_DEBUG
#else
# define FACTORY_DEBUG_FLAGS 0
#endif

namespace ANON
{
	class Win32_Adapter_Context : public IWin32AdapterContext
	{
		int32 adapterIndex, outputIndex;

		AutoRelease<IDXGIFactory7> f;
		AutoRelease<IDXGIAdapter4> adapter;
		AutoRelease<IDXGIOutput> output;

		DXGI_ADAPTER_DESC3 adapterDesc;
		DXGI_OUTPUT_DESC outputDesc;

		AdapterContext* ac = nullptr;
	public:
		Win32_Adapter_Context(int32 iAdapterIndex, int32 iOutputIndex)
		{
			VALIDATE_HR(CreateDXGIFactory2(FACTORY_DEBUG_FLAGS, __uuidof(IDXGIFactory7), (void**)&f));

			AutoRelease<IDXGIAdapter1> adapter1;
			if FAILED(f->EnumAdapters1(adapterIndex, &adapter1))
			{
				adapterIndex = 0;
				VALIDATE_HR(f->EnumAdapters1(0, &adapter1));
			}
			
			if FAILED(adapter1->EnumOutputs(outputIndex, &output))
			{
				VALIDATE_HR(adapter1->EnumOutputs(0, &output));
			}

			VALIDATE_HR(adapter1->QueryInterface(&adapter))

			VALIDATE_HR(adapter->GetDesc3(&adapterDesc));
			VALIDATE_HR(output->GetDesc(&outputDesc));

			ac = new AdapterContext{ *f, *adapter, *output };
		}

		~Win32_Adapter_Context()
		{
			delete ac;
		}

		void Free() override
		{
			delete this;
		}

		AdapterContext& AC() override
		{
			return *ac;
		}
	};
}

namespace Rococo::Graphics
{
	void ValidateHR(HRESULT hr, const char* badcall, const char* function, const char* file, int lineNumber)
	{
		if FAILED(hr)
		{
			Throw(hr, "Windows call failed: %s.\n%s in\n %s line %d.", badcall, function, file, lineNumber);
		}
	}

	IWin32AdapterContext* CreateWin32AdapterContext(int32 adapterIndex, int32 outputIndex)
	{
		return new ANON::Win32_Adapter_Context(adapterIndex, outputIndex);
	}
}

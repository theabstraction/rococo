#include <rococo.os.win32.h>
#include <rococo.api.h>
#include <rococo.os.h>

#define ROCOCO_GRAPHICS_API ROCOCO_API_EXPORT

#include <rococo.win32.rendering.h>

#include "dx11helpers.inl"

#include <dxgi.h>
#include <d3d11.h>


#include <rococo.strings.h>

#include "dx11.factory.h"
#include "dx11.renderer.h"

using namespace Rococo::Strings;
using namespace Rococo::Graphics;

namespace ANON
{
	using namespace Rococo;

	class DX11Factory : public IGraphicsWindowFactory, public DX11::IFactoryResources
	{
		FactorySpec spec;
		IO::IInstallation& installation;
		IGraphicsLogger& logger;
		Rococo::Graphics::IShaderOptions& shaderOptions;

		AutoRelease<IDXGIAdapter> adapter1;
		AutoRelease<ID3D11DeviceContext> dc;
		AutoRelease<ID3D11Device> device;
		AutoRelease<IDXGIFactory1> factory;
		AutoRelease<ID3D11Debug> debug;

		AutoFree<Rococo::DX11::IDX11Renderer> renderer;

		~DX11Factory()
		{
			device = nullptr;
			dc = nullptr;
			adapter1 = nullptr;
			factory = nullptr;
			renderer = nullptr;
			debug = nullptr;
			if (atom) UnregisterClassA((cstr)atom, spec.hResourceInstance);
		}

		IRenderer& Renderer()
		{
			return *renderer;
		}

		ATOM atom;
	public:
		DX11Factory(IO::IInstallation& _installation, IGraphicsLogger& _logger, const FactorySpec& _spec, Rococo::Graphics::IShaderOptions& _options) :
			installation(_installation),
			logger(_logger),
			spec(_spec),
			shaderOptions(_options)
		{
			VALIDATEDX11(CreateDXGIFactory1(IID_IDXGIFactory1, (void**)&factory));
			VALIDATEDX11(factory->EnumAdapters(_spec.adapterIndex, &adapter1));

			DXGI_ADAPTER_DESC desc;
			adapter1->GetDesc(&desc);

			D3D_FEATURE_LEVEL featureLevelNeeded[] = { D3D_FEATURE_LEVEL_11_1 };
			D3D_FEATURE_LEVEL featureLevelFound;

			UINT flags;

#ifdef _DEBUG
			if (Rococo::OS::IsDebugging())
			{
				flags = D3D11_CREATE_DEVICE_DEBUG;
			}
			else
			{
				flags = 0;
			}
#else
			flags = 0;
#endif
			flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

			HRESULT hr = D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
				featureLevelNeeded, 1, D3D11_SDK_VERSION, &device, &featureLevelFound, &dc);

			if FAILED(hr)
			{
				Throw(hr, "D3D11CreateDevice failed");
			}

			if (featureLevelFound != D3D_FEATURE_LEVEL_11_1)
			{
				Throw(0, "DX 11.1 is required for this application");
			}

			device->QueryInterface(IID_PPV_ARGS(&debug));

			DX11::Factory rendererFactory { *device, *dc, *factory, (UINT) spec.adapterIndex, *this, installation, logger };

			renderer = CreateDX11Renderer(rendererFactory, _options);

			WNDCLASSEXA classDef = { 0 };
			classDef.cbSize = sizeof(classDef);
			classDef.style = 0;
			classDef.cbWndExtra = 0;
	//		classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
			classDef.hCursor = NULL; // LoadCursor(nullptr, IDC_ARROW);
			classDef.hIcon = spec.largeIcon;
			classDef.hIconSm = spec.smallIcon;
			classDef.hInstance = spec.hResourceInstance;

			char atomName[128];
			SafeFormat(atomName, sizeof(atomName), "DX11FactoryWindow_%llx", this);
			classDef.lpszClassName = atomName;
			classDef.lpszMenuName = NULL;
			classDef.lpfnWndProc = DefWindowProcA;

			atom = RegisterClassExA(&classDef);

			if (atom == 0)
			{
				Throw(GetLastError(), "Error creating DX11Factory atom. Bad hIcon/hInstance maybe?");
			}
		}

		IGraphicsWindow* CreateGraphicsWindow(IWindowEventHandler& eventHandler, const WindowSpec& windowSpec, bool linkedToDX11Controls) override
		{
			DX11::Factory ourfactory{ *device, *dc, *factory, spec.adapterIndex, *this, installation, logger };
			return DX11::CreateDX11GraphicsWindow(eventHandler, ourfactory, *renderer, atom, windowSpec, linkedToDX11Controls);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	using namespace Rococo::DX11;

	ROCOCO_GRAPHICS_API IGraphicsWindowFactory* CreateGraphicsWindowFactory(IO::IInstallation& installation, IGraphicsLogger& logger, const FactorySpec& spec, Rococo::Graphics::IShaderOptions& options)
	{
		if (spec.preparePix)
		{
			DX11::PreparePixDebugger();
		}
		return new ANON::DX11Factory(installation, logger, spec, options);
	}

	bool DX11_TryGetAdapterInfo(int index, AdapterDesc& d)
	{
		AutoRelease<IDXGIFactory> factory;
		VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)&factory));

		AutoRelease<IDXGIAdapter> testAdapter;
		if (factory->EnumAdapters(index, &testAdapter) == DXGI_ERROR_NOT_FOUND)
		{
			return false;
		}

		DXGI_ADAPTER_DESC desc;
		testAdapter->GetDesc(&desc);

		SafeFormat(d.description, sizeof(d.description), "%ls rev %u", desc.Description, desc.Revision);
		d.sysMemoryMB = desc.DedicatedSystemMemory / 1_megabytes;
		d.videoMemoryMB = desc.DedicatedVideoMemory / 1_megabytes;
		d.sharedMemoryMB = desc.SharedSystemMemory / 1_megabytes;

		return true;
	}

	ROCOCO_GRAPHICS_API IGraphicsLogger* CreateStandardOutputLogger()
	{
		struct Logger : public IGraphicsLogger
		{
			void Log(cstr format, ...) override
			{
				char buffer[2048];

				va_list args;
				va_start(args, format);
				SafeVFormat(buffer, sizeof(buffer), format, args);
				OS::PrintDebug("%s", buffer);
			}

			void Free() override
			{
				delete this;
			}

			void OnMessageException(IException& ex, uint32 uMsg)
			{
				OS::PrintDebug("Exception was thrown in message handler for message %u", uMsg);
				char errBuffer[4096];
				OS::BuildExceptionString(errBuffer, sizeof(errBuffer), ex, true);
				OS::PrintDebug("%s", errBuffer);

				Windows::ShowErrorBox(Windows::NoParent(), ex, "DX11 Message Exception");

				PostQuitMessage(ex.ErrorCode());
			}
		};
		return new Logger;
	}
}
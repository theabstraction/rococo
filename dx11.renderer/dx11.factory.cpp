#include <rococo.os.win32.h>
#include <rococo.api.h>
#include <rococo.dx11.renderer.win32.h>

#include "dx11helpers.inl"

#include <dxgi.h>
#include <d3d11.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include "dx11.factory.h"

namespace ANON
{
	using namespace Rococo;

	class DX11Factory : public IDX11Factory, public DX11::IFactoryResources
	{
		FactorySpec spec;
		IInstallation& installation;
		IDX11Logger& logger;

		AutoRelease<IDXGIAdapter> adapter;
		AutoRelease<ID3D11DeviceContext> dc;
		AutoRelease<ID3D11Device> device;
		AutoRelease<IDXGIFactory> factory;
		AutoRelease<ID3D11Debug> debug;

		~DX11Factory()
		{
			device = nullptr;
			dc = nullptr;
			adapter = nullptr;
			factory = nullptr;
			if (debug)
			{
				debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
				debug = nullptr;
			}

			if (atom) UnregisterClassA((cstr)atom, spec.hResourceInstance);
		}

		ATOM atom;
	public:
		DX11Factory(IInstallation& _installation, IDX11Logger& _logger, const FactorySpec& _spec) :
			installation(_installation),
			logger(_logger),
			spec(_spec)
		{
			VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)&factory));
			VALIDATEDX11(factory->EnumAdapters(_spec.adapterIndex, &adapter));

			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

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

			HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
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

			WNDCLASSEXA classDef = { 0 };
			classDef.cbSize = sizeof(classDef);
			classDef.style = 0;
			classDef.cbWndExtra = 0;
			classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
			classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
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

		bool TryGetAdapterInfo(int index, AdapterDesc& d) override
		{
			AutoRelease<IDXGIAdapter> testAdapter;
			if (factory->EnumAdapters(index, &testAdapter) == DXGI_ERROR_NOT_FOUND)
			{
				return false;
			}

			DXGI_ADAPTER_DESC desc;
			testAdapter->GetDesc(&desc);

			SafeFormat(d.description, sizeof(d.description), "%S rev %u", desc.Description, desc.Revision);
			d.sysMemoryMB = desc.DedicatedSystemMemory / 1_megabytes;
			d.videoMemoryMB = desc.DedicatedVideoMemory / 1_megabytes;
			d.sharedMemoryMB = desc.SharedSystemMemory / 1_megabytes;

			return true;
		}

		IDX11GraphicsWindow* CreateDX11Window(const WindowSpec& spec) override
		{
			DX11::Factory ourfactory{ *device, *dc, *factory, *this, installation, logger };
			return DX11::CreateDX11GraphicsWindow(ourfactory, atom, spec);
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

	IDX11Factory* CreateDX11Factory(IInstallation& installation, IDX11Logger& logger, const FactorySpec& spec)
	{
		return new ANON::DX11Factory(installation, logger, spec);
	}

	IDX11Logger* CreateStandardOutputLogger()
	{
		struct Logger : public IDX11Logger
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

				OS::ShowErrorBox(Windows::NoParent(), ex, "DX11 Message Exception");

				PostQuitMessage(ex.ErrorCode());
			}
		};
		return new Logger;
	}
}
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
		}

	public:
		DX11Factory(IInstallation& _installation, IDX11Logger& _logger) :
			installation(_installation),
			logger(_logger)
		{
			cstr cmd = GetCommandLineA();

			auto adapterPrefix = "-adapter.index:"_fstring;
			cstr indexPtr = strstr(cmd, adapterPrefix);

			UINT index = 0;
			if (indexPtr != nullptr)
			{
				index = atoi(indexPtr + adapterPrefix.length);
			}

			VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)&factory));
			VALIDATEDX11(factory->EnumAdapters(index, &adapter));

			if (strstr(cmd, "-adapter.list"))
			{
				logger.Log("Adapter List:\n");
				for (UINT i = 0; i < 100; ++i)
				{
					AutoRelease<IDXGIAdapter> testAdapters;
					if (factory->EnumAdapters(i, &testAdapters) == DXGI_ERROR_NOT_FOUND)
					{
						break;
					}

					DXGI_ADAPTER_DESC desc;
					testAdapters->GetDesc(&desc);

					logger.Log("Adapter #%u: %S rev %u %s\n", i, desc.Description, desc.Revision, (i == index) ? "- selected." : "");
					logger.Log(" Sys Mem: %lluMB\n", desc.DedicatedSystemMemory / 1_megabytes);
					logger.Log(" Vid Mem: %lluMB\n", desc.DedicatedVideoMemory / 1_megabytes);
					logger.Log(" Shared Mem: %lluMB\n", desc.SharedSystemMemory / 1_megabytes);
				}
			}
			else
			{
				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);

				logger.Log("Adapter #%u: %S rev %u - selected.\n", index, desc.Description, desc.Revision);
				logger.Log(" Sys Mem: %lluMB\n", desc.DedicatedSystemMemory / 1_megabytes);
				logger.Log(" Vid Mem: %lluMB\n", desc.DedicatedVideoMemory / 1_megabytes);
				logger.Log(" Shared Mem: %lluMB\n", desc.SharedSystemMemory / 1_megabytes);
				logger.Log(" --> add -adapter.list on command line to see all adapters\n");
			}

			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			D3D_FEATURE_LEVEL featureLevelNeeded[] = { D3D_FEATURE_LEVEL_11_1 };
			D3D_FEATURE_LEVEL featureLevelFound;

			UINT flags;

#ifdef _DEBUG
			flags = D3D11_CREATE_DEVICE_DEBUG;
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
		}

		IDX11Window* CreateDX11Window()
		{
			DX11::Factory Ourfactory{ *device, *dc, *factory, *this, installation };
			return DX11::CreateDX11Window(Ourfactory);
		}

		void Free() override
		{
			delete this;
		}

		void ManageWindow(HWND hWnd) override
		{
			VALIDATEDX11(factory->MakeWindowAssociation(hWnd, 0));
		}
	};
}

namespace Rococo
{
	using namespace Rococo::DX11;

	IDX11Factory* CreateDX11Factory(IInstallation& installation, IDX11Logger& logger)
	{
		return new ANON::DX11Factory(installation, logger);
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
		};
		return new Logger;
	}
}
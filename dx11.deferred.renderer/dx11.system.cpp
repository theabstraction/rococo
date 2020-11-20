#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.DirectX.h>

#include <dxgi1_6.h>
#include <d3d11_4.h>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace Rococo::Graphics
{
	IDX11Window* CreateDX11Window(IDX11System& system, DX11WindowContext& wc);
}

namespace ANON
{
#ifdef _DEBUG
# define D3D11CreateDevice_Flags D3D11_CREATE_DEVICE_DEBUG /*| D3D11_CREATE_DEVICE_DEBUGGABLE */
#else
# define D3D11CreateDevice_Flags 0
#endif

	class DX11System: public IDX11System
	{
		AdapterContext& ac;

		AutoRelease<ID3D11Device5> device5;
		AutoRelease<ID3D11DeviceContext4> dc;

		AutoFree<ID3DShaderCompiler> dxCompiler;
		AutoFree<IShaderCache> shaders;
		AutoFree<ITextureCache> textures;
	public:
		DX11System(AdapterContext& ref_ac, IInstallation& installation): ac(ref_ac),
			dxCompiler(Rococo::Graphics::DirectX::CreateD3DCompiler(installation)),
			shaders(Rococo::Graphics::CreateShaderCache(*dxCompiler, installation))
		{
			D3D_FEATURE_LEVEL levels[1] = { D3D_FEATURE_LEVEL_11_1 };
			D3D_FEATURE_LEVEL level;
			UINT flags = D3D11CreateDevice_Flags;
			UINT SDKVersion = D3D11_SDK_VERSION;

			AutoRelease<ID3D11Device> device;
			AutoRelease<ID3D11DeviceContext> context;
			HRESULT hr = D3D11CreateDevice(&ac.adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, levels, 1, SDKVersion, &device, &level, &context);
			if FAILED(hr)
			{
				Throw(hr, "Cannot initialize DX11.1 on this computer.");
			}

			if (level != D3D_FEATURE_LEVEL_11_1)
			{
				Throw(0, "Cannot initialize DX11.1 on this computer.");
			}

			VALIDATE_HR(device->QueryInterface(&device5));
			VALIDATE_HR(context->QueryInterface(&dc));

			textures = Rococo::Graphics::CreateTextureCache(installation, *device5, *dc);
		}

		ID3D11Device5& Device() override
		{
			return *device5;
		}

		ID3D11DeviceContext4& DC() override
		{
			return *dc;
		}

		IDXGIFactory7& Factory() override
		{
			return ac.f;
		}

		IShaderCache& Shaders() override
		{
			return *shaders;
		}

		ITextureCache& Textures() override
		{
			return *textures;
		}

		IDX11Window* CreateDX11Window(DX11WindowContext& wc)
		{
			return Rococo::Graphics::CreateDX11Window(*this, wc);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation)
	{
		return new ANON::DX11System(ac, installation);
	}
}
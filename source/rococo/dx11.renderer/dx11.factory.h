#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory;

namespace Rococo
{
	struct IDX11Window;

	namespace DX11
	{
		struct IFactoryResources
		{
		};

		struct Factory
		{
			ID3D11Device& device;
			ID3D11DeviceContext& dc;
			IDXGIFactory1& factory;
			const UINT adapterIndex;

			IFactoryResources& resources;
			IO::IInstallation& installation;
			IGraphicsLogger& logger;
		};

		struct IDX11Renderer;

		IGraphicsWindow* CreateDX11GraphicsWindow(Factory& factory, Rococo::DX11::IDX11Renderer& renderer, ATOM windowClass, const WindowSpec& spec, bool linkedToDX11Controls);
		IDX11Renderer* CreateDX11Renderer(Factory& factory, IShaderOptions& options);
	}
}
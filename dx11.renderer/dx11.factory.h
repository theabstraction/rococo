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
			virtual void ManageWindow(HWND hWnd) = 0;
		};

		struct Factory
		{
			ID3D11Device& device;
			ID3D11DeviceContext& dc;
			IDXGIFactory& factory;

			IFactoryResources& resources;
			IInstallation& installation;
		};

		IDX11Window* CreateDX11Window(Factory& factory);
	}
}
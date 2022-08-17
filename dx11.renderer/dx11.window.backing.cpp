#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include "rococo.dx11.api.h"
#include "dx11helpers.inl"

using namespace Rococo::DX11;

static uint32 g_windowCount = 0;

struct DX11WindowBacking: IDX11WindowBacking, Windows::IWindow
{
	HWND hWnd;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IDXGIFactory& factory;
	IDX11TextureManager& textures;
	ID_TEXTURE depthBufferId;
	int windowIndex = 0;

	Vec2i screenSpan{ -1, -1 };

	AutoRelease<IDXGISwapChain> mainSwapChain;
	AutoRelease<ID3D11RenderTargetView> mainBackBufferView;

	bool useVerticalBlank = true;

	DX11WindowBacking(ID3D11Device& _device, ID3D11DeviceContext& _dc, HWND _hWnd, IDXGIFactory& _factory, IDX11TextureManager& _textures):
		hWnd(_hWnd), device(_device), dc(_dc), factory(_factory), textures(_textures)
	{
		if (!IsWindow(hWnd)) Throw(0, "%s: hWnd was not a window", __FUNCTION__);

		windowIndex = ++g_windowCount;
	}

	ID3D11RenderTargetView* BackBufferView() override
	{
		return mainBackBufferView;
	}

	ID_TEXTURE DepthBufferId() const override
	{
		return depthBufferId;
	}

	Vec2i Span() const override
	{
		return screenSpan;
	}

	Windows::IWindow& Window() override
	{
		return *this;
	}

	operator HWND() const override
	{
		return hWnd;
	}

	void Present() override
	{
		if (mainSwapChain)
		{
			UINT syncInterval = useVerticalBlank ? 1 : 0;
			UINT flags = 0;
			mainSwapChain->Present(syncInterval, flags);
		}
	}

	void ResetOutputBuffersForWindow()
	{
		RECT rect;
		GetClientRect(hWnd, &rect);

		Vec2i newSpan;
		newSpan.x = rect.right - rect.left;
		newSpan.y = rect.bottom - rect.top;

		if (newSpan == screenSpan)
		{
			return;
		}

		if (newSpan.x < 1 || newSpan.y < 1)
		{
			return;
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = DX11::GetSwapChainDescription(hWnd);
		VALIDATEDX11(factory.CreateSwapChain((ID3D11Device*)&device, &swapChainDesc, &mainSwapChain));

		AutoRelease<ID3D11Texture2D> backBuffer;
		VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

		mainBackBufferView.Detach();
		VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

		char depthBufferName[40];
		SecureFormat(depthBufferName, "hWnd_%llX_depth_%u", hWnd, windowIndex);

		if (!depthBufferId)
		{
			depthBufferId = textures.CreateDepthTarget(depthBufferName, newSpan.x, newSpan.y);
		}
		else
		{
			TextureBind& t = textures.GetTexture(depthBufferId);
			t.depthView->Release();
			t.shaderView->Release();
			t.texture->Release();
			t = CreateDepthTarget(device, newSpan.x, newSpan.y);
		}

		screenSpan = newSpan;
	}

	void SwitchToWindowMode() override
	{
		BOOL isFullScreen;
		AutoRelease<IDXGIOutput> output;
		if SUCCEEDED(mainSwapChain->GetFullscreenState(&isFullScreen, &output))
		{
			if (isFullScreen)
			{
				mainSwapChain->SetFullscreenState(false, nullptr);
			}
		}
	}

	void Free() override
	{
		delete this;
	}
};

namespace Rococo::DX11
{
	IDX11WindowBacking* CreateDX11WindowBacking(ID3D11Device& device, ID3D11DeviceContext& dc, HWND hWnd, IDXGIFactory& factory, IDX11TextureManager& textures)
	{
		return new DX11WindowBacking(device, dc, hWnd, factory, textures);
	}
}

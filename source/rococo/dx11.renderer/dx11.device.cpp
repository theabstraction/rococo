#include "dx11.renderer.h"
#include "dx11helpers.inl"

namespace Rococo::DX11
{
	void ShowVenueForDevice(IMathsVisitor& visitor, ID3D11Device& device)
	{
		UINT flags = device.GetCreationFlags();

		if (flags & D3D11_CREATE_DEVICE_SINGLETHREADED)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_SINGLETHREADED");
		}

		if (flags & D3D11_CREATE_DEVICE_DEBUG)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DEBUG");
		}

		if (flags & D3D11_CREATE_DEVICE_SWITCH_TO_REF)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_SWITCH_TO_REF");
		}

		if (flags & D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS");
		}

		if (flags & D3D11_CREATE_DEVICE_BGRA_SUPPORT)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_BGRA_SUPPORT");
		}

		if (flags & D3D11_CREATE_DEVICE_DEBUGGABLE)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DEBUGGABLE");
		}

		if (flags & D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY");
		}

		if (flags & D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT");
		}

		if (flags & D3D11_CREATE_DEVICE_VIDEO_SUPPORT)
		{
			visitor.ShowString(" -> device flags | ", "D3D11_CREATE_DEVICE_VIDEO_SUPPORT");
		}
	}

	TextureBind CreateDepthTarget(ID3D11Device& device, int32 width, int32 height)
	{
		ID3D11Texture2D* tex2D = nullptr;
		ID3D11DepthStencilView* depthView = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;

		try
		{
			D3D11_TEXTURE2D_DESC desc;
			desc.ArraySize = 1;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.Format = DXGI_FORMAT_R32_TYPELESS;
			desc.Height = height;
			desc.Width = width;
			desc.MipLevels = 1;
			desc.MiscFlags = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &tex2D));

			D3D11_DEPTH_STENCIL_VIEW_DESC sdesc;
			sdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			sdesc.Texture2D.MipSlice = 0;
			sdesc.Format = DXGI_FORMAT_D32_FLOAT;
			sdesc.Flags = 0;
			VALIDATEDX11(device.CreateDepthStencilView(tex2D, &sdesc, &depthView));

			D3D11_SHADER_RESOURCE_VIEW_DESC rdesc;
			rdesc.Texture2D.MipLevels = 1;
			rdesc.Texture2D.MostDetailedMip = 0;
			rdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			rdesc.Format = DXGI_FORMAT_R32_FLOAT;
			VALIDATEDX11(device.CreateShaderResourceView(tex2D, &rdesc, &srv));
		}
		catch (IException&)
		{
			if (tex2D) tex2D->Release();
			if (depthView) depthView->Release();
			if (srv) srv->Release();
			throw;
		}

		return { tex2D, srv, nullptr, depthView };
	}

	TextureBind CreateRenderTarget(ID3D11Device& device, int32 width, int32 height)
	{
		ID3D11Texture2D* tex2D = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11RenderTargetView* rtv = nullptr;

		try
		{
			D3D11_TEXTURE2D_DESC desc;
			desc.ArraySize = 1;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
			desc.Height = height;
			desc.Width = width;
			desc.MipLevels = 1;
			desc.MiscFlags = 0;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &tex2D));

			D3D11_SHADER_RESOURCE_VIEW_DESC rdesc;
			rdesc.Texture2D.MipLevels = 1;
			rdesc.Texture2D.MostDetailedMip = 0;
			rdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			rdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			VALIDATEDX11(device.CreateShaderResourceView(tex2D, &rdesc, &srv));

			D3D11_RENDER_TARGET_VIEW_DESC rtdesc;
			rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtdesc.Texture2D.MipSlice = 0;
			rtdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			VALIDATEDX11(device.CreateRenderTargetView(tex2D, &rtdesc, &rtv));
		}
		catch (IException&)
		{
			if (tex2D) tex2D->Release();
			if (rtv) rtv->Release();
			if (srv) srv->Release();
			throw;
		}

		return TextureBind{ tex2D, srv, rtv };
	}
}
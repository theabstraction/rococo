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
	IDX11Window* CreateDX11Window(IDX11System& system, DX11WindowContext& wc, ITextureSupervisor& textures);
}

namespace ANON
{
#ifdef _DEBUG
# define D3D11CreateDevice_Flags D3D11_CREATE_DEVICE_DEBUG /*| D3D11_CREATE_DEVICE_DEBUGGABLE */
#else
# define D3D11CreateDevice_Flags 0
#endif

	class DX11System: public IDX11System, public IDX11DeviceContext
	{
		AdapterContext& ac;

		AutoRelease<ID3D11Device5> device5;
		AutoRelease<ID3D11DeviceContext4> dc;

		AutoFree<ID3DShaderCompiler> dxCompiler;
		AutoFree<IShaderCache> shaders;
		AutoFree<ITextureSupervisor> textures;

		CRITICAL_SECTION sync;
	public:
		DX11System(AdapterContext& ref_ac, IInstallation& installation): ac(ref_ac),
			dxCompiler(Rococo::Graphics::DirectX::CreateD3DCompiler(installation)),
			shaders(Rococo::Graphics::CreateShaderCache(*dxCompiler, installation))
		{
			InitializeCriticalSection(&sync);

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
				Throw(0, "Cannot initialize DX11.1 on this computer. Feature level not supported");
			}

			VALIDATE_HR(device->QueryInterface(&device5));
			VALIDATE_HR(context->QueryInterface(&dc));

			textures = Rococo::Graphics::CreateTextureCache(installation, *device5, *this);
		}

		void Lock() override
		{
			EnterCriticalSection(&sync);
		}

		void Unlock() override
		{
			LeaveCriticalSection(&sync);
		}

		virtual ~DX11System()
		{
			DeleteCriticalSection(&sync);
		}

		void GenerateMips(ID3D11ShaderResourceView* pShaderResourceView) override
		{
			Lock();
			dc->GenerateMips(pShaderResourceView);
			Unlock();
		}

		void UpdateSubresource(ID3D11Resource& resource, uint32 subresourceIndex, const D3D11_BOX& box, const void* pixels, uint32 lineSpan, uint32 srcDepth) override
		{
			Lock();
			dc->UpdateSubresource(&resource, subresourceIndex, &box, pixels, lineSpan, srcDepth);
			Unlock();
		}

		void OMSetRenderTargets(uint32 NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
		{
			Lock();
			dc->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
			Unlock();
		}

		void PSSetShaderResources(uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) override
		{
			Lock();
			dc->PSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
			Unlock();
		}

		void VSSetShaderResources(uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) override
		{
			Lock();
			dc->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
			Unlock();
		}

		void ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const RGBA& colour) override
		{
			Lock();
			dc->ClearRenderTargetView(pRenderTargetView, &colour.red);
			Unlock();
		}

		void ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, uint32 ClearFlags, float Depth, uint8 Stencil) override
		{
			Lock();
			dc->ClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
			Unlock();
		}

		void OMSetBlendState(ID3D11BlendState* pBlendState, const RGBA& blendFactor, uint32 sampleMask) override
		{
			Lock();
			dc->OMSetBlendState(pBlendState, &blendFactor.red, sampleMask);
			Unlock();
		}

		void IASetInputLayout(ID3D11InputLayout* pInputLayout) override
		{
			Lock();
			dc->IASetInputLayout(pInputLayout);
			Unlock();
		}

		void OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, uint32 StencilRef) override
		{
			Lock();
			dc->OMSetDepthStencilState(pDepthStencilState, StencilRef);
			Unlock();
		}

		void RSSetScissorRects(uint32 NumRects, const RECT* pRects) override
		{
			Lock();
			dc->RSSetScissorRects(NumRects, pRects);
			Unlock();
		}

		void RSSetState(ID3D11RasterizerState* pRasterizerState) override
		{
			Lock();
			dc->RSSetState(pRasterizerState);
			Unlock();
		}

		void IASetVertexBuffers(uint32 StartSlot, uint32 NumBuffers, ID3D11Buffer* const* ppVertexBuffers, const uint32* pStrides, const uint32* pOffsets) override
		{
			Lock();
			dc->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
			Unlock();
		}

		HRESULT Map(ID3D11Resource* pResource, uint32 Subresource, D3D11_MAP MapType, uint32 MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource) override
		{
			Lock();
			HRESULT hr = dc->Map(pResource, Subresource, MapType, MapFlags, pMappedResource);
			Unlock();
			return hr;
		}

		void Unmap(ID3D11Resource* pResource, uint32 Subresource) override
		{
			Lock();
			dc->Unmap(pResource, Subresource);
			Unlock();
		}

		ID3D11Device5& Device() override
		{
			return *device5;
		}

		IDX11DeviceContext& DC() override
		{
			return *this;
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
			return Rococo::Graphics::CreateDX11Window(*this, wc, *textures);
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
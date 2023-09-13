#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.DirectX.h>

#include <dxgi1_6.h>
#include <d3d11_4.h>

#include <vector>

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
		IInstallation& installation;

		AutoRelease<ID3D11Device5> device5;
		AutoRelease<ID3D11DeviceContext4> dc;

		AutoFree<ID3DShaderCompiler> dxCompiler;
		AutoFree<IShaderCache> shaders;
		AutoFree<ITextureSupervisor> textures;
		AutoFree<IMeshCache> meshes;

		struct DX11Shader
		{
			AutoRelease<ID3D11PixelShader> ps;
			AutoRelease<ID3D11VertexShader> vs;
			ShaderId id;
		};
		std::vector<DX11Shader> dx11Shaders;

		CRITICAL_SECTION sync;
	public:
		DX11System(AdapterContext& ref_ac, IInstallation& ref_installation): 
			ac(ref_ac), installation(ref_installation),
			dxCompiler(Rococo::Graphics::DirectX::CreateD3DCompiler(ref_installation)),
			shaders(Rococo::Graphics::CreateShaderCache(*dxCompiler, ref_installation))
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

			enum { MAX_SHADERS = 128 };
			dx11Shaders.resize(MAX_SHADERS);
		}

		virtual ~DX11System()
		{
			dx11Shaders.clear();
			shaders = nullptr;
			DeleteCriticalSection(&sync);
		}

		// Define things that require a valid V-Table
		void PostConstruct()
		{
			textures = Rococo::Graphics::CreateTextureCache(installation, *device5, *this);
			meshes = CreateMeshCache(*device5, *this);
		}

		void Lock() override
		{
			EnterCriticalSection(&sync);
		}

		void Unlock() override
		{
			LeaveCriticalSection(&sync);
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

		void PSSetSamplers(uint32 startPosition, uint32 nSamplers, ID3D11SamplerState* const * samplers) override
		{
			Lock();
			dc->PSSetSamplers(startPosition, nSamplers, samplers);
			Unlock();
		}

		void VSSetShaderResources(uint32 StartSlot, uint32 NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews) override
		{
			Lock();
			dc->VSSetShaderResources(StartSlot, NumViews, ppShaderResourceViews);
			Unlock();
		}

		void PSSetConstantBuffers(uint32 startSlot, uint32 nBuffer, ID3D11Buffer* const * bufferArray) override
		{
			Lock();
			dc->PSSetConstantBuffers(startSlot, nBuffer, bufferArray);
			Unlock();
		}

		void VSSetConstantBuffers(uint32 startSlot, uint32 nBuffer, ID3D11Buffer* const* bufferArray) override
		{
			Lock();
			dc->VSSetConstantBuffers(startSlot, nBuffer, bufferArray);
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

		IMeshCache& Meshes()
		{
			return *meshes;
		}

		void Draw(uint32 vertexCount, uint32 startLocation) override
		{
			Lock();
			dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			dc->Draw(vertexCount, startLocation);
			Unlock();
		}

		ID3D11VertexShader* GetVS(LayoutId layoutId, ID_VERTEX_SHADER idVS)
		{
			U64ShaderId uVSId;
			uVSId.u64Value = idVS.value;

			auto index = uVSId.uValue.id.index - 1;
			ID3D11VertexShader* vs = nullptr;
			if (index >= dx11Shaders.size())
			{
				return nullptr;
			}
			
			if (!(vs = dx11Shaders[index].vs))
			{
				struct CLOSURE : IShaderViewGrabber
				{
					DX11System* system;
					ID3D11VertexShader* vs = nullptr;
					LayoutId layoutId;
					HRESULT hr = E_PENDING;
					void OnGrab(const ShaderView& view)
					{
						if (view.hr == S_OK && view.blob != nullptr)
						{
							if (!system->Meshes().Layouts().IsValid(layoutId))
							{
								system->Meshes().Layouts().CreateLayout(layoutId, view.blob, view.blobCapacity);
							}

							if (!system->Meshes().Layouts().IsValid(layoutId))
							{
								return;
							}

							// Okay layout worked
							hr = system->Device().CreateVertexShader(view.blob, view.blobCapacity, nullptr, &vs);
						}
					}
				} g;
				g.system = this;
				g.layoutId = layoutId;
				shaders->GrabShaderObject(uVSId.uValue.id, g);

				vs = g.vs;
				dx11Shaders[index].vs = vs;
				dx11Shaders[index].id = uVSId.uValue.id;
			}
			
			return vs;
		}

		ID3D11PixelShader* GetPS(ID_PIXEL_SHADER idPS)
		{
			U64ShaderId uPSId;
			uPSId.u64Value = idPS.value;

			auto index = uPSId.uValue.id.index - 1;
			ID3D11PixelShader* ps = nullptr;
			if (index >= dx11Shaders.size())
			{
				return nullptr;
			}
			
			if (!(ps = dx11Shaders[index].ps))
			{
				struct CLOSURE : IShaderViewGrabber
				{
					DX11System* system;
					ID3D11PixelShader* ps = nullptr;
					HRESULT hr = E_PENDING;

					void OnGrab(const ShaderView& view)
					{
						if (view.hr == S_OK && view.blob != nullptr)
						{
							// Okay layout worked
							hr = system->Device().CreatePixelShader(view.blob, view.blobCapacity, nullptr, &ps);
						}
						else
						{
							hr = view.hr;
						}
					}
				} g;
				g.system = this;
				shaders->GrabShaderObject(uPSId.uValue.id, g);
				ps = g.ps;
				dx11Shaders[index].ps = ps;
				dx11Shaders[index].id = uPSId.uValue.id;
			}

			return ps;
		}

		LayoutId lastLayoutId;

		bool UseShaders(LayoutId layoutId, ID_VERTEX_SHADER idVS, ID_PIXEL_SHADER idPS) override
		{
			auto* ps = GetPS(idPS);
			if (ps)
			{
				auto* vs = GetVS(layoutId, idVS);
				{
					if (layoutId.index != lastLayoutId.index)
					{
						if (!meshes->Layouts().SetInputLayout(layoutId))
						{
							return false;
						}

						lastLayoutId = layoutId;
					}

					Lock();
					dc->PSSetShader(ps, nullptr, 0);
					dc->VSSetShader(vs, nullptr, 0);
					Unlock();
					return true;
				}
			}

			return false;
		}

		void RSSetViewports(uint32 numViewports, const DX11_VIEWPORT* viewportArray) override
		{
			static_assert(sizeof DX11_VIEWPORT == sizeof D3D11_VIEWPORT);
			Lock();
			dc->RSSetViewports(numViewports, (const D3D11_VIEWPORT*)viewportArray);
			Unlock();
		}
	};
}

namespace Rococo::Graphics
{
	IDX11System* CreateDX11System(AdapterContext& ac, IInstallation& installation)
	{
		auto* dx11 = new ANON::DX11System(ac, installation);
		dx11->PostConstruct();
		return dx11;
	}
}
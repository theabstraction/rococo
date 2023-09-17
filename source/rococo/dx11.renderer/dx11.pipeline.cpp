#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include <vector>
#include <RAL/RAL.pipeline.h>

using namespace Rococo::Graphics;
using namespace Rococo::RAL;
using namespace Rococo::DX11;

namespace Rococo::DX11
{
	struct AmbientData
	{
		RGBA localLight;
		float fogConstant = -0.2218f; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
		float a = 0;
		float b = 0;
		float c = 0;
	};

	struct DX11Pipeline : IDX11Pipeline, RAL::IRenderStates
	{
		IO::IInstallation& installation;
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		IDX11TextureManager& textures;
		IDX11Renderer& renderer;

		AutoRelease<ID3D11RasterizerState> spriteRasterizering;
		AutoRelease<ID3D11RasterizerState> objectRasterizering;
		AutoRelease<ID3D11RasterizerState> particleRasterizering;
		AutoRelease<ID3D11RasterizerState> skyRasterizering;
		AutoRelease<ID3D11RasterizerState> shadowRasterizering;

		AutoRelease<ID3D11BlendState> alphaBlend;
		AutoRelease<ID3D11BlendState> alphaAdditiveBlend;
		AutoRelease<ID3D11BlendState> disableBlend;
		AutoRelease<ID3D11BlendState> additiveBlend;
		AutoRelease<ID3D11BlendState> plasmaBlend;

		AutoRelease<ID3D11DepthStencilState> objDepthState;
		AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;
		AutoRelease<ID3D11DepthStencilState> noDepthTestOrWrite;

		ID3D11SamplerState* defaultSamplers[16] = { 0 };

		AutoFree<IDX11Gui> gui;

		AutoFree<RAL::IPipelineSupervisor> RAL_pipeline;

		DX11Pipeline(DX11::RenderBundle& bundle) :
			installation(bundle.installation), device(bundle.device), dc(bundle.dc), textures(bundle.textures), renderer(bundle.renderer)
		{
			objDepthState = DX11::CreateObjectDepthStencilState(device);
			objDepthState_NoWrite = DX11::CreateObjectDepthStencilState_NoWrite(device);
			noDepthTestOrWrite = DX11::CreateNoDepthCheckOrWrite(device);

			spriteRasterizering = DX11::CreateSpriteRasterizer(device);
			objectRasterizering = DX11::CreateObjectRasterizer(device);
			particleRasterizering = DX11::CreateParticleRasterizer(device);
			skyRasterizering = DX11::CreateSkyRasterizer(device);
			shadowRasterizering = DX11::CreateShadowRasterizer(device);

			alphaAdditiveBlend = DX11::CreateAlphaAdditiveBlend(device);
			disableBlend = DX11::CreateNoBlend(device);
			additiveBlend = DX11::CreateAdditiveBlend(device);
			plasmaBlend = DX11::CreatePlasmaBlend(device);

			gui = CreateDX11Gui(bundle);

			alphaBlend = DX11::CreateAlphaBlend(device);

			RAL_pipeline = RAL::CreatePipeline(*this, bundle.RAL);

		}

		virtual ~DX11Pipeline()
		{
			for (auto& s : defaultSamplers)
			{
				if (s) s->Release();
			}
		}

		RAL::IPipeline& RALPipeline() override
		{
			return *RAL_pipeline;
		}

		IGui3D& Gui3D() override
		{
			return RAL_pipeline->Gui3D();
		}

		IGuiRenderContextSupervisor& Gui() override
		{
			return *gui;
		}

		IParticles& Particles() override
		{
			return RAL_pipeline->Particles();
		}

		void SetShaderTexture(uint32 textureUnitIndex, Rococo::ID_CUBE_TEXTURE cubeId) override
		{
			auto* shaderView = textures.GetShaderView(cubeId);
			if (shaderView)
			{
				dc.PSSetShaderResources(textureUnitIndex, 1, &shaderView);
			}
		}

		void AssignGuiShaderResources() override
		{
			gui->AssignShaderResourcesToDC();
		}

		void DisableBlend() override
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
		}

		void UseAdditiveBlend() override
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
		}

		void UseAlphaBlend() override
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
		}

		void UseAlphaAdditiveBlend() override
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
		}

		void TargetShadowBuffer(ID_TEXTURE shadowBufferId) override
		{
			auto shadowBind = textures.GetTexture(shadowBufferId);

			dc.OMSetDepthStencilState(nullptr, 0);
			dc.OMSetRenderTargets(0, nullptr, shadowBind.depthView);

			D3D11_TEXTURE2D_DESC desc;
			shadowBind.texture->GetDesc(&desc);

			D3D11_VIEWPORT viewport = { 0 };
			viewport.Width = (FLOAT)desc.Width;
			viewport.Height = (FLOAT)desc.Height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			dc.RSSetViewports(1, &viewport);

			dc.ClearDepthStencilView(shadowBind.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);

			dc.RSSetState(shadowRasterizering);
		}

		void UseObjectRasterizer() override
		{
			dc.RSSetState(objectRasterizering);
			dc.OMSetDepthStencilState(objDepthState, 0);
		}

		void UseParticleRasterizer() override
		{
			dc.RSSetState(particleRasterizering);
		}

		void UseSkyRasterizer() override
		{
			dc.RSSetState(skyRasterizering);
		}

		void UsePlasmaBlend() override
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		}

		void UseSpriteRasterizer() override
		{
			dc.RSSetState(spriteRasterizering);
		}

		void UseObjectDepthState() override
		{
			dc.OMSetDepthStencilState(objDepthState, 0);
		}

		void DisableWritesOnDepthState() override
		{
			dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
		}

		void SetDrawTopology(PrimitiveTopology topology) override
		{
			dc.IASetPrimitiveTopology(*reinterpret_cast<D3D11_PRIMITIVE_TOPOLOGY*>(&topology));
		}

		void Free() override
		{
			delete this;
		}

		void ShowVenue(IMathsVisitor& visitor) override
		{
			RAL_pipeline->ShowVenue(visitor);
			gui->ShowVenue(visitor);
		}

		RenderTarget GetCurrentRenderTarget(const RenderPhaseConfig& phaseConfig)
		{
			RenderTarget rt = { 0 };

			rt.depthView = textures.GetTexture(phaseConfig.depthTarget).depthView;

			if (phaseConfig.renderTarget)
			{
				rt.renderTargetView = textures.GetTexture(phaseConfig.renderTarget).renderView;
			}
			else
			{
				rt.renderTargetView = renderer.BackBuffer();
			}

			if (rt.depthView == nullptr)
			{
				Throw(0, "GetCurrentRenderTarget - bad depth buffer");
			}

			if (rt.renderTargetView == nullptr)
			{
				Throw(0, "GetCurrentRenderTarget - bad render target buffer");
			}

			return rt;
		}

		void SetAndClearCurrentRenderBuffers(const RGBA& clearColour, const RenderPhaseConfig& config) override
		{
			RenderTarget rt = GetCurrentRenderTarget(config);
			dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

			dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			if (clearColour.alpha > 0)
			{
				dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
			}
		}

		void SetSamplerDefaults(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour) override
		{
			if (defaultSamplers[index])
			{
				defaultSamplers[index]->Release();
				defaultSamplers[index] = nullptr;
			}

			auto* sampler = Rococo::DX11::GetSampler(device, filter, u, v, w, borderColour);
			defaultSamplers[index] = sampler;
		}

		void ResetSamplersToDefaults() override
		{
			dc.PSSetSamplers(0, 16, defaultSamplers);
			dc.GSSetSamplers(0, 16, defaultSamplers);
			dc.VSSetSamplers(0, 16, defaultSamplers);
		}

		void SetBoneMatrix(uint32 index, cr_m4x4 m) override
		{
			RAL_pipeline->SetBoneMatrix(index, m);
		}

		IGuiResources& GuiResources() override
		{
			return gui->Resources();
		}
	}; // DX11Pipeline

	IDX11Pipeline* CreateDX11Pipeline(RenderBundle& bundle)
	{
		return new DX11Pipeline(bundle);
	}
}

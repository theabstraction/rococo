#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include <vector>
#include <RAL/RAL.pipeline.h>
#include <rococo.subsystems.h>
#include <rococo.reflector.h>

using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Samplers;
using namespace Rococo::RAL;
using namespace Rococo::DX11;
using namespace Rococo::Reflection;

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

	cstr ToString(D3D11_TEXTURE_ADDRESS_MODE mode)
	{
		switch (mode)
		{
		case D3D11_TEXTURE_ADDRESS_WRAP:
			return "Wrap";
		case D3D11_TEXTURE_ADDRESS_MIRROR:
			return "Mirror";
		case D3D11_TEXTURE_ADDRESS_CLAMP:
			return "Clamp";
		case D3D11_TEXTURE_ADDRESS_BORDER:
			return "Border";
		case D3D11_TEXTURE_ADDRESS_MIRROR_ONCE:
			return "Mirror-Once";
		default:
			return "Unknown";
		}
	}

	cstr ToString(D3D11_COMPARISON_FUNC f)
	{
		switch (f)
		{
		case D3D11_COMPARISON_NEVER:
			return "Never";
		case D3D11_COMPARISON_LESS:
			return "LT";
		case D3D11_COMPARISON_EQUAL:
			return "EQ";
		case D3D11_COMPARISON_LESS_EQUAL:
			return "LTE";
		case D3D11_COMPARISON_GREATER:
			return "GT";
		case D3D11_COMPARISON_NOT_EQUAL:
			return "NEQ";
		case D3D11_COMPARISON_GREATER_EQUAL:
			return "GTE";
		case D3D11_COMPARISON_ALWAYS:
			return "Always";
		default:
			return "Unknown";
		}
	} 

	cstr ToString(D3D11_FILTER filter)
	{
		switch (filter)
		{
		case D3D11_FILTER_MIN_MAG_MIP_POINT:
			return "MIN_MAG_MIP_POINT";
		case D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR:
			return "MIN_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return "MIN_POINT_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR:
			return "MIN_POINT_MAG_MIP_LINEAR";
		case D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT:
			return "MIN_LINEAR_MAG_MIP_POINT";
		case D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return "MIN_LINEAR_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT:
			return "MIN_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MIN_MAG_MIP_LINEAR:
			return "MIN_MAG_MIP_LINEAR";
		case D3D11_FILTER_ANISOTROPIC:
			return "ANISOTROPIC";
		case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT:
			return "COMPARISON_MIN_MAG_MIP_POINT";
		case D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			return "COMPARISON_MIN_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return "COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			return "COMPARISON_MIN_POINT_MAG_MIP_LINEAR";
		case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			return "COMPARISON_MIN_LINEAR_MAG_MIP_POINT";
		case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return "COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			return "COMPARISON_MIN_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
			return "COMPARISON_MIN_MAG_MIP_LINEAR";
		case D3D11_FILTER_COMPARISON_ANISOTROPIC:
			return "COMPARISON_ANISOTROPIC";
		case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT:
			return "MINIMUM_MIN_MAG_MIP_POINT";
		case D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return "MINIMUM_MIN_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return "MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return "MINIMUM_MIN_POINT_MAG_MIP_LINEAR";
		case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return "MINIMUM_MIN_LINEAR_MAG_MIP_POINT";
		case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return "MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return "MINIMUM_MIN_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
			return "MINIMUM_MIN_MAG_MIP_LINEAR";
		case D3D11_FILTER_MINIMUM_ANISOTROPIC:
			return "MINIMUM_ANISOTROPIC";
		case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:
			return "MAXIMUM_MIN_MAG_MIP_POINT";
		case D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return "MAXIMUM_MIN_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return "MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return "MAXIMUM_MIN_POINT_MAG_MIP_LINEAR";
		case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return "MAXIMUM_MIN_LINEAR_MAG_MIP_POINT";
		case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return "MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
		case D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return "MAXIMUM_MIN_MAG_LINEAR_MIP_POINT";
		case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
			return "MAXIMUM_MIN_MAG_MIP_LINEAR";
		case D3D11_FILTER_MAXIMUM_ANISOTROPIC:
			return "MAXIMUM_ANISOTROPIC";
		default:
			return "Unknown";
		}
	}

	void AddElementFields(const D3D11_SAMPLER_DESC& desc, IReflectionVisitor& v)
	{
		ReflectStackFormat(v, "Address U", ToString(desc.AddressU));
		ReflectStackFormat(v, "Address V", ToString(desc.AddressV));
		ReflectStackFormat(v, "Address W", ToString(desc.AddressW));
		
		const auto* c = desc.BorderColor;
		ReflectStackFormat(v, "BorderColor", "R:%f G:%f B:%f A:%f", c[0], c[1], c[2], c[3]);
		ReflectStackFormat(v, "ComparisonFunc", ToString(desc.ComparisonFunc));
		ReflectStackFormat(v, "Filter", ToString(desc.Filter));

		ReflectStackFormat(v, "MaxAnisotropy", "%u", desc.MaxAnisotropy);
		ReflectStackFormat(v, "Min LOD", "%f", desc.MinLOD);
		ReflectStackFormat(v, "Max LOD", "%f", desc.MaxLOD);
		ReflectStackFormat(v, "Mip LOD Bias", "%f", desc.MipLODBias);
	}

	struct DX11Pipeline : IDX11Pipeline, RAL::IRenderStates, ISubsystem, IReflectionTarget
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

		[[nodiscard]] cstr SubsystemName() const override
		{
			return "DX11Pipeline";
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

		RenderTarget GetCurrentRenderTarget(const RenderOutputTargets& targets)
		{
			RenderTarget rt = { 0 };

			rt.depthView = textures.GetTexture(targets.depthTarget).depthView;

			if (targets.renderTarget)
			{
				rt.renderTargetView = textures.GetTexture(targets.renderTarget).renderView;
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

		void SetAndClearCurrentRenderBuffers(const RGBA& clearColour, const RenderOutputTargets& targets) override
		{
			RenderTarget rt = GetCurrentRenderTarget(targets);
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

		void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM parentId) override
		{
			ID_SUBSYSTEM dx11PipelineSystemId = monitor.Register(*this, parentId);
			gui->RegisterSubsystem(monitor, dx11PipelineSystemId);
			RAL_pipeline->RegisterSubsystem(monitor, dx11PipelineSystemId);
		}

		IReflectionTarget* ReflectionTarget() override
		{
			return this;
		}

		void Visit(IReflectionVisitor& v) override
		{
			Section pipeline("DX11Pipeline", v);

			{
				Container samplers("Samplers", v);

				for (int i = 0; i < 16; i++)
				{
					if (defaultSamplers[i])
					{
						EnterElement(v, "Sampler #%d", i);

						D3D11_SAMPLER_DESC desc;
						defaultSamplers[i]->GetDesc(&desc);

						AddElementFields(desc, v);

						v.LeaveElement();
					}
				}
			}

			{
				Section rasterizers("Rasterizers", v);

				Reflect(v, "spriteRasterizering", *spriteRasterizering);
				Reflect(v, "objectRasterizering", *objectRasterizering);
				Reflect(v, "particleRasterizering", *particleRasterizering);
				Reflect(v, "skyRasterizering", *skyRasterizering);
				Reflect(v, "shadowRasterizering", *shadowRasterizering);
			}


			{
				Section blending("Blending", v);
				Reflect(v, "alphaBlend", *alphaBlend);
				Reflect(v, "alphaAdditiveBlend", *alphaAdditiveBlend);
				Reflect(v, "disableBlend", *disableBlend);
				Reflect(v, "additiveBlend", *additiveBlend);
				Reflect(v, "plasmaBlend", *plasmaBlend);
			}

			{
				Section depthStencil("Depth+Stencil", v);
				Reflect(v, "objDepthState", *objDepthState);
				Reflect(v, "objDepthState_NoWrite", *objDepthState_NoWrite);
				Reflect(v, "noDepthTestOrWrite", *noDepthTestOrWrite);
			}
		}
	}; // DX11Pipeline

	IDX11Pipeline* CreateDX11Pipeline(RenderBundle& bundle)
	{
		return new DX11Pipeline(bundle);
	}
}

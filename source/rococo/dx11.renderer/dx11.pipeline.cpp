#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include "dx11.pipeline.h"

using namespace Rococo::DX11;

namespace Rococo::DX11
{
	DX11Pipeline::DX11Pipeline(DX11::RenderBundle& bundle) :
		installation(bundle.installation), device(bundle.device), dc(bundle.dc), shaders(bundle.shaders), meshes(bundle.meshes), textures(bundle.textures), renderer(bundle.renderer), rc(bundle.rc)
	{
		RAL_pipeline = RAL::CreatePipeline(*this, bundle.RAL);

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

		instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);
		lightConeBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, 3);
		
		depthRenderStateBuffer = DX11::CreateConstantBuffer<DepthRenderData>(device);

		gui = CreateDX11Gui(bundle);

		idObjVS = shaders.CreateObjectVertexShader("!shaders/compiled/object.vs");
		idObjPS = shaders.CreatePixelShader("!shaders/compiled/object.ps");
		idObj_Spotlight_NoEnvMap_PS = shaders.CreatePixelShader("!shaders/compiled/obj.spotlight.no_env.ps");
		idObj_Ambient_NoEnvMap_PS = shaders.CreatePixelShader("!shaders/compiled/obj.ambient.no_env.ps");
		idObjAmbientPS = shaders.CreatePixelShader("!shaders/compiled/ambient.ps");
		idObjAmbientVS = shaders.CreateObjectVertexShader("!shaders/compiled/ambient.vs");
		idObjVS_Shadows = shaders.CreateObjectVertexShader("!shaders/compiled/shadow.vs");
		idSkinnedObjVS_Shadows = shaders.CreateVertexShader("!shaders/compiled/skinned.shadow.vs", DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		idObjPS_Shadows = shaders.CreatePixelShader("!shaders/compiled/shadow.ps");
		idLightConePS = shaders.CreatePixelShader("!shaders/compiled/light_cone.ps");
		idLightConeVS = shaders.CreateVertexShader("!shaders/compiled/light_cone.vs", DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());

		alphaBlend = DX11::CreateAlphaBlend(device);
	}

	ID_PIXEL_SHADER DX11Pipeline::GetObjectShaderPixelId(RenderPhase phase)
	{
		switch (phaseConfig.EnvironmentalMap)
		{
		case Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE:
			switch (phase)
			{
			case RenderPhase::DetermineAmbient:
				return idObjAmbientPS;
			case RenderPhase::DetermineSpotlight:
				return idObjPS;
			case RenderPhase::DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		case Graphics::ENVIRONMENTAL_MAP_PROCEDURAL:
			switch (phase)
			{
			case RenderPhase::DetermineAmbient:
				return idObj_Ambient_NoEnvMap_PS;
			case RenderPhase::DetermineSpotlight:
				return idObj_Spotlight_NoEnvMap_PS;
			case RenderPhase::DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		default:
			Throw(0, "Environment mode %d not implemented", phaseConfig.EnvironmentalMap);
		}
	}

	void DX11Pipeline::Free()
	{
		delete this;
	}

	void DX11Pipeline::ShowVenue(IMathsVisitor& visitor)
	{
		visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities, %lld particles", trianglesThisFrame, entitiesThisFrame, 0);
		gui->ShowVenue(visitor);
	}

	void DX11Pipeline::UpdateGlobalState(const GuiMetrics& metrics, IScene& scene)
	{
		RAL_pipeline->UpdateGlobalState(metrics, scene);
	}

	RenderTarget DX11Pipeline::GetCurrentRenderTarget()
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

	void DX11Pipeline::ClearCurrentRenderBuffers(const RGBA& clearColour)
	{
		RenderTarget rt = GetCurrentRenderTarget();
		dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (clearColour.alpha > 0)
		{
			dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
		}
	}

	void DX11Pipeline::SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour)
	{
		if (samplers[index])
		{
			samplers[index]->Release();
			samplers[index] = nullptr;
		}

		auto* sampler = Rococo::DX11::GetSampler(device, filter, u, v, w, borderColour);
		samplers[index] = sampler;
	}

	void DX11Pipeline::SetShaderTexture(uint32 textureUnitIndex, Rococo::ID_CUBE_TEXTURE cubeId)
	{
		auto* shaderView = textures.GetShaderView(cubeId);
		if (shaderView)
		{
			dc.PSSetShaderResources(textureUnitIndex, 1, &shaderView);
		}
	}

	void DX11Pipeline::ResetSamplersToDefaults()
	{
		dc.PSSetSamplers(0, 16, samplers);
		dc.GSSetSamplers(0, 16, samplers);
		dc.VSSetSamplers(0, 16, samplers);
	}

	void DX11Pipeline::SetBoneMatrix(uint32 index, cr_m4x4 m)
	{
		RAL_pipeline->SetBoneMatrix(index, m);
	}

	IDX11Pipeline* CreateDX11Pipeline(RenderBundle& bundle)
	{
		return new DX11Pipeline(bundle);
	}
}

#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include "dx11.pipeline.h"

using namespace Rococo::RAL;

namespace Rococo::DX11
{
	void DX11Pipeline::RenderToShadowBuffer(IShaders& shaders, IDX11TextureManager& textures, IRenderContext& rc, DepthRenderData& drd, ID_TEXTURE shadowBuffer, IScene& scene)
	{
		auto shadowBind = textures.GetTexture(shadowBuffer);

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

		shaders.UseShaders(idSkinnedObjVS_Shadows, idObjPS_Shadows);

		RAL_pipeline->UpdateDepthRenderData(drd);

		phase = RenderPhase::DetermineShadowVolumes;
		scene.RenderShadowPass(drd, rc, true);

		shaders.UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		RAL_pipeline->UpdateDepthRenderData(drd);

		scene.RenderShadowPass(drd, rc, false);
	}

	void DX11Pipeline::RenderAmbient(IShaders& shaders, IRenderContext& rc, IScene& scene, const LightConstantBuffer& ambientLight)
	{
		phase = RenderPhase::DetermineAmbient;

		ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
		if (shaders.UseShaders(idObjAmbientVS, idPS))
		{
			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			renderer.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			if (builtFirstPass)
			{
				dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
				dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
			}
			else
			{
				dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
				builtFirstPass = true;
			}

			dc.RSSetState(objectRasterizering);

			RAL_pipeline->AssignAmbientLightToShaders(ambientLight);

			scene.RenderObjects(rc, false);
			scene.RenderObjects(rc, true);

			RAL_pipeline->Render3DGui();
			RAL_pipeline->RenderFogWithAmbient();
		}

		phase = RenderPhase::None;
	}

	void DX11Pipeline::RenderSkyBox(IScene& scene)
	{
		RAL_pipeline->RenderSkyBox(scene);
	}

	void DX11Pipeline::DisableBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAdditiveBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAlphaBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseAlphaAdditiveBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseParticleRasterizer()
	{
		dc.RSSetState(particleRasterizering);
	}

	void DX11Pipeline::UseSkyRasterizer()
	{
		dc.RSSetState(skyRasterizering);
	}

	void DX11Pipeline::UsePlasmaBlend()
	{
		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
	}

	void DX11Pipeline::UseSpriteRasterizer()
	{
		dc.RSSetState(spriteRasterizering);
	}

	void DX11Pipeline::DisableWritesOnDepthState()
	{
		dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
	}

	void DX11Pipeline::SetDrawTopology(PrimitiveTopology topology)
	{
		dc.IASetPrimitiveTopology(*reinterpret_cast<D3D11_PRIMITIVE_TOPOLOGY*>(&topology));
	}

	void DX11Pipeline::RenderSpotlightLitScene(const LightConstantBuffer& lightSubset, IScene& scene)
	{
		LightConstantBuffer light = lightSubset;

		DepthRenderData drd;
		if (PrepareDepthRenderFromLight(light, drd))
		{
			drd.randoms.x = 0.0f;
			drd.randoms.y = 1.0f;
			drd.randoms.z = 2.0f;
			drd.randoms.w = 3.0f;

			RenderToShadowBuffer(shaders, textures, rc, drd, phaseConfig.shadowBuffer, scene);

			RenderTarget rt;
			rt.depthView = textures.GetTexture(phaseConfig.depthTarget).depthView;
			rt.renderTargetView = phaseConfig.renderTarget ? textures.GetTexture(phaseConfig.renderTarget).renderView : renderer.BackBuffer();

			dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

			phase = RenderPhase::DetermineSpotlight;

			ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
			shaders.UseShaders(idObjVS, idPS);

			renderer.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			light.randoms = drd.randoms;
			light.time = drd.time;
			light.right = drd.right;
			light.up = drd.up;
			light.worldToShadowBuffer = drd.worldToScreen;

			RAL_pipeline->UpdateLightBuffer(light);
			RAL_pipeline->AssignLightStateBufferToShaders();

			FLOAT blendFactorUnused[] = { 0,0,0,0 };

			if (builtFirstPass)
			{
				dc.OMSetBlendState(additiveBlend, blendFactorUnused, 0xffffffff);
				dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
			}
			else
			{
				dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
				builtFirstPass = true;
			}

			auto shadowBind = textures.GetTexture(phaseConfig.shadowBuffer);

			dc.PSSetShaderResources(TXUNIT_SHADOW, 1, &shadowBind.shaderView);
			dc.RSSetState(objectRasterizering);

			scene.RenderObjects(rc, false);
			scene.RenderObjects(rc, true);

			RAL_pipeline->Render3DGui();
			RAL_pipeline->RenderFogWithSpotlight();

			phase = RenderPhase::None;

			dc.OMSetDepthStencilState(objDepthState, 0);
		}
	}

	void DX11Pipeline::Render3DObjects(IScene& scene)
	{
		auto now = Time::TickCount();

		dc.RSSetState(objectRasterizering);
		dc.OMSetDepthStencilState(objDepthState, 0);

		builtFirstPass = false;

		uint32 nLights = 0;
		const LightConstantBuffer* lights = scene.GetLights(nLights);
		if (lights != nullptr)
		{
			for (size_t i = 0; i < nLights; ++i)
			{
				try
				{
					RenderSpotlightLitScene(lights[i], scene);
				}
				catch (IException& ex)
				{
					Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
				}
			}

			RenderAmbient(shaders, rc, scene, lights[0]);
		}

		objCost = Time::TickCount() - now;
	}

	void DX11Pipeline::Render(const GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene)
	{
		phaseConfig.EnvironmentalMap = envMap;
		phaseConfig.shadowBuffer = RAL_pipeline->ShadowBufferId();
		phaseConfig.depthTarget = renderer.GetWindowDepthBufferId();

		if (textures.GetTexture(phaseConfig.shadowBuffer).depthView == nullptr)
		{
			Throw(0, "No shadow depth buffer set for DX1AppRenderer::Render(...)");
		}

		ID_CUBE_TEXTURE envId = scene.GetEnvironmentMap();
		renderer.SetEnvironmentMap(envId);
		
		trianglesThisFrame = 0;
		entitiesThisFrame = 0;

		lastTextureId = ID_TEXTURE::Invalid();

		renderer.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

		ClearCurrentRenderBuffers(scene.GetClearColour());

		UpdateGlobalState(metrics, scene);

		RenderTarget rt = GetCurrentRenderTarget();
		dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		ResetSamplersToDefaults();

		RenderSkyBox(scene);

		gui->AssignShaderResourcesToDC();
		ResetSamplersToDefaults();

		Render3DObjects(scene);

		RAL_pipeline->RenderPlasma();
		RAL_pipeline->DrawLightCones(scene);

		UpdateGlobalState(metrics, scene);

		if (IsGuiReady())
		{
			Time::ticks now = Time::TickCount();

			gui->RenderGui(scene, metrics, IsGuiReady());

			gui->FlushLayer();

			if (IsGuiReady())
			{
				gui->DrawCursor(metrics);

				gui->FlushLayer();
			}

			guiCost = Time::TickCount() - now;
		}
	}
} // Rococo::DX11

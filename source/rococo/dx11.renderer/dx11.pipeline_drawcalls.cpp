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
	void DX11Pipeline::Draw(MeshBuffer& m, const ObjectInstance* instances, uint32 nInstances)
	{
		if (!m.vertexBuffer)
			return;

		if (phase == RenderPhase::DetermineShadowVolumes && m.disableShadowCasting)
			return;

		ID3D11Buffer* buffers[2] = { m.vertexBuffer, m.weightsBuffer };

		entitiesThisFrame += (int64)nInstances;

		bool overrideShader = false;

		if (m.psSpotlightShader && phase == RenderPhase::DetermineSpotlight)
		{
			shaders.UseShaders(m.vsSpotlightShader, m.psSpotlightShader);
			overrideShader = true;
		}
		else if (m.psAmbientShader && phase == RenderPhase::DetermineAmbient)
		{
			shaders.UseShaders(m.vsAmbientShader, m.psAmbientShader);
			overrideShader = true;
		}

		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		if (m.alphaBlending)
		{
			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);
		}

		UINT strides[] = { sizeof(ObjectVertex), sizeof(BoneWeights) };
		UINT offsets[]{ 0, 0 };
		dc.IASetPrimitiveTopology(m.topology);
		dc.IASetVertexBuffers(0, m.weightsBuffer ? 2 : 1, buffers, strides, offsets);

		for (uint32 i = 0; i < nInstances; i++)
		{
			// dc.DrawInstances crashed the debugger, replace with single instance render call for now
			DX11::CopyStructureToBuffer(dc, instanceBuffer, instances + i, sizeof(ObjectInstance));
			dc.VSSetConstantBuffers(CBUFFER_INDEX_INSTANCE_BUFFER, 1, &instanceBuffer);
			dc.Draw(m.numberOfVertices, 0);

			trianglesThisFrame += m.numberOfVertices / 3;
		}

		if (overrideShader)
		{
			// UseShaders(currentVertexShaderId, currentPixelShaderId);	 
		}

		if (m.alphaBlending)
		{
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
		}
	}

	void DX11Pipeline::DrawLightCone(const LightConstantBuffer& light, cr_vec3 viewDir)
	{
		trianglesThisFrame += DX11::DrawLightCone(light, viewDir, dc, *lightConeBuffer);
	}

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

		DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
		dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

		phase = RenderPhase::DetermineShadowVolumes;
		scene.RenderShadowPass(drd, rc, true);

		shaders.UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
		dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

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

		DrawLightCones(scene);

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

	void DX11Pipeline::DrawLightCones(IScene& scene)
	{
		uint32 nLights = 0;
		const LightConstantBuffer* lights = scene.GetLights(nLights);

		if (lights != nullptr)
		{
			UINT strides[] = { sizeof(ObjectVertex) };
			UINT offsets[]{ 0 };

			dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			dc.IASetVertexBuffers(0, 1, &lightConeBuffer, strides, offsets);
			dc.RSSetState(spriteRasterizering);

			shaders.UseShaders(idLightConeVS, idLightConePS);

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);
			dc.OMSetDepthStencilState(objDepthState, 0);

			RAL_pipeline->AssignGlobalStateBufferToShaders();

			ObjectInstance identity;
			identity.orientation = Matrix4x4::Identity();
			identity.highlightColour = { 0 };
			DX11::CopyStructureToBuffer(dc, instanceBuffer, &identity, sizeof(ObjectInstance));
			dc.VSSetConstantBuffers(CBUFFER_INDEX_INSTANCE_BUFFER, 1, &instanceBuffer);

			Matrix4x4 camera;
			Matrix4x4 world;
			Matrix4x4 proj;
			Vec4 eye;
			Vec4 viewDir;
			scene.GetCamera(camera, world, proj, eye, viewDir);

			for (uint32 i = 0; i < nLights; ++i)
			{
				if (lights[i].hasCone)
				{
					DrawLightCone(lights[i], viewDir);
				}
			}

			dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
			dc.RSSetState(nullptr);
			dc.PSSetConstantBuffers(0, 0, nullptr);
			dc.VSSetConstantBuffers(0, 0, nullptr);
		}
	}
} // Rococo::DX11

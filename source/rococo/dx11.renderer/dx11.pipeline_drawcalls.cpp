#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <rococo.os.h>
#include <rococo.time.h>
#include "dx11.pipeline.h"

namespace Rococo::DX11
{
	void DX11Pipeline::Draw(MeshBuffer& m, const ObjectInstance* instances, uint32 nInstances)
	{
		if (!m.vertexBuffer)
			return;

		if (phase == RenderPhase_DetermineShadowVolumes && m.disableShadowCasting)
			return;

		ID3D11Buffer* buffers[2] = { m.vertexBuffer, m.weightsBuffer };

		entitiesThisFrame += (int64)nInstances;

		bool overrideShader = false;

		if (m.psSpotlightShader && phase == RenderPhase_DetermineSpotlight)
		{
			shaders.UseShaders(m.vsSpotlightShader, m.psSpotlightShader);
			overrideShader = true;
		}
		else if (m.psAmbientShader && phase == RenderPhase_DetermineAmbient)
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

	void DX11Pipeline::DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID)
	{
		if (nParticles == 0) return;
		if (!shaders.UseShaders(vsID, psID)) return;
		if (!shaders.UseGeometryShader(gsID)) return;

		dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		dc.RSSetState(particleRasterizering);
		dc.OMSetDepthStencilState(objDepthState_NoWrite, 0);

		const ParticleVertex* start = &particles[0];

		size_t i = 0;

		while (nParticles > 0)
		{
			size_t chunkSize = min(nParticles, (size_t)PARTICLE_BUFFER_VERTEX_CAPACITY);
			DX11::CopyStructureToBuffer(dc, particleBuffer, start + i, chunkSize * sizeof(ParticleVertex));

			UINT strides[1] = { sizeof(ParticleVertex) };
			UINT offsets[1] = { 0 };

			dc.IASetVertexBuffers(0, 1, &particleBuffer, strides, offsets);
			dc.Draw((UINT)chunkSize, 0);

			i += chunkSize;
			nParticles -= chunkSize;
		}

		shaders.UseGeometryShader(ID_GEOMETRY_SHADER::Invalid());
	}

	void DX11Pipeline::DrawLightCone(const Light& light, cr_vec3 viewDir)
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

		phase = RenderPhase_DetermineShadowVolumes;
		scene.RenderShadowPass(drd, rc, true);

		shaders.UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		DX11::CopyStructureToBuffer(dc, depthRenderStateBuffer, drd);
		dc.VSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_DEPTH_RENDER_DESC, 1, &depthRenderStateBuffer);

		scene.RenderShadowPass(drd, rc, false);
	}

	void DX11Pipeline::RenderAmbient(IShaders& shaders, IRenderContext& rc, IScene& scene, const Light& ambientLight)
	{
		phase = RenderPhase_DetermineAmbient;

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

			this->ambientLight = ambientLight;
			SetAmbientConstants();

			scene.RenderObjects(rc, false);
			scene.RenderObjects(rc, true);
			Render3DGui(gui3DTriangles.data(), gui3DTriangles.size());

			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			DrawParticles(fog.data(), fog.size(), idFogAmbientPS, idParticleVS, idFogAmbientGS);
		}

		phase = RenderPhase_None;
	}

	void DX11Pipeline::RenderSkyBox(IScene& scene)
	{
		ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

		ID3D11ShaderResourceView* skyCubeTextureView = textures.GetShaderView(cubeId);
		if (!skyCubeTextureView)
		{
			return;
		}

		if (!skyMeshId)
		{
			SkyVertex topNW{ -1.0f, 1.0f, 1.0f };
			SkyVertex topNE{ 1.0f, 1.0f, 1.0f };
			SkyVertex topSW{ -1.0f,-1.0f, 1.0f };
			SkyVertex topSE{ 1.0f,-1.0f, 1.0f };
			SkyVertex botNW{ -1.0f, 1.0f,-1.0f };
			SkyVertex botNE{ 1.0f, 1.0f,-1.0f };
			SkyVertex botSW{ -1.0f,-1.0f,-1.0f };
			SkyVertex botSE{ 1.0f,-1.0f,-1.0f };

			SkyVertex skyboxVertices[36] =
			{
				topSW, topNW, topNE, // top,
				topNE, topSE, topSW, // top,
				botSW, botNW, botNE, // bottom,
				botNE, botSE, botSW, // bottom,
				topNW, topSW, botSW, // West
				botSW, botNW, topNW, // West
				topNE, topSE, botSE, // East
				botSE, botNE, topNE, // East
				topNW, topNE, botNE, // North
				botNE, botNW, topNW, // North
				topSW, topSE, botSE, // South
				botSE, botSW, topSW, // South
			};

			skyMeshId = meshes.CreateSkyMesh(skyboxVertices, sizeof(skyboxVertices) / sizeof(SkyVertex));
		}

		if (shaders.UseShaders(idObjSkyVS, idObjSkyPS))
		{
			auto& mesh = meshes.GetBuffer(skyMeshId);
			UINT strides[] = { sizeof(SkyVertex) };
			UINT offsets[]{ 0 };
			dc.IASetPrimitiveTopology(mesh.topology);
			dc.IASetVertexBuffers(0, 1, &mesh.vertexBuffer, strides, offsets);
			dc.PSSetShaderResources(TXUNIT_ENV_MAP, 1, &skyCubeTextureView);

			//dc.PSSetSamplers(0, 1, &skySampler);

			dc.RSSetState(skyRasterizering);
			dc.OMSetDepthStencilState(noDepthTestOrWrite, 0);

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
			dc.Draw(mesh.numberOfVertices, 0);

			ResetSamplersToDefaults();
		}
		else
		{
			Throw(0, "DX11Renderer::RenderSkybox failed. Error setting sky shaders");
		}
	}

	void DX11Pipeline::RenderSpotlightLitScene(const Light& lightSubset, IScene& scene)
	{
		Light light = lightSubset;

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

			phase = RenderPhase_DetermineSpotlight;

			ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
			shaders.UseShaders(idObjVS, idPS);

			renderer.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			light.randoms = drd.randoms;
			light.time = drd.time;
			light.right = drd.right;
			light.up = drd.up;
			light.worldToShadowBuffer = drd.worldToScreen;

			DX11::CopyStructureToBuffer(dc, lightStateBuffer, light);
			SetupSpotlightConstants();

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

			Render3DGui(gui3DTriangles.data(), gui3DTriangles.size());

			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			DrawParticles(fog.data(), fog.size(), idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
			phase = RenderPhase_None;

			dc.OMSetDepthStencilState(objDepthState, 0);
		}
	}

	void DX11Pipeline::Render3DGui(const VertexTriangle* gui3DTriangles, size_t nTriangles)
	{
		size_t cursor = 0;

		ObjectInstance one = { Matrix4x4::Identity(), Vec3 {1.0f, 1.0f, 1.0f}, 0.0f, RGBA(1.0f, 1.0f, 1.0f, 1.0f) };

		while (nTriangles > 0)
		{
			auto* v = gui3DTriangles + cursor;

			size_t nTriangleBatchCount = min<size_t>(nTriangles, GUI3D_BUFFER_TRIANGLE_CAPACITY);

			DX11::CopyStructureToBuffer(dc, gui3DBuffer, v, nTriangleBatchCount * sizeof(VertexTriangle));

			MeshBuffer m;
			m.alphaBlending = false;
			m.disableShadowCasting = false;
			m.vertexBuffer = gui3DBuffer;
			m.weightsBuffer = nullptr;
			m.numberOfVertices = (UINT)nTriangleBatchCount * 3;
			m.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			Draw(m, &one, 1);

			nTriangles -= nTriangleBatchCount;
			cursor += nTriangleBatchCount;
		}
	}

	void DX11Pipeline::Render3DObjects(IScene& scene)
	{
		auto now = Time::TickCount();

		dc.RSSetState(objectRasterizering);
		dc.OMSetDepthStencilState(objDepthState, 0);

		builtFirstPass = false;

		uint32 nLights = 0;
		const Light* lights = scene.GetLights(nLights);
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
		phaseConfig.shadowBuffer = shadowBufferId;
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

		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		DrawParticles(plasma.data(), plasma.size(), idPlasmaPS, idParticleVS, idPlasmaGS);

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
		const Light* lights = scene.GetLights(nLights);

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

			AssignGlobalStateBufferToShaders();

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

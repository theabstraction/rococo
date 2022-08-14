#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <vector>
#include <random>

using namespace Rococo::DX11;

struct AmbientData
{
	RGBA localLight;
	float fogConstant = -0.2218f; // light = e(Rk). Where R is distance. k = -0.2218 gives modulation of 1/256 at 25 metres, reducing full brightness to dark
	float a = 0;
	float b = 0;
	float c = 0;
};

struct DX11Pipeline: IDX11Pipeline
{
	IInstallation& installation;
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IDX11Shaders& shaders;
	IDX11TextureManager& textures;
	IDX11Meshes& meshes;
	IDX11Renderer& renderer;
	IRenderContext& rc;

	AutoRelease<ID3D11RasterizerState> spriteRasterizering;
	AutoRelease<ID3D11RasterizerState> objectRasterizering;
	AutoRelease<ID3D11RasterizerState> particleRasterizering;
	AutoRelease<ID3D11RasterizerState> skyRasterizering;
	AutoRelease<ID3D11RasterizerState> shadowRasterizering;

	AutoRelease<ID3D11Buffer> depthRenderStateBuffer;
	AutoRelease<ID3D11Buffer> lightStateBuffer;
	AutoRelease<ID3D11Buffer> ambientBuffer;

	AutoRelease<ID3D11BlendState> alphaAdditiveBlend;
	AutoRelease<ID3D11BlendState> disableBlend;
	AutoRelease<ID3D11BlendState> additiveBlend;
	AutoRelease<ID3D11BlendState> plasmaBlend;

	AutoRelease<ID3D11DepthStencilState> objDepthState;
	AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;
	AutoRelease<ID3D11DepthStencilState> noDepthTestOrWrite;

	AutoRelease<ID3D11Buffer> instanceBuffer;
	AutoRelease<ID3D11Buffer> lightConeBuffer;
	AutoRelease<ID3D11Buffer> gui3DBuffer;

	ID_VERTEX_SHADER idObjVS;
	ID_PIXEL_SHADER idObjPS;
	ID_PIXEL_SHADER idObjPS_Shadows;
	ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
	ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;
	ID_PIXEL_SHADER idObjAmbientPS;
	ID_VERTEX_SHADER idObjAmbientVS;
	ID_PIXEL_SHADER idLightConePS;
	ID_VERTEX_SHADER idLightConeVS;
	ID_VERTEX_SHADER idObjVS_Shadows;
	ID_VERTEX_SHADER idSkinnedObjVS_Shadows;

	ID_VERTEX_SHADER idParticleVS;
	ID_PIXEL_SHADER idPlasmaPS;
	ID_PIXEL_SHADER idFogAmbientPS;
	ID_PIXEL_SHADER idFogSpotlightPS;
	ID_GEOMETRY_SHADER idPlasmaGS;
	ID_GEOMETRY_SHADER idFogSpotlightGS;
	ID_GEOMETRY_SHADER idFogAmbientGS;

	ID_VERTEX_SHADER idObjSkyVS;
	ID_PIXEL_SHADER idObjSkyPS;

	std::vector<ParticleVertex> fog;
	std::vector<ParticleVertex> plasma;
	std::vector<VertexTriangle> gui3DTriangles;

	ID_SYS_MESH skyMeshId;
	AutoRelease<ID3D11SamplerState> skySampler;

	ID_TEXTURE shadowBufferId; 

	AutoRelease<ID3D11BlendState> alphaBlend;

	enum RenderPhase
	{
		RenderPhase_None,
		RenderPhase_DetermineShadowVolumes,
		RenderPhase_DetermineSpotlight,
		RenderPhase_DetermineAmbient
	};

	RenderPhase phase = RenderPhase_None;

	int64 entitiesThisFrame = 0;
	int64 trianglesThisFrame = 0;
	bool builtFirstPass = false;

	Graphics::RenderPhaseConfig phaseConfig;

	AutoRelease<ID3D11Buffer> particleBuffer;

	std::default_random_engine rng;

	enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };
	enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };
	enum { GUI3D_BUFFER_VERTEX_CAPACITY = 3 * GUI3D_BUFFER_TRIANGLE_CAPACITY };

	DX11Pipeline(IInstallation& _installation, IDX11Shaders& _shaders, IDX11TextureManager& _textures, IDX11Meshes& _meshes, IDX11Renderer& _renderer, IRenderContext& _rc, ID3D11Device& _device, ID3D11DeviceContext& _dc):
		installation(_installation), device(_device), dc(_dc), shaders(_shaders), meshes(_meshes), textures(_textures), renderer(_renderer), rc(_rc)
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

		instanceBuffer = DX11::CreateConstantBuffer<ObjectInstance>(device);
		lightConeBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, 3);
		particleBuffer = DX11::CreateDynamicVertexBuffer<ParticleVertex>(device, PARTICLE_BUFFER_VERTEX_CAPACITY);

		depthRenderStateBuffer = DX11::CreateConstantBuffer<DepthRenderData>(device);
		lightStateBuffer = DX11::CreateConstantBuffer<Light>(device);

		ambientBuffer = DX11::CreateConstantBuffer<AmbientData>(device);
		gui3DBuffer = DX11::CreateDynamicVertexBuffer<ObjectVertex>(device, GUI3D_BUFFER_VERTEX_CAPACITY);

		idObjVS = shaders.CreateObjectVertexShader("!object.vs");
		idObjPS = shaders.CreatePixelShader("!object.ps");
		idObj_Spotlight_NoEnvMap_PS = shaders.CreatePixelShader("!obj.spotlight.no_env.ps");
		idObj_Ambient_NoEnvMap_PS = shaders.CreatePixelShader("!obj.ambient.no_env.ps");
		idObjAmbientPS = shaders.CreatePixelShader("!ambient.ps");
		idObjAmbientVS = shaders.CreateObjectVertexShader("!ambient.vs");
		idObjVS_Shadows = shaders.CreateObjectVertexShader("!shadow.vs");
		idSkinnedObjVS_Shadows = shaders.CreateVertexShader("!skinned.shadow.vs", DX11::GetSkinnedObjectVertexDesc(), DX11::NumberOfSkinnedObjectVertexElements());
		idObjPS_Shadows = shaders.CreatePixelShader("!shadow.ps");
		idLightConePS = shaders.CreatePixelShader("!light_cone.ps");
		idLightConeVS = shaders.CreateVertexShader("!light_cone.vs", DX11::GetObjectVertexDesc(), DX11::NumberOfObjectVertexElements());
		idParticleVS = shaders.CreateParticleVertexShader("!particle.vs");
		idPlasmaGS = shaders.CreateGeometryShader("!plasma.gs");
		idFogSpotlightGS = shaders.CreateGeometryShader("!fog.spotlight.gs");
		idFogAmbientGS = shaders.CreateGeometryShader("!fog.ambient.gs");
		idPlasmaPS = shaders.CreatePixelShader("!plasma.ps");
		idFogSpotlightPS = shaders.CreatePixelShader("!fog.spotlight.ps");
		idFogAmbientPS = shaders.CreatePixelShader("!fog.ambient.ps");
		idObjSkyVS = shaders.CreateVertexShader("!skybox.vs", DX11::GetSkyVertexDesc(), DX11::NumberOfSkyVertexElements());
		idObjSkyPS = shaders.CreatePixelShader("!skybox.ps");

		shadowBufferId = textures.CreateDepthTarget("ShadowBuffer", 1024, 1024);

		D3D11_SAMPLER_DESC desc;
		GetSkySampler(desc);
		VALIDATEDX11(device.CreateSamplerState(&desc, &skySampler));

		alphaBlend = DX11::CreateAlphaBlend(device);

		rng.seed(123456U);
	}

	virtual ~DX11Pipeline()
	{

	}

	void AddFog(const ParticleVertex& p) override
	{
		fog.push_back(p);
	}

	void AddPlasma(const ParticleVertex& p) override
	{
		plasma.push_back(p);
	}

	void ClearPlasma() override
	{
		plasma.clear();
	}

	void ClearFog() override
	{
		fog.clear();
	}

	void Free() override
	{
		delete this;
	}

	void Draw(MeshBuffer& m, const ObjectInstance* instances, uint32 nInstances)
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

	void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID) override
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

	ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase)
	{
		switch (phaseConfig.EnvironmentalMap)
		{
		case Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE:
			switch (phase)
			{
			case RenderPhase_DetermineAmbient:
				return idObjAmbientPS;
			case RenderPhase_DetermineSpotlight:
				return idObjPS;
			case RenderPhase_DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		case Graphics::ENVIRONMENTAL_MAP_PROCEDURAL:
			switch (phase)
			{
			case RenderPhase_DetermineAmbient:
				return idObj_Ambient_NoEnvMap_PS;
			case RenderPhase_DetermineSpotlight:
				return idObj_Spotlight_NoEnvMap_PS;
			case RenderPhase_DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		default:
			Throw(0, "Environment mode %d not implemented", phaseConfig.EnvironmentalMap);
		}

		return ID_PIXEL_SHADER();
	}

	bool IsGuiReady() const
	{
		return !phaseConfig.renderTarget;
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities, %lld particles", trianglesThisFrame, entitiesThisFrame, plasma.size() + fog.size());
	}

	void DrawLightCone(const Light& light, cr_vec3 viewDir)
	{
		trianglesThisFrame += DX11::DrawLightCone(light, viewDir, dc, *lightConeBuffer);
	}

	void RenderToShadowBuffer(IShaders& shaders, IDX11TextureManager& textures, IRenderContext& rc, DepthRenderData& drd, ID_TEXTURE shadowBuffer, IScene& scene)
	{
		auto shadowBind = textures.GetTexture(shadowBuffer);

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

	void SetAmbientConstants()
	{
		AmbientData ad;
		ad.localLight = ambientLight.ambient;
		ad.fogConstant = ambientLight.fogConstant;
		DX11::CopyStructureToBuffer(dc, ambientBuffer, ad);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_AMBIENT_LIGHT, 1, &ambientBuffer);
	}

	Light ambientLight = { 0 };

	void RenderAmbient(IShaders& shaders, IRenderContext& rc, IScene& scene, const Light& ambientLight, const VertexTriangle* gui3DTriangles, size_t nTriangles)
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
			Render3DGui(gui3DTriangles, nTriangles);

			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			DrawParticles(fog.data(), fog.size(), idFogAmbientPS, idParticleVS, idFogAmbientGS);
		}

		phase = RenderPhase_None;
	}

	void RenderSkyBox(IScene& scene)
	{
		ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

		ID3D11ShaderResourceView* skyCubeTextureView = renderer.CubeTextures().GetShaderView(cubeId);
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
			dc.PSSetShaderResources(0, 1, &skyCubeTextureView);

			dc.PSSetSamplers(0, 1, &skySampler);

			dc.RSSetState(skyRasterizering);
			dc.OMSetDepthStencilState(noDepthTestOrWrite, 0);

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
			dc.Draw(mesh.numberOfVertices, 0);
			
			renderer.RestoreSamplers();
		}
		else
		{
			Throw(0, "DX11Renderer::RenderSkybox failed. Error setting sky shaders");
		}
	}

	void SetupSpotlightConstants() override
	{
		dc.VSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		dc.PSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
		dc.GSSetConstantBuffers(CBUFFER_INDEX_CURRENT_SPOTLIGHT, 1, &lightStateBuffer);
	}

	void RenderSpotlightLitScene(const Light& lightSubset, IScene& scene, const VertexTriangle* gui3DTriangles, size_t nTriangles)
	{
		Light light = lightSubset;

		DepthRenderData drd;
		if (PrepareDepthRenderFromLight(light, drd))
		{
			const float f = 1.0f / rng.max();
			drd.randoms.x = rng() * f;
			drd.randoms.y = rng() * f;
			drd.randoms.z = rng() * f;
			drd.randoms.w = rng() * f;

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

			dc.PSSetShaderResources(2, 1, &shadowBind.shaderView);
			dc.RSSetState(objectRasterizering);

			scene.RenderObjects(rc, false);
			scene.RenderObjects(rc, true);

			Render3DGui(gui3DTriangles, nTriangles);

			dc.OMSetBlendState(alphaAdditiveBlend, blendFactorUnused, 0xffffffff);
			DrawParticles(fog.data(), fog.size(), idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
			phase = RenderPhase_None;

			dc.OMSetDepthStencilState(objDepthState, 0);
		}
	}

	void Render3DGui(const VertexTriangle* gui3DTriangles, size_t nTriangles)
	{
		size_t cursor = 0;

		ObjectInstance one = { Matrix4x4::Identity(), RGBA(1.0f, 1.0f, 1.0f, 1.0f) };

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

	OS::ticks objCost = 0;

	void Render3DObjects(IScene& scene, const VertexTriangle* gui3DTriangles, size_t nTriangles)
	{
		auto now = OS::CpuTicks();

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
					RenderSpotlightLitScene(lights[i], scene, gui3DTriangles, nTriangles);
				}
				catch (IException& ex)
				{
					Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
				}
			}

			RenderAmbient(shaders, rc, scene, lights[0], gui3DTriangles, nTriangles);
		}

		objCost = OS::CpuTicks() - now;
	}

	ID_TEXTURE lastTextureId;

	RenderTarget GetCurrentRenderTarget()
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

	void ClearCurrentRenderBuffers(const RGBA& clearColour)
	{
		RenderTarget rt = GetCurrentRenderTarget();
		dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		dc.ClearDepthStencilView(rt.depthView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		if (clearColour.alpha > 0)
		{
			dc.ClearRenderTargetView(rt.renderTargetView, (const FLOAT*)&clearColour);
		}
	}

	void Render(Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene, const VertexTriangle* gui3DTriangles, size_t nTriangles) override
	{
		phaseConfig.EnvironmentalMap = envMap;
		phaseConfig.shadowBuffer = shadowBufferId;
		phaseConfig.depthTarget = renderer.GetMainDepthBufferId();

		if (textures.GetTexture(phaseConfig.shadowBuffer).depthView == nullptr)
		{
			Throw(0, "No shadow depth buffer set for DX1AppRenderer::Render(...)");
		}

		trianglesThisFrame = 0;
		entitiesThisFrame = 0;

		lastTextureId = ID_TEXTURE::Invalid();

		renderer.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

		ClearCurrentRenderBuffers(scene.GetClearColour());

		renderer.UpdateGlobalState(scene);

		RenderTarget rt = GetCurrentRenderTarget();
		dc.OMSetRenderTargets(1, &rt.renderTargetView, rt.depthView);

		RenderSkyBox(scene);

		renderer.InitFontAndMaterialAndSpriteShaderResourceViewsAndSamplers();

		Render3DObjects(scene, gui3DTriangles, nTriangles);

		FLOAT blendFactorUnused[] = { 0,0,0,0 };
		dc.OMSetBlendState(plasmaBlend, blendFactorUnused, 0xffffffff);
		DrawParticles(plasma.data(), plasma.size(), idPlasmaPS, idParticleVS, idPlasmaGS);

		DrawLightCones(scene);

		renderer.UpdateGlobalState(scene);

		renderer.RenderGui(scene);
	}

	void DrawLightCones(IScene& scene)
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

			renderer.AssignGlobalStateBufferToShaders();

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

}; // DX11Pipeline

namespace Rococo::DX11
{
	IDX11Pipeline* CreateDX11Pipeline(IInstallation& installation, IDX11Shaders& shaders, IDX11TextureManager& textures, IDX11Meshes& meshes, IDX11Renderer& renderer, IRenderContext& rc, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Pipeline(installation, shaders, textures, meshes, renderer, rc, device, dc);
	}
}

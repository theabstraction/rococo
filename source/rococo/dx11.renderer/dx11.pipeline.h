#pragma once

#include <vector>

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

	// We break coding conventions here for Rococo, by putting an implementation file in the header and also including a std header in a header
	// This was a result of chasing down an unreachable code warning message.
	// I choose to keep this organization as it makes developing the pipeline a little more manageable. Only two files include the header, so there is no significant cost 
	// for the layout of the class in the files.
	struct DX11Pipeline : IDX11Pipeline, IGui3D, IParticles
	{
		IO::IInstallation& installation;
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

		AutoFree<IDX11Gui> gui;

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
		//AutoRelease<ID3D11SamplerState> envSampler;
		//AutoRelease<ID3D11SamplerState> skySampler;

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
		AutoRelease<ID3D11Buffer> globalStateBuffer;
		AutoRelease<ID3D11Buffer> sunlightStateBuffer;

		BoneMatrices boneMatrices = { 0 };
		AutoRelease<ID3D11Buffer> boneMatricesStateBuffer;

		ID3D11SamplerState* samplers[16] = { 0 };

		enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };
		enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };
		enum { GUI3D_BUFFER_VERTEX_CAPACITY = 3 * GUI3D_BUFFER_TRIANGLE_CAPACITY };

		ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase);	LightConstantBuffer ambientLight = { 0 };
		GlobalState currentGlobalState = { 0 };
		Time::ticks objCost = 0;
		ID_TEXTURE lastTextureId;
		int64 guiCost = 0;

		DX11Pipeline(DX11::RenderBundle& bundle);

		virtual ~DX11Pipeline()
		{
			for (auto& s : samplers)
			{
				if (s) s->Release();
			}
		}

		IGui3D& Gui3D() override
		{
			return *this;
		}

		IParticles& Particles() override
		{
			return *this;
		}

		void Add3DGuiTriangles(const VertexTriangle* first, const VertexTriangle* last);
		void Clear3DGuiTriangles();
		void AddFog(const ParticleVertex& p) override;
		void AddPlasma(const ParticleVertex& p) override;
		void ClearPlasma() override;
		void ClearFog() override;
		void Free() override;
		void AssignGlobalStateBufferToShaders();
		void Draw(MeshBuffer& m, const ObjectInstance* instances, uint32 nInstances);
		void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID);

		bool IsGuiReady() const
		{
			return !phaseConfig.renderTarget;
		}

		void ShowVenue(IMathsVisitor& visitor) override;
		void DrawLightCone(const LightConstantBuffer& light, cr_vec3 viewDir);
		void RenderToShadowBuffer(IShaders& shaders, IDX11TextureManager& textures, IRenderContext& rc, DepthRenderData& drd, ID_TEXTURE shadowBuffer, IScene& scene);
		void SetAmbientConstants();
		void RenderAmbient(IShaders& shaders, IRenderContext& rc, IScene& scene, const LightConstantBuffer& ambientLight);
		void RenderSkyBox(IScene& scene);
		void UpdateGlobalState(const GuiMetrics& metrics, IScene& scene);
		void SetupSpotlightConstants();
		void RenderSpotlightLitScene(const LightConstantBuffer& lightSubset, IScene& scene);
		void Render3DGui(const VertexTriangle* gui3DTriangles, size_t nTriangles);
		void Render3DObjects(IScene& scene);
		RenderTarget GetCurrentRenderTarget();
		void ClearCurrentRenderBuffers(const RGBA& clearColour);
		void SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour) override;
		void ResetSamplersToDefaults();

		// Main entrypoint for the render pipeline
		void Render(const GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene) override;

		void DrawLightCones(IScene& scene);
		void SetBoneMatrix(uint32 index, cr_m4x4 m) override;

		IGuiResources& Gui()
		{
			return gui->Gui();
		}
	}; // DX11Pipeline
}
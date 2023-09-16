#pragma once

#include <vector>
#include <RAL/RAL.pipeline.h>

using namespace Rococo::RAL;

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
	struct DX11Pipeline : IDX11Pipeline, RAL::IRenderStates
	{
		AutoFree<RAL::IPipelineSupervisor> RAL_pipeline;

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

		AutoRelease<ID3D11BlendState> alphaAdditiveBlend;
		AutoRelease<ID3D11BlendState> disableBlend;
		AutoRelease<ID3D11BlendState> additiveBlend;
		AutoRelease<ID3D11BlendState> plasmaBlend;

		AutoRelease<ID3D11DepthStencilState> objDepthState;
		AutoRelease<ID3D11DepthStencilState> objDepthState_NoWrite;
		AutoRelease<ID3D11DepthStencilState> noDepthTestOrWrite;

		AutoFree<IDX11Gui> gui;

		ID_VERTEX_SHADER idObjVS;
		ID_PIXEL_SHADER idObjPS;
		ID_PIXEL_SHADER idObjPS_Shadows;
		ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
		ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;
		ID_PIXEL_SHADER idObjAmbientPS;
		ID_VERTEX_SHADER idObjAmbientVS;
		ID_VERTEX_SHADER idObjVS_Shadows;
		ID_VERTEX_SHADER idSkinnedObjVS_Shadows;

		//AutoRelease<ID3D11SamplerState> envSampler;
		//AutoRelease<ID3D11SamplerState> skySampler;

		AutoRelease<ID3D11BlendState> alphaBlend;

		RenderPhase phase = RenderPhase::None;

		int64 entitiesThisFrame = 0;
		int64 trianglesThisFrame = 0;
		bool builtFirstPass = false;

		Graphics::RenderPhaseConfig phaseConfig;

		ID3D11SamplerState* samplers[16] = { 0 };

		ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase);	
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

		RAL::IPipeline& RALPipeline() override
		{
			return *RAL_pipeline;
		}

		void DisableBlend() override;
		void UseAdditiveBlend() override;
		void UseAlphaBlend() override;
		void UseAlphaAdditiveBlend() override;
		void DisableWritesOnDepthState() override;
		void UseParticleRasterizer() override;
		void UsePlasmaBlend() override;
		void UseSkyRasterizer() override;
		void UseSpriteRasterizer() override;

		void SetDrawTopology(PrimitiveTopology topology) override;
		void SetShaderTexture(uint32 textureUnitIndex, Rococo::ID_CUBE_TEXTURE cubeId) override;

		IGui3D& Gui3D() override
		{
			return RAL_pipeline->Gui3D();
		}

		IGuiRenderContext& Gui() override
		{
			return *gui;
		}

		IParticles& Particles() override
		{
			return RAL_pipeline->Particles();
		}

		void Free() override;
		void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID);

		bool IsGuiReady() const
		{
			return !phaseConfig.renderTarget;
		}

		void ShowVenue(IMathsVisitor& visitor) override;
		void RenderToShadowBuffer(IShaders& shaders, IDX11TextureManager& textures, IRenderContext& rc, DepthRenderData& drd, ID_TEXTURE shadowBuffer, IScene& scene);
		void RenderAmbient(IShaders& shaders, IRenderContext& rc, IScene& scene, const LightConstantBuffer& ambientLight);
		void RenderSkyBox(IScene& scene);
		void UpdateGlobalState(const GuiMetrics& metrics, IScene& scene);
		void RenderSpotlightLitScene(const LightConstantBuffer& lightSubset, IScene& scene);
		void Render3DObjects(IScene& scene);
		RenderTarget GetCurrentRenderTarget();
		void ClearCurrentRenderBuffers(const RGBA& clearColour);
		void SetSampler(uint32 index, Filter filter, AddressMode u, AddressMode v, AddressMode w, const RGBA& borderColour) override;
		void ResetSamplersToDefaults();

		// Main entrypoint for the render pipeline
		void Render(const GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, IScene& scene) override;

		void SetBoneMatrix(uint32 index, cr_m4x4 m) override;

		IGuiResources& GuiResources() override
		{
			return gui->Resources();
		}
	}; // DX11Pipeline
}
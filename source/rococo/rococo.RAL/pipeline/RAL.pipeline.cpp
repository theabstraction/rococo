#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <RAL/ral.h>
#include <RAL/RAL.pipeline.h>
#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <rococo.visitors.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace Rococo::RAL::Anon
{
	// struct VertexElement	{ cstr SemanticName; uint32 semanticIndex; VertexElementFormat format; uint32 slot;	};

	VertexElement objectVertexElements[] =
	{
		VertexElement { "position", 0, VertexElementFormat::Float3, 0 },
		VertexElement { "normal",   0, VertexElementFormat::Float3, 0 },
		VertexElement { "texcoord", 0, VertexElementFormat::Float2, 0 },
		VertexElement { "color",	0, VertexElementFormat::RGBA8U, 0 },
		VertexElement { "texcoord",	1, VertexElementFormat::Float2, 0 },
		VertexElement { nullptr,    0 ,VertexElementFormat::Float3, 0 }
	};

	struct RALPipeline: IPipelineSupervisor, IRenderPhases
	{
		IRAL& ral;
		IRenderStates& renderStates;

		AutoFree<IParticlesSupervisor> particles;
		AutoFree<IRAL_LightCones> lightCones;
		AutoFree<IRAL_Skybox> skybox;
		AutoFree<IGui3DSupervisor> gui3D;
		AutoFree<IRAL_3D_Object_Renderer> objectRenderer;

		AutoFree<IRALConstantDataBuffer> globalStateBuffer;
		AutoFree<IRALConstantDataBuffer> sunlightStateBuffer;
		AutoFree<IRALConstantDataBuffer> ambientBuffer;
		AutoFree<IRALConstantDataBuffer> boneMatricesStateBuffer;

		ID_TEXTURE shadowBufferId;

		GlobalState currentGlobalState;

		BoneMatrices boneMatrices = { 0 };

		Graphics::RenderOutputTargets outputTargets;

		Time::ticks guiCost = 0;

		RALPipeline(IRenderStates& _renderStates, IRAL& _ral): 
			renderStates(_renderStates), ral(_ral)
		{
			globalStateBuffer = ral.CreateConstantBuffer(sizeof GlobalState, 1);
			sunlightStateBuffer = ral.CreateConstantBuffer(sizeof Vec4, 1);
			ambientBuffer = ral.CreateConstantBuffer(sizeof AmbientData, 1);
			boneMatricesStateBuffer = ral.CreateConstantBuffer(sizeof BoneMatrices, 1);

			SetSamplerDefaults();

			// TODO - make this dynamic
			shadowBufferId = ral.RALTextures().CreateDepthTarget("ShadowBuffer", 2048, 2048);
			particles = CreateParticleSystem(_ral, _renderStates);
			lightCones = CreateLightCones(_ral, _renderStates, *this);
			skybox = CreateRALSkybox(_ral, _renderStates);
			gui3D = CreateGui3D(_ral, _renderStates, *this);
			objectRenderer = CreateRAL_3D_Object_Renderer(_ral, _renderStates, *this, *this);
		}

		Rococo::Graphics::IGui3D& Gui3D() override
		{
			return *gui3D;
		}

		Rococo::Graphics::IParticles& Particles() override
		{
			return *particles;
		}

		void Free() override
		{
			delete this;
		}

		void SetSamplerDefaults()
		{
			RGBA red{ 1.0f, 0, 0, 1.0f };
			RGBA transparent{ 0.0f, 0, 0, 0.0f };
			renderStates.SetSamplerDefaults(TXUNIT_FONT, Samplers::Filter_Linear, Samplers::AddressMode_Border, Samplers::AddressMode_Border, Samplers::AddressMode_Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_SHADOW, Samplers::Filter_Linear, Samplers::AddressMode_Border, Samplers::AddressMode_Border, Samplers::AddressMode_Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_ENV_MAP, Samplers::Filter_Anisotropic, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_SELECT, Samplers::Filter_Linear, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_MATERIALS, Samplers::Filter_Linear, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, Samplers::AddressMode_Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_SPRITES, Samplers::Filter_Point, Samplers::AddressMode_Border, Samplers::AddressMode_Border, Samplers::AddressMode_Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_GENERIC_TXARRAY, Samplers::Filter_Point, Samplers::AddressMode_Border, Samplers::AddressMode_Border, Samplers::AddressMode_Border, transparent);
		}

		void AssignGlobalStateBufferToShaders()
		{
			globalStateBuffer->AssignToGS(CBUFFER_INDEX_GLOBAL_STATE);
			globalStateBuffer->AssignToPS(CBUFFER_INDEX_GLOBAL_STATE);
			globalStateBuffer->AssignToVS(CBUFFER_INDEX_GLOBAL_STATE);
		}

		void AssignAmbientLightToShaders(const Rococo::Graphics::LightConstantBuffer& ambientLight) override
		{
			AmbientData ad;
			ad.localLight = ambientLight.ambient;
			ad.fogConstant = ambientLight.fogConstant;
			ambientBuffer->CopyDataToBuffer(&ad, sizeof ad);
			ambientBuffer->AssignToPS(CBUFFER_INDEX_AMBIENT_LIGHT);
		}

		void UpdateGlobalState(const GuiMetrics& metrics, IScene& scene) override
		{
			GlobalState g;
			scene.GetCamera(g.worldMatrixAndProj, g.worldMatrix, g.projMatrix, g.eye, g.viewDir);

			float aspectRatio = metrics.screenSpan.y / (float)metrics.screenSpan.x;
			g.aspect = { aspectRatio,0,0,0 };

			TextureDesc desc;
			if (ral.RALTextures().TryGetTextureDesc(OUT desc, shadowBufferId))
			{
				g.OOShadowTxWidth = 1.0f / desc.width;
			}
			else
			{
				g.OOShadowTxWidth = 1.0f;
			}

			g.unused = Vec3{ 0.0f,0.0f,0.0f };
			g.guiScale = renderStates.Gui().GetGuiScale();

			globalStateBuffer->CopyDataToBuffer(&g, sizeof g);

			currentGlobalState = g;

			AssignGlobalStateBufferToShaders();
			UpdateSunlight();
			UpdateBoneMatrices();
		}

		void UpdateBoneMatrices()
		{
			boneMatricesStateBuffer->CopyDataToBuffer(&boneMatrices, sizeof boneMatrices);
			boneMatricesStateBuffer->AssignToVS(CBUFFER_INDEX_BONE_MATRICES);
		}

		void UpdateSunlight()
		{
			Vec4 sunlight = { Sin(45_degrees), 0, Cos(45_degrees), 0 };
			sunlightStateBuffer->CopyDataToBuffer(&sunlight, sizeof sunlight);
			sunlightStateBuffer->AssignToGS(CBUFFER_INDEX_SUNLIGHT);
			sunlightStateBuffer->AssignToPS(CBUFFER_INDEX_SUNLIGHT);
			sunlightStateBuffer->AssignToVS(CBUFFER_INDEX_SUNLIGHT);
		}

		void SetBoneMatrix(uint32 index, cr_m4x4 m) override
		{
			if (index >= BoneMatrices::BONE_MATRIX_CAPACITY)
			{
				Throw(0, "Bad bone index #%u", index);
			}

			auto& target = boneMatrices.bones[index];
			target = m;
			target.row3 = Vec4{ 0, 0, 0, 1.0f };
		}

		ID_TEXTURE ShadowBufferId() const override
		{
			return shadowBufferId;
		}

		void Draw(RALMeshBuffer& m, const ObjectInstance* instances, uint32 nInstances) override
		{
			objectRenderer->Draw(m, instances, nInstances);
		}

		void RenderAmbientPhase(IScene& scene, const LightConstantBuffer& ambientLight) override
		{
			renderStates.UseObjectRasterizer();

			AssignAmbientLightToShaders(ambientLight);

			scene.RenderObjects(ral.RenderContext(), false);
			scene.RenderObjects(ral.RenderContext(), true);

			gui3D->Render3DGui();
			particles->RenderFogWithAmbient();
		}

		void RenderSpotlightPhase(IScene& scene)
		{
			renderStates.UseObjectRasterizer();

			scene.RenderObjects(ral.RenderContext(), false);
			scene.RenderObjects(ral.RenderContext(), true);

			gui3D->Render3DGui();
			particles->RenderFogWithSpotlight();
		}

		bool IsGuiReady() const
		{
			return !outputTargets.renderTarget;
		}

		void Render(const Rococo::Graphics::GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP_TYPE envMapType, Rococo::Graphics::IScene& scene)
		{
			outputTargets.depthTarget = ral.GetWindowDepthBufferId();

			ID_CUBE_TEXTURE envId = scene.GetEnvironmentMap();
			ral.SetEnvironmentMap(envId);

			ral.ExpandViewportToEntireTexture(outputTargets.depthTarget);

			renderStates.SetAndClearCurrentRenderBuffers(scene.GetClearColour(), outputTargets);

			UpdateGlobalState(metrics, scene);

			renderStates.ResetSamplersToDefaults();

			skybox->RenderSkyBox(scene);

			renderStates.AssignGuiShaderResources();
			renderStates.ResetSamplersToDefaults();

			objectRenderer->Render3DObjects(scene, outputTargets, envMapType);

			particles->RenderPlasma();
			lightCones->DrawLightCones(scene);

			UpdateGlobalState(metrics, scene);

			Time::ticks now = Time::TickCount();

			renderStates.Gui().RenderGui(scene, metrics, IsGuiReady());

			guiCost = Time::TickCount() - now;
		}

		void ShowVenue(IMathsVisitor& visitor)
		{
			objectRenderer->ShowVenue(visitor);
		}
	};
}

namespace Rococo::RAL
{
	RAL_PIPELINE_API IPipelineSupervisor* CreatePipeline(IRenderStates& renderStates, IRAL& ral)
	{
		return new Anon::RALPipeline(renderStates, ral);
	}

	RAL_PIPELINE_API const VertexElement* GetObjectVertexElements()
	{
		return Anon::objectVertexElements;
	}
}
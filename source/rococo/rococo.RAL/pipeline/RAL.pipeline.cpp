#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <RAL/ral.h>
#include <RAL/RAL.pipeline.h>
#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <rococo.visitors.h>
#include <rococo.reflector.h>
#include <rococo.subsystems.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Reflection;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Samplers;

namespace Rococo::RAL::Anon
{
	struct RALPipeline: IPipelineSupervisor, IRenderPhases, ISubsystem, IReflectionTarget
	{
		IRAL& ral;
		IRenderStates& renderStates;

		AutoFree<IParticlesSupervisor> particles;
		AutoFree<IGui3DSupervisor> gui3D;
		AutoFree<IRAL_LightCones> lightCones;
		AutoFree<IRAL_Skybox> skybox;
		AutoFree<IRAL_3D_Object_RendererSupervisor> objectRenderer;
		AutoFree<IRAL_BoneStateBufferSupervisor> boneBuffer;

		AutoFree<IRALConstantDataBuffer> globalStateBuffer;
		AutoFree<IRALConstantDataBuffer> sunlightStateBuffer;
		AutoFree<IRALConstantDataBuffer> ambientBuffer;

		GlobalState currentGlobalState;

		Graphics::RenderOutputTargets outputTargets;

		Time::Timer guiRenderTimer = "guiRenderTimer";
		Time::Timer plasmaRenderTimer = "plasmaRenderTimer";
		Time::Timer objectRenderTimer = "objectRenderTimer";
		Time::Timer objectRendererInitTime = "objectRendererInitTime";

		ID_TEXTURE noisePermutationTextureId;
		ID_TEXTURE noiseGradientTextureId;

		RALPipeline(IRenderStates& _renderStates, IRAL& _ral): 
			renderStates(_renderStates), ral(_ral)
		{
			globalStateBuffer = ral.CreateConstantBuffer(sizeof GlobalState, 1);
			sunlightStateBuffer = ral.CreateConstantBuffer(sizeof Vec4, 1);
			ambientBuffer = ral.CreateConstantBuffer(sizeof AmbientData, 1);

			SetSamplerDefaults();

			particles = CreateParticleSystem(_ral, _renderStates);
			lightCones = CreateLightCones(_ral, _renderStates, *this);
			skybox = CreateRALSkybox(_ral, _renderStates);
			gui3D = CreateGui3D(_ral, _renderStates, *this);
			TIME_FUNCTION_CALL(objectRendererInitTime, objectRenderer = CreateRAL_3D_Object_G_Buffer_Renderer(_ral, _renderStates, *this, *this));
		//	TIME_FUNCTION_CALL(objectRendererInitTime, objectRenderer = CreateRAL_3D_Object_Forward_Renderer(_ral, _renderStates, *this, *this));
			boneBuffer = CreateRALBoneStateBuffer(_ral, _renderStates);

			// Comment out InitResources(1) to force some initialization during the first frame. This allows initialization to be profiles in graphics debuggers.
		//	InitResources(1);
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

		void InitResources(int64 frameIndex)
		{
			// We init some stuff here, between Present calls, to allow graphics debuggers analyis of initialization calls as part of the frame capture.
			if (frameIndex > 0 && !noisePermutationTextureId)
			{
				noisePermutationTextureId = RAL::GeneratePermuationTexture(ral);
				noiseGradientTextureId = RAL::GenerateGradientTexture(ral);
			}
			
			if (noisePermutationTextureId) ral.RALTextures().AssignToPS(TXUNIT_NOISE_PERMUTATION, noisePermutationTextureId);
			if (noiseGradientTextureId) ral.RALTextures().AssignToPS(TXUNIT_NOISE_GRADIENT_LOOKUP, noiseGradientTextureId);
		}

		void SetSamplerDefaults()
		{
			RGBA red{ 1.0f, 0, 0, 1.0f };
			RGBA transparent{ 0.0f, 0, 0, 0.0f };
			renderStates.SetSamplerDefaults(TXUNIT_FONT, Filter::Linear, AddressMode::Border, AddressMode::Border, AddressMode::Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_SHADOW, Filter::Linear, AddressMode::Border, AddressMode::Border, AddressMode::Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_ENV_MAP, Filter::Anisotropic, AddressMode::Wrap, AddressMode::Wrap, AddressMode::Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_SELECT, Filter::Linear, AddressMode::Wrap, AddressMode::Wrap, AddressMode::Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_MATERIALS, Filter::Linear, AddressMode::Wrap, AddressMode::Wrap, AddressMode::Wrap, red);
			renderStates.SetSamplerDefaults(TXUNIT_SPRITES, Filter::Point, AddressMode::Border, AddressMode::Border, AddressMode::Border, red);
			renderStates.SetSamplerDefaults(TXUNIT_GENERIC_TXARRAY, Filter::Point, AddressMode::Border, AddressMode::Border, AddressMode::Border, transparent);
			renderStates.SetSamplerDefaults(TXUNIT_NOISE_PERMUTATION, Filter::Point, AddressMode::Wrap, AddressMode::Wrap, AddressMode::Clamp, RGBA(0, 0, 0, 1.0f));
			renderStates.SetSamplerDefaults(TXUNIT_NOISE_GRADIENT_LOOKUP, Filter::Point, AddressMode::Wrap, AddressMode::Clamp, AddressMode::Clamp, RGBA(0, 0, 0, 1.0f));
		}

		void AssignGlobalStateBufferToShaders()
		{
			globalStateBuffer->AssignToGS(CBUFFER_INDEX_GLOBAL_STATE);
			globalStateBuffer->AssignToPS(CBUFFER_INDEX_GLOBAL_STATE);
			globalStateBuffer->AssignToVS(CBUFFER_INDEX_GLOBAL_STATE);
		}

		void AssignAmbientLightToShaders(const Rococo::Graphics::LightConstantBuffer& ambientLight)
		{
			AmbientData ad;
			ad.localLight = ambientLight.ambient;
			ad.fogConstant = ambientLight.fogConstant;
			ambientBuffer->CopyDataToBuffer(&ad, sizeof ad);
			ambientBuffer->AssignToPS(CBUFFER_INDEX_AMBIENT_LIGHT);
		}

		void UpdateGlobalState(const GuiMetrics& metrics, IScene& scene)
		{
			GlobalState g;
			scene.GetCamera(g.worldMatrixAndProj, g.worldMatrix, g.projMatrix, g.eye, g.viewDir);

			float aspectRatio = metrics.screenSpan.y / (float)metrics.screenSpan.x;
			g.aspect = { aspectRatio,0,0,0 };
			g.guiScale = renderStates.Gui().GetGuiScale();

			globalStateBuffer->CopyDataToBuffer(&g, sizeof g);

			currentGlobalState = g;

			AssignGlobalStateBufferToShaders();
			UpdateSunlight();
			boneBuffer->SyncToGPU();
		}

		void UpdateSunlight()
		{
			// Used in landscape.ps
			Vec4 sunlight = { Sin(45_degrees), 0, Cos(45_degrees), 0 };
			sunlightStateBuffer->CopyDataToBuffer(&sunlight, sizeof sunlight);
			sunlightStateBuffer->AssignToGS(CBUFFER_INDEX_SUNLIGHT);
			sunlightStateBuffer->AssignToPS(CBUFFER_INDEX_SUNLIGHT);
			sunlightStateBuffer->AssignToVS(CBUFFER_INDEX_SUNLIGHT);
		}

		void DrawViaObjectRenderer(RALMeshBuffer& m, const ObjectInstance* instances, uint32 nInstances) override
		{
			objectRenderer->Draw(m, instances, nInstances);
		}

		void RenderIlluminatedEntites(IScene& scene)
		{
			renderStates.UseObjectRasterizer();

			scene.RenderObjects(ral.RenderContext(), EShadowCasterFilter::UnskinnedCastersOnly);
			scene.RenderObjects(ral.RenderContext(), EShadowCasterFilter::SkinnedCastersOnly);

			gui3D->Render3DGui();
			particles->RenderFogWithSpotlight();
		}

		void RenderAmbientPhase(IScene& scene, const LightConstantBuffer& ambientLight) override
		{
			AssignAmbientLightToShaders(ambientLight);
			RenderIlluminatedEntites(scene);
		}

		void RenderToGBuffers(Rococo::Graphics::IScene& scene)
		{
			renderStates.UseObjectRasterizer();



			scene.RenderObjects(ral.RenderContext(), EShadowCasterFilter::UnskinnedCastersOnly);
			scene.RenderObjects(ral.RenderContext(), EShadowCasterFilter::SkinnedCastersOnly);

			gui3D->Render3DGui();
		}

		void RenderSpotlightPhase(IScene& scene)
		{
			RenderIlluminatedEntites(scene);
		}

		bool IsRenderingToWindow() const
		{
			return !outputTargets.renderTarget;
		}

		// Entry point for rendering
		void RenderLayers(const Rococo::Graphics::GuiMetrics& metrics, Rococo::Graphics::IScene& scene) override
		{
			outputTargets.depthTarget = ral.GetWindowDepthBufferId();

			ID_CUBE_TEXTURE envId = scene.GetEnvironmentMap();
			ral.SetEnvironmentMap(envId);

			TextureDesc desc;
			ral.RALTextures().TryGetTextureDesc(OUT desc, outputTargets.depthTarget);

			ral.ExpandViewportToEntireSpan({ (int)desc.width, (int)desc.height });

			renderStates.SetAndClearCurrentRenderBuffers(scene.GetClearColour(), outputTargets);

			UpdateGlobalState(metrics, scene);

			renderStates.ResetSamplersToDefaults();
			renderStates.AssignGuiShaderResources();

			InitResources(metrics.frameIndex);

			TIME_FUNCTION_CALL(objectRenderTimer, objectRenderer->Render3DObjects(scene, outputTargets, *skybox));

			renderStates.AssignGuiShaderResources();
			renderStates.ResetSamplersToDefaults();

			lightCones->DrawLightCones(scene);

			TIME_FUNCTION_CALL(plasmaRenderTimer, particles->RenderPlasma());

			if (IsRenderingToWindow())
			{
				TIME_FUNCTION_CALL(guiRenderTimer, renderStates.Gui().RenderGui(scene, metrics));
			}
		}

		void ShowVenue(IMathsVisitor& visitor)
		{
			objectRenderer->ShowVenue(visitor);
		}

		void SetBoneMatrix(uint32 index, cr_m4x4 m) override
		{
			boneBuffer->SetBoneMatrix(index, m);
		}

		void RegisterSubsystem(ISubsystemMonitor&, ID_SUBSYSTEM) override
		{
			
		}

		cstr SubsystemName() const override
		{
			return "RALPipeline";
		}

		IReflectionTarget* ReflectionTarget() override
		{
			return this;
		}

		void Visit(IReflectionVisitor&) override
		{

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
		static const VertexElement objectVertexElements[] =
		{
			// struct VertexElement	{ cstr SemanticName; uint32 semanticIndex; VertexElementFormat format; uint32 slot;	};
			VertexElement { "position", 0, VertexElementFormat::Float3, 0 },
			VertexElement { "normal",   0, VertexElementFormat::Float3, 0 },
			VertexElement { "texcoord", 0, VertexElementFormat::Float2, 0 },
			VertexElement { "color",	0, VertexElementFormat::RGBA8U, 0 },
			VertexElement { "texcoord",	1, VertexElementFormat::Float2, 0 },
			VertexElement { nullptr,    0 ,VertexElementFormat::Float3, 0 }
		};

		return objectVertexElements;
	}
}
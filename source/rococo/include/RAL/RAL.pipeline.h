#pragma once

#include <rococo.types.h>

#ifndef RAL_PIPELINE_API
# define RAL_PIPELINE_API
#endif

namespace Rococo
{
	struct ID_CUBE_TEXTURE;
	struct IMathsVisitor;
}

namespace Rococo::Graphics
{
	struct DepthRenderData;
	struct VertexElement;
	struct VertexTriangle;
	struct IGui3D;
	struct IGui3DSupervisor;
	struct IParticles;
	struct IParticlesSupervisor;
	struct GuiMetrics;
	struct IScene;
	struct IGuiRenderContextSupervisor;
	struct LightConstantBuffer;
	struct RenderOutputTargets;
	enum class ENVIRONMENTAL_MAP_TYPE;

	namespace Samplers
	{
		enum class Filter : int32;
		enum class AddressMode : int32;
	}
}

namespace Rococo::RAL
{
	struct IRAL;
	struct RALMeshBuffer;

	enum class PrimitiveTopology : uint32;

	enum class RenderPhase: uint32
	{
		None,
		DetermineShadowVolumes,
		DetermineSpotlight,
		DetermineAmbient
	};

	ROCOCO_INTERFACE IRenderStates
	{
		virtual void AssignGuiShaderResources() = 0;
		virtual void DisableBlend() = 0;
		virtual void DisableWritesOnDepthState() = 0;

		virtual void ResetSamplersToDefaults() = 0;

		virtual void SetAndClearCurrentRenderBuffers(const RGBA& clearColour, const Rococo::Graphics::RenderOutputTargets& targets) = 0;
		virtual void SetDrawTopology(PrimitiveTopology topology) = 0;
		virtual void SetSamplerDefaults(uint32 index, Rococo::Graphics::Samplers::Filter filter, Rococo::Graphics::Samplers::AddressMode u, Rococo::Graphics::Samplers::AddressMode v, Rococo::Graphics::Samplers::AddressMode w, const RGBA& borderColour) = 0;
		virtual void SetShaderTexture(uint32 textureUnitIndex, Rococo::ID_CUBE_TEXTURE cubeId) = 0;

		virtual void TargetShadowBuffer(ID_TEXTURE id) = 0;

		virtual void UseAdditiveBlend() = 0;
		virtual void UseAlphaBlend() = 0;
		virtual void UseAlphaAdditiveBlend() = 0;
		virtual void UseObjectRasterizer() = 0;
		virtual void UseObjectDepthState() = 0;
		virtual void UseParticleRasterizer() = 0;
		virtual void UsePlasmaBlend() = 0;
		virtual void UseSkyRasterizer() = 0;
		virtual void UseSpriteRasterizer() = 0;

		virtual Rococo::Graphics::IGuiRenderContextSupervisor& Gui() = 0;
	};

	// IPipeline orders rendering calls to properly format the video output
	ROCOCO_INTERFACE IPipeline
	{
		virtual void AssignGlobalStateBufferToShaders() = 0;
		virtual void DrawViaObjectRenderer(RALMeshBuffer& m, const Rococo::Graphics::ObjectInstance* instances, uint32 nInstances) = 0;
		virtual void RenderLayers(const Rococo::Graphics::GuiMetrics& metrics, Rococo::Graphics::IScene& scene) = 0;
		virtual void SetBoneMatrix(uint32 index, cr_m4x4 m) = 0;

		virtual Rococo::Graphics::IGui3D& Gui3D() = 0;
		virtual Rococo::Graphics::IParticles& Particles() = 0;
	};

	ROCOCO_INTERFACE IRAL_LightCones
	{
		virtual void DrawLightCones(Rococo::Graphics::IScene & scene) = 0;
		virtual void Free() = 0;
	};

	RAL_PIPELINE_API IRAL_LightCones* CreateLightCones(IRAL& ral, IRenderStates& renderStates, IPipeline& pipeline);

	ROCOCO_INTERFACE IPipelineSupervisor : IPipeline
	{
		virtual void Free() = 0;
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
		virtual void RegisterSubsystem(ISubsystemMonitor& monitor, ID_SUBSYSTEM parentId) = 0;
	};

	RAL_PIPELINE_API IPipelineSupervisor* CreatePipeline(IRenderStates& renderStates, IRAL& ral);

	RAL_PIPELINE_API Rococo::Graphics::IParticlesSupervisor* CreateParticleSystem(IRAL& ral, IRenderStates& renderStates);

	RAL_PIPELINE_API const Rococo::Graphics::VertexElement* GetObjectVertexElements();

	ROCOCO_INTERFACE IRAL_Skybox
	{
		virtual void Free() = 0;
		virtual void DrawSkyBox(Rococo::Graphics::IScene& scene) = 0;
	};

	RAL_PIPELINE_API IRAL_Skybox* CreateRALSkybox(IRAL& ral, IRenderStates& renderStates);
	RAL_PIPELINE_API Rococo::Graphics::IGui3DSupervisor* CreateGui3D(IRAL& ral, IRenderStates& renderStates, IPipeline& pipeline);

	ROCOCO_INTERFACE IRenderPhases
	{
		virtual void RenderAmbientPhase(Rococo::Graphics::IScene& scene, const Rococo::Graphics::LightConstantBuffer & ambientLight) = 0;
		virtual void RenderToGBuffers(Rococo::Graphics::IScene& scene) = 0;
		virtual void RenderSpotlightPhase(Rococo::Graphics::IScene& scene) = 0;
	};

	ROCOCO_INTERFACE IRAL_3D_Object_Renderer
	{
		// Invoked by the pipeline for every object in the scene. This may be called multiple times per scene render, as some renderers
		// require multiple phases - shadows, ambient and spotlight calculations may be sepearate phases, for example.
		// This call will be in the callstack of the Render3DObjects method below
		virtual void Draw(RALMeshBuffer& m, const Rococo::Graphics::ObjectInstance* instances, uint32 nInstances) = 0;

		// This is the entry point each frame for rendering. The scene will be queried a number of time for each object in the scene.
		// The objects are individually rendered with the Draw method, with the implementation adjusting render state appropriate for the 
		// rendering phase.
		virtual void Render3DObjects(Rococo::Graphics::IScene& scene, const  Rococo::Graphics::RenderOutputTargets& targets) = 0;

		// Invoked by a visitor to expose some internal data to instrumentation
		virtual void ShowVenue(IMathsVisitor& visitor) = 0;
	};

	ROCOCO_INTERFACE IRAL_3D_Object_RendererSupervisor: IRAL_3D_Object_Renderer
	{
		virtual void Free() = 0;
	};

	RAL_PIPELINE_API IRAL_3D_Object_RendererSupervisor* CreateRAL_3D_Object_Forward_Renderer(IRAL& ral, IRenderStates& renderStates, IRenderPhases& phases, IPipeline& pipeline);
	RAL_PIPELINE_API IRAL_3D_Object_RendererSupervisor* CreateRAL_3D_Object_G_Buffer_Renderer(IRAL& ral, IRenderStates& renderStates, IRenderPhases& phases, IPipeline& pipeline);

	ROCOCO_INTERFACE IRAL_BoneStateBuffer
	{
		virtual void SetBoneMatrix(uint32 index, cr_m4x4 m) = 0;
	};

	ROCOCO_INTERFACE IRAL_BoneStateBufferSupervisor : IRAL_BoneStateBuffer
	{
		virtual void Free() = 0;
		virtual void SyncToGPU() = 0;
	};

	RAL_PIPELINE_API IRAL_BoneStateBufferSupervisor* CreateRALBoneStateBuffer(IRAL& ral, IRenderStates& renderStates);
}
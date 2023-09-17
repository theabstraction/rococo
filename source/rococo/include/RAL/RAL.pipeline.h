#pragma once

#include <rococo.types.h>

#ifndef RAL_PIPELINE_API
# define RAL_PIPELINE_API
#endif

namespace Rococo
{
	struct ID_CUBE_TEXTURE;
}

namespace Rococo::Graphics
{
	struct DepthRenderData;
	struct VertexTriangle;
	struct IGui3D;
	struct IParticles;
	struct GuiMetrics;
	struct IScene;
	struct IGuiRenderContextSupervisor;
	struct LightConstantBuffer;
	struct RenderPhaseConfig;
	enum ENVIRONMENTAL_MAP;

	namespace Samplers
	{
		enum Filter : int32;
		enum AddressMode : int32;
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

		virtual void SetAndClearCurrentRenderBuffers(const RGBA& clearColour, const Rococo::Graphics::RenderPhaseConfig& config) = 0;
		virtual void SetDrawTopology(PrimitiveTopology topology) = 0;
		virtual void SetSampler(uint32 index, Rococo::Graphics::Samplers::Filter filter, Rococo::Graphics::Samplers::AddressMode u, Rococo::Graphics::Samplers::AddressMode v, Rococo::Graphics::Samplers::AddressMode w, const RGBA& borderColour) = 0;
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
		virtual void Add3DGuiTriangles(const Rococo::Graphics::VertexTriangle* first, const Rococo::Graphics::VertexTriangle* last) = 0;
		virtual void AssignAmbientLightToShaders(const Rococo::Graphics::LightConstantBuffer& ambientLight) = 0;
		virtual void AssignGlobalStateBufferToShaders() = 0;
		virtual void AssignLightStateBufferToShaders() = 0;
		virtual void Clear3DGuiTriangles() = 0;
		virtual void Draw(RALMeshBuffer& m, const Rococo::Graphics::ObjectInstance* instances, uint32 nInstances) = 0;
		virtual void DrawLightCones(Rococo::Graphics::IScene& scene) = 0;
		virtual void Render3DGui() = 0;
		virtual void RenderFogWithAmbient() = 0;
		virtual void RenderFogWithSpotlight() = 0;
		virtual void RenderPlasma() = 0;
		virtual void RenderSkyBox(Rococo::Graphics::IScene& scene) = 0;
		virtual void Render(const Rococo::Graphics::GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, Rococo::Graphics::IScene& scene) = 0;
		virtual void SetBoneMatrix(uint32 index, cr_m4x4 m) = 0;
		virtual ID_TEXTURE ShadowBufferId() const = 0;
		virtual void UpdateDepthRenderData(const Rococo::Graphics::DepthRenderData& drd) = 0;
		virtual void UpdateGlobalState(const Rococo::Graphics::GuiMetrics& metrics, Rococo::Graphics::IScene& scene) = 0;
		virtual void UpdateLightBuffer(const Rococo::Graphics::LightConstantBuffer& light) = 0;

		virtual Rococo::Graphics::IGui3D& Gui3D() = 0;
		virtual Rococo::Graphics::IParticles& Particles() = 0;
	};

	ROCOCO_INTERFACE IPipelineSupervisor : IPipeline
	{
		virtual void Free() = 0;
	};

	RAL_PIPELINE_API IPipelineSupervisor* CreatePipeline(IRenderStates& renderStates, IRAL& ral);
}
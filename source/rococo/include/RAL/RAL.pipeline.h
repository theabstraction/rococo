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
	struct VertexTriangle;
	struct IGui3D;
	struct IParticles;
	struct GuiMetrics;
	struct IScene;
	struct IGuiRenderContext;
	struct LightConstantBuffer;

	namespace Samplers
	{
		enum Filter : int32;
		enum AddressMode : int32;
	}
}

namespace Rococo::RAL
{
	struct IRAL;

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
		virtual void DisableBlend() = 0;
		virtual void DisableWritesOnDepthState() = 0;

		virtual void SetDrawTopology(PrimitiveTopology topology) = 0;
		virtual void SetSampler(uint32 index, Rococo::Graphics::Samplers::Filter filter, Rococo::Graphics::Samplers::AddressMode u, Rococo::Graphics::Samplers::AddressMode v, Rococo::Graphics::Samplers::AddressMode w, const RGBA& borderColour) = 0;
		virtual void SetShaderTexture(uint32 textureUnitIndex, Rococo::ID_CUBE_TEXTURE cubeId) = 0;

		virtual void UseAdditiveBlend() = 0;
		virtual void UseAlphaAdditiveBlend() = 0;
		virtual void UseParticleRasterizer() = 0;
		virtual void UsePlasmaBlend() = 0;
		virtual void UseSkyRasterizer() = 0;

		virtual Rococo::Graphics::IGuiRenderContext& Gui() = 0;
	};

	// [R]enderer [A]bstraction [L]ayer pipeline orders rendering calls to properly format the video output
	ROCOCO_INTERFACE IPipeline
	{
		virtual void Add3DGuiTriangles(const Rococo::Graphics::VertexTriangle* first, const Rococo::Graphics::VertexTriangle* last) = 0;
		virtual void AssignAmbientLightToShaders(const Rococo::Graphics::LightConstantBuffer& ambientLight) = 0;
		virtual void AssignGlobalStateBufferToShaders() = 0;
		virtual void AssignLightStateBufferToShaders() = 0;
		virtual void Clear3DGuiTriangles() = 0;
		virtual void Render3DGui() = 0;
		virtual void RenderFogWithAmbient() = 0;
		virtual void RenderFogWithSpotlight() = 0;
		virtual void RenderPlasma() = 0;
		virtual void RenderSkyBox(Rococo::Graphics::IScene& scene) = 0;
		virtual ID_TEXTURE ShadowBufferId() const = 0;
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
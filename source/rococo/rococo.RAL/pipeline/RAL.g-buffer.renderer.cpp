#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <rococo.visitors.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>
#include <rococo.renderer.formats.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

static void PrepareShadowDepthDescFromLight(const LightConstantBuffer& light, ShadowRenderData& shadowData)
{
	if (!TryNormalize(light.direction, shadowData.direction))
	{
		Throw(0, "%s: light direction normalization failed", __FUNCTION__);
	}

	shadowData.direction.w = 0;
	shadowData.eye = Vec4::FromVec3(light.position, 1.0f);
	shadowData.fov = light.fov;

	Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(shadowData.direction);
	Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);
	shadowData.right = cameraToDirectionRot * Vec4{ 1, 0, 0, 0 };
	shadowData.up = cameraToDirectionRot * Vec4{ 0, 1, 0, 0 };
	shadowData.world = directionToCameraRot * Matrix4x4::Translate(-shadowData.eye);
	shadowData.nearPlane = light.nearPlane;
	shadowData.farPlane = light.farPlane;

	Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(shadowData.fov, 1.0f, shadowData.nearPlane, shadowData.farPlane);

	shadowData.worldToScreen = cameraToScreen * shadowData.world;

	Time::ticks t = Time::TickCount();
	Time::ticks ticksPerSecond = Time::TickHz();
	Time::ticks oneMinute = ticksPerSecond * 60;
	Time::ticks secondOfMinute = t % oneMinute;

	shadowData.time = Seconds{ (secondOfMinute / (float)ticksPerSecond) * 0.9999f };
}

struct GBuffers : IGBuffers
{
	AutoFree<IRenderTarget_Colour> ColourBuffer;
	AutoFree<IRenderTarget_Depth> DepthBuffer;

	IRenderTarget& GetTarget(size_t index) override
	{
		switch (index)
		{
		case 0:
			return ColourBuffer->RenderTarget();
		case 1:
			return DepthBuffer->RenderTarget();
		default:
			Throw(0, __FUNCTION__": index %d out of bounds", index);
		}
	}

	size_t NumberOfTargets() const override
	{
		return 2;
	}
};

struct RAL_G_Buffer_3D_Object_Renderer : IRAL_3D_Object_RendererSupervisor
{
	struct FullScreenQuadVertex
	{
		Vec2 normalizedCoordinates;
		Vec2 uv;
	};

	IRAL& ral;
	IRenderStates& renderStates;
	IRenderPhases& renderPhases;
	IPipeline& pipeline;

	ID_VERTEX_SHADER idObjVS;
	ID_PIXEL_SHADER idObjPS;
	ID_PIXEL_SHADER idObjPS_Shadows;
	ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
	ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;
	ID_PIXEL_SHADER idObjGBufferPS;
	ID_VERTEX_SHADER idObjAmbientVS;
	ID_VERTEX_SHADER idObjVS_Shadows;
	ID_VERTEX_SHADER idSkinnedObjVS_Shadows;
	ID_VERTEX_SHADER idFSQuadVS;
	ID_PIXEL_SHADER idFinalPassPS;

	RenderPhase phase = RenderPhase::None;
	bool builtFirstPass = false;

	AutoFree<IRALConstantDataBuffer> instanceBuffer;
	AutoFree<IRALConstantDataBuffer> depthRenderStateBuffer;
	AutoFree<IRALConstantDataBuffer> lightStateBuffer;
	AutoFree<IRALVertexDataBuffer> fullscreenQuadBuffer;

	ID_TEXTURE shadowBufferId;

	int64 entitiesThisFrame = 0;
	int64 trianglesThisFrame = 0;

	GBuffers G;

	RAL_G_Buffer_3D_Object_Renderer(IRAL& _ral, IRenderStates& _renderStates, IRenderPhases& _phases, IPipeline& _pipeline) : ral(_ral), renderStates(_renderStates), renderPhases(_phases), pipeline(_pipeline)
	{
		idObjVS = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/object.vs");
		idObjPS = ral.Shaders().CreatePixelShader("!shaders/compiled/object.ps");
		idObjGBufferPS = ral.Shaders().CreatePixelShader("!shaders/compiled/object.gbuffer.ps");
		idObjAmbientVS = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/ambient.vs");
		idObjVS_Shadows = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/shadow.vs");
		idObjPS_Shadows = ral.Shaders().CreatePixelShader("!shaders/compiled/shadow.ps");
		idFinalPassPS = ral.Shaders().CreatePixelShader("!shaders/compiled/gbuffer.final_pass.ps");

		/*
		struct VertexElement
		{
			cstr semanticName;
			uint32 semanticIndex;
			VertexElementFormat format;
			uint32 slot;
		};
		*/
		VertexElement fullQuadVertexDesc[] = 
		{
			{ "position", 0, VertexElementFormat::Float2, 0 },
			{ "texcoord", 0, VertexElementFormat::Float2, 0 },
			{ nullptr, 0, VertexElementFormat::Float1, 0 }
		};
		idFSQuadVS = ral.Shaders().CreateVertexShader("!shaders/compiled/gbuffer.fullscreen_quad.vs", fullQuadVertexDesc);

		idObj_Spotlight_NoEnvMap_PS = ral.Shaders().CreatePixelShader("!shaders/compiled/obj.spotlight.no_env.ps");
		idObj_Ambient_NoEnvMap_PS = ral.Shaders().CreatePixelShader("!shaders/compiled/obj.ambient.no_env.ps");

		VertexElement skinnedVertexElements[] =
		{
			VertexElement { "position",		0, VertexElementFormat::Float3, 0 },
			VertexElement { "normal",		0, VertexElementFormat::Float3, 0 },
			VertexElement { "texcoord",		0, VertexElementFormat::Float2, 0 },
			VertexElement { "color",		0, VertexElementFormat::RGBA8U, 0 },
			VertexElement { "texcoord",		1, VertexElementFormat::Float2, 0 },
			VertexElement { "blendindices",	0, VertexElementFormat::Float1, 1 },
			VertexElement { "blendweight",	0, VertexElementFormat::Float1, 1 },
			VertexElement { "blendindices",	1, VertexElementFormat::Float1, 1 },
			VertexElement { "blendweight",	1, VertexElementFormat::Float1, 1 },
			VertexElement { nullptr,		0 ,VertexElementFormat::Float3, 0  }
		};

		idSkinnedObjVS_Shadows = ral.Shaders().CreateVertexShader("!shaders/compiled/skinned.shadow.vs", skinnedVertexElements);

		instanceBuffer = ral.CreateConstantBuffer(sizeof ObjectInstance, 1);
		depthRenderStateBuffer = ral.CreateConstantBuffer(sizeof DepthRenderData, 1);
		lightStateBuffer = ral.CreateConstantBuffer(sizeof LightConstantBuffer, 1);

		fullscreenQuadBuffer = ral.CreateDynamicVertexBuffer(sizeof FullScreenQuadVertex, 6);

		FullScreenQuadVertex fullscreenQuadGeometry[] =
		{
			{{-1.0f, -1.0f}, { 0.0f, 0.0f}},
			{{ 1.0f, -1.0f}, { 1.0f, 0.0f}},
			{{-1.0f,  1.0f}, { 0.0f, 1.0f}},
			{{-1.0f,  1.0f}, { 0.0f, 1.0f}},
			{{ 1.0f, -1.0f}, { 1.0f, 0.0f}},
			{{ 1.0f,  1.0f}, { 1.0f, 1.0f}},
		};

		static_assert(sizeof fullscreenQuadGeometry == sizeof FullScreenQuadVertex * 6);

		fullscreenQuadBuffer->CopyDataToBuffer(fullscreenQuadGeometry, sizeof fullscreenQuadGeometry);

		// TODO - make this dynamic
		shadowBufferId = ral.RALTextures().CreateDepthTarget("ShadowBuffer", 2048, 2048);

		G.ColourBuffer = ral.RALTextures().CreateDynamicRenderTarget("G.ColourBuffer");
		G.DepthBuffer = ral.RALTextures().CreateDynamicDepthTarget("G.DepthBuffer");
	}

	void Free() override
	{
		delete this;
	}

	// This is the entry point for 3D rendering using this class as the G-buffer renderer
	// targets.renderTarget of -1 indicates we are rendering to the window directly, and not to a texture
	void Render3DObjects(IScene& scene, const RenderOutputTargets& targets) override
	{
		trianglesThisFrame = 0;
		entitiesThisFrame = 0;

		renderStates.UseObjectRasterizer();
		
		builtFirstPass = false;

		Vec2i span = ral.RALTextures().GetTextureSpan(targets.renderTarget ? targets.renderTarget : ral.GetWindowDepthBufferId());
		
		if (span.x <= 0 || span.y <= 0)
		{
			return;
		}

		G.ColourBuffer->RenderTarget().MatchSpan(span);
		G.DepthBuffer->RenderTarget().MatchSpan(span);

		Lights lights = scene.GetLights();
		if (lights.lightArray != nullptr && lights.count > 0 && LengthSq(lights.lightArray[0].direction) > 0)
		{
			// The first light has the highest priority, and provides the shadows
			ShadowRenderData shadows;
			PrepareShadowDepthDescFromLight(lights.lightArray[0], shadows);
			RenderToShadowBuffer(shadows, scene);
		}
			
		ral.RALTextures().SetRenderTarget(G, targets.depthTarget);

		ral.RALTextures().AssignToPS(TXUNIT_SHADOW, shadowBufferId);

		RenderToGBuffers(scene);
		RenderGBuffersToScreen(targets);
	}

	// Render the scene from the POV of the light-source into the depth-shadow-buffer
	void RenderToShadowBuffer(ShadowRenderData& shadowRenderData, IScene& scene)
	{
		renderStates.TargetShadowBuffer(shadowBufferId);

		ral.Shaders().UseShaders(idSkinnedObjVS_Shadows, idObjPS_Shadows);

		UpdateDepthRenderData(shadowRenderData);

		phase = RenderPhase::DetermineShadowVolumes;

		// The scene will callback with the Draw(...) method of this class for each skinned shadow casters, such as a viking warrior
		scene.RenderShadowPass(shadowRenderData, ral.RenderContext(), EShadowCasterFilter::SkinnedCastersOnly);

		ral.Shaders().UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		UpdateDepthRenderData(shadowRenderData);

		// The scene will callback with the Draw(...) method of this class for each non-skinned shadow casters such as a viking helment
		scene.RenderShadowPass(shadowRenderData, ral.RenderContext(), EShadowCasterFilter::UnskinnedCastersOnly);

		// We now have populated the shadow buffer, which allows our remaining render calls to determine whether a pixel vertex is in shadow
	}

	void RenderToGBuffers(IScene& scene)
	{
		phase = RenderPhase::DetermineAmbient;

		if (ral.Shaders().UseShaders(idObjVS, idObjGBufferPS))
		{
			renderPhases.RenderToGBuffers(scene);
		}

		phase = RenderPhase::None;
	}

	// Assumes we have set the render target and depth target to the screen
	void RenderGBuffersToScreen(const RenderOutputTargets& targets)
	{
		if (ral.Shaders().UseShaders(idFSQuadVS, idFinalPassPS))
		{
			Vec2i span = ral.RALTextures().GetRenderTargetSpan(targets.renderTarget);
			ral.ExpandViewportToEntireSpan(span);
			ral.RALTextures().SetRenderTarget(targets.depthTarget, targets.renderTarget);
			ral.ClearBoundVertexBufferArray();
			ral.BindVertexBuffer(fullscreenQuadBuffer, sizeof FullScreenQuadVertex, 0);
			ral.CommitBoundVertexBuffers();
			renderStates.AssignGBufferToPS(G);
			renderStates.SetDrawTopology(PrimitiveTopology::TRIANGLELIST);
			renderStates.UseSkyRasterizer();
			renderStates.DisableWritesOnDepthState();
			renderStates.DisableBlend();
			ral.Draw(6, 0);
		}
	}

	void AssignLightStateBufferToShaders()
	{
		lightStateBuffer->AssignToGS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
		lightStateBuffer->AssignToPS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
		lightStateBuffer->AssignToVS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
	}

	void UpdateDepthRenderData(const DepthRenderData& data)
	{
		depthRenderStateBuffer->CopyDataToBuffer(&data, sizeof data);
		depthRenderStateBuffer->AssignToVS(CBUFFER_INDEX_DEPTH_RENDER_DESC);
		depthRenderStateBuffer->AssignToPS(CBUFFER_INDEX_DEPTH_RENDER_DESC);
	}

	void UpdateLightBuffer(const LightConstantBuffer& light)
	{
		lightStateBuffer->CopyDataToBuffer(&light, sizeof light);
	}

	void Draw(RALMeshBuffer& m, const ObjectInstance* instances, uint32 nInstances)
	{
		if (!m.vertexBuffer)
			return;

		if (phase == RenderPhase::DetermineShadowVolumes && m.disableShadowCasting)
			return;

		entitiesThisFrame += (int64)nInstances;

		bool overrideShader = false;

		if (m.psSpotlightShader && phase == RenderPhase::DetermineSpotlight)
		{
			ral.Shaders().UseShaders(m.vsSpotlightShader, m.psSpotlightShader);
			overrideShader = true;
		}
		else if (m.psAmbientShader && phase == RenderPhase::DetermineAmbient)
		{
			ral.Shaders().UseShaders(m.vsAmbientShader, m.psAmbientShader);
			overrideShader = true;
		}

		if (m.alphaBlending)
		{
			renderStates.UseAlphaAdditiveBlend();
			renderStates.DisableWritesOnDepthState();
		}

		renderStates.SetDrawTopology(m.topology);

		ral.ClearBoundVertexBufferArray();
		ral.BindVertexBuffer(m.vertexBuffer, sizeof ObjectVertex, 0);

		if (m.weightsBuffer)
		{
			ral.BindVertexBuffer(m.weightsBuffer, sizeof BoneWeights, 0);
		}

		ral.CommitBoundVertexBuffers();

		for (uint32 i = 0; i < nInstances; i++)
		{
			// dc.DrawInstances crashed the debugger, replace with single instance render call for now
			instanceBuffer->CopyDataToBuffer(instances + i, sizeof(ObjectInstance));
			instanceBuffer->AssignToVS(CBUFFER_INDEX_INSTANCE_BUFFER);

			ral.Draw(m.numberOfVertices, 0);

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
				renderStates.UseAdditiveBlend();
				renderStates.DisableWritesOnDepthState();
			}
			else
			{
				renderStates.DisableBlend();
				builtFirstPass = true;
			}
		}
	}

	void SetAccumulativeBlending()
	{
		if (builtFirstPass)
		{
			renderStates.UseAdditiveBlend();
			renderStates.DisableWritesOnDepthState();
		}
		else
		{
			renderStates.DisableBlend();
			builtFirstPass = true;
		}
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities", trianglesThisFrame, entitiesThisFrame);
	}
};

namespace Rococo::RAL
{
	RAL_PIPELINE_API IRAL_3D_Object_RendererSupervisor* CreateRAL_3D_Object_G_Buffer_Renderer(IRAL& ral, IRenderStates& renderStates, IRenderPhases& phases, IPipeline& pipeline)
	{
		return new RAL_G_Buffer_3D_Object_Renderer(ral, renderStates, phases, pipeline);
	}
}
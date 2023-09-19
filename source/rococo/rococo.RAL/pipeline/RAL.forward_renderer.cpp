#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <rococo.visitors.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

bool PrepareDepthRenderFromLight(const LightConstantBuffer& light, DepthRenderData& drd)
{
	if (!TryNormalize(light.direction, drd.direction))
	{
		return false;
	}

	drd.direction.w = 0;
	drd.eye = Vec4::FromVec3(light.position, 1.0f);
	drd.fov = light.fov;

	Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(drd.direction);
	Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);
	drd.right = cameraToDirectionRot * Vec4{ 1, 0, 0, 0 };
	drd.up = cameraToDirectionRot * Vec4{ 0, 1, 0, 0 };
	drd.worldToCamera = directionToCameraRot * Matrix4x4::Translate(-drd.eye);
	drd.nearPlane = light.nearPlane;
	drd.farPlane = light.farPlane;

	Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(drd.fov, 1.0f, drd.nearPlane, drd.farPlane);

	drd.worldToScreen = cameraToScreen * drd.worldToCamera;

	Time::ticks t = Time::TickCount();
	Time::ticks ticksPerSecond = Time::TickHz();
	Time::ticks oneMinute = ticksPerSecond * 60;
	Time::ticks secondOfMinute = t % oneMinute;

	drd.time = Seconds{ (secondOfMinute / (float)ticksPerSecond) * 0.9999f };

	return true;
}


struct RAL_3D_Object_Renderer : IRAL_3D_Object_Renderer
{
	IRAL& ral;
	IRenderStates& renderStates;
	IRenderPhases& renderPhases;
	IPipeline& pipeline;

	ID_VERTEX_SHADER idObjVS;
	ID_PIXEL_SHADER idObjPS;
	ID_PIXEL_SHADER idObjPS_Shadows;
	ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
	ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;
	ID_PIXEL_SHADER idObjAmbientPS;
	ID_VERTEX_SHADER idObjAmbientVS;
	ID_VERTEX_SHADER idObjVS_Shadows;
	ID_VERTEX_SHADER idSkinnedObjVS_Shadows;

	RenderPhase phase = RenderPhase::None;
	bool builtFirstPass = false;

	AutoFree<IRALConstantDataBuffer> instanceBuffer;
	AutoFree<IRALConstantDataBuffer> depthRenderStateBuffer;
	AutoFree<IRALConstantDataBuffer> lightStateBuffer;

	ID_TEXTURE shadowBufferId;

	int64 entitiesThisFrame = 0;
	int64 trianglesThisFrame = 0;

	RAL_3D_Object_Renderer(IRAL& _ral, IRenderStates& _renderStates, IRenderPhases& _phases, IPipeline& _pipeline) : ral(_ral), renderStates(_renderStates), renderPhases(_phases), pipeline(_pipeline)
	{
		idObjVS = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/object.vs");
		idObjPS = ral.Shaders().CreatePixelShader("!shaders/compiled/object.ps");
		idObjAmbientPS = ral.Shaders().CreatePixelShader("!shaders/compiled/ambient.ps");
		idObjAmbientVS = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/ambient.vs");
		idObjVS_Shadows = ral.Shaders().CreateObjectVertexShader("!shaders/compiled/shadow.vs");
		idObjPS_Shadows = ral.Shaders().CreatePixelShader("!shaders/compiled/shadow.ps");

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

		// TODO - make this dynamic
		shadowBufferId = ral.RALTextures().CreateDepthTarget("ShadowBuffer", 2048, 2048);
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

	ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase, ENVIRONMENTAL_MAP_TYPE envMapType)
	{
		switch (envMapType)
		{
		case Graphics::ENVIRONMENTAL_MAP_TYPE::FIXED_CUBE:
			switch (phase)
			{
			case RenderPhase::DetermineAmbient:
				return idObjAmbientPS;
			case RenderPhase::DetermineSpotlight:
				return idObjPS;
			case RenderPhase::DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		case Graphics::ENVIRONMENTAL_MAP_TYPE::PROCEDURAL:
			switch (phase)
			{
			case RenderPhase::DetermineAmbient:
				return idObj_Ambient_NoEnvMap_PS;
			case RenderPhase::DetermineSpotlight:
				return idObj_Spotlight_NoEnvMap_PS;
			case RenderPhase::DetermineShadowVolumes:
				return idObjPS_Shadows;
			default:
				Throw(0, "Unknown render phase: %d", phase);
			}
		default:
			Throw(0, "Environment mode %d not implemented", envMapType);
		}
	}

	void Draw(RALMeshBuffer& m, const ObjectInstance* instances, uint32 nInstances) override
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

	void Free() override
	{
		delete this;
	}

	// This is the entry point for 3D rendering using this class as the forward renderer
	// targets.renderTarget of -1 indicated we are rendering to the window directly, and not to a texture
	void Render3DObjects(IScene& scene, const RenderOutputTargets& targets, ENVIRONMENTAL_MAP_TYPE envMapType) override
	{
		trianglesThisFrame = 0;
		entitiesThisFrame = 0;

		renderStates.UseObjectRasterizer();

		builtFirstPass = false;

		auto lights = scene.GetLights();
		if (lights.lightArray != nullptr)
		{
			for (size_t i = 0; i < lights.count; ++i)
			{
				try
				{
					RenderSpotlightLitScene(lights.lightArray[i], scene, targets, envMapType);
				}
				catch (IException& ex)
				{
					Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
				}
			}

			RenderAmbient(scene, lights.lightArray[0], targets, envMapType);
		}
	}

	void RenderSpotlightLitScene(const LightConstantBuffer& lightSubset, IScene& scene, const RenderOutputTargets& targets, ENVIRONMENTAL_MAP_TYPE envMapType)
	{
		LightConstantBuffer light = lightSubset;

		DepthRenderData drd;
		if (PrepareDepthRenderFromLight(light, drd))
		{
			RenderToShadowBuffer(drd, scene);

			ral.RALTextures().SetRenderTarget(targets.depthTarget, targets.renderTarget);

			phase = RenderPhase::DetermineSpotlight;

			ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase, envMapType);
			ral.Shaders().UseShaders(idObjVS, idPS);

			ral.ExpandViewportToEntireTexture(targets.depthTarget);

			light.time = drd.time;
			light.right = drd.right;
			light.up = drd.up;
			light.worldToShadowBuffer = drd.worldToScreen;

			TextureDesc desc;
			light.OOShadowTxWidth = ral.RALTextures().TryGetTextureDesc(OUT desc, shadowBufferId) && desc.width > 0 ? (1.0f / desc.width) : 1.0f;

			UpdateLightBuffer(light);
			AssignLightStateBufferToShaders();

			if (builtFirstPass)
			{
				renderStates.UseAlphaAdditiveBlend();
				renderStates.DisableWritesOnDepthState();
			}
			else
			{
				renderStates.DisableBlend();
				builtFirstPass = true;
			}

			ral.RALTextures().AssignToPS(TXUNIT_SHADOW, shadowBufferId);

			renderPhases.RenderSpotlightPhase(scene);

			phase = RenderPhase::None;

			renderStates.UseObjectDepthState();
		}
	}

	void RenderToShadowBuffer(DepthRenderData& drd, IScene& scene)
	{
		renderStates.TargetShadowBuffer(shadowBufferId);

		ral.Shaders().UseShaders(idSkinnedObjVS_Shadows, idObjPS_Shadows);

		UpdateDepthRenderData(drd);

		phase = RenderPhase::DetermineShadowVolumes;
		scene.RenderShadowPass(drd, ral.RenderContext(), true);

		ral.Shaders().UseShaders(idObjVS_Shadows, idObjPS_Shadows);

		UpdateDepthRenderData(drd);

		scene.RenderShadowPass(drd, ral.RenderContext(), false);
	}

	void RenderAmbient(IScene& scene, const LightConstantBuffer& ambientLight, const RenderOutputTargets& targets, ENVIRONMENTAL_MAP_TYPE envMapType)
	{
		phase = RenderPhase::DetermineAmbient;

		ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase, envMapType);
		if (ral.Shaders().UseShaders(idObjAmbientVS, idPS))
		{
			float blendFactorUnused[] = { 0,0,0,0 };
			ral.ExpandViewportToEntireTexture(targets.depthTarget);

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

			renderPhases.RenderAmbientPhase(scene, ambientLight);
		}

		phase = RenderPhase::None;
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities", trianglesThisFrame, entitiesThisFrame);
	}
};

namespace Rococo::RAL
{
	IRAL_3D_Object_Renderer* CreateRAL_3D_Object_Renderer(IRAL& ral, IRenderStates& renderStates, IRenderPhases& phases, IPipeline& pipeline)
	{
		return new RAL_3D_Object_Renderer(ral, renderStates, phases, pipeline);
	}
}
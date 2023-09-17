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

	struct RALPipeline: IPipelineSupervisor
	{
		IRAL& ral;
		IRenderStates& renderStates;
		RenderPhase phase = RenderPhase::None;
		int64 entitiesThisFrame = 0;
		int64 trianglesThisFrame = 0;
		Time::ticks guiCost = 0;
		Time::ticks objCost = 0;
		bool builtFirstPass = false;
		AutoFree<IParticlesSupervisor> particles;
		AutoFree<IRAL_LightCones> lightCones;
		AutoFree<IRAL_Skybox> skybox;
		AutoFree<IGui3DSupervisor> gui3D;

		AutoFree<IRALConstantDataBuffer> instanceBuffer;
		AutoFree<IRALConstantDataBuffer> globalStateBuffer;
		AutoFree<IRALConstantDataBuffer> lightStateBuffer;
		AutoFree<IRALConstantDataBuffer> sunlightStateBuffer;
		AutoFree<IRALConstantDataBuffer> ambientBuffer;
		AutoFree<IRALConstantDataBuffer> boneMatricesStateBuffer;
		AutoFree<IRALConstantDataBuffer> depthRenderStateBuffer;

		ID_TEXTURE shadowBufferId;

		GlobalState currentGlobalState;

		BoneMatrices boneMatrices = { 0 };

		ID_VERTEX_SHADER idObjVS;
		ID_PIXEL_SHADER idObjPS;
		ID_PIXEL_SHADER idObjPS_Shadows;
		ID_PIXEL_SHADER idObj_Spotlight_NoEnvMap_PS;
		ID_PIXEL_SHADER idObj_Ambient_NoEnvMap_PS;
		ID_PIXEL_SHADER idObjAmbientPS;
		ID_VERTEX_SHADER idObjAmbientVS;
		ID_VERTEX_SHADER idObjVS_Shadows;
		ID_VERTEX_SHADER idSkinnedObjVS_Shadows;

		Graphics::RenderPhaseConfig phaseConfig;

		ID_TEXTURE lastTextureId;

		RALPipeline(IRenderStates& _renderStates, IRAL& _ral): renderStates(_renderStates), ral(_ral)
		{
			instanceBuffer = ral.CreateConstantBuffer(sizeof ObjectInstance, 1);
			globalStateBuffer = ral.CreateConstantBuffer(sizeof GlobalState, 1);
			lightStateBuffer = ral.CreateConstantBuffer(sizeof LightConstantBuffer, 1);
			sunlightStateBuffer = ral.CreateConstantBuffer(sizeof Vec4, 1);
			ambientBuffer = ral.CreateConstantBuffer(sizeof AmbientData, 1);
			boneMatricesStateBuffer = ral.CreateConstantBuffer(sizeof BoneMatrices, 1);
			depthRenderStateBuffer = ral.CreateConstantBuffer(sizeof DepthRenderData, 1);

			idObjVS				= ral.Shaders().CreateObjectVertexShader("!shaders/compiled/object.vs");
			idObjPS				= ral.Shaders().CreatePixelShader("!shaders/compiled/object.ps");
			idObjAmbientPS		= ral.Shaders().CreatePixelShader("!shaders/compiled/ambient.ps");
			idObjAmbientVS		= ral.Shaders().CreateObjectVertexShader("!shaders/compiled/ambient.vs");
			idObjVS_Shadows		= ral.Shaders().CreateObjectVertexShader("!shaders/compiled/shadow.vs");
			idObjPS_Shadows		= ral.Shaders().CreatePixelShader("!shaders/compiled/shadow.ps");

			idObj_Spotlight_NoEnvMap_PS = ral.Shaders().CreatePixelShader("!shaders/compiled/obj.spotlight.no_env.ps");
			idObj_Ambient_NoEnvMap_PS	= ral.Shaders().CreatePixelShader("!shaders/compiled/obj.ambient.no_env.ps");
			idSkinnedObjVS_Shadows		= ral.Shaders().CreateVertexShader("!shaders/compiled/skinned.shadow.vs", skinnedVertexElements);

			SetSamplerDefaults();

			// TODO - make this dynamic
			shadowBufferId = ral.RALTextures().CreateDepthTarget("ShadowBuffer", 2048, 2048);
			particles = CreateParticleSystem(_ral, _renderStates);
			lightCones = CreateLightCones(_ral, _renderStates, *this);
			skybox = CreateRALSkybox(_ral, _renderStates);
			gui3D = CreateGui3D(_ral, _renderStates, *this);
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

		static bool PrepareDepthRenderFromLight(const LightConstantBuffer& light, DepthRenderData& drd)
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

		ID_PIXEL_SHADER GetObjectShaderPixelId(RenderPhase phase)
		{
			switch (phaseConfig.EnvironmentalMap)
			{
			case Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE:
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
			case Graphics::ENVIRONMENTAL_MAP_PROCEDURAL:
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
				Throw(0, "Environment mode %d not implemented", phaseConfig.EnvironmentalMap);
			}
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

		void AssignLightStateBufferToShaders() override
		{
			lightStateBuffer->AssignToGS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
			lightStateBuffer->AssignToPS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
			lightStateBuffer->AssignToVS(CBUFFER_INDEX_CURRENT_SPOTLIGHT);
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

		void UpdateDepthRenderData(const DepthRenderData& data) override
		{
			depthRenderStateBuffer->CopyDataToBuffer(&data, sizeof data);
			depthRenderStateBuffer->AssignToVS(CBUFFER_INDEX_DEPTH_RENDER_DESC);
			depthRenderStateBuffer->AssignToPS(CBUFFER_INDEX_DEPTH_RENDER_DESC);
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

		void UpdateLightBuffer(const LightConstantBuffer& light) override
		{
			lightStateBuffer->CopyDataToBuffer(&light, sizeof light);
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

		bool IsGuiReady() const
		{
			return !phaseConfig.renderTarget;
		}

		void Render(const Rococo::Graphics::GuiMetrics& metrics, Graphics::ENVIRONMENTAL_MAP envMap, Rococo::Graphics::IScene& scene)
		{
			phaseConfig.EnvironmentalMap = envMap;
			phaseConfig.shadowBuffer = shadowBufferId;
			phaseConfig.depthTarget = ral.GetWindowDepthBufferId();

			if (!shadowBufferId)
			{
				Throw(0, "No shadow depth buffer set for DX1AppRenderer::Render(...)");
			}

			ID_CUBE_TEXTURE envId = scene.GetEnvironmentMap();
			ral.SetEnvironmentMap(envId);

			trianglesThisFrame = 0;
			entitiesThisFrame = 0;

			lastTextureId = ID_TEXTURE::Invalid();

			ral.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

			renderStates.SetAndClearCurrentRenderBuffers(scene.GetClearColour(), phaseConfig);

			UpdateGlobalState(metrics, scene);

			renderStates.ResetSamplersToDefaults();

			skybox->RenderSkyBox(scene);

			renderStates.AssignGuiShaderResources();
			renderStates.ResetSamplersToDefaults();

			Render3DObjects(scene);

			particles->RenderPlasma();
			lightCones->DrawLightCones(scene);

			UpdateGlobalState(metrics, scene);

			Time::ticks now = Time::TickCount();

			renderStates.Gui().RenderGui(scene, metrics, IsGuiReady());

			guiCost = Time::TickCount() - now;
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

		void RenderAmbient(IScene& scene, const LightConstantBuffer& ambientLight)
		{
			phase = RenderPhase::DetermineAmbient;

			ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
			if (ral.Shaders().UseShaders(idObjAmbientVS, idPS))
			{
				float blendFactorUnused[] = { 0,0,0,0 };
				ral.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

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

				renderStates.UseObjectRasterizer();

				AssignAmbientLightToShaders(ambientLight);

				scene.RenderObjects(ral.RenderContext(), false);
				scene.RenderObjects(ral.RenderContext(), true);

				gui3D->Render3DGui();
				particles->RenderFogWithAmbient();
			}

			phase = RenderPhase::None;
		}

		void ShowVenue(IMathsVisitor& visitor)
		{
			visitor.ShowString("Geometry this frame", "%lld triangles. %lld entities, %lld particles", trianglesThisFrame, entitiesThisFrame, 0);
		}

		void Render3DObjects(IScene& scene)
		{
			auto now = Time::TickCount();

			renderStates.UseObjectRasterizer();

			builtFirstPass = false;

			auto lights = scene.GetLights();
			if (lights.lightArray != nullptr)
			{
				for (size_t i = 0; i < lights.count; ++i)
				{
					try
					{
						RenderSpotlightLitScene(lights.lightArray[i], scene);
					}
					catch (IException& ex)
					{
						Throw(ex.ErrorCode(), "Error lighting scene with light #%d: %s", i, ex.Message());
					}
				}

				RenderAmbient(scene, lights.lightArray[0]);
			}

			objCost = Time::TickCount() - now;
		}

		void RenderSpotlightLitScene(const LightConstantBuffer& lightSubset, IScene& scene)
		{
			LightConstantBuffer light = lightSubset;

			DepthRenderData drd;
			if (PrepareDepthRenderFromLight(light, drd))
			{
				drd.randoms.x = 0.0f;
				drd.randoms.y = 1.0f;
				drd.randoms.z = 2.0f;
				drd.randoms.w = 3.0f;

				RenderToShadowBuffer(drd, scene);

				ral.RALTextures().SetRenderTarget(phaseConfig.depthTarget, phaseConfig.renderTarget);

				phase = RenderPhase::DetermineSpotlight;

				ID_PIXEL_SHADER idPS = GetObjectShaderPixelId(phase);
				ral.Shaders().UseShaders(idObjVS, idPS);

				ral.ExpandViewportToEntireTexture(phaseConfig.depthTarget);

				light.randoms = drd.randoms;
				light.time = drd.time;
				light.right = drd.right;
				light.up = drd.up;
				light.worldToShadowBuffer = drd.worldToScreen;

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

				ral.RALTextures().AssignToPS(TXUNIT_SHADOW, phaseConfig.shadowBuffer);

				renderStates.UseObjectRasterizer();

				scene.RenderObjects(ral.RenderContext(), false);
				scene.RenderObjects(ral.RenderContext(), true);

				gui3D->Render3DGui();
				particles->RenderFogWithSpotlight();

				phase = RenderPhase::None;

				renderStates.UseObjectDepthState();
			}
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
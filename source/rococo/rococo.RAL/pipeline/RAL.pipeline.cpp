#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <RAL/ral.h>
#include <RAL/RAL.pipeline.h>
#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <rococo.time.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace Rococo::RAL::Anon
{
	enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };
	enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };

	// struct VertexElement	{ cstr SemanticName; uint32 semanticIndex; VertexElementFormat format; uint32 slot;	};

	VertexElement skyVertexElements[] =
	{
		VertexElement { "position", 0, VertexElementFormat::Float3, 0 },
		VertexElement { nullptr,    0 ,VertexElementFormat::Float3, 0 }
	};

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

	struct RALPipeline: IPipelineSupervisor, IGui3D, IParticles
	{
		IRAL& ral;
		IRenderStates& renderStates;
		RenderPhase phase = RenderPhase::None;
		int64 entitiesThisFrame = 0;
		int64 trianglesThisFrame = 0;
		Time::ticks guiCost = 0;
		Time::ticks objCost = 0;
		bool builtFirstPass = false;

		std::vector<VertexTriangle> gui3DTriangles;
		std::vector<ParticleVertex> fog;
		std::vector<ParticleVertex> plasma;

		AutoFree<IRALVertexDataBuffer> gui3DBuffer;
		AutoFree<IRALVertexDataBuffer> particleBuffer;
		AutoFree<IRALVertexDataBuffer> lightConeBuffer;

		AutoFree<IRALConstantDataBuffer> instanceBuffer;
		AutoFree<IRALConstantDataBuffer> globalStateBuffer;
		AutoFree<IRALConstantDataBuffer> lightStateBuffer;
		AutoFree<IRALConstantDataBuffer> sunlightStateBuffer;
		AutoFree<IRALConstantDataBuffer> ambientBuffer;
		AutoFree<IRALConstantDataBuffer> boneMatricesStateBuffer;
		AutoFree<IRALConstantDataBuffer> depthRenderStateBuffer;

		ID_VERTEX_SHADER idParticleVS;
		ID_PIXEL_SHADER idPlasmaPS;
		ID_GEOMETRY_SHADER idPlasmaGS;

		ID_PIXEL_SHADER idFogAmbientPS;
		ID_PIXEL_SHADER idFogSpotlightPS;
		ID_GEOMETRY_SHADER idFogSpotlightGS;
		ID_GEOMETRY_SHADER idFogAmbientGS;

		ID_TEXTURE shadowBufferId;

		GlobalState currentGlobalState;

		ID_SYS_MESH skyMeshId;
		ID_VERTEX_SHADER idObjSkyVS;
		ID_PIXEL_SHADER idObjSkyPS;

		BoneMatrices boneMatrices = { 0 };

		ID_PIXEL_SHADER idLightConePS;
		ID_VERTEX_SHADER idLightConeVS;

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
			gui3DBuffer = ral.CreateDynamicVertexBuffer(sizeof VertexTriangle, GUI3D_BUFFER_TRIANGLE_CAPACITY);
			particleBuffer = ral.CreateDynamicVertexBuffer(sizeof ParticleVertex, PARTICLE_BUFFER_VERTEX_CAPACITY);
			lightConeBuffer = ral.CreateDynamicVertexBuffer(sizeof ObjectVertex, 3);
			instanceBuffer = ral.CreateConstantBuffer(sizeof ObjectInstance, 1);
			globalStateBuffer = ral.CreateConstantBuffer(sizeof GlobalState, 1);
			lightStateBuffer = ral.CreateConstantBuffer(sizeof LightConstantBuffer, 1);
			sunlightStateBuffer = ral.CreateConstantBuffer(sizeof Vec4, 1);
			ambientBuffer = ral.CreateConstantBuffer(sizeof AmbientData, 1);
			boneMatricesStateBuffer = ral.CreateConstantBuffer(sizeof BoneMatrices, 1);
			depthRenderStateBuffer = ral.CreateConstantBuffer(sizeof DepthRenderData, 1);

			idParticleVS		= ral.Shaders().CreateParticleVertexShader("!shaders/compiled/particle.vs");
			idPlasmaGS			= ral.Shaders().CreateGeometryShader("!shaders/compiled/plasma.gs");
			idFogSpotlightGS	= ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.spotlight.gs");
			idFogAmbientGS		= ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.ambient.gs");
			idPlasmaPS			= ral.Shaders().CreatePixelShader("!shaders/compiled/plasma.ps");
			idFogSpotlightPS	= ral.Shaders().CreatePixelShader("!shaders/compiled/fog.spotlight.ps");
			idFogAmbientPS		= ral.Shaders().CreatePixelShader("!shaders/compiled/fog.ambient.ps");
			idObjSkyVS			= ral.Shaders().CreateVertexShader("!shaders/compiled/skybox.vs", skyVertexElements);
			idObjSkyPS			= ral.Shaders().CreatePixelShader("!shaders/compiled/skybox.ps");
			idLightConePS		= ral.Shaders().CreatePixelShader("!shaders/compiled/light_cone.ps");
			idLightConeVS		= ral.Shaders().CreateVertexShader("!shaders/compiled/light_cone.vs", objectVertexElements);

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

		Rococo::Graphics::IGui3D& Gui3D() override
		{
			return *this;
		}

		Rococo::Graphics::IParticles& Particles() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void Add3DGuiTriangles(const VertexTriangle* first, const VertexTriangle* last) override
		{
			for (auto i = first; i != last; ++i)
			{
				gui3DTriangles.push_back(*i);
			}
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

		void Clear3DGuiTriangles() override
		{
			gui3DTriangles.clear();
		}

		void Render3DGui() override
		{
			size_t cursor = 0;

			ObjectInstance one = { Matrix4x4::Identity(), Vec3 {1.0f, 1.0f, 1.0f}, 0.0f, RGBA(1.0f, 1.0f, 1.0f, 1.0f) };

			size_t nTriangles = gui3DTriangles.size();

			while (nTriangles)
			{
				auto* v = gui3DTriangles.data() + cursor;

				size_t nTriangleBatchCount = min<size_t>(nTriangles, GUI3D_BUFFER_TRIANGLE_CAPACITY);

				gui3DBuffer->CopyDataToBuffer(v, nTriangleBatchCount * sizeof(VertexTriangle));

				RALMeshBuffer m;
				m.alphaBlending = false;
				m.disableShadowCasting = false;
				m.vertexBuffer = gui3DBuffer;
				m.weightsBuffer = nullptr;
				m.numberOfVertices = (uint32) nTriangleBatchCount * 3;
				m.topology = PrimitiveTopology::TRIANGLELIST;

				Draw(m, &one, 1);

				nTriangles -= nTriangleBatchCount;
				cursor += nTriangleBatchCount;
			}
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

		void RenderFogWithAmbient() override
		{
			renderStates.UseAlphaAdditiveBlend();
			DrawParticles(fog.data(), fog.size(), idFogAmbientPS, idParticleVS, idFogAmbientGS);
		}

		void RenderFogWithSpotlight() override
		{
			renderStates.UseAlphaAdditiveBlend();
			DrawParticles(fog.data(), fog.size(), idFogSpotlightPS, idParticleVS, idFogSpotlightGS);
		}

		void RenderPlasma() override
		{
			renderStates.UsePlasmaBlend();
			DrawParticles(plasma.data(), plasma.size(), idPlasmaPS, idParticleVS, idPlasmaGS);
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

		void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID)
		{
			if (nParticles == 0) return;
			if (!ral.Shaders().UseShaders(vsID, psID)) return;
			if (!ral.Shaders().UseGeometryShader(gsID)) return;

			renderStates.SetDrawTopology(PrimitiveTopology::POINTLIST);
			renderStates.UseParticleRasterizer();
			renderStates.DisableWritesOnDepthState();

			const ParticleVertex* start = &particles[0];

			size_t i = 0;

			if (nParticles > 0)
			{
				ral.ClearBoundVertexBufferArray();
				ral.BindVertexBuffer(particleBuffer, sizeof ParticleVertex, 0);
				ral.CommitBoundVertexBuffers();
			}


			while (nParticles > 0)
			{
				size_t chunkSize = min(nParticles, (size_t)PARTICLE_BUFFER_VERTEX_CAPACITY);
				particleBuffer->CopyDataToBuffer(start + i, chunkSize * sizeof(ParticleVertex));

				ral.Draw((uint32)chunkSize, 0);

				i += chunkSize;
				nParticles -= chunkSize;
			}

			ral.Shaders().UseGeometryShader(ID_GEOMETRY_SHADER::Invalid());
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

		void DrawLightCone(const LightConstantBuffer& light, cr_vec3 viewDir)
		{
			/* our aim is to render a cross section of the light cone as a single alpha blended triangle
					B
					'
				'
				' (cutoffTheta)
			A   -----------------> Light direction D
				'
				'
					'
						C


				A is the position of the light facing D. We need to compute B and C

				The triangle ABC is parallel to the screen (with screem direction F), which means BC.F = 0

				A unit vector p parallel to BC is thus the normalized cross product of F and D.

				p = |D x F|.

				We can now construct B and C by taking basis vectors d and p from A.

				Given we specify a length k of the light cone, along its central axis,
				then the radius R along the vector p to construct B and C is such that
				R / k = tan ( cutoffTheta )

				The component of d of D parallel to the screen is | F x p |

			*/

			const Vec3& D = light.direction;

			Vec3 p = Cross(D, viewDir);

			if (LengthSq(p) < 0.15)
			{
				// Too acute, do not render
				return;
			}

			p = Normalize(p);

			if (fabsf(light.cutoffCosAngle) < 0.1)
			{
				// Angle too obtuse, do not render
				return;
			}

			float cutOffAngle = acosf(light.cutoffCosAngle);
			float tanCutoff = tanf(cutOffAngle);

			auto coneLength = 1.0_metres;
			auto radius = coneLength * tanCutoff;

			Vec3 d = Normalize(Cross(viewDir, p));

			ObjectVertex v[3] = { 0 };
			v[0].position = light.position;
			v[0].material.colour = RGBAb(255, 255, 255, 128);
			v[0].uv.x = 0;
			v[1].uv.y = 0;

			v[1].position = light.position + coneLength * D + radius * p;
			v[1].material.colour = RGBAb(255, 255, 255, 0);
			v[1].uv.x = 1.0f;
			v[1].uv.y = -1.0f;

			v[2].position = light.position + coneLength * D - radius * p;
			v[2].material.colour = RGBAb(255, 255, 255, 0);
			v[2].uv.x = 1.0f;
			v[2].uv.y = -1.0f;

			lightConeBuffer->CopyDataToBuffer(&v, sizeof v);

			ral.Draw(sizeof v / sizeof ObjectVertex, 0);
		}

		void DrawLightCones(IScene& scene) override
		{
			auto lights = scene.GetLights();
			if (lights.lightArray != nullptr)
			{
				ral.ClearBoundVertexBufferArray();
				ral.BindVertexBuffer(lightConeBuffer, sizeof ObjectVertex, 0);
				ral.CommitBoundVertexBuffers();
				renderStates.SetDrawTopology(PrimitiveTopology::TRIANGLELIST);
				renderStates.UseSpriteRasterizer();
				ral.Shaders().UseShaders(idLightConeVS, idLightConePS);
				renderStates.UseAlphaBlend();
				renderStates.DisableWritesOnDepthState();

				AssignGlobalStateBufferToShaders();

				ObjectInstance identity;
				identity.orientation = Matrix4x4::Identity();
				identity.highlightColour = { 0 };

				instanceBuffer->CopyDataToBuffer(&identity, sizeof identity);
				instanceBuffer->AssignToVS(CBUFFER_INDEX_INSTANCE_BUFFER);

				Matrix4x4 camera;
				Matrix4x4 world;
				Matrix4x4 proj;
				Vec4 eye;
				Vec4 viewDir;
				scene.GetCamera(camera, world, proj, eye, viewDir);

				for (uint32 i = 0; i < lights.count; ++i)
				{
					if (lights.lightArray[i].hasCone)
					{
						DrawLightCone(lights.lightArray[i], viewDir);
					}
				}

				/*
				dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
				dc.RSSetState(nullptr);
				dc.PSSetConstantBuffers(0, 0, nullptr);
				dc.VSSetConstantBuffers(0, 0, nullptr);
				*/
			}
		}

		void AddFog(const ParticleVertex& p)
		{
			fog.push_back(p);
		}

		void AddPlasma(const ParticleVertex& p)
		{
			plasma.push_back(p);
		}

		void ClearPlasma()
		{
			plasma.clear();
		}

		void ClearFog()
		{
			fog.clear();
		}

		void RenderSkyBox(IScene& scene)
		{
			ID_CUBE_TEXTURE cubeId = scene.GetSkyboxCubeId();

			if (!skyMeshId)
			{
				SkyVertex topNW{ -1.0f, 1.0f, 1.0f };
				SkyVertex topNE{ 1.0f, 1.0f, 1.0f };
				SkyVertex topSW{ -1.0f,-1.0f, 1.0f };
				SkyVertex topSE{ 1.0f,-1.0f, 1.0f };
				SkyVertex botNW{ -1.0f, 1.0f,-1.0f };
				SkyVertex botNE{ 1.0f, 1.0f,-1.0f };
				SkyVertex botSW{ -1.0f,-1.0f,-1.0f };
				SkyVertex botSE{ 1.0f,-1.0f,-1.0f };

				SkyVertex skyboxVertices[36] =
				{
					topSW, topNW, topNE, // top,
					topNE, topSE, topSW, // top,
					botSW, botNW, botNE, // bottom,
					botNE, botSE, botSW, // bottom,
					topNW, topSW, botSW, // West
					botSW, botNW, topNW, // West
					topNE, topSE, botSE, // East
					botSE, botNE, topNE, // East
					topNW, topNE, botNE, // North
					botNE, botNW, topNW, // North
					topSW, topSE, botSE, // South
					botSE, botSW, topSW, // South
				};

				skyMeshId = ral.Meshes().CreateSkyMesh(skyboxVertices, sizeof(skyboxVertices) / sizeof(SkyVertex));
			}

			if (ral.Shaders().UseShaders(idObjSkyVS, idObjSkyPS))
			{
				ral.ClearBoundVertexBufferArray();
				ral.BindVertexBuffer(skyMeshId, sizeof(SkyVertex), 0);
				ral.CommitBoundVertexBuffers();

				renderStates.SetShaderTexture(TXUNIT_ENV_MAP, cubeId);
				renderStates.SetDrawTopology(PrimitiveTopology::TRIANGLELIST);
				renderStates.UseSkyRasterizer();
				renderStates.DisableWritesOnDepthState();
				renderStates.DisableBlend();

				ral.Draw(36, 0);

				renderStates.ResetSamplersToDefaults();
			}
			else
			{
				Throw(0, "DX11Renderer::RenderSkybox failed. Error setting sky shaders");
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

			RenderSkyBox(scene);

			renderStates.AssignGuiShaderResources();
			renderStates.ResetSamplersToDefaults();

			Render3DObjects(scene);

			RenderPlasma();
			DrawLightCones(scene);

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

				Render3DGui();
				RenderFogWithAmbient();
			}

			phase = RenderPhase::None;
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

				Render3DGui();
				RenderFogWithSpotlight();

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
}
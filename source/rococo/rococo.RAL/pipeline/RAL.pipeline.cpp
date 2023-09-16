#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <RAL/ral.h>
#include <RAL/RAL.pipeline.h>
#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace Rococo::RAL::Anon
{
	enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };
	enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };

	struct RALPipeline: IPipelineSupervisor, IGui3D, IParticles
	{
		IRAL& ral;
		IRenderStates& renderStates;
		RenderPhase phase = RenderPhase::None;
		int64 entitiesThisFrame = 0;
		int64 trianglesThisFrame = 0;
		bool builtFirstPass = false;

		std::vector<VertexTriangle> gui3DTriangles;
		std::vector<ParticleVertex> fog;
		std::vector<ParticleVertex> plasma;

		AutoFree<IRALVertexDataBuffer> gui3DBuffer;
		AutoFree<IRALConstantDataBuffer> instanceBuffer;
		AutoFree<IRALVertexDataBuffer> particleBuffer;

		ID_VERTEX_SHADER idParticleVS;
		ID_PIXEL_SHADER idPlasmaPS;
		ID_GEOMETRY_SHADER idPlasmaGS;

		ID_PIXEL_SHADER idFogAmbientPS;
		ID_PIXEL_SHADER idFogSpotlightPS;
		ID_GEOMETRY_SHADER idFogSpotlightGS;
		ID_GEOMETRY_SHADER idFogAmbientGS;

		RALPipeline(IRenderStates& _renderStates, IRAL& _ral): renderStates(_renderStates), ral(_ral)
		{
			gui3DBuffer = ral.CreateDynamicVertexBuffer(sizeof VertexTriangle, GUI3D_BUFFER_TRIANGLE_CAPACITY);
			instanceBuffer = ral.CreateConstantBuffer(sizeof ObjectInstance, 1);
			particleBuffer = ral.CreateDynamicVertexBuffer(sizeof ParticleVertex, PARTICLE_BUFFER_VERTEX_CAPACITY);

			idParticleVS =  ral.Shaders().CreateParticleVertexShader("!shaders/compiled/particle.vs");
			idPlasmaGS = ral.Shaders().CreateGeometryShader("!shaders/compiled/plasma.gs");
			idFogSpotlightGS = ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.spotlight.gs");
			idFogAmbientGS = ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.ambient.gs");
			idPlasmaPS = ral.Shaders().CreatePixelShader("!shaders/compiled/plasma.ps");
			idFogSpotlightPS = ral.Shaders().CreatePixelShader("!shaders/compiled/fog.spotlight.ps");
			idFogAmbientPS = ral.Shaders().CreatePixelShader("!shaders/compiled/fog.ambient.ps");
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

				ral.Draw(chunkSize, 0);

				i += chunkSize;
				nParticles -= chunkSize;
			}

			ral.Shaders().UseGeometryShader(ID_GEOMETRY_SHADER::Invalid());
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
			ral.BindVertexBuffer(m.weightsBuffer, sizeof BoneWeights, 0);
			ral.CommitBoundVertexBuffers();

			for (uint32 i = 0; i < nInstances; i++)
			{
				// dc.DrawInstances crashed the debugger, replace with single instance render call for now
				instanceBuffer->CopyDataToBuffer(instances + i, sizeof(ObjectInstance));
				instanceBuffer->AssignToGPU(CBUFFER_INDEX_INSTANCE_BUFFER);

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
	};
}

namespace Rococo::RAL
{
	RAL_PIPELINE_API IPipelineSupervisor* CreatePipeline(IRenderStates& renderStates, IRAL& ral)
	{
		return new Anon::RALPipeline(renderStates, ral);
	}
}
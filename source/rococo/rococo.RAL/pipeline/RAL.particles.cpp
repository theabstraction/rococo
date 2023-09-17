#include <rococo.types.h>

// #define RAL_PIPELINE_API ROCOCO_API_EXPORT

#include <rococo.renderer.types.h>
#include <rococo.renderer.h>
#include <RAL\RAL.h>
#include <RAL\RAL.pipeline.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::RAL;

enum { PARTICLE_BUFFER_VERTEX_CAPACITY = 1024 };

struct Particles : IParticlesSupervisor
{
	std::vector<ParticleVertex> fog;
	std::vector<ParticleVertex> plasma;

	AutoFree<IRALVertexDataBuffer> particleBuffer;
	ID_VERTEX_SHADER idParticleVS;
	ID_PIXEL_SHADER idPlasmaPS;
	ID_GEOMETRY_SHADER idPlasmaGS;
	ID_PIXEL_SHADER idFogAmbientPS;
	ID_PIXEL_SHADER idFogSpotlightPS;
	ID_GEOMETRY_SHADER idFogSpotlightGS;
	ID_GEOMETRY_SHADER idFogAmbientGS;

	IRAL& ral;
	IRenderStates& renderStates;

	Particles(IRAL& _ral, IRenderStates& _renderStates) : ral(_ral), renderStates(_renderStates)
	{
		particleBuffer = _ral.CreateDynamicVertexBuffer(sizeof ParticleVertex, PARTICLE_BUFFER_VERTEX_CAPACITY);

		idParticleVS = _ral.Shaders().CreateParticleVertexShader("!shaders/compiled/particle.vs");
		idPlasmaGS = _ral.Shaders().CreateGeometryShader("!shaders/compiled/plasma.gs");
		idFogSpotlightGS = _ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.spotlight.gs");
		idFogAmbientGS = _ral.Shaders().CreateGeometryShader("!shaders/compiled/fog.ambient.gs");
		idPlasmaPS = _ral.Shaders().CreatePixelShader("!shaders/compiled/plasma.ps");
		idFogSpotlightPS = _ral.Shaders().CreatePixelShader("!shaders/compiled/fog.spotlight.ps");
		idFogAmbientPS = _ral.Shaders().CreatePixelShader("!shaders/compiled/fog.ambient.ps");
	}

	void Free() override
	{
		delete this;
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

	void DrawParticles(const ParticleVertex* particles, size_t nParticles, ID_PIXEL_SHADER psID, ID_VERTEX_SHADER vsID, ID_GEOMETRY_SHADER gsID) override
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

	void AddFog(const ParticleVertex& p) override
	{
		fog.push_back(p);
	}

	void AddPlasma(const ParticleVertex& p) override
	{
		plasma.push_back(p);
	}

	void ClearPlasma() override
	{
		plasma.clear();
	}

	void ClearFog() override
	{
		fog.clear();
	}
};

namespace Rococo::RAL
{
	IParticlesSupervisor* CreateParticleSystem(IRAL& ral, IRenderStates& renderStates)
	{
		return new Particles(ral, renderStates);
	}
}
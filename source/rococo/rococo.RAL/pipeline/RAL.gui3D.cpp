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

struct RAL_3DGui : IGui3DSupervisor
{
	enum { GUI3D_BUFFER_TRIANGLE_CAPACITY = 1024 };

	IRAL& ral;
	IRenderStates& renderStates;
	IPipeline& pipeline;
	std::vector<VertexTriangle> gui3DTriangles;
	AutoFree<IRALVertexDataBuffer> gui3DBuffer;

	RAL_3DGui(IRAL& _ral, IRenderStates& _renderStates, IPipeline& _pipeline) : ral(_ral), renderStates(_renderStates), pipeline(_pipeline)
	{
		gui3DBuffer = ral.CreateDynamicVertexBuffer(sizeof VertexTriangle, GUI3D_BUFFER_TRIANGLE_CAPACITY);
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

	void Clear3DGuiTriangles()
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
			m.numberOfVertices = (uint32)nTriangleBatchCount * 3;
			m.topology = PrimitiveTopology::TRIANGLELIST;

			pipeline.Draw(m, &one, 1);

			nTriangles -= nTriangleBatchCount;
			cursor += nTriangleBatchCount;
		}
	}
};

namespace Rococo::RAL
{
	IGui3DSupervisor* CreateGui3D(IRAL& ral, IRenderStates& renderStates, IPipeline& pipeline)
	{
		return new RAL_3DGui(ral, renderStates, pipeline);
	}
}
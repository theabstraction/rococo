#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include <RAL/RAL.h>
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include <vector>

using namespace Rococo::DX11;
using namespace Rococo::Strings;
using namespace Rococo::RAL;
using namespace Rococo::Graphics;

struct DX11Meshes : public IDX11Meshes
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;

	std::vector<RAL::RALMeshBuffer> meshBuffers;

	int64 meshUpdateCount = 0;

	DX11Meshes(ID3D11Device& _device, ID3D11DeviceContext& _dc): device(_device), dc(_dc)
	{

	}

	virtual ~DX11Meshes()
	{
		ClearMeshes();
	}

	void Free() override
	{
		delete this;
	}

	ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
	{
		auto* meshBuffer = vertices ? DX11::CreateRALImmutableVertexBuffer(device, dc, vertices, nVertices) : nullptr;
		auto* weightsBuffer = weights ? DX11::CreateRALImmutableVertexBuffer(device, dc, weights, nVertices) : nullptr;
		meshBuffers.push_back(RAL::RALMeshBuffer{ meshBuffer, weightsBuffer, nVertices, RAL::PrimitiveTopology::TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false });
		int32 index = (int32)meshBuffers.size();
		return ID_SYS_MESH(index - 1);
	}

	ID_SYS_MESH CreateSkyMesh(const SkyVertex* vertices, uint32 nVertices) override
	{
		auto* meshBuffer = vertices ? DX11::CreateRALImmutableVertexBuffer(device, dc, vertices, nVertices) : nullptr;
		meshBuffers.push_back(RAL::RALMeshBuffer{ meshBuffer, nullptr, nVertices, RAL::PrimitiveTopology::TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false });
		int32 index = (int32)meshBuffers.size();
		return ID_SYS_MESH(index - 1);
	}

	void GetMeshDesc(char desc[256], ID_SYS_MESH id) override
	{
		if (!id || id.value >= meshBuffers.size())
		{
			CopyString(desc, 256, "invalid id");
		}
		else
		{
			auto& m = meshBuffers[id.value];

			if (m.vertexBuffer)
			{
				UINT byteWidth = 0;
				D3D11_BUFFER_DESC bdesc;
				ToDX11(*m.vertexBuffer).GetDesc(&bdesc);
				byteWidth += bdesc.ByteWidth;

				if (m.weightsBuffer)
				{
					ToDX11(*m.weightsBuffer).GetDesc(&bdesc);
					byteWidth += bdesc.ByteWidth;
				}
				SafeFormat(desc, 256, " %p %6d %svertices. %6u bytes", m.vertexBuffer, m.numberOfVertices, m.weightsBuffer != nullptr ? "(weighted) " : "", byteWidth);
			}
			else
			{
				CopyString(desc, 256, "Null object");
			}
		}
	}

	void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive) override
	{
		if (!id || id.value >= meshBuffers.size())
		{
			Throw(0, "DX11ApRenderer::SetShadowCasting(...): unknown id");
		}
		else
		{
			auto& m = meshBuffers[id.value];
			m.disableShadowCasting = !isActive;
		}
	}

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowDecimal("Number of meshes", (int64)meshBuffers.size());
		visitor.ShowDecimal("Mesh updates", meshUpdateCount);
	}

	void ClearMeshes() override
	{
		for (auto& x : meshBuffers)
		{
			if (x.vertexBuffer) x.vertexBuffer->Free();
			if (x.weightsBuffer) x.weightsBuffer->Free();
		}

		meshBuffers.clear();
	}

	void DeleteMesh(ID_SYS_MESH id) override
	{
		if (id.value < 0 || id.value >= meshBuffers.size()) Throw(0, "DX11AppRenderer::DeleteMesh(...): Bad ID_SYS_MESH");

		meshBuffers[id.value].numberOfVertices = 0;

		if (meshBuffers[id.value].vertexBuffer)
		{
			meshBuffers[id.value].vertexBuffer->Free();
			meshBuffers[id.value].vertexBuffer = nullptr;
		}

		if (meshBuffers[id.value].weightsBuffer)
		{
			meshBuffers[id.value].weightsBuffer->Free();
			meshBuffers[id.value].weightsBuffer = nullptr;
		}
	}

	RAL::RALMeshBuffer& GetBuffer(ID_SYS_MESH id) override
	{
		if (id.value < 0 || id.value >= meshBuffers.size())
		{
			Throw(E_INVALIDARG, "renderer.SetSpecialAmbientShader(ID_SYS_MESH id, ....) - Bad id ");
		}

		auto& m = meshBuffers[id.value];
		return m;
	}

	void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
	{
		auto& m = GetBuffer(id);

		meshUpdateCount++;

		m.numberOfVertices = nVertices;

		auto* newMesh = vertices != nullptr ? DX11::CreateRALImmutableVertexBuffer(device, dc, vertices, nVertices) : nullptr;
		auto* newWeights = weights != nullptr ? DX11::CreateRALImmutableVertexBuffer(device, dc, weights, nVertices) : nullptr;

		if (m.vertexBuffer) m.vertexBuffer->Free();
		m.vertexBuffer = newMesh;

		if (m.weightsBuffer) m.weightsBuffer->Free();
		m.weightsBuffer = newWeights;
	}
};

namespace Rococo::DX11
{
	IDX11Meshes* CreateMeshManager(ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11Meshes(device, dc);
	}
}
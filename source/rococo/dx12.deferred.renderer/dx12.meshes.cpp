#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.window.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include "rococo.dx12.helpers.inl"
#include <rococo.auto-release.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing
#include <vector>
#include <rococo.renderer.h>
#include <rococo.textures.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	struct MeshBuffer
	{
		void* vertexBuffer;
		void* weightsBuffer;
		UINT numberOfVertices;
		D3D_PRIMITIVE_TOPOLOGY topology;
		ID_PIXEL_SHADER psSpotlightShader;
		ID_PIXEL_SHADER psAmbientShader;
		ID_VERTEX_SHADER vsSpotlightShader;
		ID_VERTEX_SHADER vsAmbientShader;
		bool alphaBlending;
		bool disableShadowCasting;
	};

	class DX12MeshBuffers : public IDX12MeshBuffers
	{
	private:
		std::vector<MeshBuffer> meshBuffers;
		int64 meshUpdateCount = 0;

	public:
		DX12MeshBuffers(DX12WindowInternalContext& ic)
		{

		}

		ID_SYS_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			/*
			ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
			ID3D11Buffer* weightsBuffer = weights ? DX11::CreateImmutableVertexBuffer(device, weights, nVertices) : nullptr;
			meshBuffers.push_back(MeshBuffer{ meshBuffer, weightsBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false });
			int32 index = (int32)meshBuffers.size();
			return ID_SYS_MESH(index - 1);
			*/
			Throw(0, "Not implemented");
		}

		ID_SYS_MESH CreateSkyMesh(const SkyVertex* vertices, uint32 nVertices) override
		{
			/*
			ID3D11Buffer* meshBuffer = vertices ? DX11::CreateImmutableVertexBuffer(device, vertices, nVertices) : nullptr;
			meshBuffers.push_back(MeshBuffer{ meshBuffer, nullptr, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, ID_PIXEL_SHADER(), ID_PIXEL_SHADER(), ID_VERTEX_SHADER(), ID_VERTEX_SHADER(), false, false });
			int32 index = (int32)meshBuffers.size();
			return ID_SYS_MESH(index - 1);
			*/
			Throw(0, "Not implemented");
		}

		virtual ~DX12MeshBuffers()
		{
			Clear();
		}

		void Clear() override
		{
			for (auto& x : meshBuffers)
			{
				
			}

			meshBuffers.clear();
		}

		void DeleteMesh(ID_SYS_MESH id)
		{
			Throw(0, "Not implemented");
		}

		void GetDesc(char desc[256], ID_SYS_MESH id)
		{
			if (!id || id.value >= meshBuffers.size())
			{
				SafeFormat(desc, 256, "invalid id");
			}
			else
			{
				auto& m = meshBuffers[id.value];

				if (m.vertexBuffer)
				{
					SafeFormat(desc, 256, " %p %6d %svertices. %6u bytes", m.vertexBuffer, m.numberOfVertices, m.weightsBuffer != nullptr ? "(weighted) " : "", 0);
				}
				else
				{
					SafeFormat(desc, 256, "Null object");
				}
			}
		}

		void Free() override
		{
			delete this;
		}

		void UpdateMesh(ID_SYS_MESH id, const ObjectVertex* vertices, uint32 nVertices, const BoneWeights* weights) override
		{
			if (id.value < 0 || id.value >= meshBuffers.size())
			{
				Throw(E_INVALIDARG, "%s - Bad id ", __FUNCTION__);
			}

			meshUpdateCount++;

			meshBuffers[id.value].numberOfVertices = nVertices;
		}

		void SetShadowCasting(ID_SYS_MESH id, boolean32 isActive)
		{
			if (!id || id.value >= meshBuffers.size())
			{
				Throw(E_INVALIDARG, "%s: Bad id", __FUNCTION__);
			}
			else
			{
				auto& m = meshBuffers[id.value];
				m.disableShadowCasting = !isActive;
			}
		}
	};
}

namespace Rococo::Graphics
{
	IDX12MeshBuffers* CreateMeshBuffers(DX12WindowInternalContext& ic)
	{
		return new ANON::DX12MeshBuffers(ic);
	}
}
#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>

#include <d3d11_4.h>

#include <vector>

#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Graphics;

struct LayoutId
{
	uint32 index;
};

struct MeshId
{
	uint32 index : 22;
	int32 vertexLayout : 10;
};

static_assert(sizeof(MeshId) == sizeof(uint32));

struct MeshGremlin
{
	MeshId id;
	HString name;
	std::vector<float> data;
};

enum class HLSL_Semantic
{
	BINORMAL,
	BLENDINDICES,
	BLENDWEIGHT,
	COLOR,
	NORMAL,
	POSITION,
	POSITIONT,
	PSIZE,
	TANGENT,
	TEXCOORD
};

ROCOCOAPI IVertexLayoutBuilder
{
	virtual void AddRGBAb(uint32 semanticIndex) = 0;
	virtual void AddFloat(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
	virtual void AddFloat2(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
	virtual void AddFloat3(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
	virtual void AddFloat4(HLSL_Semantic semantic, uint32 semanticIndex) = 0;
	virtual void Clear() = 0;
	virtual LayoutId Commit(const fstring& name) = 0;
	virtual void SetInputSlot(uint32 slot) = 0;
};

ROCOCOAPI IVertexLayouts
{
	virtual IVertexLayoutBuilder& GetBuilder() = 0;
	virtual void SetInputLayout(LayoutId layoutIndex) = 0;
};

ROCOCOAPI IVertexLayoutBuilderSupervisor : public IVertexLayouts
{
	virtual void Free() = 0;
};

enum class MeshType: uint32
{
	Dynamic,
	Immutable
};

struct MeshIndex
{
	uint32 index : 30;
	MeshType type : 2;
};

static_assert(sizeof(MeshIndex) == sizeof(uint32));

ROCOCOAPI IMeshPopulator
{
	virtual void Clear() = 0;
	virtual void Reserve(size_t sizeofVertex, size_t nVertices) = 0;
	virtual void AddVertex(const void* pData, size_t sizeofVertex) = 0;
	virtual MeshIndex CommitDynamic(const fstring& name, const fstring& vertexLayout) = 0;
	virtual MeshIndex CommitImmutable(const fstring& name, const fstring& vertexLayout) = 0;
	virtual void SetStride(size_t sizeofVertex) = 0;

	template<class T> void Add(const T& t)
	{
		AddVertex(&t, sizeof(t));
	}

	template<class T> void SetVertexType()
	{
		SetStride(sizeof(T));
	}
};

ROCOCOAPI IMeshCache: public IMeshPopulator
{
};

struct MeshGenie
{
	LayoutId layout;
	AutoRelease<ID3D11Buffer> buffer;
	size_t capacityInBytes;
	size_t activeBytes;
	uint32 stride;
};

namespace ANON
{
	struct VertexField
	{

	};

	const std::unordered_map<HLSL_Semantic, cstr> mapSemanticToText =
	{
		{ HLSL_Semantic::BINORMAL,		"binormal"		},
		{ HLSL_Semantic::BLENDINDICES,	"blendindices"	},
		{ HLSL_Semantic::BLENDWEIGHT,	"blendweight"	},
		{ HLSL_Semantic::COLOR,			"color"			},
		{ HLSL_Semantic::NORMAL,		"normal"		},
		{ HLSL_Semantic::POSITION,		"position"		},
		{ HLSL_Semantic::POSITIONT,		"positiont"		},
		{ HLSL_Semantic::PSIZE,			"psize"			},
		{ HLSL_Semantic::TANGENT,		"tangent"		},
		{ HLSL_Semantic::TEXCOORD,		"texcoord"		}
	};

	class LayoutBuilder: public IVertexLayoutBuilderSupervisor, private IVertexLayoutBuilder
	{
		ID3D11Device5& device;
		ID3D11DeviceContext4& dc;
		struct VertexLayout
		{
			HString name;
			std::vector<D3D11_INPUT_ELEMENT_DESC> descs;
			AutoRelease<ID3D11InputLayout> layout;
		};

		std::vector<VertexLayout> layouts;
		stringmap<LayoutId> nameToLayout;
		VertexLayout descBuilder;

	public:
		LayoutBuilder(ID3D11Device5& ref_device, ID3D11DeviceContext4& ref_dc): device(ref_device), dc(ref_dc)
		{
			
		}

		LayoutId Get(const fstring& name)
		{
			if (name.length < 1) Throw(0, "% [name] blank", __FUNCTION__);
			auto i = nameToLayout.find(name);
			if (i == nameToLayout.end())
			{
				Throw(0, "%(%s) - name not found", __FUNCTION__, name.buffer);
			}
			return i->second;
		}

		IVertexLayoutBuilder& GetBuilder() override
		{
			return *this;
		}

		void CreateLayout(LayoutId id, const void* shaderSignature, size_t lenBytes)
		{
			if (id.index >= layouts.size())
			{
				Throw(0, "%s: bad layout index", __FUNCTION__);
			}

			auto& l = layouts[id.index];

			ID3D11InputLayout* layout = nullptr;
			HRESULT hr = device.CreateInputLayout(l.descs.data(), (uint32) l.descs.size(), shaderSignature, lenBytes, &layout);
			if FAILED(hr)
			{
				Throw(0, "%s: layout %s could not be created. Check the output for warning messages", __FUNCTION__, l.name.c_str());
			}

			l.layout = layout;
		}

		void SetInputLayout(LayoutId id) override
		{
			if (id.index >= layouts.size())
			{
				Throw(0, "%s: bad layout index", __FUNCTION__);
			}

			auto& l = layouts[id.index];
			if (l.layout == nullptr) Throw(0, "%s: no layout created for %s", __FUNCTION__, l.name.c_str());
			dc.IASetInputLayout(l.layout);
		}

		void Clear() override
		{
			descBuilder.name = "";
			descBuilder.layout = nullptr;
			descBuilder.descs.clear();
		}

		LayoutId Commit(const fstring& name) override
		{
			int32 index = 0;

			if (name.length < 1)
			{
				Throw(0, "%s: blank [name]", __FUNCTION__);
			}

			auto i = nameToLayout.find(name);
			if (i != nameToLayout.end())
			{
				return i->second;
			}

			if (descBuilder.descs.empty())
			{
				Throw(0, "%s(%s): the builder was empty.", __FUNCTION__, name.buffer);
			}

			descBuilder.name = name;
			layouts.push_back(descBuilder);

			Clear();

			auto id = LayoutId{ (uint32)(layouts.size() - 1) };

			nameToLayout.insert(name, id);
			return id;
		}

		uint32 inputSlot = 0;
		D3D11_INPUT_CLASSIFICATION inputClass = D3D11_INPUT_PER_VERTEX_DATA;
		uint32 instanceDataStepRate = 0;

		void SetInputSlot(uint32 slot) override
		{
			if (slot >= 16) Throw(0, "%s: [%d] - maximum value is 15", __FUNCTION__, slot);
			this->inputSlot = slot;
		}

		void InitStandardDesc(D3D11_INPUT_ELEMENT_DESC& desc, HLSL_Semantic semantic, uint32 semanticIndex)
		{
			auto i = mapSemanticToText.find(semantic);
			if (i == mapSemanticToText.end())
			{
				Throw(0, "%s: unknown semantic %d", __FUNCTION__, semantic);
			}

			desc.SemanticName = i->second;
			desc.SemanticIndex = semanticIndex;
			desc.Format = DXGI_FORMAT_R32_FLOAT;
			desc.InputSlot = inputSlot;
			desc.InputSlotClass = inputClass;
			desc.InstanceDataStepRate = 0;
			desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		}

		void AddRaw(HLSL_Semantic semantic, uint32 semanticIndex, DXGI_FORMAT format)
		{
			D3D11_INPUT_ELEMENT_DESC desc;
			InitStandardDesc(desc, semantic, semanticIndex);

			for (auto& line : descBuilder.descs)
			{
				if (line.SemanticName == desc.SemanticName && line.SemanticIndex == desc.SemanticIndex)
				{
					Throw(0, "%s(%s, %d, ...) duplicate index", __FUNCTION__, desc.SemanticName, semanticIndex);
				}
			}

			desc.Format = format;
			descBuilder.descs.push_back(desc);
		}

		void AddRGBAb(uint32 semanticIndex)
		{
			AddRaw(HLSL_Semantic::COLOR, semanticIndex, DXGI_FORMAT_R8G8B8A8_UNORM);
		}

		void AddFloat(HLSL_Semantic semantic, uint32 semanticIndex)  override
		{
			AddRaw(semantic, semanticIndex, DXGI_FORMAT_R32_FLOAT);
		}

		void AddFloat2(HLSL_Semantic semantic, uint32 semanticIndex)  override
		{
			AddRaw(semantic, semanticIndex, DXGI_FORMAT_R32G32_FLOAT);
		}

		void AddFloat3(HLSL_Semantic semantic, uint32 semanticIndex)  override
		{
			AddRaw(semantic, semanticIndex, DXGI_FORMAT_R32G32B32_FLOAT);
		}

		void AddFloat4(HLSL_Semantic semantic, uint32 semanticIndex)  override
		{
			AddRaw(semantic, semanticIndex, DXGI_FORMAT_R32G32B32A32_FLOAT);
		}

		void Free() override
		{
			delete this;
		}
	};

	class MeshCache : public IMeshCache 
	{
		std::vector<uint8> tempData;
		size_t sizeofVertex = 0;

		std::vector<MeshGenie> meshes;
		stringmap<MeshIndex> nameToId;

		LayoutBuilder layouts;

		ID3D11Device5& device;
		ID3D11DeviceContext4& dc;

	public:
		MeshCache(ID3D11Device5& ref_device, ID3D11DeviceContext4& ref_dc): 
			layouts(ref_device, ref_dc), device(ref_device), dc(ref_dc)
		{

		}

		void Clear() override
		{
			tempData.clear();
			sizeofVertex = 0;
		}

		void Ready(MeshIndex id, uint32 slot)
		{
			auto index = id.index - 1;
			if (index >= meshes.size())
			{
				Throw(0, "%s: bad MeshIndex id", __FUNCTION__);
			}

			auto& g = meshes[index];

			uint32 offset = 0;
			dc.IASetVertexBuffers(slot, 1, &g.buffer, &g.stride, &offset);
			layouts.SetInputLayout(g.layout);
			
		}

		void SetStride(size_t sizeofVertex) override
		{
			if (sizeofVertex < 2 || sizeofVertex > 4096) Throw(0, "sizeofVertex domain is [2,4096]");
			this->sizeofVertex = sizeofVertex;
		}

		MeshIndex CommitDynamic(const fstring& name, const fstring& vertexLayout) override
		{
			if (name.length < 1) Throw(0, "%s: blank name", __FUNCTION__);

			if (sizeofVertex == 0)
			{
				Throw(0, "%s(%s,%s) - sizeofVertex was zero", __FUNCTION__, name.buffer, vertexLayout.buffer);
			}
			
			if ((tempData.size() % sizeofVertex) != 0)
			{
				Throw(0, "%s(%s,%s) - vertex queue length was not integer multiple of sizeofVertex", __FUNCTION__, name.buffer, vertexLayout.buffer);
			}

			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = (uint32)tempData.size();
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = tempData.data();
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			auto i = nameToId.find(name);
			if (i != nameToId.end())
			{
				auto& genie = meshes[i->second.index - 1];
				if (i->second.type != MeshType::Dynamic)
				{
					Throw(0, "%s: %s is bound already. And it is not marked dynamic", __FUNCTION__, name.buffer);
				}

				if (tempData.size() > genie.capacityInBytes)
				{
					Throw(0, "%s(%s) cannot update dynamic buffer. The vertex queue was longer than the buffer capacity", __FUNCTION__, name);
				}

				genie.activeBytes = 0;
				D3D11_MAPPED_SUBRESOURCE x;
				VALIDATE_HR(dc.Map(genie.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &x));
				memcpy(x.pData, tempData.data(), tempData.size());
				dc.Unmap(genie.buffer, 0);
				genie.activeBytes = tempData.size();

				tempData.clear();
				return i->second;
			}
			else
			{
				MeshGenie g;
				g.layout = layouts.Get(vertexLayout);
				g.capacityInBytes = desc.ByteWidth;
				g.activeBytes = desc.ByteWidth;
				g.stride = (uint32) sizeofVertex;
				VALIDATE_HR(device.CreateBuffer(&desc, &data, &g.buffer));
				meshes.push_back(g);

				MeshIndex meshId;
				meshId.index = meshes.size();
				meshId.type = MeshType::Immutable;
				nameToId.insert(name, meshId);

				tempData.clear();

				return meshId;
			}
		}

		MeshIndex CommitImmutable(const fstring& name, const fstring& vertexLayout) override
		{
			if (name.length < 1) Throw(0, "%s: blank name", __FUNCTION__);

			if (sizeofVertex == 0)
			{
				Throw(0, "%s(%s,%s) - sizeofVertex was zero", __FUNCTION__, name.buffer, vertexLayout.buffer);
			}

			if ((tempData.size() % sizeofVertex) != 0)
			{
				Throw(0, "%s(%s,%s) - vertex queue length was not integer multiple of sizeofVertex", __FUNCTION__, name.buffer, vertexLayout.buffer);
			}

			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = (uint32)tempData.size();
			desc.Usage = D3D11_USAGE_IMMUTABLE;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.StructureByteStride = 0;
			D3D11_SUBRESOURCE_DATA data;
			data.pSysMem = tempData.data();
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;

			auto i = nameToId.find(name);
			if (i != nameToId.end())
			{
				auto& genie = meshes[i->second.index - 1];
				if (i->second.type != MeshType::Immutable)
				{
					Throw(0, "%s: %s is bound already. And it is not marked immutable", __FUNCTION__, name.buffer);
				}
				genie.buffer = nullptr;
				genie.capacityInBytes = 0;
				genie.activeBytes = 0;
				VALIDATE_HR(device.CreateBuffer(&desc, &data, &genie.buffer));
				genie.capacityInBytes = desc.ByteWidth;
				genie.activeBytes = desc.ByteWidth;
				tempData.clear();
				return i->second;
			}
			else
			{
				MeshGenie g;
				g.layout = layouts.Get(vertexLayout);
				g.capacityInBytes = desc.ByteWidth;
				g.activeBytes = desc.ByteWidth;
				g.stride = (uint32) sizeofVertex;
				VALIDATE_HR(device.CreateBuffer(&desc, &data, &g.buffer));
				meshes.push_back(g);

				MeshIndex meshId;
				meshId.index = meshes.size();
				meshId.type = MeshType::Immutable;
				nameToId.insert(name, meshId);

				tempData.clear();

				return meshId;
			}
		}

		void AddVertex(const void* pData, size_t sizeofVertex) override
		{
			const uint8* p = (const uint8*) pData;
			for (size_t i = 0; i < sizeofVertex; ++i)
			{
				tempData.push_back(p[i]);
			}
		}

		void Reserve(size_t sizeofVertex, size_t nVertices) override
		{
			try
			{
				meshes.reserve(sizeofVertex * nVertices);
			}
			catch (...)
			{
				Throw(0, "%s. Bad allocation", __FUNCTION__);
			}
		}
	public:
		IVertexLayoutBuilderSupervisor& Layouts();
	};
}

namespace Rococo::Graphics
{
	IMeshCache* CreateMeshCache(ID3D11Device5& device, ID3D11DeviceContext4& dc)
	{
		return new ANON::MeshCache(device, dc);
	}
}
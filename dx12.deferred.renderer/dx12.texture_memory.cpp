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
#include <rococo.hashtable.h>
#include <rococo.handles.h>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	inline uint64 ComputeByteSize(const TextureRecordData& data)
	{
		if (data.alphaPixels)
		{
			return sizeof(RGBAb) * data.meta.span.x * data.meta.span.y;
		}
		else if (data.alphaPixels)
		{
			return sizeof(GRAYSCALE) * data.meta.span.x * data.meta.span.y;
		}
		else
		{
			return 0;
		}
	}

	template<class T>
	inline uint64 ComputeByteSize(Vec2i span)
	{
		uint64 nPixels = ((uint64)span.x) * ((uint64)span.y);
		return sizeof(T) * nPixels;
	}

	void AppendResourceDescriptors_Materials(std::vector<D3D12_RESOURCE_DESC>& descs, int32 span, uint16 nMaterials)
	{
		D3D12_RESOURCE_DESC desc;
		desc.Alignment = ComputeByteSize<RGBAb>(Vec2i{ span,span }) < 4_kilobytes ? 4_kilobytes : 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = nMaterials;
		desc.Width = span;
		desc.Height = span;
		desc.MipLevels = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc = { 1 , 0 };
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		descs.push_back(desc);
 	}

	void AppendResourceDescriptors_ArrayFont(std::vector<D3D12_RESOURCE_DESC>& descs, int32 span, uint16 nMaterials)
	{
		D3D12_RESOURCE_DESC desc;
		desc.Alignment = ComputeByteSize<GRAYSCALE>(Vec2i{ span,span }) < 4_kilobytes ? 4_kilobytes : 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = nMaterials;
		desc.Width = span;
		desc.Height = span;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.SampleDesc = { 1 , 0 };
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		descs.push_back(desc);
	}

	void AppendResourceDescriptors_ScalableFont(std::vector<D3D12_RESOURCE_DESC>& descs, int32 span)
	{
		D3D12_RESOURCE_DESC desc;
		desc.Alignment = ComputeByteSize<GRAYSCALE>(Vec2i{ span,span }) < 4_kilobytes ? 4_kilobytes : 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = 1;
		desc.Width = span;
		desc.Height = span;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.SampleDesc = { 1 , 0 };
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		descs.push_back(desc);
	}

	void AppendResourceDescriptors_Sprites(std::vector<D3D12_RESOURCE_DESC>& descs, int32 span, uint16 nSpritePanes)
	{
		D3D12_RESOURCE_DESC desc;
		desc.Alignment = ComputeByteSize<RGBAb>(Vec2i{ span,span }) < 4_kilobytes ? 4_kilobytes : 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.DepthOrArraySize = nSpritePanes;
		desc.Width = span;
		desc.Height = span;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc = { 1 , 0 };
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		descs.push_back(desc);
	}

	class MPLAT_DX12TxMemory : public ITextureMemory
	{
	private:
		DX12WindowInternalContext ic;

		uint64 maxTxMemory = 0;
		uint64 reserveAt = 0;

		AutoRelease<ID3D12Heap> heap;

	public:
		MPLAT_DX12TxMemory(DX12WindowInternalContext ref_ic) : ic(ref_ic) 
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO info;
			VALIDATE_HR(ic.adapter.QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info));

			std::vector<D3D12_RESOURCE_DESC> descs;
			AppendResourceDescriptors_Materials(descs, 1024, 64);
			AppendResourceDescriptors_ArrayFont(descs, 128, 128);
			AppendResourceDescriptors_ArrayFont(descs, 64, 128);
			AppendResourceDescriptors_ArrayFont(descs, 32, 128);
			AppendResourceDescriptors_ArrayFont(descs, 16, 128);
			AppendResourceDescriptors_ScalableFont(descs, 1024);
			AppendResourceDescriptors_Sprites(descs, 2048, 4);

			D3D12_RESOURCE_ALLOCATION_INFO allocInfo =
				ic.device.GetResourceAllocationInfo(0, (uint32) descs.size(), descs.data());

			DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalInfo;
			VALIDATE_HR(ic.adapter.QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalInfo));
			bool isUMA = (nonLocalInfo.AvailableForReservation == 0);

			maxTxMemory = info.CurrentReservation >> 1;

			if (allocInfo.SizeInBytes > maxTxMemory)
			{
				Throw(0, "Memory requirements too high for the hardware. Require %llu MB, but only have %llu MB", allocInfo.SizeInBytes / 1_megabytes, maxTxMemory / 1_megabytes);
			}

			D3D12_HEAP_DESC heapDesc;
			heapDesc.Alignment = allocInfo.Alignment;
			heapDesc.SizeInBytes = allocInfo.SizeInBytes;
			heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			heapDesc.Properties.VisibleNodeMask = 0;
			heapDesc.Properties.CreationNodeMask = 0;
			heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapDesc.Properties.MemoryPoolPreference = isUMA ? D3D12_MEMORY_POOL_UNKNOWN : D3D12_MEMORY_POOL_L1;
			VALIDATE_HR(ic.device.CreateHeap(&heapDesc, _uuidof(ID3D12Heap), (void**) &heap));

			maxTxMemory = info.CurrentReservation >> 1;
			reserveAt = 0;
		}

		virtual ~MPLAT_DX12TxMemory()
		{

		}

		void Commit(const TextureRecordData& data) override
		{
			uint64 byteSize = ComputeByteSize(data);

		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	ITextureMemory* Create_MPlat_Standard_TextureMemory(DX12WindowInternalContext& ic)
	{
		return new ANON::MPLAT_DX12TxMemory(ic);
	}
}
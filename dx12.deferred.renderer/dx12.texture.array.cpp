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
	class DX12TextureArray : public IDX12TextureArray
	{
		CompileTimeStringConstant friendlyName;
		int32 maxWidth = 1024;
		int32 nTextureCount = 0;
		Vec2i span{ 0,0 };
		DX12TextureArraySpec spec;
		DX12WindowInternalContext ic;
		bool isMipMapped;

		AutoRelease<ID3D12Resource> tx3D;
	public:
		DX12TextureArray(CompileTimeStringConstant pName, bool bIsMipMapped, DX12TextureArraySpec& refSpec, DX12WindowInternalContext& refIc):
			friendlyName(pName), spec(refSpec), ic(refIc), isMipMapped(bIsMipMapped)
		{
			
		}

		virtual ~DX12TextureArray()
		{

		}

		void Free() override
		{
			delete this;
		}

		void SetDimensions(int32 width, int32 height, int nElements) override
		{
			span = { width, height };
			nTextureCount = nElements;

			tx3D = spec.txMemory.Commit2DArray(friendlyName, width, height, nElements, TextureInternalFormat::Greyscale_R8, isMipMapped);
		}

		void WriteSubImage(int32 index, const GRAYSCALE* pixels, int32 width, int32 height) override
		{
			DXGI_QUERY_VIDEO_MEMORY_INFO info;
			ic.adapter.QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info);
			bool isUMA = info.AvailableForReservation == 0;

			Throw(0, "Not implemented - I do not have the resources to develop this on all the required architecture");

			if (isUMA)
			{
				uint32 srcRowPitch, srcDepthPitch;
				D3D12_BOX box;
				box.left = 0;
				box.right = width;
				box.top = 0;
				box.bottom = height;
				box.front = index;
				box.back = index + 1;

				srcRowPitch = width;
				srcDepthPitch = width * height;
				VALIDATE_HR(tx3D->WriteToSubresource(0, &box, pixels, srcRowPitch, srcDepthPitch));
			}
			else
			{

			}
		}

		void WriteSubImage(int32 index, const RGBAb* pixels, const GuiRect& rect) override
		{
			Throw(0, "Not implemented");
		}

		int MaxWidth() const override
		{
			return maxWidth;
		}

		int TextureCount() const override
		{
			return nTextureCount;
		}
	};
}

namespace Rococo::Graphics
{
	IDX12TextureArray* CreateDX12TextureArray(CompileTimeStringConstant friendlyName, bool isMipMapped, DX12TextureArraySpec& spec, DX12WindowInternalContext& ic)
	{
		return new ANON::DX12TextureArray(friendlyName, isMipMapped, spec, ic);
	}
}
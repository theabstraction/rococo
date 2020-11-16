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
	public:
		DX12TextureArray(DX12TextureArraySpec& spec, DX12WindowInternalContext& ic)
		{

		}

		virtual ~DX12TextureArray()
		{

		}

		void Free() override
		{
			delete this;
		}

		void ResetWidth(int32 width, int32 height) override
		{
			Throw(0, "Not implemented");
		}

		void Resize(int32 nElements) override
		{
			Throw(0, "Not implemented");
		}

		void WriteSubImage(int32 index, const GRAYSCALE* pixels, int32 width, int32 height) override
		{
			Throw(0, "Not implemented");
		}

		void WriteSubImage(int32 index, const RGBAb* pixels, const GuiRect& rect) override
		{
			Throw(0, "Not implemented");
		}

		ID_TEXTURE LoadTexture(IBuffer& rawImageBuffer, cstr uniqueName) override
		{
			Throw(0, "Not implemented");
		}

		int MaxWidth() const override
		{
			Throw(0, "Not implemented");
		}

		int TextureCount() const override
		{
			Throw(0, "Not implemented");
		}
	};
}

namespace Rococo::Graphics
{
	IDX12TextureArray* CreateDX12TextureArray(DX12TextureArraySpec& spec, DX12WindowInternalContext& ic)
	{
		return new ANON::DX12TextureArray(spec, ic);
	}
}
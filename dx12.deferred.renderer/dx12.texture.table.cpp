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

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	struct TextureBind
	{
		void* texture;
		void* shaderView;
		void* renderView = nullptr;
		void* depthView = nullptr;
	};

	struct TextureItem
	{
		ID_TEXTURE id;
		std::string name;

		bool operator < (const TextureItem& other)
		{
			return name < other.name;
		}
	};

	std::vector<TextureItem> orderedTextureList;

	class DX12TextureTable : public IDX12TextureTable
	{
	private:
		std::vector<TextureBind> textures;
		stringmap<ID_TEXTURE> mapNameToTexture;
	public:
		DX12TextureTable(IInstallation& installation, DX12WindowInternalContext& ic)
		{

		}

		ID_TEXTURE LoadTexture(IBuffer& buffer, cstr uniqueName) override
		{
			return ID_TEXTURE::Invalid();
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	IDX12TextureTable* CreateTextureTable(IInstallation& installation, DX12WindowInternalContext& ic)
	{
		return new ANON::DX12TextureTable(installation, ic);
	}
}
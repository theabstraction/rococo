#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include <algorithm>
//#include "rococo.imaging.h"

using namespace Rococo::DX11;

struct TextureItem
{
	ID_TEXTURE id;
	std::string name;

	bool operator < (const TextureItem& other)
	{
		return name < other.name;
	}
};

struct DX11TextureManager : IDX11TextureManager
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	AutoFree<IExpandingBuffer> loadBuffer;
	DX11::TextureLoader textureLoader;
	std::vector<DX11::TextureBind> textures;
	stringmap<ID_TEXTURE> mapNameToTexture;
	stringmap<ID_TEXTURE> nameToGenericTextureId;
	std::vector<TextureItem> orderedTextureList;
	std::unordered_map<ID_TEXTURE, IDX11TextureArray*, ID_TEXTURE> genericTextureArray;

	DX11TextureManager(IInstallation& installation, ID3D11Device& _device, ID3D11DeviceContext& _dc):
		device(_device),
		dc(_dc),
		loadBuffer(CreateExpandingBuffer(256_kilobytes)),
		textureLoader(installation, device, dc, *loadBuffer)
	{

	}

	int64 Size() const override
	{
		return (int64) textures.size() + genericTextureArray.size();
	}

	virtual ~DX11TextureManager()
	{
		for (auto& t : textures)
		{
			if (t.texture) t.texture->Release();
			if (t.shaderView) t.shaderView->Release();
			if (t.renderView) t.renderView->Release();
			if (t.depthView) t.depthView->Release();
		}

		for (auto& t : genericTextureArray)
		{
			t.second->Free();
		}
	}
		
	void Free() override
	{
		delete this;
	}

	ID_TEXTURE LoadAlphaTextureArray(cstr uniqueName, Vec2i span, int32 nElements, ITextureLoadEnumerator& enumerator) override
	{
		auto i = nameToGenericTextureId.find(uniqueName);
		if (i != nameToGenericTextureId.end()) return i->second;

		IDX11TextureArray* array = DX11::LoadAlphaTextureArray(device, span, nElements, enumerator, dc);

		auto id = ID_TEXTURE{ (size_t)array | 0x8000000000000000LL };
		nameToGenericTextureId[uniqueName] = id;
		genericTextureArray[id] = array;
		return id;
	}

	void SetGenericTextureArray(ID_TEXTURE id) override
	{
		auto i = genericTextureArray.find(id);
		if (i != genericTextureArray.end())
		{
			ID3D11ShaderResourceView* gtaViews[1] = { i->second->View() };
			dc.PSSetShaderResources(TXUNIT_GENERIC_TXARRAY, 1, gtaViews);
		}
	}

	ID_TEXTURE LoadTexture(IBuffer& buffer, cstr uniqueName) override
	{
		auto i = mapNameToTexture.find(uniqueName);
		if (i != mapNameToTexture.end())
		{
			return i->second;
		}

		auto bind = textureLoader.LoadColourBitmap(uniqueName);
		textures.push_back(bind);

		auto id = ID_TEXTURE(textures.size());
		mapNameToTexture.insert(uniqueName, id);

		orderedTextureList.clear();

		return id;
	}

	IDX11TextureLoader& Loader() override
	{
		return textureLoader;
	}

	ID_TEXTURE CreateDepthTarget(cstr targetName, int32 width, int32 height) override
	{
		TextureBind tb = DX11::CreateDepthTarget(device, width, height);
		textures.push_back(tb);
		ID_TEXTURE id = ID_TEXTURE{ textures.size() };
		char name[32];
		SafeFormat(name, "%s_%llu", targetName, id.value);
		mapNameToTexture[targetName] = id;
		return id;
	}

	ID_TEXTURE CreateRenderTarget(cstr renderTargetName, int32 width, int32 height) override
	{
		TextureBind tb = DX11::CreateRenderTarget(device, width, height);
		textures.push_back(tb);
		auto id = ID_TEXTURE(textures.size());
		char name[32];
		SafeFormat(name, "%s_%llu", renderTargetName, id.value);
		mapNameToTexture[name] = id;
		return id;
	}

	ID_TEXTURE FindTexture(cstr name) const
	{
		auto i = mapNameToTexture.find(name);
		return i != mapNameToTexture.end() ? i->second : ID_TEXTURE::Invalid();
	}

	bool TryGetTextureDesc(TextureDesc& desc, ID_TEXTURE id) const override
	{
		size_t index = id.value - 1;
		if (index < 0 || index >= textures.size())
		{
			return false;
		}

		const auto& t = textures[index];
		if (!t.texture) return false;
		GetTextureDesc(desc, *t.texture);
		return true;
	}

	void ShowTextureVenue(IMathsVisitor& visitor) override
	{
		if (orderedTextureList.empty())
		{
			for (auto& t : mapNameToTexture)
			{
				orderedTextureList.push_back({ t.second, t.first });
			}

			std::sort(orderedTextureList.begin(), orderedTextureList.end());
		}

		for (auto& t : orderedTextureList)
		{
			auto& tx = textures[t.id.value - 1];

			D3D11_TEXTURE2D_DESC desc;
			tx.texture->GetDesc(&desc);
			char name[64];
			SafeFormat(name, 64, "TxId %u", t.id.value);
			visitor.ShowSelectableString("overlay.select.texture", name, "  %s - 0x%p. %d x %d. %d levels", t.name.c_str(), (const void*)tx.texture, desc.Width, desc.Height, desc.MipLevels);
		}
	}

	TextureBind& GetTexture(ID_TEXTURE id) override
	{
		size_t index = id.value - 1;
		if (index >= textures.size())
		{
			Throw(0, "%s: Bad texture id", __FUNCTION__);
		}

		auto& t = textures[index];
		return t;
	}
};

namespace Rococo::DX11
{
	IDX11TextureManager* CreateTextureManager(IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11TextureManager(installation, device, dc);
	}
}
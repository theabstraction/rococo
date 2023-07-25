#include "dx11.renderer.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include <algorithm>
#include <string>
#include "rococo.imaging.h"

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

struct MipMappedTextureArray : IMipMappedTextureArrayContainerSupervisor
{
	//AutoFree<IDX11TextureArray> txArray;

	MipMappedTextureArray()
	{

	}

	void SetDimensions(uint32 span, uint32 numberOfElements)
	{
		UNUSED(span);
		UNUSED(numberOfElements);
	}

	void Free() override
	{
		delete this;
	}
};

struct DX11TextureManager : IDX11TextureManager, ICubeTextures
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	AutoFree<IExpandingBuffer> loadBuffer;
	DX11::TextureLoader textureLoader;
	std::vector<DX11::TextureBind> textures;
	stringmap<ID_TEXTURE> mapNameToTexture;
	stringmap<ID_TEXTURE> nameToGenericTextureId;
	std::vector<TextureItem> orderedTextureList;
	AutoFree<IDX11CubeTextures> cubeTextures;
	std::unordered_map<ID_TEXTURE, IDX11TextureArray*, ID_TEXTURE> genericTextureArray;
	AutoFree<IDX11Materials> materials;
	TextureBind backBuffer;

	DX11TextureManager(IO::IInstallation& installation, ID3D11Device& _device, ID3D11DeviceContext& _dc):
		device(_device),
		dc(_dc),
		loadBuffer(CreateExpandingBuffer(256_kilobytes)),
		textureLoader(installation, device, dc, *loadBuffer),
		cubeTextures(CreateCubeTextureManager(device, dc)),
		materials(CreateMaterials(installation, device, dc))
	{

	}

	IMipMappedTextureArrayContainerSupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span)
	{
		AutoFree<MipMappedTextureArray> tx = new MipMappedTextureArray();

		try
		{
			tx->SetDimensions(span, numberOfElements);
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "%s: %s\nError allocating %u elements of span %u x %u", __FUNCTION__, numberOfElements, span, span);
		}
		catch (std::exception& stdEx)
		{
			Throw(0, "%s: %s\nError allocating %u elements of span %u x %u", __FUNCTION__, stdEx.what(), numberOfElements, span, span);
		}
		
		tx.Release();

		return tx;
	}

	IDX11Materials& Materials() override
	{
		return *materials;
	}

	ICubeTextures& CubeTextures() override
	{
		return *this;
	}

	ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) override
	{
		return cubeTextures->CreateCubeTexture(textureLoader, path, extension);
	}

	void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) override
	{
		return cubeTextures->SyncCubeTexture(XMaxFace, XMinFace, YMaxFace, YMinFace, ZMaxFace, ZMinFace, materials->Textures());
	}

	ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id)
	{
		return cubeTextures->GetShaderView(id);
	}

	ID3D11ShaderResourceView* GetCubeShaderResourceView()
	{
		return cubeTextures->ShaderResourceView();
	}

	bool DecompressJPeg(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const
	{
		bool decompressed = Imaging::DecompressJPeg(loadEvents, sourceBuffer, dataLengthBytes);
		return decompressed;
	}

	bool DecompressTiff(Imaging::IImageLoadEvents& loadEvents, const unsigned char* sourceBuffer, size_t dataLengthBytes) const
	{
		bool decompressed = Imaging::DecompressTiff(loadEvents, sourceBuffer, dataLengthBytes);
		return decompressed;
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
				TextureItem item { t.second, (cstr) t.first };
				orderedTextureList.push_back(item);
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
	IDX11TextureManager* CreateTextureManager(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new DX11TextureManager(installation, device, dc);
	}
}
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

enum class EComponentType
{
	UNORM_8BITS, // 8 bits per component, interpreted as 0.0 to 1.0
};

struct MipMappedTextureArray : Textures::IMipMappedTextureArraySupervisor
{
	ID3D11Device& device;
	ID3D11DeviceContext* activeDC;
	uint32 span;
	uint32 numberOfMipLevels;
	uint32 numberOfElements;
	TextureBind tb;
	TextureBind tbExportTexture;
	DXGI_FORMAT format;
	TextureArrayCreationFlags flags;

	MipMappedTextureArray(ID3D11Device& _device, ID3D11DeviceContext& dc, EComponentType componentType, uint32 numberOfComponents, uint32 _span, uint32 _numberOfElements, TextureArrayCreationFlags _flags):
		device(_device), span(_span), numberOfElements(_numberOfElements), numberOfMipLevels(0), format(DXGI_FORMAT::DXGI_FORMAT_UNKNOWN), activeDC(&dc), flags(_flags)
	{
		if (_span == 0)
		{
			Throw(0, "MipMappedTextureArray: Zero span");
		}

		if (_numberOfElements > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
		{
			Throw(0, "%s: DirectX11 has a limit of %u elements per array. %u were requested", __FUNCTION__, D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, numberOfElements);
		}

		uint32 q = span;
		while (q > 1)
		{
			q = q >> 1;
			numberOfMipLevels++;
		}

		uint32 requiredSpan = 1 << numberOfMipLevels;
		if (requiredSpan != span)
		{
			Throw(0, "MipMappedTextureArray: Bad span (%u). Try %u.", span, requiredSpan);
		}

		if (componentType != EComponentType::UNORM_8BITS)
		{
			Throw(0, "MipMappedTextureArray: Unknown component type");
		}

		cstr formatName = "unknown";

		switch(numberOfComponents)
		{
		case 0:
			Throw(0, "MipMappedTextureArray: zero components");
			break;
		case 1:
			format = DXGI_FORMAT_R8_UNORM;
			formatName = "R8";
			break;
		case 2:
			Throw(0, "MipMappedTextureArray: two components specified, but not yet implemented for the Rococo DirectX11 renderer");
			break;
		case 3:
			Throw(0, "MipMappedTextureArray: three components specified, but DirectX 11 does not support 3 components at 8-bits per component");
			break;
		case 4:
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			formatName = "RGBAb";
			break;
		default:
			Throw(0, "MipMappedTextureArray: %u components specified, but DirectX 11 only supports 1 or 4 channels at 8-bits per channel", numberOfComponents);
			break;
		}

		UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (flags.allowMipMapGeneration)
		{
			bindFlags |= D3D11_BIND_RENDER_TARGET;
		}

		D3D11_TEXTURE2D_DESC textureArrayDesc;
		textureArrayDesc.Width = span;
		textureArrayDesc.Height = span;
		textureArrayDesc.MipLevels = 0;
		textureArrayDesc.ArraySize = (UINT)numberOfElements;
		textureArrayDesc.Format = format;
		textureArrayDesc.SampleDesc.Count = 1;
		textureArrayDesc.SampleDesc.Quality = 0;
		textureArrayDesc.Usage = D3D11_USAGE_DEFAULT;
		textureArrayDesc.BindFlags = bindFlags;
		textureArrayDesc.CPUAccessFlags = 0;
		textureArrayDesc.MiscFlags = flags.allowMipMapGeneration ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
		
		try
		{
			VALIDATEDX11(device.CreateTexture2D(&textureArrayDesc, nullptr, &tb.texture));
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "DirectX11 refuses to allow a MipMappedTextureArray(%s) to have span %u x %u and have %u elements.", formatName, span, span, numberOfElements);
		}

		if (!flags.allowCPUread)
		{
			return;
		}

		// To export textures that are mip mapped we need to first mip map the texture then copy the resource to a texture with CPU read enabled.
		// We cannot directly export the mip mapped texture, because it is shader bound, which prohibits CPU reads.

		D3D11_TEXTURE2D_DESC textureForExport;
		textureForExport.Width = span;
		textureForExport.Height = span;
		textureForExport.MipLevels = 0;
		textureForExport.ArraySize = 1;
		textureForExport.Format = format;
		textureForExport.SampleDesc.Count = 1;
		textureForExport.SampleDesc.Quality = 0;
		textureForExport.Usage = D3D11_USAGE_STAGING;
		textureForExport.BindFlags = 0;
		textureForExport.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		textureForExport.MiscFlags = 0;

		try
		{
			VALIDATEDX11(device.CreateTexture2D(&textureForExport, nullptr, &tbExportTexture.texture));
		}
		catch (IException& ex)
		{
			tb.texture->Release();
			Throw(ex.ErrorCode(), "DirectX11 refuses to allow the mip-map export texture (%s) to have span %u x %u", formatName, span, span);
		}
	}

	~MipMappedTextureArray()
	{
		if (tb.texture) tb.texture->Release();
		if (tbExportTexture.texture) tbExportTexture.texture->Release();
	}

	void GenerateMipMappedSubLevels(uint32 index, uint32 mipMapLevel) override
	{
		if (!activeDC)
		{
			Throw(0, "%s: no activeDC", __FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		int mipSlice = numberOfMipLevels - mipMapLevel - 1;

		AutoRelease<ID3D11ShaderResourceView> pShaderResourceView = nullptr;
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		desc.Format = format;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		desc.Texture2DArray.ArraySize = 1;
		desc.Texture2DArray.FirstArraySlice = index;
		desc.Texture2DArray.MipLevels = (UINT)-1;
		desc.Texture2DArray.MostDetailedMip = mipSlice;

		HRESULT hr = device.CreateShaderResourceView(tb.texture, &desc, &pShaderResourceView);
		if FAILED(hr)
		{
			Throw(hr, "%s failed to create shader view", __FUNCTION__);
		}

		activeDC->GenerateMips(pShaderResourceView);
	}

	uint32 TexelSpan() const override
	{
		return span;
	}

	uint32 NumberOfElements() const override
	{
		return numberOfElements;
	}

	uint32 NumberOfMipLevels() const override
	{
		return numberOfMipLevels;
	}

	bool ReadSubImage(uint32 index, uint32 mipMapLevel, uint32 bytesPerTexel, uint8* mipMapLevelDataDestination) override
	{
		if (!flags.allowCPUread)
		{
			Throw(0, "The texture array was not set to allow CPU read.");
		}

		if (!activeDC)
		{
			Throw(0, "%s: no active DC", __FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			Throw(0, "%s: format is not RGBA but image passed was RGBA", __FUNCTION__);
		}

		uint32 levelSpan = 1 << mipMapLevel;
		UINT linePitch = levelSpan * bytesPerTexel;
		UINT srcDepth = levelSpan * linePitch;

		UINT mipSlice = numberOfMipLevels - mipMapLevel - 1;

		UINT exportSubresourceIndex = D3D11CalcSubresource(mipSlice, 0, numberOfMipLevels);
		UINT arraySubresourceIndex = D3D11CalcSubresource(mipSlice, index, numberOfMipLevels);

		// To read the array we first copy to the export texture that has CPU read enabled.
		// We cannot read the array directly to CPU, because CPU read is prohibited with shader bound textures.

		activeDC->CopySubresourceRegion(tbExportTexture.texture, exportSubresourceIndex, 0, 0, 0, tb.texture, arraySubresourceIndex, nullptr);
	
		bool blockingCall = true;

		if (!tb.texture) return false;

		D3D11_MAPPED_SUBRESOURCE m = { 0 };
		HRESULT hr = activeDC->Map(tbExportTexture.texture, exportSubresourceIndex, D3D11_MAP_READ, blockingCall ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT, &m);
		if FAILED(hr)
		{
			Throw(hr, "%s: Mapping from GPU to CPU failed", __FUNCTION__);
		}

		const void* readPtr = m.pData;

		if (readPtr != nullptr)
		{
			memcpy(mipMapLevelDataDestination, readPtr, srcDepth);
		}

		activeDC->Unmap(tbExportTexture.texture, exportSubresourceIndex);
		return true;
	}	


	void WriteSubImage(uint32 index, uint32 mipMapLevel, const RGBAb* pixels, const GuiRect& targetLocation) override
	{
		if (!activeDC)
		{
			Throw(0, "%s: no active DC", __FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			Throw(0, "%s: format is not RGBA but image passed was RGBA", __FUNCTION__);
		}

		UINT mipSlice = numberOfMipLevels - mipMapLevel - 1;

		UINT subresourceIndex = D3D11CalcSubresource(mipSlice, (UINT)index, 1);
		Vec2i targetSpan = Span(targetLocation);
		D3D11_BOX box;
		box.left = targetLocation.left;
		box.right = targetLocation.right;
		box.back = 1;
		box.front = 0;
		box.top = targetLocation.top;
		box.bottom = targetLocation.bottom;

		uint32 levelSpan = 1 << mipMapLevel;

		if (box.left > levelSpan || box.right > levelSpan)
		{
			Throw(0, "%s: bad box [left=%u, right=%u]. Level span: %u", __FUNCTION__, box.left, box.right, levelSpan);
		}

		if (box.bottom > levelSpan || box.top > levelSpan)
		{
			Throw(0, "%s: bad box [bottom=%u, top=%u]. Level span: %u", __FUNCTION__, box.bottom, box.top, levelSpan);
		}

		UINT srcDepth = Sq(levelSpan) * sizeof(RGBAb);
		if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, pixels, levelSpan * sizeof(RGBAb), srcDepth);
	}

	void WriteSubImage(uint32 index, uint32 mipMapLevel, const uint8* alphaTexels, const GuiRect& targetLocation) override
	{
		if (!activeDC)
		{
			Throw(0, "%s: no active DC", __FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __FUNCTION__, index, mipMapLevel);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8_UNORM)
		{
			Throw(0, "%s: format is not an 8-bit UNORM alpha map but image passed was an 8-bit alpha map", __FUNCTION__);
		}

		UINT mipSlice = numberOfMipLevels - mipMapLevel - 1;

		UINT subresourceIndex = D3D11CalcSubresource(mipSlice, (UINT)index, numberOfMipLevels);
		Vec2i targetSpan = Span(targetLocation);
		D3D11_BOX box;
		box.left = targetLocation.left;
		box.right = targetLocation.right;
		box.back = 1;
		box.front = 0;
		box.top = targetLocation.top;
		box.bottom = targetLocation.bottom;

		uint32 levelSpan = 1 << mipMapLevel;

		if (box.left > levelSpan || box.right > levelSpan)
		{
			Throw(0, "%s: bad box [left=%u, right=%u]. Level span: %u", __FUNCTION__, box.left, box.right, levelSpan);
		}

		if (box.bottom > levelSpan || box.top > levelSpan)
		{
			Throw(0, "%s: bad box [bottom=%u, top=%u]. Level span: %u", __FUNCTION__, box.bottom, box.top, levelSpan);
		}

		UINT srcDepth = Sq(levelSpan) * sizeof(RGBAb);
		if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, alphaTexels, levelSpan * sizeof(uint8), srcDepth);
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
	std::unordered_map<ID_TEXTURE, IDX11BitmapArray*, ID_TEXTURE> genericTextureArray;
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

	Textures::IMipMappedTextureArraySupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span, TextureArrayCreationFlags flags)
	{
		AutoFree<MipMappedTextureArray> tx = nullptr;

		try
		{
			tx = new MipMappedTextureArray(device, dc, EComponentType::UNORM_8BITS, 4, span, numberOfElements, flags);
		}
		catch (IException&)
		{
			throw;
		}
		catch (std::exception& stdEx)
		{
			Throw(0, "%s: %s\nError allocating %u elements of span %u x %u", __FUNCTION__, stdEx.what(), numberOfElements, span, span);
		}
		
		return tx.Release();
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

	void CompressJPeg(const RGBAb* data, Vec2i span, cstr filename, int quality) const override
	{
		Imaging::CompressJPeg(data, span, filename, quality);
	}

	void CompressTiff(const RGBAb* data, Vec2i span, cstr filename) const override
	{
		Imaging::CompressTiff(data, span, filename);
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

		IDX11BitmapArray* array = DX11::LoadAlphaBitmapArray(device, span, nElements, enumerator, dc);

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
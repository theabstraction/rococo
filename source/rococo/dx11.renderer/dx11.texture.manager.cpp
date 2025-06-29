#include "dx11.renderer.h"
#include "rococo.io.h"
#include "rococo.renderer.h"
#include "dx11helpers.inl"
#include "dx11buffers.inl"
#include "rococo.hashtable.h"
#include <algorithm>
#include <string>
#include "rococo.imaging.h"

using namespace Rococo::DX11;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

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
			Throw(0, "%s: DirectX11 has a limit of %u elements per array. %u were requested", __ROCOCO_FUNCTION__, D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION, numberOfElements);
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
			Throw(0, "%s: no activeDC", __ROCOCO_FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __ROCOCO_FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __ROCOCO_FUNCTION__, mipMapLevel, numberOfMipLevels);
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
			Throw(hr, "%s failed to create shader view", __ROCOCO_FUNCTION__);
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
			Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __ROCOCO_FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __ROCOCO_FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			Throw(0, "%s: format is not RGBA but image passed was RGBA", __ROCOCO_FUNCTION__);
		}

		uint32 levelSpan = 1 << mipMapLevel;
		UINT lineLength = levelSpan * bytesPerTexel;

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
			Throw(hr, "%s: Mapping from GPU to CPU failed", __ROCOCO_FUNCTION__);
		}

		const uint8* readPtr = (const uint8*) m.pData;
		uint8* writePtr = mipMapLevelDataDestination;

		if (readPtr != nullptr)
		{
			for (uint32 row = 0; row < levelSpan; row++)
			{
				memcpy(writePtr, readPtr, lineLength);
				readPtr += m.RowPitch;
				writePtr += sizeof(RGBAb) * levelSpan;
			}
		}

		activeDC->Unmap(tbExportTexture.texture, exportSubresourceIndex);
		return true;
	}	


	void WriteSubImage(uint32 index, uint32 mipMapLevel, const RGBAb* pixels, const GuiRect& targetLocation) override
	{
		if (!activeDC)
		{
			Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __ROCOCO_FUNCTION__, index, numberOfElements);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __ROCOCO_FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			Throw(0, "%s: format is not RGBA but image passed was RGBA", __ROCOCO_FUNCTION__);
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
			Throw(0, "%s: bad box [left=%u, right=%u]. Level span: %u", __ROCOCO_FUNCTION__, box.left, box.right, levelSpan);
		}

		if (box.bottom > levelSpan || box.top > levelSpan)
		{
			Throw(0, "%s: bad box [bottom=%u, top=%u]. Level span: %u", __ROCOCO_FUNCTION__, box.bottom, box.top, levelSpan);
		}

		UINT srcDepth = Sq(levelSpan) * sizeof(RGBAb);
		if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, pixels, levelSpan * sizeof(RGBAb), srcDepth);
	}

	void WriteSubImage(uint32 index, uint32 mipMapLevel, const uint8* alphaTexels, const GuiRect& targetLocation) override
	{
		if (!activeDC)
		{
			Throw(0, "%s: no active DC", __ROCOCO_FUNCTION__);
		}

		if (index >= numberOfElements)
		{
			Throw(0, "%s: index %u >= numberOfElements %u", __ROCOCO_FUNCTION__, index, mipMapLevel);
		}

		if (mipMapLevel > numberOfMipLevels)
		{
			Throw(0, "%s: mipMapLevel %u >= numberOfMipLevels %u", __ROCOCO_FUNCTION__, mipMapLevel, numberOfMipLevels);
		}

		if (format != DXGI_FORMAT_R8_UNORM)
		{
			Throw(0, "%s: format is not an 8-bit UNORM alpha map but image passed was an 8-bit alpha map", __ROCOCO_FUNCTION__);
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
			Throw(0, "%s: bad box [left=%u, right=%u]. Level span: %u", __ROCOCO_FUNCTION__, box.left, box.right, levelSpan);
		}

		if (box.bottom > levelSpan || box.top > levelSpan)
		{
			Throw(0, "%s: bad box [bottom=%u, top=%u]. Level span: %u", __ROCOCO_FUNCTION__, box.bottom, box.top, levelSpan);
		}

		UINT srcDepth = Sq(levelSpan) * sizeof(RGBAb);
		if (tb.texture) activeDC->UpdateSubresource(tb.texture, subresourceIndex, &box, alphaTexels, levelSpan * sizeof(uint8), srcDepth);
	}

	void Free() override
	{
		delete this;
	}
};

struct VolatileTextureItem
{
	ID3D11Texture2D* tx2D = nullptr;
	HString pingPath;
	int32 lastError = 0;
};

struct DX11RenderTarget : RAL::IRenderTargetSupervisor, RAL::ISysRenderTarget, RAL::ISysShaderView
{
	HString name;
	ID3D11RenderTargetView* view = nullptr;
	ID3D11ShaderResourceView* shaderView = nullptr;
	ID3D11Texture2D* texture2D = nullptr;
	IDX11TextureManager& textures;
	ID3D11Device& device;
	UINT width = 0;
	UINT height = 0;
	DXGI_FORMAT format;
	
	DX11RenderTarget(DXGI_FORMAT _format, cstr _name, IDX11TextureManager& _textures, ID3D11Device& _device): format(_format), name(_name), textures(_textures), device(_device)
	{

	}

	~DX11RenderTarget()
	{
		if (view) view->Release();
		view = nullptr;

		if (texture2D) texture2D->Release();
		texture2D = nullptr;

		if (shaderView) shaderView->Release();
		shaderView = nullptr;
	}

	void Free() override
	{
		delete this;
	}

	void MatchSpan(Vec2i span) override
	{
		if (span.x <= 0 || span.y <= 0)
		{
			return;
		}
			
		if (texture2D != nullptr && (width == (UINT) span.x && height == (UINT) span.y))
		{
			return;
		}

		width = span.x;
		height = span.y;

		if (view) view->Release();
		view = nullptr;

		if (texture2D) texture2D->Release();	
		texture2D = nullptr;

		if (shaderView) shaderView->Release();
		shaderView = nullptr;

		D3D11_TEXTURE2D_DESC desc;
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; 
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		VALIDATEDX11(device.CreateTexture2D(&desc, NULL, &texture2D));

		D3D11_RENDER_TARGET_VIEW_DESC rtdesc;
		rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtdesc.Texture1D.MipSlice = 0;
		rtdesc.Format = format;
		VALIDATEDX11(device.CreateRenderTargetView(texture2D, &rtdesc, &view));

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderDesc;
		shaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderDesc.Texture2D.MipLevels = 1;
		shaderDesc.Texture2D.MostDetailedMip = 0;
		shaderDesc.Format = format;
		VALIDATEDX11(device.CreateShaderResourceView(texture2D, &shaderDesc, &shaderView));
	}

	RAL::ISysRenderTarget& SysRenderTarget() override
	{
		return *this;
	}

	ID3D11RenderTargetView* GetView() override
	{
		return view;
	}

	ID3D11ShaderResourceView* GetShaderView() override
	{
		return shaderView;
	}

	RAL::ISysShaderView& SysShaderView() override
	{
		return *this;
	}
};

RAL::IRenderTargetSupervisor* CreateDynamicRenderTargetObject(cstr name, IDX11TextureManager& textures, ID3D11Device& device)
{
	return new DX11RenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM, name, textures, device);
}

RAL::IRenderTargetSupervisor* CreateDynamicDepthTargetObject(cstr name, IDX11TextureManager& textures, ID3D11Device& device)
{
	return new DX11RenderTarget(DXGI_FORMAT_R32_FLOAT, name, textures, device);
}

RAL::IRenderTargetSupervisor* CreateDynamicNormalTargetObject(cstr name, IDX11TextureManager& textures, ID3D11Device& device)
{
	return new DX11RenderTarget(DXGI_FORMAT_R8G8B8A8_SNORM, name, textures, device);
}

RAL::IRenderTargetSupervisor* CreateDynamicVec4TargetObject(cstr name, IDX11TextureManager& textures, ID3D11Device& device)
{
	return new DX11RenderTarget(DXGI_FORMAT_R32G32B32A32_FLOAT, name, textures, device);
}

struct DX11TextureManager : IDX11TextureManager, ICubeTextures
{
	ID3D11Device& device;
	ID3D11DeviceContext& dc;
	IDX11SpecialResources& specialResources;
	AutoFree<IExpandingBuffer> loadBuffer;
	DX11::TextureLoader textureLoader;
	std::vector<DX11::TextureBind> textures;
	stringmap<ID_TEXTURE> mapNameToTexture;
	stringmap<ID_TEXTURE> nameToGenericTextureId;
	stringmap<ID_VOLATILE_BITMAP> nameToVolatileBitmap;
	std::vector<VolatileTextureItem> idToVolatileBitmapSpec;
	std::vector<TextureItem> orderedTextureList;
	AutoFree<IDX11CubeTextures> cubeTextures;
	std::unordered_map<ID_TEXTURE, IDX11BitmapArray*, ID_TEXTURE> genericTextureArray;
	AutoFree<IDX11Materials> materials;
	TextureBind backBuffer;

	DX11TextureManager(IO::IInstallation& installation, ID3D11Device& _device, ID3D11DeviceContext& _dc, IDX11SpecialResources& _specialResources):
		device(_device),
		dc(_dc),
		specialResources(_specialResources),
		loadBuffer(CreateExpandingBuffer(256_kilobytes)),
		textureLoader(installation, device, dc, *loadBuffer),
		cubeTextures(CreateCubeTextureManager(device, dc)),
		materials(CreateMaterials(installation, device, dc))
	{

	}

	RAL::IRenderTargetSupervisor* CreateDynamicRenderTarget(cstr name) override
	{
		return CreateDynamicRenderTargetObject(name, *this, device);
	}

	RAL::IRenderTargetSupervisor* CreateDynamicDepthTarget(cstr name) override
	{
		return CreateDynamicDepthTargetObject(name, *this, device);
	}

	RAL::IRenderTargetSupervisor* CreateDynamicNormalTarget(cstr name) override
	{
		return CreateDynamicNormalTargetObject(name, *this, device);
	}

	RAL::IRenderTargetSupervisor* CreateDynamicVec4Target(cstr name) override
	{
		return CreateDynamicVec4TargetObject(name, *this, device);
	}

	Textures::IMipMappedTextureArraySupervisor* DefineRGBATextureArray(uint32 numberOfElements, uint32 span, TextureArrayCreationFlags flags)
	{
		try
		{
			return new MipMappedTextureArray(device, dc, EComponentType::UNORM_8BITS, 4, span, numberOfElements, flags);
		}
		catch (std::exception& stdEx)
		{
			Throw(0, "%s: %s\nError allocating %u elements of span %u x %u", __ROCOCO_FUNCTION__, stdEx.what(), numberOfElements, span, span);
		}
	}

	IDX11Materials& Materials() override
	{
		return *materials;
	}

	ICubeTextures& CubeTextures() override
	{
		return *this;
	}

	IDX11CubeTextures& DX11CubeTextures() override
	{
		return *cubeTextures;
	}

	ID_CUBE_TEXTURE CreateCubeTexture(cstr path, cstr extension) override
	{
		return cubeTextures->CreateCubeTexture(textureLoader, path, extension);
	}

	ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id) override
	{
		return cubeTextures->GetShaderView(id);
	}

	ID_CUBE_TEXTURE CreateSkyboxCubeIdFromMaterialArrayIndices(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace) override
	{
		return cubeTextures->CreateCubeTextureFromMaterialArray(XMaxFace, XMinFace, YMaxFace, YMinFace, ZMaxFace, ZMinFace, materials->Textures());
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

		for (auto& t : idToVolatileBitmapSpec)
		{
			if (t.tx2D) t.tx2D->Release();
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

	void AssignToPS(uint32 unitId, ID_TEXTURE texture) override
	{
		auto& tb =  GetTexture(texture);
		if (tb.shaderView)
		{
			dc.PSSetShaderResources(unitId, 1, &tb.shaderView);
		}
	}

	void AssignMaterialsToPS() override
	{
		auto* view = materials->Textures().View();
		dc.PSGetShaderResources(TXUNIT_MATERIALS, 1, &view);
	}

	void SetRenderTarget(ID_TEXTURE depthTarget, ID_TEXTURE renderTarget) override
	{
		auto depth = GetTextureNoThrow(depthTarget);
		auto colour = GetTextureNoThrow(renderTarget);
		if (colour.renderView)
		{
			dc.OMSetRenderTargets(1, &colour.renderView, depth.depthView);
		}
		else
		{
			auto* colourView = specialResources.BackBuffer();
			dc.OMSetRenderTargets(1, &colourView, depth.depthView);
		}
	}

	void SetRenderTarget(RAL::IGBuffers& g, ID_TEXTURE depthTarget) override
	{
		size_t nGBuffers = g.NumberOfTargets();

		ID3D11RenderTargetView** views = (ID3D11RenderTargetView**)alloca(sizeof(ID3D11RenderTargetView*) * nGBuffers);

		for (size_t i = 0; i < nGBuffers; i++)
		{
			views[i] = g.GetTarget(i).SysRenderTarget().GetView();
			if (views[i] == nullptr)
			{
				Throw(0, "no ID3D11RenderTargetView for GBuffer[%d] %s", i);
			}
		}

		auto depth = GetTextureNoThrow(depthTarget);

		dc.OMSetRenderTargets((UINT) nGBuffers, views, depth.depthView);
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

	ID_VOLATILE_BITMAP CreateVolatileBitmap(cstr pingPath) override
	{
		auto i = nameToVolatileBitmap.find(pingPath);
		if (i == nameToVolatileBitmap.end())
		{
			VolatileTextureItem item;
			item.pingPath = pingPath;

			ID_VOLATILE_BITMAP id(idToVolatileBitmapSpec.size());
			idToVolatileBitmapSpec.push_back(item);

			i = nameToVolatileBitmap.insert(pingPath, id).first;
		}

		return i->second;
	}

	ID3D11Texture2D* GetVolatileBitmap(ID_VOLATILE_BITMAP id) override
	{
		if (id.value >= idToVolatileBitmapSpec.size())
		{
			return nullptr;
		}

		auto& item = idToVolatileBitmapSpec[id.value];

		if (item.tx2D)
		{
			return item.tx2D;		
		}

		if (item.lastError)
		{
			return nullptr;
		}

		try
		{
			item.tx2D = textureLoader.LoadColourBitmapDirect(item.pingPath);
			return item.tx2D;
		}
		catch (IException& ex)
		{
			item.lastError = ex.ErrorCode();
			if (item.lastError == 0)
			{
				item.lastError = E_FAIL;
			}

			return nullptr;
		}
	}

	bool DecacheVolatileBitmap(ID_VOLATILE_BITMAP id)
	{
		if (id.value < idToVolatileBitmapSpec.size())
		{
			auto& item = idToVolatileBitmapSpec[id.value];
			if (item.tx2D)
			{
				item.tx2D->Release();
				item.tx2D = nullptr;
				return true;
			}
		}

		return false;
	}

	ID_TEXTURE CreateRenderTarget(cstr renderTargetName, int32 width, int32 height, TextureFormat format) override
	{
		TextureBind tb = DX11::CreateRenderTarget(device, width, height, format);
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

	Vec2i GetTextureSpan(ID_TEXTURE id) const override
	{
		TextureDesc desc;
		if (!TryGetTextureDesc(OUT desc, id))
		{
			return { 0,0 };
		}

		Vec2i span = { (int32)desc.width, (int32)desc.height };
		return span;
	}

	Vec2i GetRenderTargetSpan(ID_TEXTURE id) const override
	{
		TextureDesc desc;
		if (!TryGetTextureDesc(OUT desc, id))
		{
			auto* backBuffer = specialResources.BackBuffer();
			if (backBuffer)
			{
				Vec2i span = { 0,0 };

				ID3D11Resource* bbResource = nullptr;
				backBuffer->GetResource(OUT &bbResource);
				ID3D11Texture2D* tx2D = nullptr;
				if (SUCCEEDED(bbResource->QueryInterface<ID3D11Texture2D>(&tx2D)))
				{
					D3D11_TEXTURE2D_DESC bbRDesc;
					tx2D->GetDesc(OUT & bbRDesc);
					span = { (int32)bbRDesc.Width, (int32)bbRDesc.Height };
				}
				else
				{
					span = { 0,0 };
				}
				tx2D->Release();
				bbResource->Release();
				return span;
			}
			else
			{
				return { 0,0 };
			}
		}

		Vec2i span = { (int32)desc.width, (int32)desc.height };
		return span;
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
			Throw(0, "%s: Bad texture id", __ROCOCO_FUNCTION__);
		}

		auto& t = textures[index];
		return t;
	}

	TextureBind GetTextureNoThrow(ID_TEXTURE id)
	{
		size_t index = id.value - 1;
		if (index >= textures.size())
		{
			return TextureBind();
		}

		auto& t = textures[index];
		return t;
	}
};

namespace Rococo::DX11
{
	IDX11TextureManager* CreateTextureManager(IO::IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& dc, IDX11SpecialResources& specialResources)
	{
		return new DX11TextureManager(installation, device, dc, specialResources);
	}
}
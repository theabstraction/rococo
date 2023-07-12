#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>
#include <rococo.textures.h>

#include <d3d11_4.h>
//#include <dxgi1_6.h>
#include <vector>

#include <rococo.imaging.h>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Textures;

namespace ANON
{
	class UVAtlas;

	struct ElementUpdate
	{
		HRESULT hr;
		cstr message;
		int32 elementIndex;
		ID3D11Texture2D1* tx2D;
	};

	struct ITextureLoader
	{
		virtual ID3D11Texture2D1* LoadTexture2D_Array(cstr resourceName, DXGI_FORMAT format, bool mipMap, Vec2i span, const char* elements[], IEventCallback<ElementUpdate>& onBadElement) = 0;
		virtual ID3D11Texture2D1* LoadTexture2D_UVAtlas(UVAtlas* atlas, cstr resourceName, const char* elements[], IEventCallback<ElementUpdate>& onUpdate) = 0;
		virtual ID3D11Texture2D1* LoadTexture2D(cstr resourceName, DXGI_FORMAT format, bool mipMap) = 0;
		virtual void Free() = 0;
	};

	struct TifAndJPG_Loader : ICompressedResourceLoader
	{
		IInstallation& installation;

		TifAndJPG_Loader(IInstallation& ref_installation) : installation(ref_installation) {}

		void Load(cstr pingPath, IEventCallback<CompressedTextureBuffer>& onLoad) override
		{
			COMPRESSED_TYPE type;
			auto ext = GetFileExtension(pingPath);
			if (EqI(ext, ".tif") || EqI(ext, ".tiff"))
			{
				type = COMPRESSED_TYPE_TIF;
			}
			else if (EqI(ext, ".jpg") || EqI(ext, ".jpeg"))
			{
				type = COMPRESSED_TYPE_JPG;
			}
			else
			{
				Throw(0, "Unknown file type for UV Atlas image: %s", pingPath);
			}

			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(0);
			installation.LoadResource(pingPath, *buffer, 64_megabytes);
			Textures::CompressedTextureBuffer ctb{ buffer->GetData(), buffer->Length(), type };
			onLoad.OnEvent(ctb);
		}
	};

	struct UVAtlasBuilder : ITextureArray
	{
		ID3D11Device5& device;
		IDX11DeviceContext& dc;
		AutoRelease<ID3D11Texture2D1> tx2D;
		uint32 count = 0;
		int32 width = 0;

		UVAtlasBuilder(ID3D11Device5& ref_device, IDX11DeviceContext& ref_dc) : device(ref_device), dc(ref_dc) {}

		void AddTexture() override
		{
			if (tx2D)
			{
				Throw(0, "%s: Cannot add another texture to the texture atlas. The atlas is already fully specified", __FUNCTION__);
			}
			count++;
		}

		void Seal()
		{
			if (tx2D) return;
			if (count == 0) Throw(0, "%s: The texture count was zero", __FUNCTION__);
			if (width == 0) Throw(0, "%s: The texture width was zero", __FUNCTION__);

			D3D11_TEXTURE2D_DESC1 desc;
			desc.ArraySize = count;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Height = width;
			desc.Width = width;
			desc.MipLevels = 1;
			desc.SampleDesc = { 1, 0 };
			desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.MiscFlags = 0;
			VALIDATE_HR(device.CreateTexture2D1(&desc, nullptr, &tx2D));
		}

		void ResetWidth(int32 width) override
		{
			if (width > MaxWidth())
			{
				Throw(0, "%s: Cannot reset width. It is greater than the maximum of %d", __FUNCTION__, MaxWidth());
			}

			this->width = width;
		}

		void ResetWidth(int32 width, int32 height) override
		{
			if (width != height)
			{
				Throw(0, "%s. Width != Height", __FUNCTION__);
			}

			ResetWidth(width);
		}

		void WriteSubImage(size_t index, const RGBAb* pixels, const GuiRect& targetLocation) override
		{
			Seal();

			UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, 1);

			D3D11_BOX box;
			box.left = targetLocation.left;
			box.right = targetLocation.right;
			box.back = 1;
			box.front = 0;
			box.top = targetLocation.top;
			box.bottom = targetLocation.bottom;

			auto lineSpan = (size_t)Width(targetLocation) * sizeof(RGBAb);
			auto srcDepth = lineSpan * (size_t)Height(targetLocation);
			dc.UpdateSubresource(*tx2D, subresourceIndex, box, pixels, (uint32)lineSpan, (uint32)srcDepth);
		}

		void WriteSubImage(size_t index, const uint8* grayScalePixels, Vec2i span) override
		{
			Throw(0, "Texture UV Atlas not supported for grayscale formats");
		}

		int32 MaxWidth() const override
		{
			return 2048;
		}

		size_t TextureCount() const override
		{
			return count;
		}
	};

	void ValidateName(cstr fn, cstr name)
	{
		if (name == nullptr || *name == 0)
		{
			Throw(0, "%s: [name] was blank", fn);
		}
	}

	template<class T> void Release(T* ptr)
	{
		if (ptr) ptr->Release();
	}

	struct TextureElement
	{
		HString resourceName;
		HRESULT hr;
		HString err;
	};

	class UVAtlas : public ITextureArrayBuilderSupervisor
	{
	private:
		TifAndJPG_Loader imageLoader;
		UVAtlasBuilder uvaBuilder;

		AutoFree<ITextureArrayBuilderSupervisor> innerBuilder;
	public:
		UVAtlas(IInstallation& installation, ID3D11Device5& device, IDX11DeviceContext& dc) :
			imageLoader(installation),
			uvaBuilder(device, dc)
		{
			innerBuilder = CreateTextureArrayBuilder(imageLoader, uvaBuilder);
		}

		ID3D11Texture2D1* TakeOwnershipOfTx2D()
		{
			uvaBuilder.tx2D->AddRef();
			return uvaBuilder.tx2D.Detach();
		}

		void AddBitmap(cstr name) override
		{
			return innerBuilder->AddBitmap(name);
		}

		bool TryGetBitmapLocation(cstr name, BitmapLocation& location) override
		{
			return innerBuilder->TryGetBitmapLocation(name, location);
		}

		void BuildTextures(int32 minWidth, IEventCallback<BitmapUpdate>* onUpdate = nullptr) override
		{
			innerBuilder->BuildTextures(minWidth, onUpdate);
		}

		void Clear() override
		{

		}

		void Free() override
		{
			innerBuilder = nullptr;
			delete this;
		}
	};

	// A Gremlin is like a Daemon, only smaller and greener
	struct TextureGremlin
	{
		cstr resourceName = ""; // points to the key in the nameToIds table
		TextureType type = TextureType::None;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		ID3D11ShaderResourceView1* shaderResource = nullptr;
		ID3D11RenderTargetView1* renderTarget = nullptr;
		ID3D11Texture2D1* tx2D = nullptr;
		ID3D11DepthStencilView* depthStencilView = nullptr;
		HString errString;
		HRESULT errNumber = E_PENDING;
		bool isMipMapped = false;
		bool arrayFinalized = false;
		Vec2i span{ 0,0 };
		std::vector<TextureElement> elements;
		UVAtlas* uvAtlas = nullptr;

		// Release cached resourced - N.B it does not erase key metrics, error codes or the element list
		void Release()
		{
			ANON::Release(shaderResource);
			ANON::Release(renderTarget);
			ANON::Release(tx2D);
			ANON::Release(depthStencilView);
			shaderResource = nullptr;
			renderTarget = nullptr;
			tx2D = nullptr;
			depthStencilView = nullptr;
		}
	};

	using namespace Rococo::OS;

	bool LoadIfTiffsOrJpegs(IInstallation& installation, cstr resourceName, Rococo::Imaging::IImageLoadEvents& onLoad)
	{
		auto* ext = GetFileExtension(resourceName);

		if (ext == nullptr)
		{
			return false;
		}
		else if (EqI(ext, ".tiff") || EqI(ext, ".tif"))
		{
			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(0);
			installation.LoadResource(resourceName, *buffer, 64_megabytes);
			Rococo::Imaging::DecompressTiff(onLoad, buffer->GetData(), buffer->Length());
			return true;
		}
		else if (EqI(ext, ".jpg") || EqI(ext, ".jpeg"))
		{
			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(0);
			installation.LoadResource(resourceName, *buffer, 64_megabytes);
			Rococo::Imaging::DecompressJPeg(onLoad, buffer->GetData(), buffer->Length());
			return true;
		}

		return false;
	}

	class ImageFileTextureLoader : public ITextureLoader
	{
		IInstallation& installation;
		ID3D11Device5& device;
		IDX11DeviceContext& dc;
	public:
		ImageFileTextureLoader(IInstallation& ref_installation, ID3D11Device5& ref_device, IDX11DeviceContext& ref_dc):
			installation(ref_installation), device(ref_device), dc(ref_dc)
		{

		}

		void Free() override
		{
			delete this;
		}

		ID3D11Texture2D1* LoadTexture2D(cstr resourceName, DXGI_FORMAT format, bool mipMap) override
		{
			struct CLOSURE : public Imaging::IImageLoadEvents
			{
				DXGI_FORMAT format;
				ID3D11Device5* device = nullptr;
				ID3D11Texture2D1* tx2D = nullptr;
				bool mipMap = false;

				void OnError(const char* message) override
				{
					Throw(E_FAIL, "%s", message);
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* texels) override
				{
					if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
					{
						Throw(E_FAIL, "The target image file was RGB/RGBA.");
					}

					D3D11_TEXTURE2D_DESC1 desc;
					desc.ArraySize = 1;
					desc.Format = format;
					desc.Height = span.x;
					desc.Width = span.y;
					desc.MipLevels = mipMap ? 0 : 1;
					desc.SampleDesc = { 1, 0 };
					desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.CPUAccessFlags = 0;
					desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					if (mipMap) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
					desc.MiscFlags = mipMap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
					D3D11_SUBRESOURCE_DATA data;
					data.pSysMem = texels;
					data.SysMemSlicePitch = 0;
					data.SysMemPitch = sizeof(RGBAb) * desc.Width;
					HRESULT hr = device->CreateTexture2D1(&desc, &data, &tx2D);
					if FAILED(hr)
					{
						Throw(hr, "device->CreateTexture2D1 failed");
					}
				}

				void OnAlphaImage(const Vec2i& span, const GRAYSCALE* texels) override
				{
					if (format != DXGI_FORMAT_R8_UNORM)
					{
						Throw(E_FAIL, "The target image file was greyscale.");
					}

					D3D11_TEXTURE2D_DESC1 desc;
					desc.ArraySize = 1;
					desc.Format = format;
					desc.Height = span.x;
					desc.Width = span.y;
					desc.MipLevels = 1;
					desc.SampleDesc = { 1, 0 };
					desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
					desc.Usage = D3D11_USAGE_DEFAULT;
					desc.CPUAccessFlags = 0;
					desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					desc.MiscFlags = 0;
					D3D11_SUBRESOURCE_DATA data;
					data.pSysMem = texels;
					data.SysMemSlicePitch = 0;
					data.SysMemPitch = sizeof(GRAYSCALE) * desc.Width;
					HRESULT hr = device->CreateTexture2D1(&desc, &data, &tx2D);
					if FAILED(hr)
					{
						Throw(hr, "device->CreateTexture2D1 failed");
					}
				}
			} onLoad;
			onLoad.format = format;
			onLoad.device = &device;
			onLoad.mipMap = mipMap;

			return LoadIfTiffsOrJpegs(installation, resourceName, onLoad) ? onLoad.tx2D : nullptr;
		}

		ID3D11Texture2D1* LoadTexture2D_UVAtlas(UVAtlas* uvAtlas, cstr resourceName , const char* elements[], IEventCallback<ElementUpdate>& onUpdate)
		{
			int32 nElements = 0;
			for (auto* e = elements; *e != nullptr; e++)
			{
				uvAtlas->AddBitmap(*e);
			}

			struct CLOSURE : IEventCallback<BitmapUpdate>
			{
				const char** elements;
				IEventCallback<ElementUpdate>* eu;
				void OnEvent(BitmapUpdate& bu) override
				{
					// Route the update to the correct element in the texture
					int32 index = 0;
					for (auto* e = elements; *e != nullptr; e++)
					{
						if (Eq(bu.name, *e))
						{
							ElementUpdate update{ bu.hr, bu.msg, index, nullptr };
							eu->OnEvent(update);
							break;
						}
						index++;
					}
				}
			} onBitmapUpdate;
			onBitmapUpdate.elements = elements;
			onBitmapUpdate.eu = &onUpdate;

			uvAtlas->BuildTextures(512, &onBitmapUpdate);

			return uvAtlas->TakeOwnershipOfTx2D();
		}

		ID3D11Texture2D1* LoadTexture2D_Array(cstr resourceName, DXGI_FORMAT format, bool mipMap, Vec2i span, const char* elements[], IEventCallback<ElementUpdate>& onUpdate)
		{
			int32 nElements = 0;
			for (auto* e = elements; *e != nullptr; e++)
			{
				nElements++;
			}

			D3D11_TEXTURE2D_DESC1 desc;
			desc.ArraySize = nElements;
			desc.Format = format;
			desc.Height = span.x;
			desc.Width = span.y;
			desc.MipLevels = mipMap ? 0 : 1;
			desc.SampleDesc = { 1, 0 };
			desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			if (mipMap) desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
			desc.MiscFlags = mipMap ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
			
			AutoRelease<ID3D11Texture2D1> tx2D;
			HRESULT hr = device.CreateTexture2D1(&desc, nullptr, &tx2D);
			if FAILED(hr)
			{
				Throw(hr, "%s: device.CreateTexture2D1(&desc, nullptr, &tx2D) failed for %s", __FUNCTION__, resourceName);
			}

			int32 index = 0;
			for(auto* e = elements; *e != nullptr; e++)
			{
				struct CLOSURE : public Imaging::IImageLoadEvents
				{
					DXGI_FORMAT format;
					ID3D11Device5* device = nullptr;
					IDX11DeviceContext* dc = nullptr;
					ID3D11Texture2D1* tx2D = nullptr;
					bool mipMap = false;
					int index = 0;
					Vec2i span{ 0,0 };

					void OnError(const char* message) override
					{
						Throw(E_FAIL, "%s", message);
					}

					void ValidateSpan(Vec2i span)
					{
						if (span != this->span)
						{
							Throw(0, "The source image had dimensions %d x %d. The requirement was %d x %d", span.x, span.y, this->span.x, this->span.y);
						}
					}

					void WriteElement(const void* texels, size_t sizeofTexel)
					{
						D3D11_TEXTURE2D_DESC desc;
						tx2D->GetDesc(&desc);

						UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, desc.MipLevels);

						D3D11_BOX box;
						box.left = 0;
						box.right = span.x;
						box.back = 1;
						box.front = 0;
						box.top = 0;
						box.bottom = span.y;

						auto srcDepth = (size_t) span.x * (size_t) span.y * sizeofTexel;
						dc->UpdateSubresource(*tx2D, subresourceIndex, box, texels, (uint32)((size_t) span.x * sizeofTexel), (uint32) srcDepth);
					}

					void OnRGBAImage(const Vec2i& span, const RGBAb* texels) override
					{
						if (format != DXGI_FORMAT_R8G8B8A8_UNORM)
						{
							Throw(E_FAIL, "The target image file was RGB/RGBA, but the requirment is GREYSCALE.");
						}

						ValidateSpan(span);
						WriteElement(texels, sizeof(RGBAb));
					}

					void OnAlphaImage(const Vec2i& span, const GRAYSCALE* texels) override
					{
						if (format != DXGI_FORMAT_R8_UNORM)
						{
							Throw(E_FAIL, "The target image file was greyscale, but the requirement is RGB/RGBA");
						}

						ValidateSpan(span);
						WriteElement(texels, sizeof(GRAYSCALE));
					}
				} onLoad;
				onLoad.format = format;
				onLoad.device = &device;
				onLoad.dc = &dc;
				onLoad.tx2D = tx2D;
				onLoad.mipMap = mipMap;
				onLoad.index = index;
				onLoad.span = span;

				try
				{
					if (!LoadIfTiffsOrJpegs(installation, *e, onLoad))
					{
						Throw(0, "Resource not recognized as TIFF or JIPEG", e, index);
					}
					else
					{
						ElementUpdate ok{ S_OK, "", index, tx2D };
						onUpdate.OnEvent(ok);
					}
				}
				catch (IException& ex)
				{
					ElementUpdate bad{ ex.ErrorCode(), ex.Message(), index, tx2D };
					onUpdate.OnEvent( bad );
				}

				index++;
			}

			if (mipMap)
			{
				AutoRelease<ID3D11ShaderResourceView> mipsView;
				device.CreateShaderResourceView(tx2D, nullptr, &mipsView);
				dc.GenerateMips(mipsView);
			}

			tx2D->AddRef();
			return tx2D.Detach();
		}
	};

	class TextureCache : public ITextureSupervisor, private OS::IThreadJob
	{
		IInstallation& installation;
		ID3D11Device5& device;
		IDX11DeviceContext& dc;

		std::vector<TextureGremlin> textures;
		stringmap<TextureId> nameToIds;

		std::vector<TextureId> inputQueue;

		AutoFree<IThreadSupervisor> thread;
		AutoFree<ICriticalSection> sync;

		ImageFileTextureLoader iftl;

		std::vector<ITextureLoader*> loaders;

		void SetError_OnThread(TextureId id, IException& ex)
		{
			Lock lock(sync);

			auto& t = textures[id.index - 1];
			t.errNumber = ex.ErrorCode();
			t.errString = ex.Message();
		}

		void Update_OnThread(TextureId id, ID3D11Texture2D1* txId)
		{
			Lock lock(sync);

			auto& t = textures[id.index - 1];
			t.Release();
			t.tx2D = txId;
			t.errNumber = S_OK;
			t.errString = "";

			if (txId)
			{
				D3D11_TEXTURE2D_DESC desc;
				txId->GetDesc(&desc);

				t.span = Vec2i{ (int32) desc.Width, (int32) desc.Height };
			}
		}

		void OnUpdateElement(TextureId id, ElementUpdate& update)
		{
			Lock lock(sync);

			auto& t = textures[id.index - 1];
			t.elements[update.elementIndex].hr = update.hr;
			t.elements[update.elementIndex].err = update.message;
		}

		void Reload_OnThread(UVAtlas* atlas, TextureId id, DXGI_FORMAT format, cstr resourcePath, bool mipMap, Vec2i span, const char* elementNames[])
		{
			try
			{
				for (auto* l : loaders)
				{
					switch (id.type)
					{
						case TextureType::T2D:
						{
							auto tx2D = l->LoadTexture2D(resourcePath, format, mipMap);
							if (tx2D != nullptr)
							{
								Update_OnThread(id, tx2D);
								return;
							}
							break;
						}
						case TextureType::T2D_Array:
						{
							struct CLOSURE : IEventCallback<ElementUpdate>
							{
								TextureCache* This;
								TextureId id;
								void OnEvent(ElementUpdate& bad) override
								{
									This->OnUpdateElement(id, bad);
								}
							} onUpdate;
							onUpdate.This = this;
							onUpdate.id = id;
							auto tx2D = l->LoadTexture2D_Array(resourcePath, format, mipMap, span, elementNames, onUpdate);
							if (tx2D != nullptr)
							{
								Update_OnThread(id, tx2D);
								return;
							}
							break;
						}
						case TextureType::T2D_UVAtlas:
						{
							struct CLOSURE : IEventCallback<ElementUpdate>
							{
								TextureCache* This;
								TextureId id;
								void OnEvent(ElementUpdate& bad) override
								{
									This->OnUpdateElement(id, bad);
								}
							} onUpdate;
							onUpdate.This = this;
							onUpdate.id = id;
							auto tx2D = l->LoadTexture2D_UVAtlas(atlas, resourcePath, elementNames, onUpdate);
							Update_OnThread(id, tx2D);
							return;
							break;
						}
					}
				}

				Throw(0, "No texture loader could generate the texture");
			}
			catch (IException& ex)
			{
				SetError_OnThread(id, ex);
			}
		}

		std::vector<cstr> elementProxy;

		uint32 RunThread(IThreadControl& control) override
		{
			while (control.IsRunning())
			{
				control.SleepUntilAysncEvent(1000);

				while (control.IsRunning())
				{
					sync->Lock();

					if (inputQueue.empty())
					{
						sync->Unlock();
						break;
					}

					auto id = inputQueue.back();
					inputQueue.pop_back();

					auto& t = textures[ToIndex(id, __FUNCTION__)];

					// Copy members on the stack before unlocking

					U8FilePath path;
					DXGI_FORMAT format = t.format;
					Format(path, "%s", t.resourceName);
					bool mipMapped = t.isMipMapped;
					Vec2i span = t.span;

					elementProxy.resize(t.elements.size() + 1);

					for (size_t i = 0; i < t.elements.size(); ++i)
					{
						elementProxy[i] = t.elements[i].resourceName;
					}

					elementProxy[t.elements.size()] = nullptr;

					auto* atlas = t.uvAtlas;

					sync->Unlock();

					Reload_OnThread(atlas, id, format, path, mipMapped, span, elementProxy.data());
				}
			}
			return 0;
		}
	public:
		TextureCache(IInstallation& ref_installation, ID3D11Device5& ref_device, IDX11DeviceContext& ref_dc):
			installation(ref_installation), device(ref_device), dc(ref_dc), iftl(ref_installation, ref_device, ref_dc)
		{
			loaders.push_back(&iftl);
			thread = CreateRococoThread(this, 0);
			sync = thread->CreateCriticalSection();
		}

		void Start()
		{
			// Resume the thread after the constructor has returned so V-Tables are ship shape
			thread->Resume();
		}

		TextureId NextId(TextureType type, uint32 bitPlanes, uint32 bitsPerPlane)
		{
			TextureId id;
			id.index = (uint32) textures.size() + 1;
			id.type = type;
			id.bitPlanes = bitPlanes;
			id.bitsPerPlane = bitsPerPlane;
			return id;
		}

		int32 AddElementToArray(TextureId id, cstr name) override
		{
			ValidateName(__FUNCTION__, name);

			uint64 index = ToIndex(id, __FUNCTION__);

			auto& t = textures[index];
			if (t.type != TextureType::T2D_Array && t.type != TextureType::T2D_UVAtlas)
			{
				Throw(0, "%s(%s): The texture is not an array", __FUNCTION__, name);
			}

			if (t.arrayFinalized)
			{
				Throw(0, "%s(%s): the array was finalized", __FUNCTION__, name);
			}

			Lock lock(sync);

			int32 elementIndex = (int32) t.elements.size();
			t.elements.push_back({ name,E_PENDING,"" });
			return elementIndex;
		}

		TextureId AddGeneric(cstr name, TextureType type, DXGI_FORMAT format, Vec2i span)
		{
			uint32 bitPlanes, bitsPerPlane;

			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				bitPlanes = 4;
				bitsPerPlane = 8;
				break;
			case DXGI_FORMAT_R8_UNORM:
				bitPlanes = 1;
				bitsPerPlane = 8;
				break;
			case DXGI_FORMAT_D32_FLOAT:
				bitPlanes = 1;
				bitsPerPlane = 32;
				break;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				bitPlanes = 0;
				bitsPerPlane = 24;
				break;
			default:
				bitPlanes = 0;
				bitsPerPlane = 0;
			}

			auto i = nameToIds.find(name);
			if (i == nameToIds.end())
			{
				i = nameToIds.insert(name, NextId(type, bitPlanes, bitsPerPlane)).first;
				TextureGremlin gremlin;
				gremlin.type = type;
				gremlin.resourceName = i->first;
				gremlin.format = format;
				gremlin.span = span;
				textures.push_back(gremlin);
			}

			return i->second;
		}

		TextureId AddDepthStencil(cstr name, Vec2i span, uint32 depthBits, uint32 stencilBits) override
		{
			ValidateName(__FUNCTION__, name);

			TextureId id;
			if (depthBits == 24 && stencilBits == 8)
			{
				id = AddGeneric(name, TextureType::T2D, DXGI_FORMAT_D24_UNORM_S8_UINT, span);
			}
			else if (depthBits == 32 && stencilBits == 0)
			{
				id = AddGeneric(name, TextureType::T2D, DXGI_FORMAT_D32_FLOAT, span);
			}
			else
			{
				Throw(0, "Depth+Stencil bit format not recognized. Try 24+8 or 32+0");
			}

			auto& t = textures[id.index - 1];
			t.Release();

			t.span = span;

			D3D11_TEXTURE2D_DESC1 desc;
			desc.ArraySize = 1;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			desc.CPUAccessFlags = 0;
			desc.Format = t.format;
			desc.Height = span.x;
			desc.Width = span.y;
			desc.MipLevels = 1;
			desc.MiscFlags = 0;
			desc.SampleDesc = { 1,0 };
			desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
			desc.Usage = D3D11_USAGE_DEFAULT;
			HRESULT hr = device.CreateTexture2D1(&desc, nullptr, &t.tx2D);
			if FAILED(hr)
			{
				t.errNumber = hr;
				char err[128];
				SafeFormat(err, "device.CreateTexture2D1 failed for Depth-Stencil %u+%u", depthBits, stencilBits);
				t.errString = err;
				return id;
			}

			hr = device.CreateDepthStencilView(t.tx2D, nullptr, &t.depthStencilView);
			if FAILED(hr)
			{
				t.errNumber = hr;
				char err[128];
				SafeFormat(err, "device.CreateDepthStencilView failed for Depth-Stencil %u+%u", depthBits, stencilBits);
				t.errString = err;
				return id;
			}

			return id;
		}

		TextureId AddTx2D_Grey(cstr name) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::T2D, DXGI_FORMAT_R8_UNORM, Vec2i{ 0,0 });
		}

		TextureId AddTx2D_RGBAb(cstr name) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::T2D, DXGI_FORMAT_R8G8B8A8_UNORM, Vec2i{ 0,0 });
		}

		TextureId AddTx2DArray_Grey(cstr name, Vec2i span) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::T2D_Array, DXGI_FORMAT_R8_UNORM, span);
		}

		TextureId AddTx2DArray_RGBAb(cstr name, Vec2i span) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::T2D_Array, DXGI_FORMAT_R8G8B8A8_UNORM, span);
		}

		TextureId AddTx2D_UVAtlas(cstr name) override
		{
			ValidateName(__FUNCTION__, name);
			auto id = AddGeneric(name, TextureType::T2D_UVAtlas, DXGI_FORMAT_R8G8B8A8_UNORM, { 0,0 });
			uint64 index = ToIndex(id, __FUNCTION__);

			auto uvAtlas = new UVAtlas(installation, device, dc);
			textures[index].uvAtlas = uvAtlas;
			return id;
		}

		TextureId AddTx2D_Direct(cstr name, ID3D11Texture2D1* tx2D) override
		{
			ValidateName(__FUNCTION__, name);
			if (tx2D == nullptr) Throw(E_POINTER, "%s(%s,...). The pointer was null", __FUNCTION__, name);

			D3D11_TEXTURE2D_DESC desc;
			tx2D->GetDesc(&desc);

			TextureType type;
			switch (desc.ArraySize)
			{
			case 1:
				type = TextureType::T2D;
				break;
			case 0:
				Throw(0, "%s: unexpected desc.ArraySize = 0", __FUNCTION__);
				break;
			default:
				type = TextureType::T2D_Array;
			}

			Vec2i span{ (int32) desc.Width, (int32) desc.Height };

			TextureId id = AddGeneric(name, type, desc.Format, span);

			auto& t = textures[id.index - 1];
			
			Lock lock(sync);

			t.tx2D = tx2D;
			t.isMipMapped = desc.MipLevels != 1;
			t.format = desc.Format;

			if ((desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) != 0)
			{
				HRESULT hr = device.CreateDepthStencilView(t.tx2D, nullptr, &t.depthStencilView);
				if FAILED(hr)
				{
					t.errNumber = hr;
					char err[128];
					SafeFormat(err, "device.CreateDepthStencilView failed");
					t.errString = err;
				}
			}

			if ((desc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0)
			{
				HRESULT hr = device.CreateRenderTargetView1(t.tx2D, nullptr, &t.renderTarget);
				if FAILED(hr)
				{
					t.errNumber = hr;
					char err[128];
					SafeFormat(err, "device.CreateRenderTargetView1 failed");
					t.errString = err;
				}
			}

			if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0)
			{
				HRESULT hr = device.CreateShaderResourceView1(t.tx2D, nullptr, &t.shaderResource);
				if FAILED(hr)
				{
					t.errNumber = hr;
					char err[128];
					SafeFormat(err, "device.CreateShaderResourceView1 failed");
					t.errString = err;
				}
			}

			return id;
		}

		void ResizeDepthStencil(TextureId id, Vec2i span) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);
			auto& t = textures[index];

			Lock lock(sync);

			t.Release();

			t.span = span;

			D3D11_TEXTURE2D_DESC1 desc;
			desc.ArraySize = 1;
			desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			desc.CPUAccessFlags = 0;
			desc.Format = t.format;
			desc.Height = span.x;
			desc.Width = span.y;
			desc.MipLevels = 1;
			desc.MiscFlags = 0;
			desc.SampleDesc = { 1,0 };
			desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
			desc.Usage = D3D11_USAGE_DEFAULT;
			HRESULT hr = device.CreateTexture2D1(&desc, nullptr, &t.tx2D);
			if FAILED(hr)
			{
				t.errNumber = hr;
				char err[128];
				SafeFormat(err, "%s: device.CreateTexture2D1 failed", __FUNCTION__);
				t.errString = err;
				return;
			}

			hr = device.CreateDepthStencilView(t.tx2D, nullptr, &t.depthStencilView);
			if FAILED(hr)
			{
				t.errNumber = hr;
				char err[128];
				SafeFormat(err, "%s: device.CreateDepthStencilView failed", __FUNCTION__);
				t.errString = err;
			}
		}

		void SetTx2D_Direct(TextureId id, ID3D11Texture2D1* tx2D) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);
			auto& t = textures[index];

			Lock lock(sync);

			t.Release();

			D3D11_TEXTURE2D_DESC desc;
			tx2D->GetDesc(&desc);
			t.span = { (int32) desc.Width, (int32) desc.Height };

			if (t.format != desc.Format)
			{
				Throw(0, "%s: cannot set texture, the argument refers to a texture with a different format", __FUNCTION__);
			}

			t.tx2D = tx2D;

			if ((desc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0)
			{
				HRESULT hr = device.CreateRenderTargetView1(t.tx2D, nullptr, &t.renderTarget);
				if FAILED(hr)
				{
					t.errNumber = hr;
					char err[128];
					SafeFormat(err, "device.CreateRenderTargetView1 failed");
					t.errString = err;
				}
			}

			if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0)
			{
				HRESULT hr = device.CreateShaderResourceView1(t.tx2D, nullptr, &t.shaderResource);
				if FAILED(hr)
				{
					t.errNumber = hr;
					char err[128];
					SafeFormat(err, "device.CreateShaderResourceView1 failed");
					t.errString = err;
				}
			}
		}

		void UpdateArray(TextureId id, uint32 index, const GRAYSCALE* pixels, Vec2i span) override
		{
			auto textureindex = ToIndex(id, __FUNCTION__);

			auto& t = textures[textureindex];

			Lock lock(sync);

			if (!t.tx2D) Throw(t.errNumber, "%s: no texture exists for %s", __FUNCTION__, t.resourceName);

			if (t.format != DXGI_FORMAT_R8_UNORM) Throw(0, "%s: %s not greyscale", __FUNCTION__, t.resourceName);

			D3D11_TEXTURE2D_DESC desc;
			t.tx2D->GetDesc(&desc);

			if (span.x != desc.Width || span.y != desc.Height)
			{
				Throw(0, "%s: %s. Bad span", __FUNCTION__, t.resourceName);
			}

			UINT subresourceIndex = D3D11CalcSubresource(0, index, desc.MipLevels);

			D3D11_BOX box;
			box.left = 0;
			box.right = span.x;
			box.top = 0;
			box.bottom = span.y;
			box.front = 0;
			box.back = 1;

			uint32 lineSpan = span.x * sizeof(GRAYSCALE);
			uint32 srcDepth = span.y * lineSpan;

			dc.UpdateSubresource(*t.tx2D, subresourceIndex, box, pixels, lineSpan, srcDepth);
		}

		Textures::ITextureArrayBuilder& GetSpriteBuilder(TextureId id)
		{
			auto index = ToIndex(id, __FUNCTION__);

			auto& t = textures[index];
			if (!t.uvAtlas)
			{
				Throw(0, "%s: texture %s has no atlas.", __FUNCTION__, t.resourceName);
			}

			return *t.uvAtlas;
		}

		void UpdateSpanFromSystem(TextureId id) override
		{
			auto index = ToIndex(id, __FUNCTION__);

			auto& t = textures[index];

			Lock loc(sync);

			if (t.tx2D == nullptr)
			{
				Throw(0, "%s: the system texture was null", __FUNCTION__);
			}

			D3D11_TEXTURE2D_DESC1 desc;
			t.tx2D->GetDesc1(&desc);
				
			t.span = { (int32)desc.Width, (int32)desc.Height };
		}

		Vec2i GetSpan(TextureId id) const override
		{
			auto index = ToIndex(id, __FUNCTION__);

			auto& t = textures[index];
			return t.span;
		}

		void UseTexturesAsRenderTargets(const RenderTarget* targets, uint32 nTargets, TextureId idDepthStencil)
		{
			auto* pViewArray = (ID3D11RenderTargetView**)_alloca(sizeof(ID3D11RenderTargetView*) * nTargets);

			Lock lock(sync);

			for (uint32 i = 0; i < nTargets; i++)
			{
				auto& target = targets[i];
				auto index = target.id.index - 1;
				if (index >= textures.size()) Throw(0, "%s: Bad id at position %llu", __FUNCTION__, i);


				auto& t = textures[index];

				if (t.tx2D)
				{
					if (t.renderTarget)
					{
						D3D11_RENDER_TARGET_VIEW_DESC1 desc;
						t.renderTarget->GetDesc1(&desc);
						if (desc.Texture2D.MipSlice != target.mipMapIndex)
						{
							t.renderTarget->Release();
							t.renderTarget = nullptr;
						}
					}

					if (t.renderTarget == nullptr)
					{
						D3D11_TEXTURE2D_DESC1 txDesc;
						t.tx2D->GetDesc1(&txDesc);

						D3D11_RENDER_TARGET_VIEW_DESC1 rtDesc;

						switch (t.type)
						{
						case TextureType::T2D:
							rtDesc.Texture2D.MipSlice = target.mipMapIndex;
							rtDesc.Texture2D.PlaneSlice = 0;
							rtDesc.Format = txDesc.Format;
							rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
							break;
						case TextureType::T2D_Array:
						case TextureType::T2D_UVAtlas:
							Throw(0, "Texture %s assigned to render target %llu was an array", t.resourceName, i);
						default:
							Throw(0, "Unknown texture type %d", t.type);
						}

						HRESULT hr = device.CreateRenderTargetView1(t.tx2D, &rtDesc, &t.renderTarget);
						if FAILED(hr)
						{
							Throw(hr, "Failed to use %s as render target", t.resourceName);
						}
					}
					pViewArray[i] = t.renderTarget;
				}
			}

			ID3D11DepthStencilView* depthView = nullptr;

			if (idDepthStencil.index != 0)
			{
				auto dindex = idDepthStencil.index - 1;
				if (dindex >= textures.size()) Throw(0, "%s: Bad depth stencil id", __FUNCTION__);
				auto& d = textures[dindex];
				if (d.tx2D == nullptr)
				{
					Throw(d.errNumber, "%s: the texture was not bound as a depth stencil object");

					if (d.type != TextureType::DepthStencil)
					{
						Throw(d.errNumber, "%s: the texture was not a depth stencil object");
					}

					depthView = d.depthStencilView;
				}
			}

			dc.OMSetRenderTargets(nTargets, nTargets > 0 ? pViewArray : nullptr, depthView);
		}

		bool AssignTextureToShaders(TextureId id, uint32 textureUnit) override
		{
			auto index = ToIndex(id, __FUNCTION__);

			auto& t = textures[index];

			Lock lock(sync);

			if (t.tx2D)
			{
				if (t.shaderResource == nullptr)
				{
					D3D11_TEXTURE2D_DESC1 txDesc;
					t.tx2D->GetDesc1(&txDesc);

					D3D11_SHADER_RESOURCE_VIEW_DESC1 desc;
					desc.Format = t.format;
					switch (t.type)
					{
					case TextureType::T2D:
						desc.Texture2D.MipLevels = txDesc.MipLevels;
						desc.Texture2D.MostDetailedMip = 0;
						desc.Texture2D.PlaneSlice = 0;
						desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
						break;
					case TextureType::T2D_Array:
					case TextureType::T2D_UVAtlas:
						desc.Texture2DArray.MipLevels = txDesc.MipLevels;
						desc.Texture2DArray.ArraySize = txDesc.ArraySize;
						desc.Texture2DArray.FirstArraySlice = 0;
						desc.Texture2DArray.MostDetailedMip = 0;
						desc.Texture2DArray.PlaneSlice = 0;
						desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
						break;
					default:
						Throw(0, "Unknown texture type %d", t.type);
					}

					VALIDATE_HR(device.CreateShaderResourceView1(t.tx2D, &desc, &t.shaderResource));
				}

				ID3D11ShaderResourceView* views[1] = { t.shaderResource };
				dc.PSSetShaderResources(textureUnit, 1, views);
				dc.VSSetShaderResources(textureUnit, 1, views);
				return true;
			}
			else
			{
				return false;
			}
		}

		void EnableMipMapping(TextureId id) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);
			textures[index].isMipMapped = true;
		}

		void Release(TextureId id) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);
			Lock lock(sync);
			auto& t = textures[index];
			t.Release();
		}

		void ClearRenderTarget(TextureId id, const RGBA& clearColour) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);

			Lock lock(sync);
			auto* t = textures[index].renderTarget;
			if (t)
			{
				dc.ClearRenderTargetView(t, clearColour);
			}
			else
			{
				Throw(0, "%s: No RenderTargetView associated with the texture", __FUNCTION__);
			}
		}

		void ClearDepthBuffer(TextureId id, float depth, uint8 stencil) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);

			Lock lock(sync);
			auto* d = textures[index].depthStencilView;
			if (d)
			{
				dc.ClearDepthStencilView(d, D3D11_CLEAR_DEPTH | D3D11_CLEAR_DEPTH, depth, stencil);
			}
			else
			{
				Throw(0, "%s: No DepthStencilView associated with the texture", __FUNCTION__);
			}
		}

		void InitAsBlankArray(TextureId id, uint32 nElements) override
		{
			uint64 index = ToIndex(id, __FUNCTION__);
			auto& t = textures[index];

			if (t.type != TextureType::T2D_Array)
			{
				Throw(0, "%s: texture %s was not an array", __FUNCTION__, t.resourceName);
			}
			
			Lock lock(sync);

			// TODO - put a guard in in the case that ReloadAsset has been called with the same id
			if (t.tx2D == nullptr)
			{
				D3D11_TEXTURE2D_DESC1 desc;
				desc.ArraySize = nElements;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.Format = t.format;
				desc.Height = t.span.x;
				desc.Width = t.span.y;
				desc.MipLevels = t.isMipMapped ? 0 : 1;
				desc.MiscFlags = 0;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.SampleDesc = { 1,0 };
				desc.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
				t.errNumber = device.CreateTexture2D1(&desc, nullptr, &t.tx2D);
				if FAILED(t.errNumber)
				{
					char err[256];
					SafeFormat(err, "device.CreateTexture2D1 failed");
					t.errString = err;
				}
				else
				{
					t.errString = "";
				}
			}
		}

		uint64 ToIndex(TextureId id, cstr src) const
		{
			uint64 index = id.index - 1;
			if (index >= textures.size())
			{
				Throw(0, "%s: Bad Id", src);
			}
			return index;
		}

		void ReloadAsset(TextureId id) override
		{
			uint64 index = ToIndex(id, __FUNCTION__); // This will throw if the id is invalid

			Lock lock(sync);		
			inputQueue.push_back(id);
		}

		virtual ~TextureCache()
		{
			thread = nullptr;

			for (auto& gremlin : textures)
			{
				gremlin.Release();
				Rococo::Free(gremlin.uvAtlas);
			}
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	ITextureSupervisor* CreateTextureCache(IInstallation& installation, ID3D11Device5& device, IDX11DeviceContext& dc)
	{
		auto* t = new ANON::TextureCache(installation, device, dc);
		t->Start();
		return t;
	}
}
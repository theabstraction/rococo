#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <vector>

#include <rococo.imaging.h>

using namespace Rococo;
using namespace Rococo::Graphics;

enum class TextureType: uint32
{
	TextureType_None,
	TextureType_2D,
	TextureType_2D_Array
};

struct TextureId
{
	uint32 index : 24;
	TextureType type : 4;
	uint32 unused : 4;
};

static_assert(sizeof(TextureId) == sizeof(uint32));

ROCOCOAPI ITextureCache
{
	virtual TextureId AddTx2D_Grey(cstr name) = 0;
	virtual TextureId AddTx2DArray_Grey(cstr name, Vec2i span) = 0;
	virtual TextureId AddTx2D_RGBAb(cstr name) = 0;
	virtual TextureId AddTx2DArray_RGBAb(cstr name, Vec2i span) = 0;
	virtual int32 AddElementToArray(cstr name) = 0;
	virtual void EnableMipMapping(TextureId id) = 0;
	virtual void ReloadAsset(TextureId id) = 0;
	virtual void Free() = 0;
};

namespace ANON
{
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

	struct TextureGremlin
	{
		cstr resourceName = ""; // points to the key in the nameToIds table
		TextureType type = TextureType::TextureType_None;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		ID3D11ShaderResourceView1* shaderResource = nullptr;
		ID3D11RenderTargetView1* renderTarget = nullptr;
		ID3D11Texture2D1* tx2D = nullptr;
		HString errString;
		HRESULT errNumber = S_OK;
		bool isMipMapped = false;
		bool arrayFinalized = false;
		Vec2i span{ 0,0 };
		std::vector<HString> elements;

		void Release()
		{
			ANON::Release(shaderResource);
			ANON::Release(renderTarget);
			ANON::Release(tx2D);
			shaderResource = nullptr;
			renderTarget = nullptr;
			tx2D = nullptr;
		}
	};

	using namespace Rococo::OS;

	struct ITextureLoader
	{
		virtual ID3D11Texture2D1* LoadTexture2D_Array(cstr resourceName, DXGI_FORMAT format, bool mipMap, Vec2i span, const char* elements[]) = 0;
		virtual ID3D11Texture2D1* LoadTexture2D(cstr resourceName, DXGI_FORMAT format, bool mipMap) = 0;
		virtual void Free() = 0;
	};

	bool LoadIfTiffsOrJpegs(IInstallation& installation, cstr resourceName, Rococo::Imaging::IImageLoadEvents& onLoad)
	{
		auto* ext = GetFileExtension(resourceName);
		if (EqI(ext, ".tiff") || EqI(ext, ".tif"))
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
		ID3D11DeviceContext4& dc;
	public:
		ImageFileTextureLoader(IInstallation& ref_installation, ID3D11Device5& ref_device, ID3D11DeviceContext4& ref_dc):
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
					desc.Usage = D3D11_USAGE_IMMUTABLE;
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
					desc.Usage = D3D11_USAGE_IMMUTABLE;
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

		ID3D11Texture2D1* LoadTexture2D_Array(cstr resourceName, DXGI_FORMAT format, bool mipMap, Vec2i span, const char* elements[])  override
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
			desc.Usage = D3D11_USAGE_IMMUTABLE;
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
					ID3D11Texture2D1* tx2D = nullptr;
					bool mipMap = false;
					int index = 0;
					Vec2i span{ 0,0 };

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
						desc.Usage = D3D11_USAGE_IMMUTABLE;
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

					void WriteElement(const void* texels)
					{
						UINT subresourceIndex = D3D11CalcSubresource(0, (UINT)index, 1);

						D3D11_BOX box;
						box.left = 0;
						box.right = span.x;
						box.back = 1;
						box.front = 0;
						box.top = 0;
						box.bottom = span.y;

						UINT srcDepth = span.x * span.y * sizeof(RGBAb);
						dc.UpdateSubresource(tb.texture, subresourceIndex, &box, pixels, span.x * sizeof(RGBAb), srcDepth);
					}

					void OnAlphaImage(const Vec2i& span, const GRAYSCALE* texels) override
					{
						if (format != DXGI_FORMAT_R8_UNORM)
						{
							Throw(E_FAIL, "The target image file was greyscale.");
						}
					}
				} onLoad;
				onLoad.format = format;
				onLoad.device = &device;
				onLoad.mipMap = mipMap;
				onLoad.index = index++;
				onLoad.span = span;
				LoadIfTiffsOrJpegs(installation, resourceName, onLoad);
			}
		}
	};

	class TextureCache : public ITextureCache, private OS::IThreadJob
	{
		IInstallation& installation;
		ID3D11Device5& device;

		std::vector<TextureGremlin> textures;
		stringmap<TextureId> nameToIds;

		std::vector<TextureId> inputQueue;

		AutoFree<IThreadSupervisor> thread;
		AutoFree<ICriticalSection> sync;

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
		}

		void Reload_OnThread(TextureId id, DXGI_FORMAT format, cstr resourcePath, bool mipMap, Vec2i span, const char* elementNames[])
		{
			try
			{
				for (auto* l : loaders)
				{
					switch (id.type)
					{
						case TextureType::TextureType_2D:
						{
							auto tx2D = l->LoadTexture2D(resourcePath, format, mipMap);
							if (tx2D != nullptr)
							{
								Update_OnThread(id, tx2D);
								return;
							}
							break;
						}
						case TextureType::TextureType_2D_Array:
						{
							auto tx2D = l->LoadTexture2D_Array(resourcePath, format, mipMap, span, elementNames);
							if (tx2D != nullptr)
							{
								Update_OnThread(id, tx2D);
								return;
							}
							break;
						}
					}
				}
			}
			catch (IException& ex)
			{
				SetError_OnThread(id, ex);
			}
		}

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

					auto& t = textures[id.index - 1];

					// Copy members on the stack before unlocking

					U8FilePath path;
					DXGI_FORMAT format = t.format;
					Format(path, "%s", t.resourceName);
					bool mipMapped = t.isMipMapped;
					Vec2i span = t.span;

					auto* elementNames = (const char**)(_alloca((t.elements.size() + 1) * sizeof(const char*)));
					for (size_t i = 0; i < t.elements.size(); ++i)
					{
						elementNames[i] = t.elements[i];
					}

					elementNames[t.elements.size()] = nullptr;

					sync->Unlock();

					Reload_OnThread(id, format, path, mipMapped, span, elementNames);
				}
			}
		}
	public:
		TextureCache(IInstallation& ref_installation, ID3D11Device5& ref_device):
			installation(ref_installation), device(ref_device)
		{
			thread = CreateRococoThread(this, 0);
			sync = thread->CreateCriticalSection();
		}

		void Start()
		{
			thread->Resume();
		}

		TextureId NextId(TextureType type)
		{
			TextureId id;
			id.index = (uint32) textures.size() + 1;
			id.type = type;
			id.unused = 0;
			return id;
		}

		int32 AddElementToArray(cstr name) override
		{
			if (textures.empty())Throw(0, "%s(%s): the texture cache is empty", __FUNCTION__, name);

			auto& back = textures.back();
			if (back.type != TextureType::TextureType_2D_Array)
			{
				Throw(0, "%s(%s): The current texture in the texture cache is not an array", __FUNCTION__, name);
			}

			if (name == nullptr || name[0] == 0)
			{
				if (back.elements.empty())
				{
					Throw(0, "%s: the element list was empty", __FUNCTION__);
				}

				back.arrayFinalized = true;
				return (int32) back.elements.size();
			}

			if (back.arrayFinalized)
			{
				Throw(0, "%s(%s): the array was finalized", __FUNCTION__, name);
			}

			Lock lock(sync);

			int32 index = (int32) back.elements.size();
			back.elements.push_back(name);
			return index;
		}

		TextureId AddGeneric(cstr name, TextureType type, DXGI_FORMAT format, Vec2i span
		{
			auto i = nameToIds.find(name);
			if (i == nameToIds.end())
			{
				i = nameToIds.insert(name, NextId(type)).first;
				TextureGremlin gremlin;
				gremlin.type = type;
				gremlin.resourceName = i->first;
				gremlin.format = format;
				gremlin.span = span;
				textures.push_back(gremlin);
			}

			return i->second;
		}

		TextureId AddTx2D_Grey(cstr name) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::TextureType_2D, DXGI_FORMAT_R8_UNORM, Vec2i{ 0,0 });
		}

		TextureId AddTx2D_RGBAb(cstr name) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::TextureType_2D, DXGI_FORMAT_R8G8B8A8_UNORM, Vec2i{ 0,0 });
		}

		TextureId AddTx2DArray_Grey(cstr name, Vec2i span) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::TextureType_2D_Array, DXGI_FORMAT_R8_UNORM, span);
		}

		TextureId AddTx2DArray_RGBAb(cstr name, Vec2i span) override
		{
			ValidateName(__FUNCTION__, name);
			return AddGeneric(name, TextureType::TextureType_2D_Array, DXGI_FORMAT_R8G8B8A8_UNORM, span);
		}

		void EnableMipMapping(TextureId id) override
		{
			uint32 index = id.index - 1;
			if (index >= (uint32) textures.size())
			{
				Throw(0, "%s: Bad Id", __FUNCTION__);
			}

			Lock lock(sync);
			textures[index].isMipMapped = true;
		}

		void ReloadAsset(TextureId id) override
		{
			Lock lock(sync);		
			inputQueue.push_back(id);
		}

		virtual ~TextureCache()
		{
			thread = nullptr;

			for (auto& gremlin : textures)
			{
				gremlin.Release();
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
	ITextureCache* CreateTextureCache(IInstallation& installation, ID3D11Device5& device)
	{
		auto* t = new ANON::TextureCache(installation, device);
		t->Start();
		return t;
	}
}
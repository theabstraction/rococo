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

cstr ToString(TextureInternalFormat f)
{
	switch (f)
	{
	case TextureInternalFormat::Greyscale_R8: return "Greyscale_R8";
	case TextureInternalFormat::RGBAb: return "RGBAb";
	default: Throw(0, "Unknown TextureInternalFormat %d", (int)f);
	}
}

struct TextureRecord
{

};

template<class TEXEL> TEXEL* AllocateImage(Vec2i span, const TEXEL* source = nullptr)
{
	uint64 texelCount = ((uint64)span.x) * ((uint64) span.y);
	uint64 byteCount = texelCount * sizeof(TEXEL);
	auto* t = (TEXEL*)_aligned_malloc(byteCount, 16);
	if (t == nullptr)
	{
		Throw(0, "%s: Could not allocate image memory", __FUNCTION__);
	}

	if (source != nullptr)
	{
		memcpy(t, source, byteCount);
	}

	return t;
}

template<class TEXEL> void FreeImage(TEXEL* data)
{
	_aligned_free(data);
}

void ValidateFormat(TextureInternalFormat f, TextureInternalFormat expected)
{
	if (f != expected)
	{
		Throw(0, "Expected internal format %s, but format was %s", ToString(expected), ToString(f));
	}
}

namespace ANON
{
	typedef HandleTable<TextureRecord, 24> TRecords;
	typedef TRecords::Handle TexHandle;

	enum { MAX_TEXTURE_RECORDS = 100'000 };

	class TextureManager: public ITextureManager
	{
		typedef HandleTable<TextureRecord, 24> TRecords;
		TRecords records;

		uint64 nextId = 1;
		stringmap<ID_TEXTURE> mapNameToId;
		std::vector<TextureRecordData> mapIdToData;

		std::vector<ITextureLoader*> loaders;
		AutoFree<ITextureLoader> loader_JPG;
		AutoFree<ITextureLoader> loader_TIF;
	public:
		TextureManager(IInstallation& installation):
			records("TextureRecords", MAX_TEXTURE_RECORDS),
			loader_JPG(CreateJPEGLoader(installation)),
			loader_TIF(CreateTiffLoader(installation))
		{
			mapIdToData.reserve(4096);

			loaders.push_back(loader_TIF);
			loaders.push_back(loader_JPG);
		}

		virtual ~TextureManager()
		{
			for (auto& r : mapIdToData)
			{
				FreeImage(r.alphaPixels);
				FreeImage(r.colourPixels);
			}
		}

		TextureRecordData& GetRecData(ID_TEXTURE id)
		{
			auto index = id.value - 1;

#ifdef _DEBUG
			if (index >= mapIdToData.size()) Throw(0, "%s: bad id [%llu]", __FUNCTION__, id.value);
#endif // DEBUG

			auto& r = mapIdToData[index];
			return r;
		}

		ID_TEXTURE Bind(cstr resourceName, TextureInternalFormat format) override
		{
			if (resourceName == nullptr || *resourceName == 0)
			{
				Throw(0, "%s: blank [resourceName]", __FUNCTION__);
			}

			auto i = mapNameToId.find(resourceName);
			if (i == mapNameToId.end())
			{
				auto index = nextId - 1;
				i = mapNameToId.insert(resourceName, ID_TEXTURE{ nextId++ }).first;
				mapIdToData.push_back(TextureRecordData{ {i->first, {0,0}, format}, nullptr, nullptr});
			}
			else
			{
				auto& r = GetRecData(i->second);
				if (r.meta.format != format)
				{
					Throw(0, "The format %s of the cached texture %s does not match that of the argument %s passed to %s", ToString(r.meta.format), resourceName, ToString(format), __FUNCTION__);
				}
			}

			return i->second;
		}

		void Reload(cstr resourceName) override
		{
			if (resourceName == nullptr || *resourceName == 0)
			{
				Throw(0, "%s: blank [resourceName]", __FUNCTION__);
			}

			auto i = mapNameToId.find(resourceName);
			if (i == mapNameToId.end())
			{
				Throw(0, "%s: No such resource [%s] bound", __FUNCTION__, resourceName);
			}

			struct CLOSURE : ILoadEvent
			{
				cstr resourceName;
				TextureRecordData* rec;

				void OnGreyscale(const GRAYSCALE* alphaPixels, Vec2i span) override
				{
					ValidateFormat(rec->meta.format, TextureInternalFormat::Greyscale_R8);

					if (span != rec->meta.span || rec->alphaPixels == nullptr)
					{
						FreeImage(rec->alphaPixels);
						rec->meta.span = span;
						rec->alphaPixels = AllocateImage<GRAYSCALE>(span, alphaPixels);
					}
				}

				void OnRGBAb(const RGBAb* colourPixels, Vec2i span)override
				{
					ValidateFormat(rec->meta.format, TextureInternalFormat::RGBAb);

					if (span != rec->meta.span || rec->colourPixels == nullptr)
					{
						FreeImage(rec->colourPixels);
						rec->meta.span = span;
						rec->colourPixels = AllocateImage<RGBAb>(span, colourPixels);
					}
				}

			} onLoad;

			onLoad.resourceName = resourceName;
			onLoad.rec = &GetRecData(i->second);

			for (auto* loader: loaders)
			{
				if (loader->TryLoad(resourceName, onLoad))
				{
					return;
				}
			}

			Throw(0, "%s: No loader could resolve [%s]", __FUNCTION__, resourceName);
		}

		TextureMetaData& TryGetById(ID_TEXTURE id) noexcept override
		{
			auto index = id.value - 1;
			if (index >= mapIdToData.size())
			{
				static TextureMetaData null;
				return null;
			}
			else
			{
				return  mapIdToData[index].meta;
			}
		}

		TextureMetaData& GetById(ID_TEXTURE id) override
		{
			auto& m = TryGetById(id);
			if (m.name == nullptr) Throw(0, "%s: no such id", __FUNCTION__);
			return m;
		}

		void Free() override
		{
			delete this;
		}

		void AddTextureLoader(ITextureLoader* loader) override
		{
			loaders.push_back(loader);
		}

		void Commit(ID_TEXTURE id, ITextureMemory& memory) override
		{
			auto& r = GetRecData(id);
			memory.Commit(r);
		}
	};
}

namespace Rococo::Graphics
{
	ITextureManager* CreateTextureManager(IInstallation& installation)
	{
		return new ANON::TextureManager(installation);
	}
}
#include <rococo.types.h>
#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT

#include <assets/assets.texture.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.functional.h>
#include <list>
#include <vector>
#include <rococo.renderer.h>
#include <rococo.imaging.h>
#include <rococo.textures.h>

namespace ANON
{
	enum { INVALID_ARRAY_INDEX = 0xFFFFFFFF };
	using namespace Rococo;
	using namespace Rococo::Assets;
	using namespace Rococo::Graphics;

	cstr emptyString = "";

	class TextureAssetWithLife;

	ROCOCO_INTERFACE ITextureAssetFactoryCEO : ITextureAssetFactorySupervisor
	{
		virtual void MarkForDeath(TextureAssetWithLife* asset) noexcept = 0;
		virtual IFileAssetFactory& FileAssets() = 0;
		virtual void ParseImage(const FileData& data, cstr path, TImageLoadEvent& onParse) = 0;
		virtual ITextureAssetsForEngine& Engine() = 0;
	};

	ROCOCO_INTERFACE ITextureAssetLifeCeo : IAssetLifeSupervisor
	{
	};

	// Glues the texture controller to the texture asset factory.
	// By separating this out from the controller, we relieve the implementor of controller
	// from worrying about the glue
	struct TextureAsset : ITextureAssetSupervisor
	{
		ITextureAssetLifeCeo& life;
		ITextureAssetFactoryCEO& ceo;
		AutoFree<ITextureControllerSupervisor> controller;

		AssetStatus status;
		AssetRef<IFileAsset> fileAssetRef;
		uint32 arrayIndex = INVALID_ARRAY_INDEX;
		
		TextureAsset(cstr _pingPath, ITextureAssetFactoryCEO& _ceo, ITextureAssetLifeCeo& _life): ceo(_ceo), life(_life)
		{
			status.pingPath = to_fstring(_pingPath);
			controller = CreateTextureController(*this);
		}

		ITextureAssetsForEngine& Engine() override
		{
			return ceo.Engine();
		}

		void ParseImage(const FileData& data, cstr path, TImageLoadEvent onParse) override
		{
			ceo.ParseImage(data, path, onParse);
		}

		bool TryQueueLoadImage(cstr path, int mipMapLevel) override
		{
			if (fileAssetRef)
			{
				// An outstanding load is in progress
				return false;
			}
			// While the file is loading we cannot allow the invalidation of [this] pointer so we increase the ref count of the asset life by 1
			life.AddRef();

			auto onLoad = [this, mipMapLevel](IFileAsset& fileAsset)
			{
				AssetAutoRelease hold(life);

				if (fileAsset.IsError())
				{
					char buffer[1024];
					fileAsset.GetErrorAndStatusLength(status.statusCode, buffer, sizeof buffer);
					status.statusText = buffer;
				}

				controller->OnLoadFile(fileAsset, mipMapLevel);
			};

			try
			{
				fileAssetRef = ceo.FileAssets().CreateFileAsset(path, onLoad);
				return true;
			}
			catch (...)
			{
				// If we throw here, then the file system no longer has an outstanding reference to the texture callback, so release the life ref
				life.ReleaseRef();
				throw;
			}
		}

		void SetError(int statusCode, cstr message) override
		{
			status.statusCode = statusCode;
			status.statusText = message;
			status.isError = true;
		}

		ITextureController& Tx() override
		{
			return *controller;
		}

		cstr Path() const override
		{
			return status.pingPath;
		}

		size_t GetErrorAndStatusLength(int& statusCode, char* buffer, size_t nBytesInBuffer) const override
		{
			return status.GetErrorAndStatusLength(statusCode, buffer, nBytesInBuffer);
		}

		void Validate(bool addFilename, bool addFunctionName) override
		{
			status.Validate(addFilename, addFunctionName);
		}
	};

	// Manages the reference counting of the texture asset and wraps the asset
	class TextureAssetWithLife : public ITextureAssetLifeCeo
	{
		long refCount = 0;
		TAsyncOnTextureLoadEvent onLoadEvent;
	public:
		TextureAsset asset;

		TextureAssetWithLife(ITextureAssetFactoryCEO& ceo, cstr pingPath, TAsyncOnTextureLoadEvent& _onLoadEvent) : asset(pingPath, ceo, *this), onLoadEvent(_onLoadEvent)
		{

		}

		uint32 ReferenceCount() const override
		{
			return refCount;
		}

		const fstring Path() const override
		{
			return asset.status.pingPath;
		}

		uint32 AddRef() noexcept override
		{
			return _InterlockedIncrement(&refCount);
		}

		uint32 ReleaseRef() noexcept override
		{
			uint32 currentCount = _InterlockedDecrement(&refCount);
			if (currentCount == 0)
			{
				asset.ceo.MarkForDeath(this);
			}

			return currentCount;
		}
	};

	struct TextureAssetManager : ITextureAssetFactoryCEO, ITextureAssetsForEngine
	{
		IFileAssetFactory& fileManager;
		ITextureManager& engineTextures;
		stringmap<TextureAssetWithLife*> textures;
		AutoFree<OS::ICriticalSection> sync;
		AutoFree<Rococo::Graphics::Textures::IMipMappedTextureArraySupervisor> rgbaArray;
		std::vector<uint32> freeIndices;
		std::vector<TextureAsset*> mapIndexToAsset;

		TextureAssetManager(ITextureManager& _engineTextures, IFileAssetFactory& _fileManager): engineTextures(_engineTextures), fileManager(_fileManager)
		{
			sync = OS::CreateCriticalSection();
		}

		virtual ~TextureAssetManager()
		{
			for (auto i : textures)
			{
				delete i.second;
			}
		}

		ITextureAssetsForEngine& Engine()
		{
			return *this;
		}

		ITextureAssetFactory& Factory()
		{
			return *this;
		}

		AssetRef<ITextureAsset> CreateTextureAsset(const char* pingPath, TAsyncOnTextureLoadEvent onLoad = NoTextureCallback) override
		{
			if (pingPath == nullptr || *pingPath == 0) Throw(0, "%s: Blank ping path", __FUNCTION__);

			OS::Lock lockedSection(sync);

			auto insertRef = textures.insert(pingPath, nullptr);
			auto mapIterator = insertRef.first;
			bool wasInserted = insertRef.second;

			TextureAssetWithLife* assetWrapper;

			if (wasInserted)
			{
				try
				{
					assetWrapper = new TextureAssetWithLife(*this, mapIterator->first, onLoad);
					mapIterator->second = assetWrapper;
				}
				catch (...)
				{
					textures.erase(mapIterator);
					throw;
				}
			}
			else
			{
				assetWrapper = mapIterator->second;
			}

			return AssetRef<ITextureAsset>(&assetWrapper->asset, static_cast<IAssetLifeSupervisor*>(assetWrapper));
		}

		void MarkForDeath(TextureAssetWithLife* wrapper) noexcept override
		{
			auto i = textures.find(wrapper->Path());
			if (i == textures.end())
			{
				// Memory leak
				OS::TripDebugger();
				return;
			}

			textures.erase(i);

			if (wrapper->asset.arrayIndex < mapIndexToAsset.size())
			{
				auto* mappedAsset = mapIndexToAsset[wrapper->asset.arrayIndex];
				if (mappedAsset == &wrapper->asset)
				{
					mapIndexToAsset[wrapper->asset.arrayIndex] = nullptr;
					freeIndices.push_back(wrapper->asset.arrayIndex);
				}
				else
				{
					// Bad, means we lost sync between array indices and assets
					OS::TripDebugger();
				}
			}
			delete wrapper;
		}

		void ParseImage(const FileData& data, cstr path, TImageLoadEvent& onParse) override
		{
			struct ANON: Imaging::IImageLoadEvents
			{
				TImageLoadEvent& onParse;

				ANON(TImageLoadEvent& _onParse) : onParse(_onParse)
				{

				}

				void OnError(const char* message) override
				{
					onParse.Invoke(TexelSpec{ 0,0 }, { 0,0 }, (const uint8*) message);
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
				{
					TexelSpec rgbaSpec;
					rgbaSpec.bitPlaneCount = 3;
					rgbaSpec.bitsPerBitPlane = 8;
					onParse.Invoke(rgbaSpec, span, (const uint8*) data);
				}

				void OnAlphaImage(const Vec2i& span, const uint8* data) override
				{
					TexelSpec alphaSpec;
					alphaSpec.bitPlaneCount = 1;
					alphaSpec.bitsPerBitPlane = 8;
					onParse.Invoke(alphaSpec, span, data);
				}
			} routeImaging(onParse);

			cstr extension = Strings::GetFileExtension(path);
			if (extension)
			{
				if (Strings::EqI(extension, ".jpg") || Strings::EqI(extension, ".jpeg"))
				{
					engineTextures.DecompressJPeg(routeImaging, data.data, data.nBytes);
					return;
				}
				else if (Strings::EqI(extension, ".tif") || Strings::EqI(extension, ".tiff"))
				{
					engineTextures.DecompressTiff(routeImaging, data.data, data.nBytes);
					return;
				}
			}
			
			onParse.Invoke(TexelSpec{ 0,0 }, { 0,0 }, (const uint8*) "expecting .jpg or .tif");
		}

		int32 engineSpanQuality = 1024;

		bool AttachToGPU(ITextureAsset& asset) override
		{
			auto& ourAsset = static_cast<TextureAsset&>(asset);
			if (ourAsset.arrayIndex == INVALID_ARRAY_INDEX)
			{
				if (freeIndices.empty())
				{
					return false;
				}

				ourAsset.arrayIndex = freeIndices.back();
				freeIndices.pop_back();
				mapIndexToAsset[ourAsset.arrayIndex] = &ourAsset;
			}

			return true;
		}

		void ReleaseFromGPU(ITextureAsset& asset) override
		{
			auto& ourAsset = static_cast<TextureAsset&>(asset);
			uint32 i = ourAsset.arrayIndex;
			if (i != INVALID_ARRAY_INDEX)
			{
				if (i < mapIndexToAsset.size())
				{
					if (mapIndexToAsset[i] == &ourAsset)
					{
						mapIndexToAsset[i] = nullptr;
						freeIndices.push_back(i);
					}
					else
					{
						Throw(0, "%s: Algorithimic error.\n i = %u. ourAsset = %s. Array[i] asset = %s", __FUNCTION__, i, ourAsset.Path(), mapIndexToAsset[i] ? mapIndexToAsset[i]->Path() : "<nullptr>");
					}
				}
				else
				{
					Throw(0, "%s: Algorithimic error.\n i = %u. ourAsset = %s. i > map size of %llu", __FUNCTION__, i, ourAsset.Path(), mapIndexToAsset.size());
				}
				ourAsset.arrayIndex = INVALID_ARRAY_INDEX;
			}
		}

		bool DownsampleToEngineQuality(TexelSpec imageSpec, Vec2i span, const uint8* texels, Rococo::Function<void(Vec2i downSampledSpan, const uint8* downSampledTexels)> onDownsample) const override
		{
			UNUSED(imageSpec);
			UNUSED(span);
			UNUSED(texels);
			UNUSED(onDownsample);
			return false;
		}

		void SetEngineTextureArray(uint32 spanInPixels, int32 numberOfElementsInArray) override
		{
			if (spanInPixels == 0) spanInPixels = 1024; // default to 1024
			uint32 maxLevel = 0;
			uint32 q = spanInPixels;
			while (q > 1)
			{
				q = q >> 1;
				maxLevel++;
			}

			uint32 maxLevelSpan = 1 << maxLevel;

			if (maxLevelSpan != spanInPixels)
			{
				Throw(0, "%s: The span must be a power of 2", __FUNCTION__);
			}

			if (maxLevel > ITextureController::MAX_LEVEL_INDEX)
			{
				Throw(0, "%s: The maximum span is %llu", __FUNCTION__, ITextureController::MAX_LEVEL_INDEX);
			}

			engineSpanQuality = spanInPixels;

			freeIndices.clear();
			mapIndexToAsset.clear();
			rgbaArray = nullptr;

			for (auto i : textures)
			{
				i.second->asset.arrayIndex = (uint32)-1;
			}

			rgbaArray = engineTextures.DefineRGBATextureArray(numberOfElementsInArray, spanInPixels);

			freeIndices.reserve(numberOfElementsInArray);

			for (int i = numberOfElementsInArray - 1; i >= 0; i--)
			{
				freeIndices.push_back(i);
			}

			mapIndexToAsset.resize(numberOfElementsInArray);
			std::fill(mapIndexToAsset.begin(), mapIndexToAsset.end(), nullptr);
		}

		int GetEngineTextureQualitySpan() const override
		{
			return engineSpanQuality;
		}

		IFileAssetFactory& FileAssets()
		{
			return fileManager;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Assets
{
	ROCOCO_ASSETS_API ITextureAssetFactorySupervisor* CreateTextureAssetFactory(Graphics::ITextureManager& engineTextures, IFileAssetFactory& fileManager)
	{
		return new ANON::TextureAssetManager(engineTextures, fileManager);
	}
}
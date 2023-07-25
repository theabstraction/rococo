#include <rococo.types.h>
#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT

#include <assets/assets.texture.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.functional.h>
#include <list>
#include <rococo.renderer.h>
#include <rococo.imaging.h>

namespace ANON
{
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
	};

	ROCOCO_INTERFACE ITextureAssetLifeCeo : IAssetLifeSupervisor
	{
	};

	struct TextureAsset : ITextureAssetSupervisor
	{
		ITextureAssetLifeCeo& life;
		ITextureAssetFactoryCEO& ceo;
		AutoFree<ITextureControllerSupervisor> controller;

		AssetStatus status;
		AssetRef<IFileAsset> fileAssetRef;
		
		TextureAsset(cstr _pingPath, ITextureAssetFactoryCEO& _ceo, ITextureAssetLifeCeo& _life): ceo(_ceo), life(_life)
		{
			status.pingPath = to_fstring(_pingPath);
			controller = CreateTextureController(*this);
		}

		void ParseImage(const FileData& data, cstr path, TImageLoadEvent onParse) override
		{
			ceo.ParseImage(data, path, onParse);
		}

		void QueueLoadImage(cstr path, int mipMapLevel) override
		{
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

	struct TextureAssetManager : ITextureAssetFactoryCEO
	{
		IFileAssetFactory& fileManager;
		ITextureManager& engineTextures;
		stringmap<TextureAssetWithLife*> textures;
		AutoFree<OS::ICriticalSection> sync;

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

		void SetEngineTextureArray(int32 spanInPixels, int32 numberOfElementsInArray) override
		{
			UNUSED(spanInPixels);
			UNUSED(numberOfElementsInArray);
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
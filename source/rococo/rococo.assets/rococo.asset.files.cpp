#include <rococo.types.h>

#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT

#include <assets/assets.files.h>
#include <rococo.strings.h>
#include <rococo.hashtable.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.functional.h>
#include <list>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Assets;
	using namespace Rococo::Strings;

	const char* const emptyString = "";

	struct FileAsset: IFileAsset
	{
		fstring pingPath; // This points to the fileAssets map key below, which is persistent for the lifetime of the FileAsset
		AutoFree<IExpandingBuffer> fileMap;
		HString status;
		int statusCode = 0;
		volatile bool isReady = false; // The last thing to be set to true when the FileAssetFactory has completed loading the asset, or completed poplating the status codes
		volatile bool isLoaded = false;
		volatile bool isError = false;

		FileAsset(cstr _pingPath): pingPath(to_fstring(_pingPath))
		{

		}

		bool IsLoaded() const override
		{
			return isReady && isLoaded;
		}

		bool IsError() const override
		{
			return isReady && isError;
		}

		cstr Path() const override
		{
			return pingPath;
		}

		FileData RawData() const override
		{
			return (isReady && isLoaded) ? FileData { fileMap->GetData(), fileMap->Length() } : FileData{ 0, 0 };
		}

		fstring ToRawString() const override
		{
			if (!IsLoaded()) return fstring{ emptyString, 0 };

			if (fileMap->Length() >= (uint64) Limits::FSTRING_LENGTH_LIMIT)
			{
				Throw(0, "%s - %s:\n [asset length %llu] > %llu, which is the limit of fstrings\n", __FUNCTION__, (cstr)pingPath, fileMap->Length(), (uint64)Limits::FSTRING_LENGTH_LIMIT);
			}

			return fstring{ (cstr)fileMap->GetData(), (int32) fileMap->Length() };
		}

		fstring ToString() const override
		{
			if (!IsLoaded()) return fstring{ emptyString, 0 };
			return to_fstring((cstr)fileMap->GetData());
		}

		size_t GetErrorAndStatusLength(int& statusCode, char* buffer, size_t nBytesInBuffer) const override
		{
#ifdef _WIN32
			enum { NOT_READY = /* WAIT_IO_COMPLETION */ 0x000000C0L };
#else
			static_assert(false, "Not implemented");
#endif
			if (!isReady)
			{
				auto message = "File is not ready yet"_fstring;
				if (buffer && nBytesInBuffer)
				{
					SafeFormat(buffer, nBytesInBuffer, message.buffer);
				}

				statusCode = NOT_READY;
				return message.length + 1;
			}

			statusCode = this->statusCode;
			if (buffer && nBytesInBuffer)
			{
				SafeFormat(buffer, nBytesInBuffer, "%s", status.length() == 0 ? emptyString : status.c_str());
			}

			return status.length() + 1;
		}

		void Validate(bool addFilename, bool addFunctioname) override
		{
			if (!isReady || !isError) return;
			if (addFilename)
			{
				Throw(statusCode, "%s%s%s:\n %s", addFunctioname ? __FUNCTION__ : "", addFunctioname ? " - " : "", pingPath, status.length() > 0 ? status.c_str() : "<unknown error>");
			}
			else
			{
				Throw(statusCode, "%s%s%s", addFunctioname ? __FUNCTION__ : "", addFunctioname ? ": " : "", status.length() > 0 ? status.c_str() : "<unknown error>");
			}
		}
	};

	ROCOCO_INTERFACE IFileAssetFactoryCEO : IFileAssetFactorySupervisor
	{
		virtual IO::IInstallation& Installation() = 0;
	};

	class FileAssetWithLife: public IAssetLifeSupervisor
	{
		IFileAssetFactoryCEO& ceo;
		long refCount = 0;
		
		TAsyncOnLoadEvent onLoadEvent;
	public:
		FileAssetWithLife(IFileAssetFactoryCEO& _ceo, cstr pingPath, TAsyncOnLoadEvent& _onLoadEvent): ceo(_ceo), asset(pingPath), onLoadEvent(_onLoadEvent)
		{

		}

		void DeliverChristmasPresents()
		{
			onLoadEvent(asset);
		}

		void LoadFromThread(OS::IThreadControl& /* thread */) noexcept
		{
			try
			{
				asset.fileMap = CreateExpandingBuffer(0);
			}
			catch (...)
			{
				// If we ran out of memory here we are probably in the pooh anyway, but try to report the error anyway
				asset.statusCode = 0;
				asset.status = "Insufficient memory";
			}

			try
			{
				ceo.Installation().LoadResource(asset.pingPath, *asset.fileMap, 1_gigabytes);
				asset.isLoaded = true;
			}
			catch (IException& ex)
			{
				asset.statusCode = ex.ErrorCode();
				asset.status = ex.Message();
			}

			asset.isReady = true;
		}

		FileAsset asset;
		
		uint32 ReferenceCount() const override
		{
			return refCount;
		}

		const fstring Path() const override
		{
			return asset.pingPath;
		}

		uint32 AddRef() override
		{
			return _InterlockedIncrement(&refCount);
		}

		uint32 ReleaseRef() override
		{
			refCount--;
			return _InterlockedDecrement(&refCount);
		}
	};

	struct FileAssetFactory : IFileAssetFactoryCEO, OS::IThreadJob
	{
		IAssetManager& manager;
		IO::IInstallation& installation;
		stringmap<FileAssetWithLife*> fileAssets;
		AutoFree<OS::IThreadSupervisor> loaderThread;
		AutoFree<OS::ICriticalSection> sync;
		std::list<FastStringKey> unloadedItems;
		std::list<FileAssetWithLife*> newlyLoadedItems;

		FileAssetFactory(IAssetManager& _manager, IO::IInstallation& _installation):
			manager(_manager), installation(_installation)
		{
			// Potentially we could add more threads here, and use the same sync, so that they share the same unloadedItems list. Due to the sync, only one thread can modify the list at a time
			loaderThread = OS::CreateRococoThread(this, 0);
			sync = loaderThread->CreateCriticalSection();
			loaderThread->Resume();
		}

		virtual ~FileAssetFactory()
		{
			for (auto& i : fileAssets)
			{
				delete i.second;
			}
		}

		IO::IInstallation& Installation()
		{
			return installation;
		}

		AssetRef<IFileAsset> CreateFileAsset(const char* pingPath, TAsyncOnLoadEvent onLoad) override
		{
			if (pingPath == nullptr || *pingPath == 0) Throw(0, "Blank ping path");

			OS::Lock lockedSection(sync);

			auto insertRef = fileAssets.insert(pingPath, nullptr);
			auto mapIterator = insertRef.first;
			bool wasInserted = insertRef.second;

			if (wasInserted)
			{
				try
				{
					auto* wrapper = new FileAssetWithLife(*this, mapIterator->first, onLoad);
					mapIterator->second = wrapper;
				}
				catch (...)
				{
					fileAssets.erase(mapIterator);
					throw;
				}
			}
			
			FileAssetWithLife* assetWrapper = mapIterator->second;
			assetWrapper->AddRef(); // refcount is now 1

			unloadedItems.push_back(mapIterator->first);

			OS::QueueAPC(loaderThread, FileAssetFactory::WakeUp, this);

			return AssetRef<IFileAsset>(&assetWrapper->asset, static_cast<IAssetLifeSupervisor*>(assetWrapper));
		}

		static void WakeUp(FileAssetFactory* /* This */)
		{
			// wake up!
		}

		void Free() override
		{
			delete this;
		}

		void LoadTargetSyncInLoaderThread(OS::IThreadControl& control, FileAssetWithLife& wrapper)
		{
			wrapper.LoadFromThread(control);

			// All of the items here have an outstanding reference from the caller

			OS::Lock lock(sync);
			newlyLoadedItems.push_back(&wrapper);
		}

		void InvokeAsyncHandlers()
		{
			while (newlyLoadedItems.empty())
			{
				FileAssetWithLife* santa = nullptr;
				{
					OS::Lock lock(sync);
					if (!newlyLoadedItems.empty())
					{
						santa = newlyLoadedItems.front();
						newlyLoadedItems.pop_front();
					}
				}

				if (santa)
				{
					try
					{
						santa->DeliverChristmasPresents();
						santa->ReleaseRef();
					}
					catch (...)
					{
						santa->ReleaseRef();
						throw;
					}
				}
			}
		}

		uint32 RunThread(OS::IThreadControl& control) override
		{
			while (control.IsRunning())
			{
				while (!unloadedItems.empty())
				{
					FileAssetWithLife* target = nullptr;

					{
						OS::Lock lock(sync);

						if (unloadedItems.empty())
						{
							cstr key = unloadedItems.front();
							auto i = fileAssets.find(key);
							if (i != fileAssets.end())
							{
								target = i->second;
								target->AddRef();
							}
							unloadedItems.pop_front();
						}
					}

					// The lock will be released, so other threads can now unload items or append to the unload list
					if (target)
					{
						LoadTargetSyncInLoaderThread(control, *target);
					}
				}

				control.SleepUntilAysncEvent(1000);
			}

			return 0;
		}
	};
}

namespace Rococo::Assets
{
	ROCOCO_ASSETS_API void NoFileCallback(IFileAsset& asset)
	{
		UNUSED(asset);
	}

	ROCOCO_ASSETS_API IFileAssetFactorySupervisor* CreateFileAssetFactory(IAssetManager& manager, IO::IInstallation& installation)
	{
		return new ANON::FileAssetFactory(manager, installation);
	}
}

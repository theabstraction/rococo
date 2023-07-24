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

	struct FileAsset : IFileAsset
	{
		AutoFree<IExpandingBuffer> fileMap;
		AssetStatus status;

		volatile bool isLoaded = false;

		FileAsset(cstr _pingPath)
		{
			status.pingPath = to_fstring(_pingPath);
		}

		bool IsLoaded() const override
		{
			return status.isReady && isLoaded;
		}

		bool IsError() const override
		{
			return status.isReady && status.isError;
		}

		cstr Path() const override
		{
			return status.pingPath;
		}

		FileData RawData() const override
		{
			return (status.isReady && isLoaded) ? FileData { fileMap->GetData(), fileMap->Length() } : FileData{ 0, 0 };
		}

		fstring ToRawString() const override
		{
			if (!IsLoaded()) return fstring{ emptyString, 0 };

			if (fileMap->Length() >= (uint64)Limits::FSTRING_LENGTH_LIMIT)
			{
				Throw(0, "%s - %s:\n [asset length %llu] > %llu, which is the limit of fstrings\n", __FUNCTION__, (cstr)status.pingPath, fileMap->Length(), (uint64)Limits::FSTRING_LENGTH_LIMIT);
			}

			return fstring{ (cstr)fileMap->GetData(), (int32)fileMap->Length() };
		}

		fstring ToString() const override
		{
			if (!IsLoaded()) return fstring{ emptyString, 0 };
			return to_fstring((cstr)fileMap->GetData());
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

	class FileAssetWithLife;

	ROCOCO_INTERFACE IFileAssetFactoryCEO : IFileAssetFactorySupervisor
	{
		virtual IO::IInstallation & Installation() = 0;

		// Tell the CEO to suicide the offender from the system, as nobody is interested
		virtual void MarkForDeath(FileAssetWithLife* epstein) noexcept = 0;
	};

	class FileAssetWithLife : public IAssetLifeSupervisor
	{
		IFileAssetFactoryCEO& ceo;
		long refCount = 0;

		TAsyncOnLoadEvent onLoadEvent;
	public:
		FileAssetWithLife(IFileAssetFactoryCEO& _ceo, cstr pingPath, TAsyncOnLoadEvent& _onLoadEvent) : ceo(_ceo), asset(pingPath), onLoadEvent(_onLoadEvent)
		{

		}

		void DeliverChristmasPresents() noexcept
		{
			try
			{
				onLoadEvent(asset);
			}
			catch (IException& ex)
			{
				char msg[1024];
				SafeFormat(msg, "onLoadEvent threw an exception: %s", ex.Message());
				asset.status.statusText = msg;
				asset.status.statusCode = ex.ErrorCode();
				asset.status.isError = true;
			}
			catch (std::exception& stdEx)
			{
				char msg[1024];
				SafeFormat(msg, "onLoadEvent threw a std::exception: %s", stdEx.what());
				asset.status.statusText = msg;
				asset.status.statusCode = 0;
				asset.status.isError = true;
			}
			catch (...)
			{
				char msg[1024];
				SafeFormat(msg, "onLoadEvent threw an unspecified exception");
				asset.status.statusText = msg;
				asset.status.statusCode = 0;
				asset.status.isError = true;
			}
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
				asset.status.statusCode = 0;
				asset.status.statusText = "Insufficient memory";
			}

			try
			{
				ceo.Installation().LoadResource(asset.status.pingPath, *asset.fileMap, 1_gigabytes);
				asset.isLoaded = true;
			}
			catch (IException& ex)
			{
				asset.status.statusCode = ex.ErrorCode();
				asset.status.statusText = ex.Message();
			}

			asset.status.isReady = true;
		}

		FileAsset asset;

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
				ceo.MarkForDeath(this);
			}

			return currentCount;
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

		FileAssetFactory(IAssetManager& _manager, IO::IInstallation& _installation) :
			manager(_manager), installation(_installation)
		{
			// Potentially we could add more threads here, and use the same sync, so that they share the same unloadedItems list. Due to the sync, only one thread can modify the list at a time
			loaderThread = OS::CreateRococoThread(this, 0);
			sync = OS::CreateCriticalSection();
			loaderThread->Resume();
		}

		virtual ~FileAssetFactory()
		{
			loaderThread = nullptr;

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

			FileAssetWithLife* assetWrapper;

			if (wasInserted)
			{
				try
				{
					assetWrapper = new FileAssetWithLife(*this, mapIterator->first, onLoad);
					mapIterator->second = assetWrapper;
					unloadedItems.push_back(mapIterator->first);
					OS::QueueAPC(loaderThread, FileAssetFactory::WakeUp, this);
				}
				catch (...)
				{
					fileAssets.erase(mapIterator);
					throw;
				}
			}
			else
			{
				assetWrapper = mapIterator->second;
			}

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

		void MarkForDeath(FileAssetWithLife* epstein) noexcept
		{
			OS::Lock lock(sync);
			auto i = fileAssets.find(epstein->Path());
			if (i != fileAssets.end())
			{
				fileAssets.erase(i);
				delete epstein;
			}
		}

		void DeliverToThisThreadThisTick()
		{
			while (!newlyLoadedItems.empty())
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
					AssetAutoRelease hold(*santa);
					santa->DeliverChristmasPresents();
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

						if (!unloadedItems.empty())
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
	size_t AssetStatus::GetErrorAndStatusLength(int& statusCode, char* buffer, size_t nBytesInBuffer) const
	{
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
			SafeFormat(buffer, nBytesInBuffer, "%s", statusText.length() == 0 ? "" : statusText.c_str());
		}

		return statusText.length() + 1;
	}

	void AssetStatus::Validate(bool addFilename, bool addFunctioname)
	{
		if (!isReady || !isError) return;

		if (addFilename)
		{
			Throw(statusCode, "%s%s%s:\n %s", addFunctioname ? __FUNCTION__ : "", addFunctioname ? " - " : "", pingPath, statusText.length() > 0 ? statusText.c_str() : "<unknown error>");
		}
		else
		{
			Throw(statusCode, "%s%s%s", addFunctioname ? __FUNCTION__ : "", addFunctioname ? ": " : "", statusText.length() > 0 ? statusText.c_str() : "<unknown error>");
		}
	}

	ROCOCO_ASSETS_API void NoFileCallback(IFileAsset& asset)
	{
		UNUSED(asset);
	}

	ROCOCO_ASSETS_API IFileAssetFactorySupervisor* CreateFileAssetFactory(IAssetManager& manager, IO::IInstallation& installation)
	{
		return new ANON::FileAssetFactory(manager, installation);
	}
}

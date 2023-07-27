#pragma once
#include <rococo.assets.h>
#include <rococo.functional.h>
#include <rococo.strings.h>

namespace Rococo::IO
{
	struct IInstallation;
}

namespace Rococo::Assets
{
	using namespace Rococo::Strings;

	struct FileData
	{
		const uint8* data;
		size_t nBytes;
	};

	ROCOCO_INTERFACE IFileAsset : IAsset
	{
		// The API consumer can inform the asset system that the load event bound to the key is no longer valid or required. This should only be called in the main thread.
		virtual void CancelAssociatedCallback(IAsset* key) = 0;

		// If the file is ready to be viewed then IsLoaded returns true
		virtual bool IsLoaded() const = 0;

		// If an error has occured loading the file then the method returns true
		// If the method returns false the file may not be ready, check IsLoaded first
		virtual bool IsError() const = 0;

		// Returns the file data, which yields valid pointers until the asset reference count is zero, and then is undefined.
		virtual FileData RawData() const = 0;

		// Returns the file data as an fstring, which yield valid pointers until the asset reference count is zero, and then is undefined.
		// Note that the string may also contains nuls at other positions before the end.
		virtual fstring ToRawString() const = 0;

		// Iterates through the file data for the first terminating null character then returns an fstring for it. Slower than ToRawString(), but more suitable for string APIs
		virtual fstring ToString() const = 0;

		// Gets the status code (HRESULT on Win32, otherwise OS dependent) of the file error, populates and truncates the supplied buffer with an error message
		// and returns the length of the full error string with terminating null character. If buffer or nBytes in buffer are null, the buffer is ignored.
		virtual size_t GetErrorAndStatusLength(int& statusCode, char* buffer = nullptr, size_t nBytesInBuffer = 0) const = 0;

		// Throws an exception if the asset is in an error state and adds the filename and/or the function name to the error message if booleans are set to true
		virtual void Validate(bool addFilename = true, bool addFunction = true) = 0;

		virtual cstr Path() const = 0;
	};

	struct AssetStatus
	{
		fstring pingPath = { nullptr, 0 };
		HString statusText;
		int statusCode = 0;
		volatile bool isError = false;
		volatile bool isReady = false; // The last thing to be set to true when the FileAssetFactory has completed loading the asset, or completed poplating the status codes

		size_t GetErrorAndStatusLength(int& statusCode, char* buffer, size_t nBytesInBuffer) const;
		void Validate(bool addFilename, bool addFunctioname);
	};

#ifdef _WIN32
	enum { NOT_READY = /* WAIT_IO_COMPLETION */ 0x000000C0L };
#else
	static_assert(false, "Not implemented");
#endif

	using TAsyncOnLoadEvent = Rococo::Function<void(IFileAsset& asset)>;

	ROCOCO_ASSETS_API void NoFileCallback(IFileAsset& asset);

	ROCOCO_INTERFACE IFileAssetFactoryMonitor
	{
		virtual void OnFileAssetFactoryDataDelivered() = 0;
	};

	ROCOCO_INTERFACE IFileAssetFactory
	{
		// Creates a ref for asynchronous file loading.
		// The file callback will be invoked for the first caller only when the file is loaded.
		// If the file is already loaded then either IFileAsset::IsLoaded() or IFileAsset::IsError will return true.
		// It is recommended that the caller of the method ensure the onLoad callback is valid for the lifetime of the file asset factory.
		// The asset object is used to name the onLoad event. The API consumer can remove the load event from the mainthread if the  callback is no longer valid or required
		virtual AssetRef<IFileAsset> CreateFileAsset(const char* utf8Path, IAsset* uniqueLoaderKey, TAsyncOnLoadEvent onLoad = NoFileCallback) = 0;

		// Call this periodically in whichever thread is responsible for handling onLoad callbacks. It may also cause garbage collection and other housekeeping by dependent units,
		// wishing to stay in sync with the file loader.

		virtual void DeliverToThisThreadThisTick() = 0;

		virtual void RaiseError(cstr msg, int statusCode, cstr path) = 0;

		using TErrorHandler = Rococo::Function<void(cstr path, cstr message, int errorCode)>;
		virtual void SetErrorHandler(TErrorHandler errorHandler) = 0;

		virtual void AddMonitor(IFileAssetFactoryMonitor* monitor) = 0;
		virtual void RemoveMonitor(IFileAssetFactoryMonitor* uniqueKey) = 0;
	};

	ROCOCO_INTERFACE IFileAssetFactorySupervisor: IFileAssetFactory
	{
		virtual void Free() = 0;
	};

	ROCOCO_ASSETS_API IFileAssetFactorySupervisor* CreateFileAssetFactory(IAssetManager& manager, IO::IInstallation& installation);
}
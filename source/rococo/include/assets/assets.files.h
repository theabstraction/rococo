#pragma once
#include <rococo.assets.h>
#include <rococo.functional.h>

namespace Rococo::IO
{
	struct IInstallation;
}

namespace Rococo::Assets
{
	struct FileData
	{
		const uint8* data;
		size_t nBytes;
	};

	ROCOCO_INTERFACE IFileAsset : IAsset
	{
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

	using TAsyncOnLoadEvent = Rococo::Function<void(IFileAsset& asset)>;

	ROCOCO_ASSETS_API void NoFileCallback(IFileAsset& asset);

	ROCOCO_INTERFACE IFileAssetFactory
	{
		virtual AssetRef<IFileAsset> CreateFileAsset(const char* utf8Path, TAsyncOnLoadEvent onLoad = NoFileCallback) = 0;

		// Call this periodically in whichever thread is responsible for handling onLoad callbacks
		virtual void DeliverToThisThreadThisTick() = 0;
	};

	ROCOCO_INTERFACE IFileAssetFactorySupervisor: IFileAssetFactory
	{
		virtual void Free() = 0;
	};

	ROCOCO_ASSETS_API IFileAssetFactorySupervisor* CreateFileAssetFactory(IAssetManager& manager, IO::IInstallation& installation);
}
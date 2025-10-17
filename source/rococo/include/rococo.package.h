// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

namespace Rococo
{
	struct PackageFileData
	{
		const char* data;
		size_t filesize;
		U8FilePath name;
	};

	/*
		Identify the package with FriendlyName() and obtain an XXHash checksum by a call to HashCode() 
		which allows you to distinguish two identical packages.

	To enumerate directores, first call BuildDirectoryCache(const char* prefix) then grab
	the results with ForEachDirInCache(IEventCallback<const char*>& cb) const

	To enumerate files, first call size_t BuildFileCache(const char* prefix)
	then grab the results with ForEachFileInCache(IEventCallback<const char*>& cb) const

	To get a pointer and length to file data call GetFileInfo(const char* resourcePath, FileData& f)

	N.B the GetFileInfo uses O(log N) or better search algorithm,
	but the cache and enumerations methods may be a lot slower.
*/
	ROCOCO_INTERFACE IPackage
	{
		// The maximum length of a legal friendly name + 1
		enum { MAX_PACKAGE_NAME_BUFFER_LEN = 64 };

		// Return a cached string to identiy the package
		virtual cstr FriendlyName() const = 0;

		// Return a cached 64-bit XXHash code for the package
		virtual const int64 HashCode() const = 0;

		// A pointer to the raw data in the package
		virtual const char* RawData() const = 0;

		// Number of bytes of raw data
		virtual size_t RawLength() const = 0;

		// Cache a list of subdirectories and return the list size
		virtual size_t BuildDirectoryCache(const char* dir) = 0;

		// Cache a list of files that start with the prefix and return the list size
		virtual size_t BuildFileCache(const char* prefix) = 0;

		// Enumerate the files in the directory cache. File caching is locked during enumeration
		virtual void ForEachDirInCache(Strings::IStringPopulator& cb) const = 0;

		// Enumerate the directories in the directory cache. Directory caching is locked during enumeration
		virtual void ForEachFileInCache(Strings::IStringPopulator& cb) const = 0;

		// Fill in a FileData structure with the file data block
		virtual void GetFileInfo(const char* resourcePath, OUT PackageFileData& f) const = 0;

		// Fill in a FileData structure with the file data block. Return false if the path does not exist
		virtual bool TryGetFileInfo(const char* resourcePath, OUT PackageFileData& f) const = 0;
	};

	ROCOCO_INTERFACE IPackageSupervisor : IPackage
	{
		virtual void Free() = 0;
	};

	// sysPath - unix or windows format of the zip file path.
	// friendlyName - unique id for distinguishing the zip from others. 
	//              - style rule: use lower case [a-z] letters only
	// IPackageSupervisor lifetime should encompass the entities that refer to them
	ROCOCO_API IPackageSupervisor* OpenZipPackage(crwstr sysPath, const char* friendlyName);
}
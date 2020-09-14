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
	ROCOCOAPI IPackage
	{
		// Return a cached string to identiy the package
		virtual cstr FriendlyName() const = 0;

		// Return a cached 64-bit XXHash code for the package
		virtual const int64 HashCode() const = 0;

		// Cache a list of subdirectories and return the list size
		virtual size_t BuildDirectoryCache(const char* dir) = 0;

		// Cache a list of files that start with the prefix and return the list size
		virtual size_t BuildFileCache(const char* prefix) = 0;

		// Enumerate the files in the directory cache. File caching is locked during enumeration
		virtual void ForEachDirInCache(IEventCallback<const char*>& cb) const = 0;

		// Enumerate the directories in the directory cache. Directory caching is locked during enumeration
		virtual void ForEachFileInCache(IEventCallback<const char*>& cb) const = 0;

		// Fill in a FileData structure with the file data block
		virtual void GetFileInfo(const char* resourcePath, OUT PackageFileData& f) const = 0;
	};

	ROCOCOAPI IPackageSupervisor : IPackage
	{
		virtual void Free() = 0;
	};

	// sysPath - unix or windows format of the zip file path.
	// friendlyName - unique id for distinguishing the zip from others. 
	//              - style rule: use lower case [a-z] letters only
	// IPackageSupervisor lifetime should encompass the entities that refer to them
	IPackageSupervisor* OpenZipPackage(const wchar_t* sysPath, const char* friendlyName);
}
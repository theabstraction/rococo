#define ROCOCO_API __declspec(dllexport)
#include <rococo.types.h>
#include <rococo.package.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <string>

#include <algorithm>
#include <unordered_map>


using namespace Rococo;
using namespace Rococo::Strings;

namespace std
{
	/* N.B do not replace with StringKey, our U8FilePath as key only requires heap allocation
	* for the unordered_map, and not the keys themselves, and our map is meant to be transient
	* so we want to minimize heap fragmentation. We re-use the map, so it is possible that
	* after some uses, heap allocation may cease for real-world scenarios in the use of this API
	*/
	template<> struct hash<U8FilePath>
	{
		size_t operator() (const U8FilePath& f) const
		{
			return XXHash64Arg(f.buf, strlen(f.buf));
		}
	};

	bool operator == (const U8FilePath& a, const U8FilePath& b)
	{
		return Eq(a, b);
	}
}

namespace Rococo::Strings
{
	ROCOCO_API int32 Format(U8FilePath& path, cstr format, ...);
	ROCOCO_API int32 Format(WideFilePath& path, _Printf_format_string_ const wchar_t* format, ...);
}

namespace
{
	static auto version1000 = "Rococo.Package:v1.0.0.0\n"_fstring;
	static auto headerLengthString = "HeaderLength:"_fstring;
	static auto headerCodeSet = "StringFormat:UTF8\n"_fstring;
	static auto headerByteOrder = "ByteOrder:AGEM\n"_fstring;
	static auto headerFileCount = "Files:"_fstring;
	static auto newLine = "\n"_fstring;
	static auto tab = "\t"_fstring;

	typedef std::unordered_map<U8FilePath, int> TU8FilePathMap;

	void ValidateStringAndAdvance(const fstring& s, const char*& header, size_t& bufferLength)
	{
		if (s.length > bufferLength)
		{
			Throw(0, "Expecting %s in the header but insufficient length", s.buffer);
		}

		if (Compare(header, s, s.length) != 0)
		{
			Throw(0, "Expecting %s in the header", s.buffer);
		}

		header += s.length;
		bufferLength -= s.length;
	}

	struct SHeader_10000
	{
		uint64 headerLength;
		uint32 fileCount;
		const char* filenames;
	};

	SHeader_10000 ParseHeader_1000(const char* header, size_t bufferLength)
	{
		SHeader_10000 h = { 0 };

		ValidateStringAndAdvance(headerLengthString, header, bufferLength);

		if (bufferLength < 10)
		{
			Throw(0, "Insufficient data to interpret header length");
		}

		char slength[9];
		memcpy(slength, header, 8);
		slength[8] = 0;

		char* endptr = nullptr;
		h.headerLength = strtol(slength, &endptr, 10);

		header += 8;
		bufferLength -= 8;
		
		ValidateStringAndAdvance(newLine, header, bufferLength);
		ValidateStringAndAdvance(headerCodeSet, header, bufferLength);
		ValidateStringAndAdvance(headerByteOrder, header, bufferLength);
		ValidateStringAndAdvance(headerFileCount, header, bufferLength);

		char sfilecount[16] = { 0 };

		for (size_t i = 0; i < 15; ++i)
		{
			switch (header[i])
			{
			case '\n':
				sfilecount[i] = 0;
				i = 16;
				break;
			default:
				if (i >= bufferLength)
				{
					Throw(0, "Error, reached end of buffer before reading filecount");
				}
				sfilecount[i] = header[i];
				break;
			}
		}

		h.fileCount = atoi(sfilecount);
		if (h.fileCount <= 0)
		{
			Throw(0, "Error, filecount was not positive");
		}

		enum { MAX_FILES = 10000 };

		if (h.fileCount > MAX_FILES)
		{
			Throw(0, "Error. Cannot handle archives with more than %u files", MAX_FILES);
		}

		auto fclen = strlen(sfilecount);
		header += fclen;
		bufferLength -= fclen;

		ValidateStringAndAdvance(newLine, header, bufferLength);
		ValidateStringAndAdvance(newLine, header, bufferLength);

		h.filenames = header;

		return h;
	}

	struct Header
	{
		uint64 headerLength;
		uint32 fileCount;
		const char* filenames;
	};

	struct FileInfo
	{
		std::string path;
		uint64 length;
		uint64 position;
	};

	typedef std::vector<FileInfo> TFiles;

	Header ParseHeader(const char* header, size_t bufferLength)
	{
		if (version1000.length >= bufferLength)
		{
			Throw(0, "Could not find %s in the header. Header too short", version1000.buffer);
		}

		if (Compare(header, version1000, version1000.length) == 0)
		{
			auto h = ParseHeader_1000(header + version1000.length, bufferLength - version1000.length);
			return Header{ h.headerLength, h.fileCount, h.filenames };
		}
		else
		{
			Throw(0, "Could not find %s in the header. Unknown version string", version1000.buffer);
		}
	}

	size_t ReadBuffer(const char* filename, size_t bufferLength, char* path, size_t capacity, char terminator)
	{
		for (size_t i = 0; i < capacity - 1; ++i)
		{
			if (i >= bufferLength)
			{
				Throw(0, "Exhausted buffer trying to read filenames");
			}

			if (filename[i] == terminator)
			{
				path[i] = 0;
				return i;
			}
			else
			{
				path[i] = filename[i];
			}
		}

		Throw(0, "Exhausted buffer trying to find newline after filename");
	}

	struct EnumLock
	{
		bool* pLock = nullptr;

		EnumLock(bool& lock)
		{
			lock = true;
			pLock = &lock;
		}

		~EnumLock()
		{
			*pLock = false;
		}
	};

	struct SXYZMapPackage : IPackageSupervisor
	{
		AutoFree<IO::IReadOnlyBinaryMapping> map;
		U8FilePath name;

		const char* buffer = nullptr;
		size_t bufferLen = 0;
		int64 hash = 0;

		Header header = { 0 };

		TFiles files;
		TU8FilePathMap dircache;
		std::vector<U8FilePath> filecache;

		mutable bool enumLockFiles = false;
		mutable bool enumLockDirs = false;

		SXYZMapPackage(const wchar_t* filename, const char* key)
		{
			map = IO::CreateReadOnlyBinaryMapping(filename);
			buffer = map->Data();
			bufferLen = map->Length();
			hash = XXHash64Arg(buffer, bufferLen);
			Format(name, "%s", key);

			header = ParseHeader(buffer, bufferLen);

			files.reserve(header.fileCount);

			size_t bufferLeft = bufferLen - (header.filenames - buffer);

			const char* readPointer = header.filenames;

			uint64 filePos = header.headerLength;

			for (uint32 i = 0; i < header.fileCount; ++i)
			{
				U8FilePath path;

				size_t len_Path = ReadBuffer(readPointer, bufferLeft, path.buf, path.CAPACITY, '\t');
				readPointer += len_Path;
				bufferLeft -= len_Path;

				ValidateStringAndAdvance(tab, readPointer, bufferLeft);

				char slength[24];
				size_t len_slength = ReadBuffer(readPointer, bufferLeft, slength, sizeof(slength), '\n');

				char* endPtr = nullptr;
				uint64 fileLength = strtol(slength, &endPtr, 10);

				if (fileLength > 1024_megabytes)
				{
					Throw(0, "Sanity check: %s length was > 1GB", path.buf);
				}

				readPointer += len_slength;
				bufferLeft -= len_slength;

				ValidateStringAndAdvance(newLine, readPointer, bufferLeft);

				files.push_back({ std::string(path), fileLength, filePos });

				filePos += fileLength + 2; // The file + the trailing null & new line characters
			}

			ValidateStringAndAdvance(newLine, readPointer, bufferLeft);

			if (bufferLeft + header.headerLength != bufferLen)
			{
				Throw(0, "Inconsistent file buffer lengths");
			}

			if (filePos != bufferLen)
			{
				Throw(0, "Inconsistent file positions");
			}

			std::sort(files.begin(), files.end(),
				[](const FileInfo& a, const FileInfo& b)
				{
					return a.path < b.path;
				}
			);
		}

		virtual ~SXYZMapPackage()
		{
		}

		cstr FriendlyName() const override
		{
			return name;
		}

		const int64 HashCode() const override
		{
			return hash;
		}

		void GetFileInfo(const char* resourcePath, PackageFileData& f) const override
		{
			if (!TryGetFileInfo(resourcePath, f))
			{
				Throw(0, "%s: Could not find %s in the package %s", __FUNCTION__, resourcePath, name.buf);
			}
		}

		bool TryGetFileInfo(const char* resourcePath, PackageFileData& f) const override
		{
			FileInfo val =
			{
				resourcePath,
				0,
				0
			};

			auto range = std::equal_range(files.begin(), files.end(), val,
				[](const FileInfo& a, const FileInfo& b)
				{
					return a.path < b.path;
				});

			if (range.first == range.second)
			{
				*f.name.buf = 0;
				f.filesize = 0;
				f.data = nullptr;
				return false;
			}

			Format(f.name, "%s", range.first->path.c_str());
			f.filesize = range.first->length;
			f.data = buffer + range.first->position;

			return true;
		}

		size_t BuildDirectoryCache(const char* prefix) override
		{
			if (prefix == nullptr)
			{
				Throw(0, "%s: prefix was null", __FUNCTION__);
			}

			if (*prefix == '/')
			{
				Throw(0, "%s: prefix should not begin with a forward slash: /", __FUNCTION__);
			}

			auto lenPrefix = strlen(prefix);

			if (lenPrefix > 0 && !EndsWith(prefix, "/"))
			{
				Throw(0, "%s: prefix did not terminate with a forward slash: /", __FUNCTION__);
			}

			if (enumLockDirs)
			{
				Throw(0, "%s: cannot rebuild cache while the directories are being enumerated.", __FUNCTION__);
			}

			std::pair<U8FilePath, int> subdirPair;
			auto& subdir = subdirPair.first;
			memcpy_s(subdir.buf, subdir.CAPACITY, prefix, lenPrefix);

			dircache.clear();

			for (auto& f : files)
			{
				if (StartsWith(f.path.c_str(), prefix))
				{
					auto suffix = f.path.c_str() + lenPrefix;
					for (const char* s = suffix; *s != 0; s++)
					{
						if (*s == '/')
						{
							auto* mid = subdir.buf + lenPrefix;
							auto bufferLeft = subdir.CAPACITY - lenPrefix;
							memcpy_s(mid, bufferLeft, suffix, s - suffix + 1);
							subdir.buf[lenPrefix + s - suffix + 1] = 0;

							auto i = dircache.find(subdir);
							if (i == dircache.end())
							{
								dircache.insert(subdirPair);
							}

							break;
						}
					}
				}
			}

			return dircache.size();
		}

		size_t BuildFileCache(const char* prefix) override
		{
			if (prefix == nullptr)
			{
				Throw(0, "%s: prefix was null", __FUNCTION__);
			}

			if (enumLockFiles)
			{
				Throw(0, "%s: cannot rebuild cache while the directories are being enumerated.", __FUNCTION__);
			}

			auto lenPrefix = strlen(prefix);

			FileInfo PREFIX
			{
				prefix,
				0,
				0
			};

			auto range = std::equal_range(files.begin(), files.end(), PREFIX,
				[lenPrefix](const FileInfo& a, const FileInfo& b)
				{
					return Compare(a.path.c_str(), b.path.c_str(), (int64) lenPrefix) < 0;
				});

			if (range.first == range.second)
			{
				Throw(0, "%s: Could not find %s in the package %s", __FUNCTION__, prefix, name.buf);
			}

			filecache.reserve(std::distance(range.first, range.second));

			filecache.clear();

			for (auto i = range.first; i != range.second; ++i)
			{
				// If we find a slash after the prefix, it means we have a subdirectory
				// of the prefix, in which case we should not enumerate it.
				if (strstr(i->path.c_str() + lenPrefix, "/") == nullptr)
				{
					U8FilePath path;
					memcpy_s(path.buf, path.CAPACITY, i->path.c_str(), i->path.size());
					path.buf[i->path.size()] = 0;
					filecache.push_back(path);
				}
			}

			return filecache.size();
		}

		void ForEachDirInCache(IEventCallback<const char*>& cb) const override
		{
			EnumLock sync(enumLockDirs);

			for (auto& d : dircache)
			{
				cb.OnEvent(d.first);
			}
		}

		void ForEachFileInCache(IEventCallback<const char*>& cb) const override
		{
			EnumLock sync(enumLockFiles);

			for (auto& d : filecache)
			{
				cb.OnEvent(d);
			}
		}

		void Free() override
		{
			delete this;
		}

		// A pointer to the raw data in the package
		const char* RawData() const override
		{
			return buffer;
		}

		// Number of bytes of raw data
		size_t RawLength() const override
		{
			return bufferLen;
		}
	};
}

namespace Rococo
{
	ROCOCO_API IPackageSupervisor* OpenZipPackage(const wchar_t* sysPath, const char* friendlyName)
	{
		if (sysPath == NULL || friendlyName == NULL)
		{
			Throw(0, "%s: argument null", __FUNCTION__);
		}
		else
		{
			auto len = StringLength(friendlyName);
			if (len < 1 || len >= IPackageSupervisor::MAX_PACKAGE_NAME_BUFFER_LEN)
			{
				Throw(0, "%s: [friendlyName] length out of bounds. Domain: [1,%d]", __FUNCTION__, IPackageSupervisor::MAX_PACKAGE_NAME_BUFFER_LEN - 1);
			}
		}

		WideFilePath osPath;
		Format(osPath, L"%ls", sysPath);
		Rococo::OS::ToSysPath(osPath.buf);
		return new SXYZMapPackage(osPath, friendlyName);
	}
}
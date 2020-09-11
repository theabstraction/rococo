#include <rococo.api.h>
#include <rococo.package.h>
#include <rococo.io.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>

#include <string>

#include <rococo.strings.h>

#include <algorithm>

using namespace Rococo;

namespace
{
	static auto version1000 = "Rococo.Package:v1.0.0.0\n"_fstring;
	static auto headerLengthString = "HeaderLength:"_fstring;
	static auto headerCodeSet = "StringFormat:UTF8\n"_fstring;
	static auto headerByteOrder = "ByteOrder:AGEM\n"_fstring;
	static auto headerFileCount = "Files:"_fstring;
	static auto newLine = "\n"_fstring;
	static auto tab = "\t"_fstring;

	void ExpectString(const fstring& s, const char*& header, size_t& bufferLength)
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

		ExpectString(headerLengthString, header, bufferLength);

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
		
		ExpectString(newLine, header, bufferLength);
		ExpectString(headerCodeSet, header, bufferLength);
		ExpectString(headerByteOrder, header, bufferLength);
		ExpectString(headerFileCount, header, bufferLength);

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
		if (sfilecount <= 0)
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

		ExpectString(newLine, header, bufferLength);
		ExpectString(newLine, header, bufferLength);

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

	struct ZipPackage : IPackageSupervisor
	{
		AutoFree<IO::IReadOnlyBinaryMapping> map;
		U8FilePath name;

		const char* buffer = nullptr;
		size_t bufferLen = 0;
		int64 hash = 0;

		Header header = { 0 };

		TFiles files;

		ZipPackage(const wchar_t* filename, const char* key)
		{
			map = IO::CreateReadOnlyBinaryMapping(filename);
			buffer = map->Data();
			bufferLen = map->Length();
			hash = XXHash64(buffer, bufferLen);
			Format(name, "%s", key);

			header = ParseHeader(buffer, bufferLen);

			files.reserve(header.fileCount);

			size_t bufferLeft = bufferLen - (header.filenames - buffer);

			const char* readPointer = header.filenames;

			uint64 filePos = header.headerLength;

			for (uint32 i = 0; i < header.fileCount; ++i)
			{
				U8FilePath path = { 0 };

				size_t len_Path = ReadBuffer(readPointer, bufferLeft, path.buf, path.CAPACITY, '\t');
				readPointer += len_Path;
				bufferLeft -= len_Path;

				ExpectString(tab, readPointer, bufferLeft);

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

				ExpectString(newLine, readPointer, bufferLeft);

				files.push_back({ std::string(path), fileLength, filePos });

				filePos += fileLength + 2; // The file + the trailing null & new line characters
			}

			ExpectString(newLine, readPointer, bufferLeft);

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

		cstr FriendlyName() const override
		{
			return name;
		}

		const int64 HashCode() const override
		{
			return hash;
		}

		void LoadFileImageForCopying(const char* resourcePath, IFileHandler& handler) override
		{

		}

		void LoadFileImageIntoBuffer(const char* resourcePath, void* buffer, int64 capacity) override
		{

		}

		void GetFileInfo(const char* resourcePath, int index, SubPackageData& pkg) const override
		{

		}

		void GetDirectoryInfo(const char* resourcePath, int index, SubPackageData& pkg) const override
		{

		}

		int CountDirectories(const char* resourcePath) const override
		{
			if (resourcePath == nullptr ||
				*resourcePath == 0 ||
				*resourcePath == '/'
				)
			{
				for (auto& f : files)
				{
					Throw(0, "Not implemented");
				}
			}
			return 0;
		}

		int CountFiles(const char* resourcePath) const override
		{
			return 0;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IPackageSupervisor* OpenZipPackage(const wchar_t* sysPath, const char* friendlyName)
	{
		return new ZipPackage(sysPath, friendlyName);
	}
}
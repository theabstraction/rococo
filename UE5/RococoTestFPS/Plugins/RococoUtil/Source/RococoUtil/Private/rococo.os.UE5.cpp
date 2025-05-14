#define ROCOCO_API 

#include <CoreMinimal.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <time.h>
#include <stdio.h>

#include <Misc/Paths.h>
#include <GenericPlatform/GenericPlatformMisc.h>

namespace Rococo
{
	ROCOCO_API void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

	typedef FilePath<TCHAR> UnrealFilePath;

	void Throw(const FString& msg)
	{
		auto* pMsg = *msg;

		char asciiError[1024];
		int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(pMsg, msg.Len());

		TArray<uint8> buffer;
		buffer.SetNumUninitialized(nElements);

		FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer.GetData()), buffer.Num(), pMsg, nElements);

		Throw(0, "%s", buffer.GetData());
	}
}

namespace Rococo::IO
{
	ROCOCO_API void ToSysPath(char* path)
	{
		char directorySeparator = DirectorySeparatorChar();

		for (char* s = path; *s != 0; ++s)
		{
			if (*s == L'/') *s = directorySeparator;
		}
	}

	ROCOCO_API void ToUnixPath(char* path)
	{
		for (char* s = path; *s != 0; ++s)
		{
			if (*s == '\\') *s = '/';
		}
	}

	ROCOCO_API bool IsFileExistant(const char* filename)
	{
		FString filePath(filename);

		return FPaths::FileExists(filePath);
	}

	ROCOCO_API bool StripLastSubpath(char* fullpath)
	{
		char directorySeparator = DirectorySeparatorChar();

		int32 len = (int32)strlen(fullpath);
		for (int i = len - 2; i > 0; --i)
		{
			if (fullpath[i] == directorySeparator)
			{
				fullpath[i + 1] = 0;
				return true;
			}
		}

		return false;
	}

	ROCOCO_API void SanitizePath(char* path)
	{
		char directorySeparator = DirectorySeparatorChar();

		for (char* s = path; *s != 0; ++s)
		{
			if (*s == '/' || *s == '\\') *s = directorySeparator;
		}
	}

	ROCOCO_API bool MakeContainerDirectory(char* filename)
	{
		int len = (int)strlen(filename);

		for (int i = len - 2; i > 0; --i)
		{
			if (filename[i] == '\\' || filename[i] == '/')
			{
				filename[i + 1] = 0;
				return true;
			}
		}

		return false;
	}

	ROCOCO_API char DirectorySeparatorChar()
	{
		return *FGenericPlatformMisc::GetDefaultPathSeparator();
	}

	ROCOCO_API void CreateDirectoryFolder(const TCHAR* path)
	{
		IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Directory Exists?
		if (!platformFile.DirectoryExists(path))
		{
			if (!platformFile.CreateDirectory(path))
			{
				Throw(FString::Printf(TEXT("Cannot create directory %s"), path));
			}
		}
	}

	ROCOCO_API IBinaryArchive* CreateNewBinaryFile(const TCHAR* sysPath)
	{
		struct BinArchive : IBinaryArchive
		{		
			IFileHandle* file = nullptr;
			FString filename;

			BinArchive(const TCHAR* sysPath): filename(sysPath)
			{
				if (sysPath == nullptr) Throw(0, "%s: null sysPath", __FUNCTION__);

				file = IPlatformFile::GetPlatformPhysical().OpenWrite(sysPath);
				if (file == nullptr)
				{
					Throw(FString::Printf(TEXT("Error creating file: {0}"), sysPath));
				}
			}

			~BinArchive()
			{
				delete file;
			}

			uint64 Position() const override
			{
				return (uint64) file->Tell();
			}

			void Reserve(uint64 nBytes)
			{
				file->Truncate((int64) nBytes);
				SeekAbsolute(0);
			}

			void SeekAbsolute(uint64 position)
			{
				if (!file->Seek((int64)position))
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": %s - Seek failed"), *filename));
				}
			}

			void Truncate()
			{
				int64 pos = file->Tell();
				if (pos > 0)
				{
					file->Truncate(pos);
				}
			}

			void Write(size_t sizeOfElement, size_t nElements, const void* pElements)
			{
				size_t nTotalBytes = nElements * sizeOfElement;

				if (nTotalBytes >= 4096_megabytes)
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": %s - Could not write data. It was greater than 4GB in length"), *filename));
				}

				if (!file->Write((const uint8*)pElements, (int64) nTotalBytes))
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": %s - Could not write data. Disk full?"), *filename));
				}
			}

			void Free() override
			{
				delete this;
			}
		};

		return new BinArchive(sysPath);
	}
}

using namespace Rococo;

void RunRococoOSTests()
{
	char timestamp[26];
	GetTimestamp(timestamp);

	char myPath[16] = "/a/b/c";
	IO::ToSysPath(myPath);
	IO::ToUnixPath(myPath);

	if (strcmp(myPath, "/a/b/c") != 0)
	{
		check(false);
	}
}
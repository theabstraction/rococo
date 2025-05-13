#define ROCOCO_API 

#include <CoreMinimal.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <time.h>

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

	namespace IO
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
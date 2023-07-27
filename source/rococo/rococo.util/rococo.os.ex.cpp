#define ROCOCO_API __declspec(dllexport)
#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.functional.h>
#include <vector>

using namespace Rococo;

namespace
{
	struct AutoFile
	{
		HANDLE hFile = INVALID_HANDLE_VALUE;

		AutoFile(HANDLE l_hFile) : hFile(l_hFile) {}

		~AutoFile()
		{
			if (hFile != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hFile);
			}
		}

		operator HANDLE () { return hFile; }

		void ReadBuffer(DWORD capacity, char* buffer)
		{
			DWORD bufferLeft = capacity;
			DWORD startIndex = 0;
			while (bufferLeft > 0)
			{
				DWORD bytesRead;
				if (!ReadFile(hFile, buffer + startIndex, bufferLeft, &bytesRead, NULL))
				{
					Throw(GetLastError(), "Read failure");
				}

				if (bytesRead == 0)
				{
					// graceful completion
					break;
				}

				startIndex += bytesRead;
				bufferLeft -= bytesRead;
			}
		}
	};
}

namespace Rococo::OS
{
	ROCOCO_API void LoadAsciiTextFile(Function<void(cstr)> onLoad, const wchar_t* filename)
	{
		std::vector<char> asciiData;

		{ // File is locked in this codesection
			AutoFile f{ CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL) };

			if (f.hFile == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "LoadAsciiTextFile: Cannot open file %ls", filename);
			}

			LARGE_INTEGER len;
			if (!GetFileSizeEx(f, &len))
			{
				Throw(GetLastError(), "LoadAsciiTextFile: Cannot determine file size %ls", filename);
			}

			if (len.QuadPart >= (int64)2048_megabytes)
			{
				Throw(GetLastError(), "LoadAsciiTextFile: File too large - length must be less than 2GB.\n%ls", filename);
			}

			asciiData.resize(len.QuadPart + 1);

			try
			{
				f.ReadBuffer((DWORD)len.QuadPart, asciiData.data());
				asciiData.push_back(0);
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "LoadAsciiTextFile: %s.\n%ls", ex.Message(), filename);
			}

		} // File is no longer locked

		onLoad.InvokeElseThrow(asciiData.data());
	}

	void LoadBinaryFile(Function<void(uint8* buffer, size_t fileLength)> onLoad, const wchar_t* filename)
	{
		std::vector<uint8> binData;

		{ // File is locked in this codesection
			AutoFile f{ CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL) };

			if (f.hFile == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "LoadBinaryFile: Cannot open file %ls", filename);
			}

			LARGE_INTEGER len;
			if (!GetFileSizeEx(f, &len))
			{
				Throw(GetLastError(), "LoadBinaryFile: Cannot determine file size %ls", filename);
			}

			if (len.QuadPart >= (int64)2048_megabytes)
			{
				Throw(GetLastError(), "LoadBinaryFile: File too large - length must be less than 2GB.\n%ls", filename);
			}

			binData.resize(len.QuadPart);

			try
			{
				f.ReadBuffer((DWORD)len.QuadPart, (char*) binData.data());
			}
			catch (IException& ex)
			{
				Throw(ex.ErrorCode(), "LoadBinaryFile: %s.\n%ls", ex.Message(), filename);
			}

		} // File is no longer locked

		onLoad.InvokeElseThrow(binData.data(), binData.size());
	}
}


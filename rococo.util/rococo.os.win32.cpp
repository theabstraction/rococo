#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <windows.h>
#include <Psapi.h>
#include <rococo.api.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdlib.h>
#include <rococo.io.h>
#include <process.h>
#include <vector>
#include <unordered_map>
#include <shlobj.h>
#include <comip.h>
#include <Shlwapi.h>
#include <rococo.strings.h>
#include <rococo.debugging.h>
#include <timeapi.h>
#include <algorithm>

#pragma comment(lib, "Shlwapi.lib")

#include <stdlib.h>
#include <rococo.debugging.h>
#include <dbghelp.h>

#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Winmm.lib")

#include <ctime>
#include <shellapi.h>
#include <rococo.strings.h>
#include <rococo.os.h>

namespace Rococo
{
	constexpr fstring packageprefix = "Package["_fstring;

	void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

	namespace IO
	{
		IBinaryArchive* CreateNewBinaryFile(const wchar_t* sysPath)
		{
			struct Win32BinArchive: IBinaryArchive
			{
				HANDLE hFile = INVALID_HANDLE_VALUE;

				Win32BinArchive(const wchar_t* sysPath)
				{
					if (sysPath == nullptr) Throw(0, "%s: null sysPath", __FUNCTION__);
					hFile = CreateFileW(sysPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						Throw(GetLastError(), "Error creating file %ls", sysPath);
					}
				}

				~Win32BinArchive()
				{
					if (hFile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(hFile);
					}
				}

				uint64 Position() const override
				{
					LARGE_INTEGER zero;
					zero.QuadPart = 0;

					LARGE_INTEGER pos;
					if (!SetFilePointerEx(hFile, zero, &pos, FILE_CURRENT))
					{
						Throw(GetLastError(), "SetFilePointerEx: Could not get file position");
					}

					return pos.QuadPart;
				}

				void Reserve(uint64 nBytes)
				{
					SeekAbsolute(nBytes);
					if (!SetEndOfFile(hFile))
					{
						Throw(GetLastError(), "Could not set file length to %llu kb", nBytes / 1024);
					}
					SeekAbsolute(0);
				}

				void SeekAbsolute(uint64 position)
				{
					LARGE_INTEGER iPos;
					LARGE_INTEGER finalPos;
					iPos.QuadPart = position;
					if (!SetFilePointerEx(hFile, iPos, &finalPos, FILE_BEGIN))
					{
						Throw(GetLastError(), "Could not seek to position %llu", position);
					}

					if (iPos.QuadPart != finalPos.QuadPart)
					{
						Throw(0, "Unexpected file position returned from SetFilePointerEx");
					}
				}

				void Truncate()
				{
					if (!SetEndOfFile(hFile))
					{
						Throw(GetLastError(), "Could not truncate");
					}
				}

				void Write(size_t sizeOfElement, size_t nElements, const void* pElements)
				{
					size_t nTotalBytes = nElements * sizeOfElement;

					if (nTotalBytes >= 4096_megabytes)
					{
						Throw(0, "Could not write data. It was greater than 4GB in length");
					}
					
					DWORD nTotal = (DWORD) nTotalBytes;
					DWORD nWritten = 0;
					if (!WriteFile(hFile, pElements, nTotal, &nWritten, NULL) || nWritten != nTotal)
					{
						Throw(GetLastError(), "Error writing %u bytes. Disk full?", nTotal);
					}
				}

				void Free() override
				{
					delete this;
				}
			};

			return new Win32BinArchive(sysPath);
		}

		IBinarySource* ReadBinarySource(const wchar_t* sysPath)
		{
			struct Win32BinFile : IBinarySource
			{
				HANDLE hFile;

				Win32BinFile(const wchar_t* sysPath)
				{
					if (sysPath == nullptr) Throw(0, "%s: null sysPath", __FUNCTION__);
					hFile = CreateFileW(sysPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						Throw(GetLastError(), "Error opening file");
					}
				}

				uint32 Read(uint32 capacity, void* pElements) override
				{
					DWORD bytesRead = 0;
					if (!ReadFile(hFile, pElements, capacity, &bytesRead, NULL))
					{
						Throw(GetLastError(), "Error reading data");
					}
					return bytesRead;
				}

				void Free() override
				{
					delete this;
				}
			};

			return new Win32BinFile(sysPath);
		}

		IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(const wchar_t* sysPath)
		{
			struct Win32ROBinMapping : IReadOnlyBinaryMapping
			{
				void* pMem = NULL;
				uint64 length = 0;

				HANDLE hFile = INVALID_HANDLE_VALUE;
				HANDLE hMap = NULL;

				Win32ROBinMapping(const wchar_t* sysPath)
				{
					hFile = CreateFileW(sysPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile == INVALID_HANDLE_VALUE)
					{
						Throw(GetLastError(), "Error opening file: %ls\n", sysPath);
					}

					LARGE_INTEGER len;
					if (!GetFileSizeEx(hFile, &len))
					{
						Throw(GetLastError(), "Error computing file size: %ls\n", sysPath);
					}

					length = len.QuadPart;

					hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
					if (!hMap)
					{
						Throw(GetLastError(), "Error creating map of file: %ls\n", sysPath);
						CloseHandle(hFile);
					}

					pMem = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
					if (pMem == NULL)
					{
						CloseHandle(hMap);
						CloseHandle(hFile);
						Throw(GetLastError(), "Error mapping file: %ls\n", sysPath);
					}
				}

				~Win32ROBinMapping()
				{
					if (hMap != NULL)
					{
						CloseHandle(hMap);
					}

					if (hFile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(hFile);
					}
				}

				const char* Data() const override
				{
					return (const char*) pMem;
				}

				const uint64 Length() const override
				{
					return length;
				}

				void Free() override
				{
					delete this;
				}
			};

			return new Win32ROBinMapping(sysPath);
		}


		bool IsKeyPressed(int vkeyCode)
		{
			SHORT value = GetAsyncKeyState(vkeyCode);
			return (value & 0x8000) != 0;
		}

		void CopyToClipboard(cstr asciiText)
		{
			if (!OpenClipboard(nullptr))
			{
				OS::BeepWarning();
			}
			else
			{
				EmptyClipboard();

				size_t nBytes = sizeof(char) * (strlen(asciiText) + 1);
				HGLOBAL hItem = GlobalAlloc(GMEM_MOVEABLE, nBytes);
				if (hItem)
				{
					void* pData = GlobalLock(hItem);
					if (pData)
					{
						memcpy(pData, asciiText, nBytes);
						GlobalUnlock(hItem);
						SetClipboardData(CF_TEXT, hItem);
					}
				}
				CloseClipboard();
			}
		}

		void PasteFromClipboard(char* asciiBuffer, size_t capacity)
		{
			if (!OpenClipboard(nullptr))
			{
				OS::BeepWarning();
			}
			else
			{
				HANDLE hItem = GetClipboardData(CF_TEXT);
				if (hItem != nullptr)
				{
					auto* pData = (const char*)GlobalLock(hItem);
					if (pData)
					{
						SafeFormat(asciiBuffer, capacity, "%s", pData);
						GlobalUnlock(hItem);
					}
				}

				CloseClipboard();
			}
		}

		void UseBufferlessStdout()
		{
			setvbuf(stdout, nullptr, _IONBF, 0);
		}

		bool TryGetFileAttributes(const wchar_t* sysPath, FileAttributes& attr)
		{
			if (sysPath == nullptr) Throw(0, "Rococo::IO::GetFileLength: sysPath was null");

			WIN32_FILE_ATTRIBUTE_DATA data;
			if (!GetFileAttributesExW(sysPath, GetFileExInfoStandard, &data))
			{
				attr.fileLength = GetLastError();
				attr.timestamp[0] = 0;
				return false;
			}

			uint64 high = ((uint64)data.nFileSizeHigh) & 0x00000000FFFFFFFF;
			uint64 low  = ((uint64)data.nFileSizeLow) & 0x00000000FFFFFFFF;
			uint64 len = (high << 32) + low;
			attr.fileLength = len;
			
			FILETIME ft = data.ftLastWriteTime;

			SYSTEMTIME t;
			FileTimeToSystemTime(&ft, &t);

			SafeFormat(attr.timestamp, sizeof(attr.timestamp), "%2.2d:%2.2d:%2.2d %2.2d/%2.2d/%4.4d",
				t.wHour, t.wMinute, t.wSecond, t.wDay, t.wMonth, t.wYear);

			return true;
		}
	}

	bool FileModifiedArgs::Matches(cstr resource) const
	{
		const wchar_t* a = this->sysPath;
		cstr b = resource;
		if (*b == L'!') b++;

		while (*a != 0)
		{
			if (*a == L'\\')
			{
				if (*b == L'\\' || *b == L'/')
				{
					// dandy
				}
				else
				{
					return false;
				}
			}
			else if (*a != *b) return false;

			a++;
			b++;
		}

		return *b == 0;
	}
}

#include <rococo.window.h>
#include <Commdlg.h>

namespace
{
	struct FileHandle
	{
		HANDLE hFile;

		FileHandle(HANDLE _hFile) : hFile(_hFile)
		{
		}

		bool IsValid() const
		{
			return hFile != INVALID_HANDLE_VALUE;
		}

		~FileHandle()
		{
			if (IsValid()) CloseHandle(hFile);
		}

		operator HANDLE()
		{
			return hFile;
		}
	};
}

namespace
{
	class CriticalSection: public Rococo::OS::ICriticalSection
	{
	private:
		CRITICAL_SECTION cs;

	public:
		CriticalSection()
		{
			InitializeCriticalSection(&cs);
		}

		virtual ~CriticalSection()
		{
			DeleteCriticalSection(&cs);
		}

		void Free() override
		{
			delete this;
		}

		void Lock() override
		{
			EnterCriticalSection(&cs);
		}

		void Unlock() override
		{
			LeaveCriticalSection(&cs);
		}
	};
}

namespace Rococo
{
	namespace OS
	{
		void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath)
		{
			ShellExecuteW(window, L"open", sysPath, nullptr, nullptr, SW_SHOW);
		}

		void MakeContainerDirectory(char* filename)
		{
			int len = (int)rlen(filename);

			for (int i = len - 2; i > 0; --i)
			{
				if (filename[i] == '\\')
				{
					filename[i + 1] = 0;
					return;
				}
			}
		}

		void MakeContainerDirectory(wchar_t* filename)
		{
			int len = (int)wcslen(filename);

			for (int i = len - 2; i > 0; --i)
			{
				if (filename[i] == L'\\')
				{
					filename[i + 1] = 0;
					return;
				}
			}
		}

		void ShellOpenDocument(cstr documentFilePath)
		{
			auto result = (INT_PTR) ShellExecuteA(NULL, "open", documentFilePath, NULL, NULL, SW_SHOW);
			if (result < 32)
			{
				Throw(GetLastError(), "%s: '%s'", __FUNCTION__, documentFilePath);
			}
		}

		void WakeUp(IThreadControl& thread)
		{
			struct ANON
			{
				static void WakeUp(void* context)
				{

				}
			};
			thread.QueueAPC(ANON::WakeUp, nullptr);
		}

		IThreadSupervisor* CreateRococoThread(IThreadJob* job, uint32 stacksize)
		{
			struct Supervisor;

			struct Context
			{
				IThreadJob* job;
				Supervisor* supervisor;
			};

			struct Supervisor : public IThreadSupervisor
			{
				Supervisor()
				{
					InitializeCriticalSection(&sync);
				}

				~Supervisor()
				{
					Resume();
					isRunning = false;
					QueueUserAPC(WakeUp, (HANDLE)hThread, 0);
					WaitForSingleObject((HANDLE)hThread, INFINITE);
					DeleteCriticalSection(&sync);
				}

				virtual ICriticalSection* CreateCriticalSection()
				{
					return new CriticalSection();
				}

				static void WakeUp(ULONG_PTR data)
				{

				}

				void QueueAPC(FN_APC apc, void* context) override
				{
					QueueUserAPC((PAPCFUNC) apc, (HANDLE)hThread, (ULONG_PTR) context); 
				}

				cstr GetErrorMessage(int& err) const override
				{
					err = threadErrorCode;
					return threadErrorRaised ? threadErrorMessage.c_str() : nullptr;
				}

				void SetRealTimePriority() override
				{
					SetThreadPriority((HANDLE)hThread, THREAD_PRIORITY_TIME_CRITICAL);
				}

				void SleepUntilAysncEvent(uint32 milliseconds) override
				{
					if (isRunning)
					{
						SleepEx(milliseconds, TRUE);
					}
				}

				void Free() override
				{
					delete this;
				}

				bool IsRunning() const
				{
					return isRunning;
				}

				void Resume() override
				{
					ResumeThread((HANDLE)hThread);
				}

				void Lock() override
				{
					EnterCriticalSection(&sync);
				}

				void Unlock() override
				{
					LeaveCriticalSection(&sync);
				}

				CRITICAL_SECTION sync;
				uint32 id;
				uintptr_t hThread = 0;
				bool isRunning = true;
				Context context;
				volatile bool threadErrorRaised = false;
				HString threadErrorMessage;
				int threadErrorCode = 0;
			} *supervisor = new Supervisor();


			struct ANON
			{
				static uint32 ThreadProc(void* argList)
				{
					Context* c = (Context*)argList;
					IThreadJob* thread = reinterpret_cast<IThreadJob*>(c->job);

					try
					{
						return thread->RunThread(*c->supervisor);
					}
					catch (IException& ex)
					{
						c->supervisor->threadErrorMessage = ex.Message();
						c->supervisor->threadErrorRaised = true;
						c->supervisor->threadErrorCode = ex.ErrorCode();
						return ex.ErrorCode();
					}
				}
			};

			supervisor->context = { job, supervisor };
			supervisor->hThread = _beginthreadex(nullptr, stacksize, ANON::ThreadProc, &supervisor->context, CREATE_SUSPENDED, &supervisor->id);
			return supervisor;
		}

		cstr GetAsciiCommandLine()
		{
			auto line =  GetCommandLineA();
			return line;
		}

		void PollKeys(uint8 scanArray[256])
		{
			GetKeyboardState(scanArray);
		}

		bool IsFileExistant(const wchar_t* filename)
		{
			DWORD flags = GetFileAttributesW(filename);
			return flags != INVALID_FILE_ATTRIBUTES;
		}

		bool StripLastSubpath(char* fullpath)
		{
			int32 len = (int32) strlen (fullpath);
			for (int i = len - 2; i > 0; --i)
			{
				if (fullpath[i] == '\\')
				{
					fullpath[i + 1] = 0;
					return true;
				}
			}

			return false;
		}

		bool StripLastSubpath(wchar_t* fullpath)
		{
			int32 len = (int32)wcslen(fullpath);
			for (int i = len - 2; i > 0; --i)
			{
				if (fullpath[i] == L'\\')
				{
					fullpath[i + 1] = 0;
					return true;
				}
			}

			return false;
		}
		
		void SanitizePath(wchar_t* path)
		{
			for (auto* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
			}
		}

		void SanitizePath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '/') *s = '\\';
			}
		}

		void SaveClipBoardText(cstr text, Windows::IWindow& window)
		{
			size_t len = strlen(text) + 1;

			if (OpenClipboard(window))
			{
				HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, len);
				char* lptstrCopy = (char*)GlobalLock(g);
				memcpy(lptstrCopy, text, len);
				GlobalUnlock(g);
				EmptyClipboard();
				if (!SetClipboardData(CF_TEXT, g))
				{
					GlobalFree(g);
				}

				CloseClipboard();
			}
		}

		void ToSysPath(wchar_t* path)
		{
			for (wchar_t* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
			}
		}

		void ToUnixPath(wchar_t* path)
		{
			for (wchar_t* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}

		void ToSysPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
			}
		}

		void ToUnixPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}

		void UILoop(uint32 milliseconds)
		{
			MSG msg;

			ticks count = OS::CpuTicks();
			ticks target = count + (OS::CpuHz() * milliseconds / 1000);

			while (target < OS::CpuTicks())
			{
				MsgWaitForMultipleObjects(0, nullptr, FALSE, 25, QS_ALLINPUT);

				while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		void Format_C_Error(int errorCode, char* buffer, size_t capacity)
		{
			if (0 == FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, buffer, (DWORD)capacity, NULL))
			{
				SafeFormat(buffer, capacity, "Unknown error code (%d)", errorCode);
			}
		}

		int OpenForAppend(void** fp, cstr name)
		{
			return fopen_s((FILE**)fp, name, "ab");
		}

		int OpenForRead(void** fp, cstr name)
		{
			return fopen_s((FILE**)fp, name, "rb");
		}

		ticks CpuTicks()
		{
			LARGE_INTEGER ticks;
			QueryPerformanceCounter(&ticks);
			return ticks.QuadPart;
		}

		ticks CpuHz()
		{
			LARGE_INTEGER hz;
			QueryPerformanceFrequency(&hz);
			return hz.QuadPart;
		}

		ticks UTCTime()
		{
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);
			return *(ticks*)&ft;
		}

		void FormatTime(ticks utcTime, char* buffer, size_t nBytes)
		{
			SYSTEMTIME st;
			char localDate[255], localTime[255];

			FileTimeToLocalFileTime((FILETIME*)&utcTime, (FILETIME*)&utcTime);
			FileTimeToSystemTime((FILETIME*)&utcTime, &st);

			GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, localDate, 255);
			GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, localTime, 255);
			SafeFormat(buffer, nBytes, "%s %s", localTime, localDate);
		}

		void TripDebugger()
		{
			if (IsDebuggerPresent())
			{
				__debugbreak();
			}
		}

		void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode)
		{
			if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, message, (DWORD) sizeofBuffer, nullptr))
			{
				SafeFormat(message, sizeofBuffer, "Unknown error");
			}
			else
			{
				char* s;
				for (s = message; *s != 0; s++)
				{
				}

				if (s > message+1 && s[-1] == '\n')
				{
					s[-1] = 0;
					s[-2] = 0;
				}
			}
		}

		bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window)
		{
			static COLORREF colours[16] = { 0 };

			CHOOSECOLORA c = { 0 };
			c.hwndOwner = window;
			c.lStructSize = sizeof(c);
			c.rgbResult = RGB(colour.red, colour.green, colour.blue);
			c.lpCustColors = colours;
			c.Flags = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN;

			if (ChooseColorA(&c))
			{
				colour.red = GetRValue(c.rgbResult);
				colour.green = GetGValue(c.rgbResult);
				colour.blue = GetBValue(c.rgbResult);
				return true;
			}

			return false;
		}

		bool IsDebugging()
		{
			return IsDebuggerPresent() ? true : false;
		}

		void* AllocBoundedMemory(size_t nBytes)
		{
			auto* pMem = VirtualAlloc(NULL, nBytes, MEM_COMMIT, PAGE_READWRITE);
			if (pMem == nullptr)
			{
				Throw(GetLastError(), "Could not allocate %ull bytes virtual memory", nBytes);
			}
			return pMem;
		}

		void FreeBoundedMemory(void* pMemory)
		{
			VirtualFree(pMemory, 0, MEM_RELEASE);
		}
	}

	MemoryUsage ProcessMemory()
	{
		PROCESS_MEMORY_COUNTERS counters = { 0 };
		counters.cb = sizeof(counters);
		GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
		return{ counters.PagefileUsage, counters.PeakPagefileUsage };
	}

	bool DoesModifiedFilenameMatchResourceName(cstr modifiedFilename, cstr resourceName)
	{
		cstr p = modifiedFilename;
		cstr q = resourceName + 1;

		while (*p != 0)
		{
			if (*p != *q)
			{
				if (*p == '\\' && *q == '/')
				{
					// ok
				}
				else
				{
					return false;
				}
			}

			p++;
			q++;
		}

		return *q == 0;
	}
}

namespace
{
	using namespace Rococo;

	struct FilePath
	{
		enum { CAPACITY = 260 };
		char data[CAPACITY];
		operator char*() { return data; }
		operator cstr() const { return data; }
	};

	void GetContentDirectory(const wchar_t* contentIndicatorName, WideFilePath& path, IOS& os)
	{
		WideFilePath binDirectory;
		os.GetBinDirectoryAbsolute(binDirectory);

		path = binDirectory;

		if (wcsstr(contentIndicatorName, L"\\") != nullptr)
		{
			// The indicator is part of a path
			if (os.IsFileExistant(contentIndicatorName))
			{
				Format(path, L"%s", contentIndicatorName);
				OS::MakeContainerDirectory(path.buf);
				return;
			}
		}

		size_t len = wcslen(path);

		while (len > 0)
		{
			WideFilePath indicator;
			Format(indicator, L"%s%s", path, contentIndicatorName);
			if (os.IsFileExistant(indicator))
			{
				Format(indicator, L"%s%s", path, L"content\\");
				Format(path, L"%s", indicator);
				return;
			}

			OS::MakeContainerDirectory(path.buf);

			size_t newLen = wcslen(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(0, "Could not find %ls below the executable folder '%ls'", contentIndicatorName, binDirectory.buf);
	}

	class Installation : public IInstallationSupervisor
	{
		IOS& os;
		WideFilePath contentDirectory;
		int32 len;
		std::unordered_map<std::string, std::string> macroToSubdir;
	public:
		Installation(const wchar_t* contentIndicatorName, IOS& _os) : os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);
			len = (int32)wcslen(contentDirectory);
		}

		Installation(IOS& _os, const wchar_t* contentPath) : os(_os)
		{
			len = Format(contentDirectory, L"%ls", contentPath);
		}

		void Free()  override
		{
			delete this;
		}

		IOS& OS()  override
		{
			return os;
		}

		const wchar_t* Content() const  override
		{
			return contentDirectory;
		}

		bool DoPingsMatch(cstr a, cstr b) const override
		{
			try
			{
				WideFilePath sysPathA;
				ConvertPingPathToSysPath(a, sysPathA);

				WideFilePath sysPathB;
				ConvertPingPathToSysPath(b, sysPathB);

				return wcscmp(sysPathA, sysPathB) == 0;
			}
			catch (IException&)
			{
				return false;
			}
		}

		void CompressPingPath(cstr pingPath, U8FilePath& compressedPath) const
		{
			struct MacroToSubpath
			{
				std::string macro;
				std::string subpath;

				bool operator < (const MacroToSubpath& other) const
				{
					return other.macro.size() - other.subpath.size() > macro.size() - subpath.size();
				}
			};

			std::vector<MacroToSubpath> macros;
			for (auto& i : macroToSubdir)
			{
				macros.push_back({ i.first, i.second });
			}

			std::sort(macros.begin(), macros.end()); // macros is now sorted in order of macro length

			for (auto& m : macros)
			{
				if (StartsWith(pingPath, m.subpath.c_str()))
				{
					Format(compressedPath, "%s/%s", m.macro.c_str(), pingPath + m.subpath.size());
					return;
				}
			}

			Format(compressedPath, "%s", pingPath);
		}

		cstr GetFirstSlash(cstr path) const
		{
			for (cstr p = path + 1; *p != 0; p++)
			{
				if (*p == '/')
				{
					return p;
				}
			}

			return nullptr;
		}

		void ConvertPackagePathToSysPath(cstr pingPath, WideFilePath& sysPath) const
		{
			char packName[64];
			char* p = packName;

			cstr dir = nullptr;
			for (auto s = pingPath + packageprefix.length; *s != 0; ++s)
			{
				if (*s == ']')
				{
					*p = 0;
					if (s[1] != '@')
					{
						Throw(0, "%s: Expecting ]@ after package name", pingPath);
					}
					dir = s + 2;
					break;
				}

				*p++ = *s;

				if (p - packName >= sizeof(packName) - 2)
				{
					Throw(0, "%s: Expecting ]@ after package name. The name seemed excessively long", pingPath);
				}
			}

			if (dir == nullptr)
			{
				Throw(0, "%s: Expecting @ after package name", pingPath);
			}

			sysPath = contentDirectory;
			Rococo::OS::StripLastSubpath(sysPath.buf);
			size_t len = StringLength(sysPath);
			SecureFormat(sysPath.buf + len, sysPath.CAPACITY - len, L"packages/%hs/%hs", packName, dir);
			OS::ToSysPath(sysPath.buf);
		}

		void ConvertPingPathToSysPath(cstr pingPath, WideFilePath& sysPath) const override
		{
			if (pingPath == nullptr || *pingPath == 0)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Ping path was blank");
			}

			if (strncmp(pingPath, packageprefix, packageprefix.length) == 0)
			{
				ConvertPackagePathToSysPath(pingPath, sysPath);
				return;
			}

			auto macroDir = "";
			const char* subdir = nullptr;

			if (*pingPath == '!')
			{
				subdir = pingPath + 1;

				Format(sysPath, L"%ls%hs", contentDirectory, subdir);
			}
			else if (*pingPath == '#')
			{
				auto slash = GetFirstSlash(pingPath + 1);
				if (slash == nullptr)
				{
					Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): expecting forward slash character in pingPath", pingPath);
				}

				subdir = slash + 1;

				char macro[IO::MAX_PATHLEN];
				memcpy_s(macro, sizeof(macro), pingPath, slash - pingPath);
				macro[slash - pingPath] = 0;

				auto i = macroToSubdir.find(macro);
				if (i == macroToSubdir.end())
				{
					Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown macro: %s", macro, pingPath);
				}

				macroDir = i->second.c_str();

				Format(sysPath, L"%ls%hs%hs", contentDirectory, macroDir + 1, subdir);
			}
			else
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown prefix. Expecting ! or #", pingPath);
			}

			if (strstr(pingPath, "..") != nullptr)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
			}

			OS::ToSysPath(sysPath.buf);
		}

		void ConvertSysPathToMacroPath(const wchar_t* sysPath, U8FilePath& pingPath, cstr macro) const override
		{
			U8FilePath fullPingPath;
			ConvertSysPathToPingPath(sysPath, fullPingPath);

			auto i = macroToSubdir.find(macro);
			if (i == macroToSubdir.end())
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...) No macro defined: %s", macro);
			}

			cstr expansion = i->second.c_str();
			if (strstr(fullPingPath, expansion) == nullptr)
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...\"%ls\", \"%hs\") Path not prefixed by macro: %hs", sysPath, macro, expansion);
			}

			Format(pingPath, "%s/%s", macro, fullPingPath.buf + i->second.size());
		}

		void ConvertSysPathToPingPath(const wchar_t* sysPath, U8FilePath& pingPath) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			int sysPathLen = (int) wcslen(sysPath);

			size_t contentDirLength = wcslen(contentDirectory);
			
			if (0 != _wcsnicmp(sysPath, contentDirectory, wcslen(contentDirectory)))
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' did not begin with the content folder %ls", sysPath, contentDirectory.buf);
			}

			if (wcsstr(sysPath, L"..") != nullptr)
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' - Illegal sequence in ping path: '..'", sysPath);
			}

			Format(pingPath, "!%ls", sysPath + contentDirLength);

			OS::ToUnixPath(pingPath.buf);
		}
		
		bool TryExpandMacro(cstr macroPrefixPlusPath, U8FilePath& expandedPath) override
		{
			auto slash = GetFirstSlash(macroPrefixPlusPath + 1);
			if (slash == nullptr)
			{
				Throw(0, "Installation::TryExpandMacro(\"%s\"): expecting forward slash character in pingPath", macroPrefixPlusPath);
			}

			cstr subdir = slash + 1;

			U8FilePath macro;
			memcpy_s(macro.buf, macro.CAPACITY, macroPrefixPlusPath, slash - macroPrefixPlusPath);
			macro.buf[slash - macroPrefixPlusPath] = 0;

			auto i = macroToSubdir.find(macro.buf);
			if (i == macroToSubdir.end())
			{
				return false;
			}

			Format(expandedPath, "%s%s", i->second.c_str(), subdir);
			return true;
		}

		void LoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (pingPath == nullptr || rlen(pingPath) < 2) Throw(E_INVALIDARG, "Win32OS::LoadResource failed: <resourcePath> was blank");

			WideFilePath absPath;
			if (pingPath[0] == '!' || pingPath[0] == '#')
			{
				ConvertPingPathToSysPath(pingPath, absPath);
			}
			else
			{
				Assign(absPath, pingPath);
			}

			os.LoadAbsolute(absPath, buffer, maxFileLength);
		}

		bool TryLoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (pingPath == nullptr || rlen(pingPath) < 2) Throw(E_INVALIDARG, "Win32OS::LoadResource failed: <resourcePath> was blank");

			WideFilePath absPath;
			if (pingPath[0] == '!' || pingPath[0] == '#')
			{
				ConvertPingPathToSysPath(pingPath, absPath);
			}
			else
			{
				Assign(absPath, pingPath);
			}

			if (!os.IsFileExistant(absPath))
			{
				return false;
			}

			os.LoadAbsolute(absPath, buffer, maxFileLength);

			return true;
		}

		void Macro(cstr name, cstr pingFolder) override
		{
			if (name == nullptr || *name != '#')
			{
				Throw(0, "Installation::Macro(name, ...): [name] did not begin with a hash '#' character");
			}

			if (pingFolder == nullptr || *pingFolder != '!')
			{
				Throw(0, "Installation::Macro(..., pingFolder): [pingFolder] did not begin with a ping '!' character");
			}

			U8FilePath pingRoot;
			int len = Format(pingRoot, "%s", pingFolder);
			OS::ToUnixPath(pingRoot.buf);
			if (pingRoot[len - 1] != '/')
			{
				Throw(0, "Installation::Macro(..., pingFolder): %s did not end with slash '/' character");
			}

			auto i = macroToSubdir.find(name);
			if (i != macroToSubdir.end())
			{
				Throw(0, "Installation::Macro(\"%s\", ...) - macro already defined", name);
			}

			macroToSubdir[name] = pingFolder;
		}
	};

	class LocalCriticalSection : public ILock
	{
		CRITICAL_SECTION sysCS;

	public:
		LocalCriticalSection()
		{
			InitializeCriticalSection(&sysCS);
		}

		~LocalCriticalSection()
		{
			DeleteCriticalSection(&sysCS);
		}

		void Lock()
		{
			EnterCriticalSection(&sysCS);
		}

		void Unlock()
		{
			LeaveCriticalSection(&sysCS);
		}
	};

	class Win32OS : public IOSSupervisor
	{
		wchar_t binDirectory[Rococo::IO::MAX_PATHLEN];
		HANDLE hMonitorDirectory;
		std::wstring monitorDirectoryRoot;
		uintptr_t hThread;
		unsigned threadId;
		bool isRunning;

		LocalCriticalSection threadLock;
		std::vector<std::wstring> modifiedFiles;

		IEventCallback<SysUnstableArgs>* onUnstable;

		std::unordered_map<std::wstring, Rococo::OS::ticks> lastModifiedList;

	public:
		Win32OS() :
			hMonitorDirectory(INVALID_HANDLE_VALUE),
			hThread(0),
			threadId(0),
			isRunning(false),
			onUnstable(nullptr)
		{
			auto hAppInstance = GetModuleHandle(nullptr);
			GetModuleFileNameW(hAppInstance, binDirectory, _MAX_PATH);
			OS::MakeContainerDirectory(binDirectory);
		}

		~Win32OS()
		{
			if (isRunning)
			{
				isRunning = false;
				struct wake { static VOID CALLBACK me(ULONG_PTR param) {} };
				QueueUserAPC(wake::me, HANDLE(hThread), 0);
				WaitForSingleObject(HANDLE(hThread), 5000);
				CloseHandle(HANDLE(hThread));
			}

			CloseHandle(hMonitorDirectory);
		}

		void Free() override
		{
			delete this;
		}

		void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb) override
		{
			while (!modifiedFiles.empty())
			{
				threadLock.Lock();
				std::wstring f = modifiedFiles.back();
				modifiedFiles.pop_back();
				threadLock.Unlock();

				// N.B the Windows API for scanning file changes will typically send multiple events
				// for the same file change, so we ignore the superfluous notifications in the same period

				auto i = lastModifiedList.find(f);
				if (i != lastModifiedList.end())
				{
					auto oneHalfSecond = Rococo::OS::CpuHz() / 2;
					auto dt = Rococo::OS::CpuTicks() - i->second;
					if (dt < oneHalfSecond)
					{
						// We've reported a change to that file in the last half a second, so skip 
						continue;
					}

					i->second = Rococo::OS::CpuTicks();
				}
				else
				{
					lastModifiedList[f] = Rococo::OS::CpuTicks();
				}

				cb.OnEvent(FileModifiedArgs{ f.c_str() });
			}
		}

		void FireUnstable() override
		{
			SysUnstableArgs unused;
			if (onUnstable) onUnstable->OnEvent(unused);
		}

		void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb) override
		{
			onUnstable = cb;
		}

		void OnModified(const wchar_t* filename)
		{
			Sleep(500);
			Sync sync(threadLock);

			enum { MAX_MODIFIED_QUEUE_LENGTH = 20 };
			if (modifiedFiles.size() < MAX_MODIFIED_QUEUE_LENGTH)
			{
				bool isInList = false;

				for (auto& v : modifiedFiles)
				{
					if (Eq(v.c_str(), filename))
					{
						isInList = true;
						break;
					}
				}

				if (!isInList) modifiedFiles.push_back(filename);
			}
		}

		void OnScan(const FILE_NOTIFY_INFORMATION& info)
		{
			// Give the system 500 milliseconds to finalize file modifications before we notify that it has been changed
			const FILE_NOTIFY_INFORMATION* i = &info;

			while (true)
			{
				if (i->Action == FILE_ACTION_MODIFIED)
				{
					size_t nChars = info.FileNameLength >> 1;
					if (nChars < _MAX_PATH - 1)
					{
						WideFilePath nullTerminatedFilename = { 0 };
						for (DWORD i = 0; i < nChars; ++i)
						{
							nullTerminatedFilename.buf[i] = info.FileName[i];
						}
						nullTerminatedFilename.buf[nChars] = 0;

						WideFilePath fullPath;
						Format(fullPath, L"%ls%ls", monitorDirectoryRoot.c_str(), nullTerminatedFilename.buf);

						if (!Rococo::IO::IsDirectory(fullPath))
						{
							OnModified(fullPath);
						}
					}
				}

				if (!i->NextEntryOffset) break;

				i = (const FILE_NOTIFY_INFORMATION*)(((char*)i) + i->NextEntryOffset);
			}
		}

		unsigned MonitorDirectory()
		{
			struct Context
			{
				char raw[32768];
				OVERLAPPED ovl;
				Win32OS* os;
				DWORD bytesReturned;
				HANDLE hMonitorDirectory;
				int exitCode;

				void OnScan()
				{
					auto& info = *(FILE_NOTIFY_INFORMATION*)raw;
					os->OnScan(info);
					exitCode = QueueScan();
				}

				static void WINAPI OnScan(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
				{
					Context* This = (Context*)lpOverlapped->hEvent;
					This->OnScan();
				}

				int QueueScan()
				{
					DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
					if (!ReadDirectoryChangesW(hMonitorDirectory, raw, sizeof(raw), TRUE, dwNotifyFilter, &bytesReturned, &ovl, OnScan))
					{
						return GetLastError();
					}
					else
					{
						return NO_ERROR;
					}
				}
			} c;

			ZeroMemory(&c, sizeof(c));

			c.ovl.hEvent = &c;
			c.os = this;
			c.hMonitorDirectory = hMonitorDirectory;

			c.exitCode = c.QueueScan();

			while (isRunning && c.exitCode == NO_ERROR)
			{
				SleepEx(INFINITE, TRUE);
			}

			return c.exitCode;
		}

		static unsigned _stdcall thread_monitor_directory(void *context)
		{
			Win32OS* This = (Win32OS*)context;
			return This->MonitorDirectory();
		}

		void Monitor(const wchar_t* absPath) override
		{
			if (!EndsWith(absPath, L"\\"))
			{
				Throw(0, "%s: [absPath] must end with '\\'", __FUNCTION__);
			}

			monitorDirectoryRoot = absPath;

			if (isRunning || hMonitorDirectory != INVALID_HANDLE_VALUE)
			{
				Throw(0, "A directory is already being monitored");
			}

			DWORD shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			hMonitorDirectory = CreateFileW(absPath, GENERIC_READ, shareFlags, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			if (hMonitorDirectory == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Failed to create monitor on directory %ls", absPath);
			}

			isRunning = true;
			hThread = _beginthreadex(nullptr, 65536, thread_monitor_directory, this, 0, &threadId);
		}

		bool IsFileExistant(const wchar_t* absPath) const override
		{
			return INVALID_FILE_ATTRIBUTES != GetFileAttributesW(absPath);
		}

		void ConvertUnixPathToSysPath(const wchar_t* unixPath, WideFilePath& sysPath) const override
		{
			if (unixPath == nullptr) Throw(E_INVALIDARG, "Blank path in call to os.ConvertUnixPathToSysPath");
			if (wcslen(unixPath) >= sysPath.CAPACITY)
			{
				Throw(E_INVALIDARG, "Path too long in call to os.ConvertUnixPathToSysPath");
			}

			size_t len = wcslen(unixPath);

			size_t i = 0;
			for (; i < len; ++i)
			{
				wchar_t c = unixPath[i];

				if (c == '\\') Throw(E_INVALIDARG, "Illegal backslash '\\' in unixPath in call to os.ConvertUnixPathToSysPath");

				if (c == '/')
				{
					sysPath.buf[i] = '\\';
				}
				else
				{
					sysPath.buf[i] = c;
				}
			}

			sysPath.buf[i] = 0;
		}

		void LoadAbsolute(const wchar_t* absPath, IExpandingBuffer& buffer, int64 maxFileLength) const override
		{
			FileHandle hFile = CreateFileW(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), "Win32OS::LoadResource failed: Error opening file %ls", absPath);

			LARGE_INTEGER len;
			GetFileSizeEx(hFile, &len);

			if (maxFileLength > 0 && len.QuadPart > maxFileLength)
			{
				Throw(0, "Win32OS::LoadResource failed: File <%s> was too large at over %ld bytes", absPath, maxFileLength);
			}

			buffer.Resize(len.QuadPart + 1); // This gives us space for a nul terminating character
			buffer.Resize(len.QuadPart);

			int64 bytesLeft = len.QuadPart;
			ptrdiff_t offset = 0;

			uint8* data = (uint8*)buffer.GetData();
			data[len.QuadPart] = 0;

			while (bytesLeft > 0)
			{
				DWORD chunk = (DWORD)(int32)min(bytesLeft, 65536LL);
				DWORD bytesRead = 0;
				if (!ReadFile(hFile, data + offset, chunk, &bytesRead, nullptr))
				{
					Throw(HRESULT_FROM_WIN32(GetLastError()), "Error reading file <%s>", absPath);
				}

				if (bytesRead != chunk)
				{
					Throw(0, "Win32OS::LoadResource: Error reading file <%s>. Failed to read chunk", absPath);
				}

				offset += (ptrdiff_t)chunk;
				bytesLeft -= (int64)chunk;
			}
		}

		void GetBinDirectoryAbsolute(WideFilePath& directory) const override
		{
			Format(directory, L"%s", binDirectory);
		}

		size_t MaxPath() const override
		{
			return _MAX_PATH;
		}
	};
}

namespace Rococo
{
	IOSSupervisor* GetOS()
	{
		return new Win32OS();
	}

	IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& os)
	{
		return new Installation(contentIndicatorName, os);
	}

	IInstallationSupervisor* CreateInstallationDirect(const wchar_t* contentDirectory, IOS& os)
	{
		return new Installation(os, contentDirectory);
	}

	ThreadLock::ThreadLock()
	{
		static_assert(sizeof(CRITICAL_SECTION) <= sizeof(implementation), "ThreadLock too small. Increase opaque data");
		InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	ThreadLock::~ThreadLock()
	{
		DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	void ThreadLock::Lock()
	{
		EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	void ThreadLock::Unlock()
	{
		LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	namespace OS
	{
		IAppControlSupervisor* CreateAppControl()
		{
			struct AppControl : public IAppControlSupervisor
			{
				void ShutdownApp() override
				{
					isRunning = false;
					PostQuitMessage(0);
				}

				bool IsRunning() const
				{
					return isRunning;
				}

				void Free() override
				{
					delete this;
				}

				bool isRunning = true;
			};

			return new AppControl();
		}

		void BeepWarning()
		{
			MessageBeep(MB_ICONWARNING);
		}

		void PrintDebug(const char* format, ...)
		{
#if _DEBUG
			va_list arglist;
			va_start(arglist, format);
			char line[4096];
			SafeVFormat(line, sizeof(line), format, arglist);
			OutputDebugStringA(line);
#endif
		}

		void CopyStringToClipboard(cstr text)
		{
			size_t len = strlen(text);

			HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
			memcpy(GlobalLock(hMem), text, len + 1);
			GlobalUnlock(hMem);
			OpenClipboard(0);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hMem);
			CloseClipboard();
		}

		void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack)
		{
			StackStringBuilder sb(buffer, capacity);

			if (ex.ErrorCode() != 0)
			{
				char sysMessage[256];
				cstr sep;
				DWORD code = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ex.ErrorCode(), 0, sysMessage, 256, nullptr);
				if (code == 0)
				{
					sep = "";
					sysMessage[0] = 0;
				}
				else
				{
					sep = " - ";
				}

				sb.AppendFormat(" %s%sError code: %d (0x%8.8X)\n", sysMessage, sep, ex.ErrorCode(), ex.ErrorCode());
			}

			sb << ex.Message() << "\n";

			auto stackFrames = ex.StackFrames();
			if (appendStack && stackFrames)
			{
				sb << "Stack Frames\n";
				struct ANON : Debugging::IStackFrameFormatter
				{
					StringBuilder* sb;

					void Format(const Debugging::StackFrame& sf) override
					{
						auto& s = *sb;
						s.AppendFormat("#%-2u %-32.32s ", sf.depth, sf.functionName);
						if (*sf.sourceFile)
						{
							s.AppendFormat("Line #%4u of %-64.64s ", sf.lineNumber, sf.sourceFile);
						}
						else
						{
							s.AppendFormat("%-79.79s", "");
						}
						s.AppendFormat("%-64.64s ", sf.moduleName);
						s.AppendFormat("%4.4u:%016.16llX",sf.address.segment, sf.address.offset);
						s << "\n";
					}
				} formatter;
				formatter.sb = &sb;

				stackFrames->FormatEachStackFrame(formatter);
			}
		}


		void CopyExceptionToClipboard(IException& ex)
		{
			std::vector<char> buffer;
			buffer.resize(128_kilobytes);
			BuildExceptionString(buffer.data(), buffer.size(), ex, true);
			CopyStringToClipboard(buffer.data());
		}

		void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text)
		{
			if (text.length > 1024_megabytes)
			{
				Throw(0, "Rococo::OS::SaveAsciiTextFile(%ls): Sanity check. String was > 1 gigabyte in length", filename);
			}

			HANDLE hFile = INVALID_HANDLE_VALUE;

			switch (target)
			{
				case TargetDirectory_UserDocuments:
				{
					PWSTR path;
					HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
					if (FAILED(hr) || path == nullptr)
					{
						Throw(hr, "Failed to identify user documents folder. Win32 issue?");
					}

					WCHAR* fullpath = (WCHAR*)alloca(sizeof(wchar_t) * (wcslen(path) + 1 + text.length));
					wnsprintfW(fullpath, MAX_PATH, L"%s\\%s", path, filename);
					CoTaskMemFree(path);

					hFile = CreateFileW(fullpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

					if (hFile == INVALID_HANDLE_VALUE)
					{
						Throw(GetLastError(), "Cannot create file %ls", fullpath);
					}
				}
				break;
			case TargetDirectory_Root:
				hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

				if (hFile == INVALID_HANDLE_VALUE)
				{
					Throw(GetLastError(), "Cannot create file %ls in root directory", filename);
				}
				break;
			default:
				Throw(0, "Rococo::OS::SaveAsciiTextFile(... %ls): Unrecognized target directory", filename);
				break;
			}

			DWORD bytesWritten;
			bool status = WriteFile(hFile, text.buffer, (DWORD)text.length, &bytesWritten, NULL);
			int err = GetLastError();
			CloseHandle(hFile);

			if (!status)
			{
				Throw(err, "Rococo::OS::SaveAsciiTextFile(%ls) : failed to write text to file", filename);
			}
		}
	}

	namespace IO
	{
		void GetUserPath(wchar_t* fullpath, size_t capacity, cstr shortname)
		{
			wchar_t* userDocPath;
			SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &userDocPath);

			size_t nChars = wcslen(userDocPath) + 2 + strlen(shortname);

			if (nChars > capacity) Throw(0, "Rococo::IO::GetUserPath -> Insufficient capacity in result buffer");

			_snwprintf_s(fullpath, capacity, capacity, L"%ls\\%hs", userDocPath, shortname);

			CoTaskMemFree(userDocPath);
		}

		void DeleteUserFile(cstr filename)
		{
			wchar_t* userDocPath;
			SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &userDocPath);

			size_t nChars = wcslen(userDocPath) + 2 + strlen(filename);
			size_t sizeOfBuffer = sizeof(wchar_t) * nChars;
			wchar_t* fullPath = (wchar_t*)alloca(sizeOfBuffer);
			_snwprintf_s(fullPath, nChars, nChars, L"%ls\\%hs", userDocPath, filename);
			CoTaskMemFree(userDocPath);

			BOOL success = DeleteFileW(fullPath);
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			if (success) {}
		}

		void SaveUserFile(cstr filename, cstr s)
		{
			wchar_t* userDocPath;
			SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &userDocPath);

			size_t nChars = wcslen(userDocPath) + 2 + strlen(filename);
			size_t sizeOfBuffer = sizeof(wchar_t) * nChars;
			wchar_t* fullPath = (wchar_t*)alloca(sizeOfBuffer);
			_snwprintf_s(fullPath, nChars, nChars, L"%ls\\%hs", userDocPath, filename);
			CoTaskMemFree(userDocPath);

			HANDLE hFile = CreateFileW(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD writeLength;
				WriteFile(hFile, s, (DWORD)(sizeof(char) * rlen(s)), &writeLength, nullptr);
				CloseHandle(hFile);
			}
		}

		bool IsDirectory(const wchar_t* filename)
		{
			DWORD flags = GetFileAttributesW(filename);
			return (flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		template<class T> struct ComObject
		{
			T* instance;

			ComObject() : instance(nullptr) {}
			ComObject(T* _instance) : instance(_instance) {}
			~ComObject() { if (instance) instance->Release(); }

			T* operator -> () { return instance; }
			T** operator& () { return &instance; }

			operator T* () { return instance; }
		};

		bool ChooseDirectory(char* name, size_t capacity)
		{
			class DialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
			{
			public:
				// IUnknown methods
				IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
				{
#pragma warning( push )
#pragma warning( disable : 4838)
					static const QITAB qit[] =
					{
					   QITABENT(DialogEventHandler, IFileDialogEvents),
					   QITABENT(DialogEventHandler, IFileDialogControlEvents),
					   { nullptr, 0 }
					};
					return QISearch(this, qit, riid, ppv);
#pragma warning( pop )
				}

				IFACEMETHODIMP_(ULONG) AddRef()
				{
					return InterlockedIncrement(&_cRef);
				}

				IFACEMETHODIMP_(ULONG) Release()
				{
					long cRef = InterlockedDecrement(&_cRef);
					if (!cRef)
						delete this;
					return cRef;
				}

				// IFileDialogEvents methods
				IFACEMETHODIMP OnFileOk(IFileDialog *) { return S_OK; };
				IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; };
				IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) { return S_OK; };
				IFACEMETHODIMP OnHelp(IFileDialog *) { return S_OK; };
				IFACEMETHODIMP OnSelectionChange(IFileDialog *) { return S_OK; };
				IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; };
				IFACEMETHODIMP OnTypeChange(IFileDialog *pfd) { return S_OK; };
				IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; };

				// IFileDialogControlEvents methods
				IFACEMETHODIMP OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; };
				IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize *, DWORD) { return S_OK; };
				IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL) { return S_OK; };
				IFACEMETHODIMP OnControlActivating(IFileDialogCustomize *, DWORD) { return S_OK; };

				static HRESULT CreateInstance(REFIID riid, void **ppv)
				{
					*ppv = NULL;
					DialogEventHandler *pDialogEventHandler = new (std::nothrow) DialogEventHandler();
					HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
					if (SUCCEEDED(hr))
					{
						hr = pDialogEventHandler->QueryInterface(riid, ppv);
						pDialogEventHandler->Release();
					}
					return hr;
				}

				DialogEventHandler() : _cRef(1) { };
			private:
				~DialogEventHandler() { };
				long _cRef;
			};

			ComObject<IFileDialog> pfd;
			HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
			if (FAILED(hr))
			{
				Throw(hr, "CoCreateInstance(CLSID_FileOpenDialog failed");
			}

			ComObject<IFileDialogEvents> pfde;
			hr = DialogEventHandler::CreateInstance(IID_PPV_ARGS(&pfde));
			if (FAILED(hr))
			{
				Throw(hr, "CDialogEventHandler_CreateInstance failed");
			}

			DWORD dwCookie;
			hr = pfd->Advise(pfde, &dwCookie);
			if (FAILED(hr))
			{
				Throw(hr, "pfd->Advise failed");
			}

			// Set the options on the dialog.
			DWORD dwFlags;

			// Before setting, always get the options first in order 
			// not to override existing options.
			hr = pfd->GetOptions(&dwFlags);
			if (FAILED(hr))
			{
				Throw(hr, "pfd->GetOptions failed");
			}

			// In this case, get shell items only for file system items.
			hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
			if (FAILED(hr))
			{
				Throw(hr, "pfd->SetOptions failed");
			}

			// Set the file types to display only. 
			// Notice that this is a 1-based array.
			/*
			hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
			if (FAILED(hr))
			{
			   Throw(hr, "pfd->SetFileTypes failed");
			}
			*/

			/*
			// Set the selected file type index to Word Docs for this example.
			hr = pfd->SetFileTypeIndex(INDEX_WORDDOC);
			if (FAILED(hr))
			{
			   Throw(hr, "pfd->SetFileTypeIndex failed");
			}
			*/

			/*
			// Set the default extension to be ".doc" file.
			hr = pfd->SetDefaultExtension("doc;docx");
			if (FAILED(hr))
			{
			   Throw(hr, "pfd->SetDefaultExtension failed");
			}
			*/

			// Show the dialog
			hr = pfd->Show(NULL);
			if (FAILED(hr))
			{
				if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
				{
					return false;
				}
				Throw(hr, "pfd->Show failed");
			}

			// Obtain the result once the user clicks 
			// the 'Open' button.
			// The result is an IShellItem object.
			ComObject<IShellItem> psiResult;
			hr = pfd->GetResult(&psiResult);
			if (FAILED(hr))
			{
				Throw(hr, "pfd->GetResult");
			}

			wchar_t* _name = nullptr;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &_name);
			if (FAILED(hr))
			{
				Throw(hr, "pfd->GetResult");
			}

			SafeFormat(name, capacity, "%ls", _name);

			CoTaskMemFree(_name);

			// Unhook the event handler.
			pfd->Unadvise(dwCookie);

			return true;
		}

		void EndDirectoryWithSlash(char* pathname, size_t capacity)
		{
			cstr finalChar = GetFinalNull(pathname);

			if (pathname == nullptr || pathname == finalChar)
			{
				Throw(0, "Invalid pathname in call to EndDirectoryWithSlash");
			}

			bool isSlashed = finalChar[-1] == L'\\' || finalChar[-1] == L'/';
			if (!isSlashed)
			{
				if (finalChar >= (pathname + capacity - 1))
				{
					Throw(0, "Insufficient room in directory buffer to trail with slash");
				}

				char* mutablePath = const_cast<char*>(finalChar);
				mutablePath[0] = L'/';
				mutablePath[1] = 0;
			}
		}

		void EndDirectoryWithSlash(wchar_t* pathname, size_t capacity)
		{
			const wchar_t* finalChar = GetFinalNull(pathname);

			if (pathname == nullptr || pathname == finalChar)
			{
				Throw(0, "Invalid pathname in call to EndDirectoryWithSlash");
			}

			bool isSlashed = finalChar[-1] == L'\\' || finalChar[-1] == L'/';
			if (!isSlashed)
			{
				if (finalChar >= (pathname + capacity - 1))
				{
					Throw(0, "Insufficient room in directory buffer to trail with slash");
				}

				wchar_t* mutablePath = const_cast<wchar_t*>(finalChar);
				mutablePath[0] = L'/';
				mutablePath[1] = 0;
			}
		}

		bool IsDirectory(const WIN32_FIND_DATAW& fd)
		{
			return (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}

		void* RouteSearchResult(const WIN32_FIND_DATAW& findResult, const wchar_t* root, const wchar_t* containerRelRoot, void* containerContext, IEventCallback<FileItemData>& onFile)
		{
			WideFilePath fileName;
			Format(fileName, L"%s%s%s", root, containerRelRoot, findResult.cFileName);

			FileItemData item;
			item.fullPath = fileName;
			item.itemRelContainer = findResult.cFileName;
			item.containerRelRoot = containerRelRoot;
			item.containerContext = containerContext;
			item.outContext = nullptr;
			item.isDirectory = IsDirectory(findResult);
			onFile.OnEvent(item);
			return item.outContext;
		}

		void SearchSubdirectoryAndRecurse(const wchar_t* root, const wchar_t* containerDirectory, const wchar_t* subdirectory, void* subContext, IEventCallback<FileItemData>& onFile);

		class SearchObject
		{
		private:
			HANDLE hSearch = INVALID_HANDLE_VALUE;
			WIN32_FIND_DATAW rootFindData;
			wchar_t fullSearchFilter[_MAX_PATH];
			wchar_t rootDirectory[_MAX_PATH];

		public:
			SearchObject(const wchar_t* filter)
			{
				if (filter == nullptr || filter[0] == 0)
				{
					Throw(0, "%s: <filter> was blank.", __FUNCTION__);
				}

				auto finalChar = GetFinalNull(filter)[-1];
				bool isSlashed = finalChar == L'\\' || finalChar == L'/';

				DWORD status = GetFileAttributesW(filter);

				if (status != INVALID_FILE_ATTRIBUTES && ((status & FILE_ATTRIBUTE_DIRECTORY) != 0))
				{
					SafeFormat(fullSearchFilter, L"%s%s*.*", filter, isSlashed ? L"" : L"\\");
					SafeFormat(rootDirectory, L"%s%s", filter, isSlashed ? L"" : L"\\");
				}
				else // Assume we have <dir>/*.ext or something similar
				{
					SafeFormat(fullSearchFilter, L"%s", filter);
					SafeFormat(rootDirectory, L"%s", filter);
					OS::MakeContainerDirectory(rootDirectory);
				}

				hSearch = FindFirstFileW(fullSearchFilter, &rootFindData);

				if (hSearch == INVALID_HANDLE_VALUE)
				{
					HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
					if (hr != ERROR_FILE_NOT_FOUND)
					{
						Throw(hr, "%s: %ls\n", __FUNCTION__, fullSearchFilter);
					}
					return;
				}
			}

			void RouteSearchResults(const wchar_t* root, IEventCallback<FileItemData>& onFile, void* containerContext, bool recurse)
			{
				if (root == nullptr)
				{
					root = rootDirectory;
				}
				do
				{
					auto* containerRelRoot = rootDirectory + wcslen(root);

					if (IsDirectory(rootFindData))
					{
						if (*rootFindData.cFileName != '.')
						{
							void* subContext = RouteSearchResult(rootFindData, root, containerRelRoot, containerContext, onFile);

							if (recurse)
							{
								SearchSubdirectoryAndRecurse(root, containerRelRoot, rootFindData.cFileName, subContext, onFile);
							}
						}
					}
					else
					{
						RouteSearchResult(rootFindData, root, containerRelRoot, containerContext, onFile);
					}
				} while (FindNextFileW(hSearch, &rootFindData));
			}

			~SearchObject()
			{
				if (hSearch != INVALID_HANDLE_VALUE)
				{
					FindClose(hSearch);
				}
			}
		};

		void SearchSubdirectoryAndRecurse(const wchar_t* root, const wchar_t* containerDirectory, const wchar_t* subdirectory, void* subContext, IEventCallback<FileItemData>& onFile)
		{
			struct ANON : IEventCallback<IO::FileItemData>
			{
				IEventCallback<FileItemData>* onFile;
				const wchar_t* containerDirectory;

				void OnEvent(IO::FileItemData& i) override
				{
					IO::FileItemData item;
					item.containerContext = i.containerContext;
					item.fullPath = i.fullPath;
					item.outContext = nullptr;
					item.isDirectory = i.isDirectory;
					item.containerRelRoot = L"!";
					item.itemRelContainer = i.itemRelContainer;
					onFile->OnEvent(item);
					i.outContext = item.outContext;
				}
			} cb;

			WideFilePath fullDir;
			Format(fullDir, L"%s%s%s", root, containerDirectory, subdirectory);

			cb.onFile = &onFile;
			cb.containerDirectory = fullDir;

			SearchObject searchSub(fullDir);
			searchSub.RouteSearchResults(root, cb, subContext, true);
		}

		void ForEachFileInDirectory(const wchar_t* filter, IEventCallback<FileItemData>& onFile, bool recurse, void* containerContext)
		{
			SearchObject searchObj(filter);
			searchObj.RouteSearchResults(nullptr, onFile, containerContext, recurse);
		}
	} // IO

	namespace Windows
	{
		void AddColumns(int col, int width, const char* text, HWND hReportView)
		{
			LV_COLUMNA c = { 0 };
			c.cx = width;
			c.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_MINWIDTH;
			c.cxMin = width / 2;

			char buf[16];
			_snprintf_s(buf, _TRUNCATE, "%s", text);

			c.pszText = buf;

			SendMessage(hReportView, LVM_INSERTCOLUMNA, col, (LPARAM)&c);
		}

		void SetStackViewColumns(HWND hStackView, const int columnWidths[5])
		{
			AddColumns(0, columnWidths[0], "#", hStackView);
			AddColumns(1, columnWidths[1], "Function", hStackView);
			AddColumns(2, columnWidths[2], "Source", hStackView);
			AddColumns(3, columnWidths[3], "Module", hStackView);
			AddColumns(4, columnWidths[4], "Address", hStackView);
		}

		void PopulateStackView(HWND hStackView, Rococo::IException& ex)
		{
			HANDLE hProcess = GetCurrentProcess();

			struct StackFormatter : public Rococo::Debugging::IStackFrameFormatter
			{
				HWND hStackView;

				void Format(const Rococo::Debugging::StackFrame& sf) override
				{
					LVITEMA item = { 0 };
					item.mask = LVIF_TEXT;
					char text[16];
					_snprintf_s(text, _TRUNCATE, "%d", sf.depth);
					item.pszText = text;
					item.iItem = sf.depth;
					int index = (int) SendMessage(hStackView, LVM_INSERTITEMA, 0, (LPARAM)&item);

					LVITEMA address = { 0 };
					address.iItem = index;
					address.iSubItem = 4;
					address.mask = LVIF_TEXT;
					char addresstext[24];
					_snprintf_s(addresstext, _TRUNCATE, "%04.4X:%016.16llX", sf.address.segment, sf.address.offset);
					address.pszText = addresstext;
					SendMessage(hStackView, LVM_SETITEMA, 0, (LPARAM)&address);

					LVITEMA mname = { 0 };
					mname.iItem = index;
					mname.iSubItem = 3;
					mname.mask = LVIF_TEXT;
					mname.pszText = (char*)sf.moduleName;

					const char* module = sf.moduleName;
					for (const char* p = module + strlen(module); p > sf.moduleName; --p)
					{
						if (*p == '\\')
						{
							mname.pszText = (char*)p + 1;
							break;
						}
					}

					SendMessage(hStackView, LVM_SETITEMA, 0, (LPARAM)&mname);

					LVITEMA fname = { 0 };
					fname.iItem = index;
					fname.iSubItem = 1;
					fname.mask = LVIF_TEXT;
					fname.pszText = (char*)sf.functionName;
					SendMessage(hStackView, LVM_SETITEMA, 0, (LPARAM)&fname);

					LVITEMA lineCol = { 0 };
					lineCol.iItem = index;
					lineCol.iSubItem = 2;
					lineCol.mask = LVIF_TEXT;

					if (*sf.sourceFile)
					{
						char src[256];
						_snprintf_s(src, _TRUNCATE, "%s #%d", sf.sourceFile, sf.lineNumber);
						lineCol.pszText = src;
						SendMessage(hStackView, LVM_SETITEMA, 0, (LPARAM)&lineCol);
					}
				}
			} f;
			f.hStackView = hStackView;

			auto* enumerator = ex.StackFrames();
			if (enumerator)
			{
				enumerator->FormatEachStackFrame(f);
			}
		}
	}

	namespace Debugging
	{
		void FormatStackFrames(IStackFrameFormatter& formatter)
		{
			CONTEXT context;
			context.ContextFlags = CONTEXT_FULL;
			RtlCaptureContext(&context);

			HANDLE hProcess = GetCurrentProcess();

			SymInitialize(hProcess, NULL, TRUE);
			SymSetOptions(SYMOPT_LOAD_LINES);

			STACKFRAME64 frame = { 0 };
			frame.AddrPC.Offset = context.Rip;
			frame.AddrPC.Mode = AddrModeFlat;
			frame.AddrFrame.Offset = context.Rbp;
			frame.AddrFrame.Mode = AddrModeFlat;
			frame.AddrStack.Offset = context.Rsp;
			frame.AddrStack.Mode = AddrModeFlat;

			int index = 0;

			int depth = 0;

			while (StackWalk(
				IMAGE_FILE_MACHINE_AMD64,
				hProcess,
				GetCurrentThread(),
				&frame,
				&context,
				nullptr,
				SymFunctionTableAccess64,
				SymGetModuleBase,
				nullptr
			))
			{
				if (index++ < 1)
				{
					continue; // Ignore first two stack frames -> they are our stack analysis functions!
				}
				
				Rococo::Debugging::StackFrame sf;
				sf.depth = depth++;
				sf.address.segment = frame.AddrPC.Segment;
				sf.address.offset = frame.AddrPC.Offset;

				char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
				PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
				symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
				symbol->MaxNameLength = 254;

				sf.moduleName[0] = 0;
				sf.functionName[0] = 0;
				sf.sourceFile[0] = 0;
				sf.lineNumber = 0;

				DWORD64 moduleBase = SymGetModuleBase64(hProcess, frame.AddrPC.Offset);

				if (moduleBase)
				{
					GetModuleFileNameA((HINSTANCE)moduleBase, sf.moduleName, sizeof(sf.moduleName));
				}

				if (SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, NULL, symbol))
				{
					strncpy_s(sf.functionName, symbol->Name, _TRUNCATE);
				}

				DWORD  offset = 0;
				IMAGEHLP_LINE line;
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

				if (SymGetLineFromAddr(hProcess, frame.AddrPC.Offset, &offset, &line))
				{
					strncpy_s(sf.sourceFile, line.FileName, _TRUNCATE);
					sf.lineNumber = line.LineNumber;
				}

				formatter.Format(sf);
			}
		}
	}
}

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
	void LoadAsciiTextFile(IEventCallback<cstr>& onLoad, const wchar_t* filename)
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

			if (len.QuadPart >= (int64) 2048_megabytes)
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

		onLoad.OnEvent(asciiData.data());
	}

	size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename)
	{
		if (capacity >= 2048_megabytes)
		{
			Throw(GetLastError(), "LoadAsciiTextFile: capacity must be less than 2GB.\n%ls", filename);
		}

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

		if (len.QuadPart >= (int64) capacity)
		{
			Throw(GetLastError(), "LoadAsciiTextFile: File too large - length must be less than %llu bytes.\n%ls", filename, capacity);
		}

		try
		{
			f.ReadBuffer((DWORD)len.QuadPart, data);
			data[len.QuadPart] = 0;
		}
		catch (IException& ex)
		{
			Throw(ex.ErrorCode(), "LoadAsciiTextFile: %s.\n%ls", ex.Message(), filename);
		}

		return len.QuadPart;
	}

	class AutoHKEY
	{
		HKEY hKey = nullptr;
	public:
		AutoHKEY()
		{

		}

		AutoHKEY(HKEY _hKey) : hKey(_hKey)
		{

		}

		~AutoHKEY()
		{
			if (!hKey)
			{
				RegCloseKey(hKey);
			}
		}

		HKEY* operator& () { return &hKey; }

		operator HKEY() { return hKey;  }
	};

	void GetConfigVariable(char* textBuffer, size_t lenBytes, cstr defaultValue, ConfigSection section, ConfigRootName root, cstr organization)
	{
		SecureFormat(textBuffer, lenBytes, "%s", defaultValue);

		AutoHKEY hKeySoftware;
		auto status = RegOpenKeyA(HKEY_CURRENT_USER, "Software", &hKeySoftware);
		if (status != ERROR_SUCCESS)
		{
			return;
		}

		if (!organization) organization = "Rococo - 19th Century Software";

		AutoHKEY hKeyOrganization;
		status = RegOpenKeyA(hKeySoftware, organization, &hKeyOrganization);
		
		if (status != ERROR_SUCCESS)
		{
			status = RegCreateKeyA(hKeySoftware, organization, &hKeyOrganization);
		}

		if (status != ERROR_SUCCESS)
		{
			return;
		}

		AutoHKEY hKeyRoot;
		status = RegOpenKeyA(hKeyOrganization, root.rootName, &hKeyRoot);

		if (status != ERROR_SUCCESS)
		{
			status = RegCreateKeyA(hKeyOrganization, root.rootName, &hKeyRoot);
		}

		if (status != ERROR_SUCCESS)
		{
			return;
		}

		DWORD dwType = REG_SZ;
		DWORD sizeofBuffer = (DWORD)lenBytes;
		status = RegGetValueA(hKeyRoot, NULL, section.sectionName, REG_SZ, &dwType, textBuffer, &sizeofBuffer);
		if (status != ERROR_SUCCESS)
		{
			DWORD len = (DWORD) (strlen(defaultValue) + 1);
			RegSetValueA(hKeyRoot, section.sectionName, REG_SZ, defaultValue, len);
		}
	}
} // Rococo::OS
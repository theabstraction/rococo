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

#pragma comment(lib, "Shlwapi.lib")

#include <stdlib.h>

#include <rococo.debugging.h>

#include <dbghelp.h>

#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Winmm.lib")

#include <ctime>

namespace Rococo
{
	void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

	namespace IO
	{
		void UseBufferlessStdout()
		{
			setvbuf(stdout, nullptr, _IONBF, 0);
		}
	}

	bool FileModifiedArgs::Matches(cstr resource) const
	{
		const wchar_t* a = this->resourceName;
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

namespace Rococo
{
	namespace OS
	{
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

				static void WakeUp(ULONG_PTR data)
				{

				}

				cstr GetErrorMessage() const override
				{
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
						return ex.ErrorCode();
					}
				}
			};

			TIMECAPS caps;
			UINT cbtc = sizeof(caps);
			timeGetDevCaps(&caps,cbtc);

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

	void GetContentDirectory(const wchar_t* contentIndicatorName, wchar_t path[Rococo::IO::MAX_PATHLEN], IOS& os)
	{
		wchar_t binDirectory[Rococo::IO::MAX_PATHLEN];
		os.GetBinDirectoryAbsolute(binDirectory, os.MaxPath());

		SecureFormat(path, Rococo::IO::MAX_PATHLEN, L"%s", binDirectory);

		if (wcsstr(contentIndicatorName, L"\\") != nullptr)
		{
			// The indicator is part of a path
			if (os.IsFileExistant(contentIndicatorName))
			{
				SecureFormat(path, Rococo::IO::MAX_PATHLEN, L"%s", contentIndicatorName);
				OS::MakeContainerDirectory(path);
				return;
			}
		}

		size_t len = wcslen(path);

		while (len > 0)
		{
			wchar_t indicator[Rococo::IO::MAX_PATHLEN];
			SecureFormat(indicator, FilePath::CAPACITY, L"%s%s", path, contentIndicatorName);
			if (os.IsFileExistant(indicator))
			{
				SecureFormat(indicator, FilePath::CAPACITY, L"%s%s", path, L"content\\");
				SecureFormat(path, FilePath::CAPACITY, L"%s", indicator);
				return;
			}

			OS::MakeContainerDirectory(path);

			size_t newLen = wcslen(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(0, "Could not find %S below the executable folder '%S'", contentIndicatorName, binDirectory);
	}

	class Installation : public IInstallationSupervisor
	{
		IOS& os;
		wchar_t contentDirectory[Rococo::IO::MAX_PATHLEN];
		int32 len;
		std::unordered_map<std::string, std::string> macroToSubdir;
	public:
		Installation(const wchar_t* contentIndicatorName, IOS& _os) : os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);
			len = (int32)wcslen(contentDirectory);
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
				wchar_t sysPathA[IO::MAX_PATHLEN];
				ConvertPingPathToSysPath(a, sysPathA, IO::MAX_PATHLEN);

				wchar_t sysPathB[IO::MAX_PATHLEN];
				ConvertPingPathToSysPath(b, sysPathB, IO::MAX_PATHLEN);

				return wcscmp(sysPathA, sysPathB) == 0;
			}
			catch (IException&)
			{
				return false;
			}
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

		void ConvertPingPathToSysPath(cstr pingPath, wchar_t* sysPath, size_t sysPathCapacity) const override
		{
			if (pingPath == nullptr || *pingPath == 0)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Ping path was blank");
			}

			auto macroDir = "";
			const char* subdir = nullptr;

			if (*pingPath == '!')
			{
				subdir = pingPath + 1;

				SecureFormat(sysPath, sysPathCapacity, L"%s%S", contentDirectory, subdir);
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

				SecureFormat(sysPath, sysPathCapacity, L"%s%S%S", contentDirectory, macroDir + 1, subdir);
			}
			else
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown prefix. Expecting ! or #", pingPath);
			}

			if (strstr(pingPath, "..") != nullptr)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
			}

			OS::ToSysPath(sysPath);
		}

		void ConvertSysPathToMacroPath(const wchar_t* sysPath, char* pingPath, size_t pingPathCapacity, cstr macro) const override
		{
			char fullPingPath[IO::MAX_PATHLEN];
			ConvertSysPathToPingPath(sysPath, fullPingPath, IO::MAX_PATHLEN);

			auto i = macroToSubdir.find(macro);
			if (i == macroToSubdir.end())
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...) No macro defined: %s", macro);
			}

			cstr expansion = i->second.c_str();
			if (strstr(fullPingPath, expansion) == nullptr)
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...\"%s\", \"%s\") Path not prefixed by macro: %s", sysPath, macro, expansion);
			}

			SecureFormat(pingPath, pingPathCapacity, "%s/%s", macro, fullPingPath + i->second.size());
		}

		void ConvertSysPathToPingPath(const wchar_t* sysPath, char* pingPath, size_t pingPathCapacity) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			int sysPathLen = (int) wcslen(sysPath);

			auto p = wcsstr(sysPath, contentDirectory);

			int32 netLength = sysPathLen - len;
			if (netLength < 0 || p != sysPath)
			{
				Throw(0, "ConvertSysPathToPingPath: path did not begin with the content folder %S", contentDirectory);
			}

			if (netLength >= (int32)pingPathCapacity)
			{
				Throw(0, "ConvertSysPathToPingPath: Insufficient space in ping path buffer");
			}

			if (wcsstr(sysPath, L"..") != nullptr)
			{
				Throw(0, "ConvertSysPathToPingPath: Illegal sequence in ping path: '..'");
			}

			SecureFormat(pingPath, pingPathCapacity, "!%S", sysPath + len);

			OS::ToUnixPath(pingPath);
		}

		void LoadResource(cstr pingPath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (pingPath == nullptr || rlen(pingPath) < 2) Throw(E_INVALIDARG, "Win32OS::LoadResource failed: <resourcePath> was blank");

			wchar_t absPath[Rococo::IO::MAX_PATHLEN];
			if (pingPath[0] == '!' || pingPath[0] == '#')
			{
				ConvertPingPathToSysPath(pingPath, absPath, Rococo::IO::MAX_PATHLEN);
			}
			else
			{
				SafeFormat(absPath, Rococo::IO::MAX_PATHLEN, L"%S", pingPath);
			}

			os.LoadAbsolute(absPath, buffer, maxFileLength);
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

			char pingRoot[IO::MAX_PATHLEN - 1];
			int len = SecureFormat(pingRoot, sizeof(pingRoot), "%s", pingFolder);
			OS::ToUnixPath(pingRoot);
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

	class CriticalSection : public ILock
	{
		CRITICAL_SECTION sysCS;

	public:
		CriticalSection()
		{
			InitializeCriticalSection(&sysCS);
		}

		~CriticalSection()
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
		uintptr_t hThread;
		unsigned threadId;
		bool isRunning;

		CriticalSection threadLock;
		std::vector<std::wstring> modifiedFiles;

		IEventCallback<SysUnstableArgs>* onUnstable;
	public:
		Win32OS() :
			hMonitorDirectory(INVALID_HANDLE_VALUE),
			hThread(0),
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

		void UTF8ToUnicode(const char* s, wchar_t* unicode, size_t cbUtf8count, size_t unicodeCapacity) override
		{
			if (0 == MultiByteToWideChar(CP_UTF8, 0, s, (int)cbUtf8count, unicode, (int)unicodeCapacity))
			{
				Throw(GetLastError(), "Could not convert UTF8 to rchar: %S", s);
			}
		}

		void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb) override
		{
			while (!modifiedFiles.empty())
			{
				threadLock.Lock();
				std::wstring f = modifiedFiles.back();
				modifiedFiles.pop_back();
				threadLock.Unlock();

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
						wchar_t nullTerminatedFilename[_MAX_PATH] = { 0 };
						for (DWORD i = 0; i < nChars; ++i)
						{
							nullTerminatedFilename[i] = info.FileName[i];
						}
						nullTerminatedFilename[nChars] = 0;
						OnModified(nullTerminatedFilename);
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
					if (!ReadDirectoryChangesW(hMonitorDirectory, raw, sizeof(raw), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, &ovl, OnScan))
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
			if (isRunning || hMonitorDirectory != INVALID_HANDLE_VALUE)
			{
				Throw(0, "A directory is already being monitored");
			}

			DWORD shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			hMonitorDirectory = CreateFileW(absPath, GENERIC_READ, shareFlags, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			if (hMonitorDirectory == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Failed to create monitor on directory %S", absPath);
			}

			isRunning = true;
			hThread = _beginthreadex(nullptr, 65536, thread_monitor_directory, this, 0, &threadId);
		}

		bool IsFileExistant(const wchar_t* absPath) const override
		{
			return INVALID_FILE_ATTRIBUTES != GetFileAttributesW(absPath);
		}

		void ConvertUnixPathToSysPath(const wchar_t* unixPath, wchar_t* sysPath, size_t bufferCapacity) const override
		{
			if (unixPath == nullptr) Throw(E_INVALIDARG, "Blank path in call to os.ConvertUnixPathToSysPath");
			if (wcslen(unixPath) >= bufferCapacity)
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
					sysPath[i] = '\\';
				}
				else
				{
					sysPath[i] = c;
				}
			}

			sysPath[i] = 0;
		}

		void LoadAbsolute(const wchar_t* absPath, IExpandingBuffer& buffer, int64 maxFileLength) const override
		{
			FileHandle hFile = CreateFileW(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), "Win32OS::LoadResource failed: Error opening file %S", absPath);

			LARGE_INTEGER len;
			GetFileSizeEx(hFile, &len);

			if (maxFileLength > 0 && len.QuadPart > maxFileLength)
			{
				Throw(0, "Win32OS::LoadResource failed: File <%s> was too large at over %ld bytes", absPath, maxFileLength);
			}

			buffer.Resize(len.QuadPart);

			int64 bytesLeft = len.QuadPart;
			ptrdiff_t offset = 0;

			uint8* data = (uint8*)buffer.GetData();

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

		void GetBinDirectoryAbsolute(wchar_t* directory, size_t capacityChars) const override
		{
			SecureFormat(directory, capacityChars, L"%s", binDirectory);
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
				Throw(0, "Rococo::OS::SaveAsciiTextFile(%S): Sanity check. String was > 1 gigabyte in length", filename);
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
				}
				break;
			case TargetDirectory_Root:
				hFile = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				break;
			default:
				Throw(0, "Rococo::OS::SaveAsciiTextFile(... %S): Unrecognized target directory", filename);
				break;
			}

			if (hFile == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Cannot create file %S in user directory", filename);
			}
			DWORD bytesWritten;
			bool status = WriteFile(hFile, text.buffer, (DWORD)text.length, &bytesWritten, NULL);
			int err = GetLastError();
			CloseHandle(hFile);

			if (!status)
			{
				Throw(err, "Rococo::OS::SaveAsciiTextFile(%S) : failed to write text to file", filename);
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

			_snwprintf_s(fullpath, capacity, capacity, L"%s\\%S", userDocPath, shortname);

			CoTaskMemFree(userDocPath);
		}

		void DeleteUserFile(cstr filename)
		{
			wchar_t* userDocPath;
			SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &userDocPath);

			size_t nChars = wcslen(userDocPath) + 2 + strlen(filename);
			size_t sizeOfBuffer = sizeof(wchar_t) * nChars;
			wchar_t* fullPath = (wchar_t*)alloca(sizeOfBuffer);
			_snwprintf_s(fullPath, nChars, nChars, L"%s\\%S", userDocPath, filename);
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
			_snwprintf_s(fullPath, nChars, nChars, L"%s\\%S", userDocPath, filename);
			CoTaskMemFree(userDocPath);

			HANDLE hFile = CreateFileW(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD writeLength;
				WriteFile(hFile, s, (DWORD)(sizeof(char) * rlen(s)), &writeLength, nullptr);
				CloseHandle(hFile);
			}
		}

		char GetFileSeparator()
		{
			return L'\\';
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

			CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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

			SafeFormat(name, capacity, "%S", _name);

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

		void ForEachFileInDirectory(const wchar_t* filter, IEventCallback<const wchar_t*>& onFile)
		{
			struct AutoSearchHandle
			{
				HANDLE hSearch = INVALID_HANDLE_VALUE;

				~AutoSearchHandle()
				{
					if (hSearch != INVALID_HANDLE_VALUE)
					{
						FindClose(hSearch);
					}
				}

				operator HANDLE () {
					return hSearch;
				}
			};

			if (filter == nullptr || filter[0] == 0)
			{
				Throw(0, "%s: <filter> was blank.", __FUNCTION__);
			}

			wchar_t fullSearchFilter[_MAX_PATH];
			wchar_t containerDirectory[_MAX_PATH];

			auto finalChar = GetFinalNull(filter)[-1];
			bool isSlashed = finalChar == L'\\' || finalChar == L'/';

			DWORD status = GetFileAttributesW(filter);
			
			if (status != INVALID_FILE_ATTRIBUTES && ((status & FILE_ATTRIBUTE_DIRECTORY) != 0))
			{
				SafeFormat(fullSearchFilter, _MAX_PATH, L"%s%s*.*", filter, isSlashed ? L"" : L"\\");
				SafeFormat(containerDirectory, _MAX_PATH, L"%s%s", filter, isSlashed ? L"" : L"\\");
			}
			else // Assume we have <dir>/*.ext or something similar
			{
				SafeFormat(fullSearchFilter, _MAX_PATH, L"%s", filter);
				SafeFormat(containerDirectory, _MAX_PATH, L"%s", filter);
				OS::MakeContainerDirectory(containerDirectory);
			}

			WIN32_FIND_DATAW findData;

			AutoSearchHandle hSearch;
			hSearch.hSearch = FindFirstFileW(fullSearchFilter, &findData);

			if (hSearch.hSearch == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() != ERROR_FILE_NOT_FOUND)
				{
					Throw(GetLastError(), "%s: %s\n", __FUNCTION__, fullSearchFilter);
				}
				return;
			}

			do
			{
				if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					// We found a file
					onFile.OnEvent(findData.cFileName);
				}
				else
				{
					// We found a directory
					if (*findData.cFileName != '.')
					{
						struct : IEventCallback<const wchar_t*>
						{
							wchar_t stem[IO::MAX_PATHLEN];

							IEventCallback<const wchar_t*>* caller;
							virtual void OnEvent(const wchar_t* name)
							{
								wchar_t subsubpath[IO::MAX_PATHLEN];
								SecureFormat(subsubpath, IO::MAX_PATHLEN, L"%s%s", stem, name);
								caller->OnEvent(subsubpath);
							}
						} subpathResult;

						wchar_t subpath[IO::MAX_PATHLEN];
						SecureFormat(subpath, IO::MAX_PATHLEN, L"%s%s\\", containerDirectory, findData.cFileName);
						subpathResult.caller = &onFile;

						SecureFormat(subpathResult.stem, IO::MAX_PATHLEN, L"%s\\", findData.cFileName);

						ForEachFileInDirectory(subpath, subpathResult);
					}
				}
			} while (FindNextFileW(hSearch, &findData));
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
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

#pragma comment(lib, "Shlwapi.lib")

#include <stdlib.h>

namespace Rococo
{
   namespace IO
   {
      void UseBufferlessStdout()
      {
         setvbuf(stdout, nullptr, _IONBF, 0);
      }
   }

   bool FileModifiedArgs::Matches(cstr resource) const
   {
      cstr a = this->resourceName;
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

   void FileModifiedArgs::GetPingPath(rchar* path, size_t capacity) const
   {
      SafeFormat(path, capacity, "!%s", resourceName);

      for (rchar* p = path; *p != 0; p++)
      {
         if (*p == '\\') *p = '/';
      }
   }
}

#include <rococo.window.h>
#include <Commdlg.h>

namespace Rococo
{
	namespace OS
	{
		bool IsFileExistant(const char* filename)
		{
			DWORD flags = GetFileAttributesA(filename);
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

		void ToSysPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '/') *s = '\\';
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

		void Format_C_Error(int errorCode, rchar* buffer, size_t capacity)
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

	void MakeContainerDirectory(rchar* filename)
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

	struct FilePath
	{
		enum { CAPACITY = 260 };
		rchar data[CAPACITY];
		operator rchar*() { return data; }
		operator cstr() const { return data; }
	};

	void GetContentDirectory(cstr contentIndicatorName, FilePath& path, IOS& os)
	{
		FilePath binDirectory;
		os.GetBinDirectoryAbsolute(binDirectory, os.MaxPath());

		StackStringBuilder sb(path.data, _MAX_PATH);
		sb << binDirectory.data;

		if (strstr(contentIndicatorName, "\\") != nullptr)
		{
			// The indicator is part of a path
			if (os.IsFileExistant(contentIndicatorName))
			{
				sb.Clear();
				sb << contentIndicatorName;
				MakeContainerDirectory(path);
				return;
			}
		}

		size_t len = rlen(path);

		while (len > 0)
		{
			FilePath indicator;
			SecureFormat(indicator.data, FilePath::CAPACITY, "%s%s", path.data, contentIndicatorName);
			if (os.IsFileExistant(indicator))
			{
				StackStringBuilder sb(path.data, _MAX_PATH, StringBuilder::BUILD_EXISTING);
				sb << "content\\";
				return;
			}

			MakeContainerDirectory(path);

			size_t newLen = rlen(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(0, "Could not find %s below the executable folder '%s'", contentIndicatorName, binDirectory);
	}

	class Installation : public IInstallationSupervisor
	{
		IOS& os;
		FilePath contentDirectory;
		int32 len;
		std::unordered_map<std::string, std::string> macroToSubdir;
	public:
		Installation(cstr contentIndicatorName, IOS& _os) : os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);
			len = (int32)strlen(contentDirectory);
		}

		void Free()  override
		{
			delete this;
		}

		IOS& OS()  override
		{
			return os;
		}

		const fstring Content() const  override
		{
			return fstring{ contentDirectory.data, len };
		}

		bool DoPingsMatch(cstr a, cstr b) const override
		{
			try
			{
				char sysPathA[IO::MAX_PATHLEN];
				ConvertPingPathToSysPath(a, sysPathA, IO::MAX_PATHLEN);

				char sysPathB[IO::MAX_PATHLEN];
				ConvertPingPathToSysPath(b, sysPathB, IO::MAX_PATHLEN);

				return Eq(sysPathA, sysPathB);
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

		void ConvertPingPathToSysPath(cstr pingPath, char* sysPath, size_t sysPathCapacity) const override
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
			}
			else
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown prefix. Expecting ! or #", pingPath);
			}

			if (strstr(pingPath, "..") != nullptr)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
			}

			int fulllen = SecureFormat(sysPath, sysPathCapacity, "%s%s%s", contentDirectory.data, macroDir + 1, subdir);
			OS::ToSysPath(sysPath);
		}

		void ConvertSysPathToMacroPath(cstr sysPath, char* pingPath, size_t pingPathCapacity, cstr macro) const override
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

		void ConvertSysPathToPingPath(cstr sysPath, char* pingPath, size_t pingPathCapacity) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			const fstring s = to_fstring(sysPath);
			auto p = strstr(sysPath, contentDirectory.data);

			int32 netLength = s.length - len;
			if (netLength < 0 || p != sysPath)
			{
				Throw(0, "ConvertSysPathToPingPath: path did not begin with the content folder %s", contentDirectory.data);
			}

			if (netLength >= (int32)pingPathCapacity)
			{
				Throw(0, "ConvertSysPathToPingPath: Insufficient space in ping path buffer");
			}

			if (strstr(sysPath, "..") != nullptr)
			{
				Throw(0, "ConvertSysPathToPingPath: Illegal sequence in ping path: '..'");
			}

			SecureFormat(pingPath, pingPathCapacity, "!%s", sysPath + len);

			OS::ToUnixPath(pingPath);
		}

		void LoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(E_INVALIDARG, "Win32OS::LoadResource failed: <resourcePath> was blank");

			FilePath absPath;
			if (resourcePath[0] == '!' || resourcePath[0] == '#')
			{
				ConvertPingPathToSysPath(resourcePath, absPath.data, absPath.CAPACITY);
			}
			else
			{
				SafeFormat(absPath.data, FilePath::CAPACITY, "%s", resourcePath);
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
		FilePath binDirectory;
		HANDLE hMonitorDirectory;
		uintptr_t hThread;
		unsigned threadId;
		bool isRunning;

		CriticalSection threadLock;
		std::vector<std::string> modifiedFiles;

		IEventCallback<SysUnstableArgs>* onUnstable;
	public:
		Win32OS() :
			hMonitorDirectory(INVALID_HANDLE_VALUE),
			hThread(0),
			isRunning(false),
			onUnstable(nullptr)
		{
			auto hAppInstance = GetModuleHandle(nullptr);
			GetModuleFileNameA(hAppInstance, binDirectory, _MAX_PATH);
			MakeContainerDirectory(binDirectory);
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

		virtual void Free()
		{
			delete this;
		}

		virtual void UTF8ToUnicode(const char* s, wchar_t* unicode, size_t cbUtf8count, size_t unicodeCapacity)
		{
			if (0 == MultiByteToWideChar(CP_UTF8, 0, s, (int)cbUtf8count, unicode, (int)unicodeCapacity))
			{
				Throw(GetLastError(), "Could not convert UTF8 to rchar: %S", s);
			}
		}

		virtual void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb)
		{
			while (!modifiedFiles.empty())
			{
				threadLock.Lock();
				std::string f = modifiedFiles.back();
				modifiedFiles.pop_back();
				threadLock.Unlock();

				cb.OnEvent(FileModifiedArgs{ f.c_str() });
			}
		}

		virtual void FireUnstable()
		{
			SysUnstableArgs unused;
			if (onUnstable) onUnstable->OnEvent(unused);
		}

		virtual void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb)
		{
			onUnstable = cb;
		}

		void OnModified(cstr filename)
		{
			Sleep(500);
			Sync sync(threadLock);

			enum { MAX_MODIFIED_QUEUE_LENGTH = 20 };
			if (modifiedFiles.size() < MAX_MODIFIED_QUEUE_LENGTH)
			{
				bool isInList = false;

				for (auto& v : modifiedFiles)
				{
					if (strcmp(v.c_str(), filename) == 0)
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
						rchar nullTerminatedFilename[_MAX_PATH] = { 0 };
						for (DWORD i = 0; i < nChars; ++i)
						{
							nullTerminatedFilename[i] = (char)info.FileName[i];
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

		virtual void Monitor(cstr absPath)
		{
			if (isRunning || hMonitorDirectory != INVALID_HANDLE_VALUE)
			{
				Throw(0, "A directory is already being monitored");
			}

			DWORD shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			hMonitorDirectory = CreateFileA(absPath, GENERIC_READ, shareFlags, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			if (hMonitorDirectory == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Failed to create monitor on directory %s", absPath);
			}

			isRunning = true;
			hThread = _beginthreadex(nullptr, 65536, thread_monitor_directory, this, 0, &threadId);
		}

		virtual bool IsFileExistant(cstr absPath) const
		{
			return INVALID_FILE_ATTRIBUTES != GetFileAttributesA(absPath);
		}

		virtual void ConvertUnixPathToSysPath(cstr unixPath, rchar* sysPath, size_t bufferCapacity) const
		{
			if (unixPath == nullptr) Throw(E_INVALIDARG, "Blank path in call to os.ConvertUnixPathToSysPath");
			if (rlen(unixPath) >= bufferCapacity)
			{
				Throw(E_INVALIDARG, "Path too long in call to os.ConvertUnixPathToSysPath");
			}

			size_t len = rlen(unixPath);

			size_t i = 0;
			for (; i < len; ++i)
			{
				rchar c = unixPath[i];

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

		virtual void LoadAbsolute(cstr absPath, IExpandingBuffer& buffer, int64 maxFileLength) const
		{
			FileHandle hFile = CreateFileA(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), "Win32OS::LoadResource failed: Error opening file %s", absPath);

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

		virtual void GetBinDirectoryAbsolute(rchar* directory, size_t capacityChars) const
		{
			SecureFormat(directory, capacityChars, "%s", binDirectory.data);
		}

		virtual size_t MaxPath() const
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

	IInstallationSupervisor* CreateInstallation(cstr contentIndicatorName, IOS& os)
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
	   bool isRunning = true;

	   bool IsRunning()
	   {
		   return isRunning;
	   }

	   void BeepWarning()
	   {
		   MessageBeep(MB_ICONWARNING);
	   }

	   void ShutdownApp()
	   {
		   isRunning = false;
		   PostQuitMessage(0);
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
   }

   namespace IO
   {
      void GetUserPath(rchar* fullpath, size_t capacity, cstr shortname)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);
         SafeFormat(fullpath, capacity, "%S\\%s", path, shortname);
      }

      void DeleteUserFile(cstr filename)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

         rchar fullpath[_MAX_PATH];
         SafeFormat(fullpath, sizeof(fullpath), "%S\\%s", path, filename);

         DeleteFileA(fullpath);
      }

      void SaveUserFile(cstr filename, cstr s)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

         rchar fullpath[_MAX_PATH];
         SafeFormat(fullpath, sizeof(fullpath), "%S\\%s", path, filename);

         HANDLE hFile = CreateFileA(fullpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
         if (hFile != INVALID_HANDLE_VALUE)
         {
            DWORD writeLength;
            WriteFile(hFile, s, (DWORD) (sizeof(rchar) * rlen(s)), &writeLength, nullptr);
            CloseHandle(hFile);
         }
      }

      rchar GetFileSeparator()
      {
         return L'\\';
      }

      template<class T> struct ComObject
      {
         T* instance;

         ComObject() : instance(nullptr) {}
         ComObject(T* _instance) : instance(_instance) {}
         ~ComObject() { if (instance) instance->Release(); }

         T* operator -> () { return instance;  }
         T** operator& () { return &instance; }

         operator T* () { return instance; }
      };

      bool ChooseDirectory(rchar* name, size_t capacity)
      {
         class DialogEventHandler : public IFileDialogEvents,  public IFileDialogControlEvents
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

      void EndDirectoryWithSlash(rchar* pathname, size_t capacity)
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

            rchar* mutablePath = const_cast<rchar*>(finalChar);
            mutablePath[0] = L'/';
            mutablePath[1] = 0;
         }
      }

      void ForEachFileInDirectory(cstr directory, IEventCallback<cstr>& onFile)
      { 
         struct Anon
         {
            HANDLE hSearch;

            ~Anon()
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

         Anon hSearch;

         rchar fullpath[MAX_PATH];
         bool isSlashed = GetFinalNull(directory)[-1] == L'\\' || GetFinalNull(directory)[-1] == L'/';
         SafeFormat(fullpath, sizeof(fullpath), "%s%s*.*", directory, isSlashed ? "" : "\\");

         WIN32_FIND_DATAA findData;
         hSearch.hSearch = FindFirstFileA(fullpath, &findData);
 
         if (hSearch.hSearch == INVALID_HANDLE_VALUE)
         {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
               Throw(GetLastError(), "Could not browse directory: %s", fullpath);
            }
            return;
         }

         do
         {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
               onFile.OnEvent(findData.cFileName);
            }
			else
			{
				if (*findData.cFileName != '.')
				{
					struct : IEventCallback<cstr>
					{
						char stem[IO::MAX_PATHLEN];

						IEventCallback<cstr>* caller;
						virtual void OnEvent(cstr name)
						{
							char subsubpath[IO::MAX_PATHLEN];
							SecureFormat(subsubpath, IO::MAX_PATHLEN, "%s%s", stem, name);
							caller->OnEvent(subsubpath);
						}
					} subpathResult; 

					char subpath[IO::MAX_PATHLEN];
					SecureFormat(subpath, IO::MAX_PATHLEN, "%s%s%s\\", directory, isSlashed ? "" : "\\" , findData.cFileName);
					subpathResult.caller = &onFile;

					SecureFormat(subpathResult.stem, IO::MAX_PATHLEN, "%s\\", findData.cFileName);

					ForEachFileInDirectory(subpath, subpathResult);
				}
			}
		 } while (FindNextFileA(hSearch, &findData));
      }
   } // IO
}
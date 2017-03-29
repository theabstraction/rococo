#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <windows.h>
#include <Psapi.h>

#include <rococo.api.h>
#include <rococo.strings.h>

#include <wchar.h>
#include <stdlib.h>

#include <rococo.io.h>
#include <process.h>

#include <vector>

#include <shlobj.h>
#include <comip.h>
#include <Shlwapi.h>

#include <rococo.strings.h>

#pragma comment(lib, "Shlwapi.lib")

namespace Rococo
{
   int64 CpuTicks()
   {
      LARGE_INTEGER ticks;
      QueryPerformanceCounter(&ticks);
      return ticks.QuadPart;
   }

   int64 CpuHz()
   {
      LARGE_INTEGER hz;
      QueryPerformanceFrequency(&hz);
      return hz.QuadPart;
   }

   bool FileModifiedArgs::Matches(const wchar_t* resource)
   {
      const wchar_t* a = this->resourceName;
      const wchar_t* b = resource;
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

   void FileModifiedArgs::GetPingPath(wchar_t* path, size_t capacity)
   {
      SafeFormat(path, capacity, _TRUNCATE, L"!%s", resourceName);

      for (wchar_t* p = path; *p != 0; p++)
      {
         if (*p == '\\') *p = '/';
      }
   }
}

namespace Rococo
{
	void TripDebugger()
	{
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
	}

	bool IsDebugging()
	{
		return IsDebuggerPresent() ? true : false;
	}

	ticks CpuClock()
	{
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		return i.QuadPart;
	}

   MemoryUsage ProcessMemory()
   {
      PROCESS_MEMORY_COUNTERS counters = { 0 };
      counters.cb = sizeof(counters);
      GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
      return{ counters.PagefileUsage, counters.PeakPagefileUsage };
   }

	void Throw(int32 errorCode, const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);

		struct : public IException
		{
			wchar_t msg[256];
			int32 errorCode;

			virtual const wchar_t* Message() const
			{
				return msg;
			}

			virtual int32 ErrorCode() const
			{
				return errorCode;
			}
		} ex;

		SafeVFormat(ex.msg, _TRUNCATE, format, args);

		ex.errorCode = errorCode;

		TripDebugger();

		throw ex;
	}

	bool DoesModifiedFilenameMatchResourceName(const wchar_t* modifiedFilename, const wchar_t* resourceName)
	{
		const wchar_t* p = modifiedFilename;
		const wchar_t* q = resourceName + 1;

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

	void MakeContainerDirectory(wchar_t* filename)
	{
		int len = (int)wcslen(filename);

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
		wchar_t data[_MAX_PATH];
		operator wchar_t*() { return data; }
	};

	void GetContentDirectory(const wchar_t* contentIndicatorName, FilePath& path, IOS& os)
	{
		FilePath binDirectory;
		os.GetBinDirectoryAbsolute(binDirectory, os.MaxPath());

		path = binDirectory;

      if (wcsstr(contentIndicatorName, L"\\") != nullptr)
      {
         // The indicator is part of a path
         if (os.IsFileExistant(contentIndicatorName))
         { 
            SecureFormat(path.data, L"%s", contentIndicatorName);
            MakeContainerDirectory(path);
            return;
         }
      }

		size_t len = wcslen(path);

		while (len > 0)
		{
			FilePath indicator;
			SecureFormat(indicator.data, L"%s%s", path.data, contentIndicatorName);
			if (os.IsFileExistant(indicator))
			{
				SecureCat(path.data, L"content\\");
				return;
			}

			MakeContainerDirectory(path);

			size_t newLen = wcslen(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(0, L"Could not find %s below the executable folder '%s'", contentIndicatorName, binDirectory);
	}

	class Installation: public IInstallationSupervisor
	{
		IOS& os;
		FilePath contentDirectory;

	public:
		Installation(const wchar_t* contentIndicatorName, IOS& _os): os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual IOS& OS()
		{
			return os;
		}

		virtual const wchar_t* Content() const
		{
			return contentDirectory.data;
		}

		virtual void LoadResource(const wchar_t* resourcePath, IExpandingBuffer& buffer, int64 maxFileLength)
		{
			if (resourcePath == nullptr || wcslen(resourcePath) < 2) Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: <resourcePath> was blank");
			if (resourcePath[0] != '!') Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: <%s> did not begin with ping '!' character", resourcePath);

			if (wcslen(resourcePath) + wcslen(contentDirectory) >= _MAX_PATH)
			{
				Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: %s%s - filename was too long", contentDirectory, resourcePath + 1);
			}

         if (wcsstr(resourcePath, L"..") != nullptr)
         {
            Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: %s - parent directory sequence '..' is forbidden", resourcePath);
         }

			FilePath sysPath;
			os.ConvertUnixPathToSysPath(resourcePath + 1, sysPath, _MAX_PATH);
			
			FilePath absPath;
			SecureFormat(absPath.data, L"%s%s", contentDirectory.data, sysPath.data);

			os.LoadAbsolute(absPath, buffer, maxFileLength);
		}
	};

	class CriticalSection: public ILock
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
		std::vector<std::wstring> modifiedFiles;

		IEventCallback<SysUnstableArgs>* onUnstable;
	public:
		Win32OS():
			hMonitorDirectory(INVALID_HANDLE_VALUE), 
			hThread(0),
			isRunning(false),
			onUnstable(nullptr)
		{
         auto hAppInstance = GetModuleHandle(nullptr);
			GetModuleFileNameW(hAppInstance, binDirectory, _MAX_PATH);
			MakeContainerDirectory(binDirectory);
		}

		~Win32OS()
		{
			if (isRunning)
			{
				isRunning = false;
				struct wake { static VOID CALLBACK me(ULONG_PTR param) {}};
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
			if (0 == MultiByteToWideChar(CP_UTF8, 0, s, (int) cbUtf8count, unicode, (int)unicodeCapacity))
			{
				Throw(GetLastError(), L"Could not convert UTF8 to wchar: %S", s);
			}
		}

		virtual void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb)
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

		virtual void FireUnstable()
		{
			SysUnstableArgs unused;
			if (onUnstable) onUnstable->OnEvent(unused);
		}

		virtual void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb)
		{
			onUnstable = cb;
		}

		void OnModified(const wchar_t* filename)
		{
			Sync sync(threadLock);

			enum { MAX_MODIFIED_QUEUE_LENGTH = 20 };
			if (modifiedFiles.size() < MAX_MODIFIED_QUEUE_LENGTH)
			{
				bool isInList = false;

				for (auto& v : modifiedFiles)
				{
					if (wcscmp(v.c_str(), filename) == 0)
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
			const FILE_NOTIFY_INFORMATION* i = &info;

			while(true)
			{
				if (i->Action == FILE_ACTION_MODIFIED)
				{
					wchar_t nullTerminatedFilename[_MAX_PATH];
					size_t nChars = info.FileNameLength >> 1;
					SafeCopy(nullTerminatedFilename, info.FileName, nChars);
					nullTerminatedFilename[nChars] = 0;
					OnModified(nullTerminatedFilename);
				}

				if (!i->NextEntryOffset) break;

				i = (const FILE_NOTIFY_INFORMATION*) (((char*) i) +  i->NextEntryOffset);
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
					auto& info = *(FILE_NOTIFY_INFORMATION*) raw;
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

		virtual void Monitor(const wchar_t* absPath)
		{
			if (isRunning || hMonitorDirectory != INVALID_HANDLE_VALUE)
			{
				Throw(0, L"A directory is already being monitored");
			}

			DWORD shareFlags = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
			hMonitorDirectory = CreateFile(absPath, GENERIC_READ, shareFlags, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
			if (hMonitorDirectory == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), L"Failed to create monitor on directory %s", absPath);
			}

			isRunning = true;
			hThread = _beginthreadex(nullptr, 65536, thread_monitor_directory, this, 0, &threadId);
		}

		virtual bool IsFileExistant(const wchar_t* absPath) const
		{
			return INVALID_FILE_ATTRIBUTES != GetFileAttributes(absPath);
		}

		virtual void ConvertUnixPathToSysPath(const wchar_t* unixPath, wchar_t* sysPath, size_t bufferCapacity) const
		{
			if (unixPath == nullptr) Throw(E_INVALIDARG, L"Blank path in call to os.ConvertUnixPathToSysPath");
			if (wcslen(unixPath) >= bufferCapacity)
			{
				Throw(E_INVALIDARG, L"Path too long in call to os.ConvertUnixPathToSysPath");
			}

			size_t len = wcslen(unixPath);

			size_t i = 0;
			for (; i < len; ++i)
			{
				wchar_t c = unixPath[i];

				if (c == '\\') Throw(E_INVALIDARG, L"Illegal backslash '\\' in unixPath in call to os.ConvertUnixPathToSysPath");

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

		virtual void LoadAbsolute(const wchar_t* absPath, IExpandingBuffer& buffer, int64 maxFileLength) const
		{
			FileHandle hFile = CreateFile(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), L"Win32OS::LoadResource failed: Error opening file %s", absPath);

			LARGE_INTEGER len;
			GetFileSizeEx(hFile, &len);

			if (maxFileLength > 0 && len.QuadPart > maxFileLength)
			{
				Throw(0, L"Win32OS::LoadResource failed: File <%s> was too large at over %ld bytes", absPath, maxFileLength);
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
					Throw(HRESULT_FROM_WIN32(GetLastError()), L"Error reading file <%s>", absPath);
				}

				if (bytesRead != chunk)
				{
					Throw(0, L"Win32OS::LoadResource: Error reading file <%s>. Failed to read chunk", absPath);
				}

				offset += (ptrdiff_t)chunk;
				bytesLeft -= (int64)chunk;
			}
		}

		virtual void GetBinDirectoryAbsolute(wchar_t* directory, size_t capacityChars) const
		{
			SecureFormat(directory, capacityChars, L"%s", binDirectory.data);
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
      void ShutdownApp()
      {
         PostQuitMessage(0);
      }
   }

   namespace IO
   {
      void GetUserPath(wchar_t* fullpath, size_t capacity, const wchar_t* shortname)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);
         SafeFormat(fullpath, capacity, _TRUNCATE, L"%s\\%s", path, shortname);
      }

      void DeleteUserFile(const wchar_t* filename)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

         wchar_t fullpath[_MAX_PATH];
         SafeFormat(fullpath, _TRUNCATE, L"%s\\%s", path, filename);

         DeleteFile(fullpath);
      }

      void SaveUserFile(const wchar_t* filename, const wchar_t* s)
      {
         wchar_t* path;
         SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

         wchar_t fullpath[_MAX_PATH];
         SafeFormat(fullpath, _TRUNCATE, L"%s\\%s", path, filename);

         HANDLE hFile = CreateFile(fullpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
         if (hFile != INVALID_HANDLE_VALUE)
         {
            DWORD writeLength;
            WriteFile(hFile, s, (DWORD) (sizeof(wchar_t) * wcslen(s)), &writeLength, nullptr);
            CloseHandle(hFile);
         }
      }

      wchar_t GetFileSeparator()
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

      bool ChooseDirectory(wchar_t* name, size_t capacity)
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
            Throw(hr, L"CoCreateInstance(CLSID_FileOpenDialog failed");
         }

         ComObject<IFileDialogEvents> pfde;
         hr = DialogEventHandler::CreateInstance(IID_PPV_ARGS(&pfde));
         if (FAILED(hr))
         {
            Throw(hr, L"CDialogEventHandler_CreateInstance failed");
         }

         DWORD dwCookie;
         hr = pfd->Advise(pfde, &dwCookie);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->Advise failed");
         }

         // Set the options on the dialog.
         DWORD dwFlags;

         // Before setting, always get the options first in order 
         // not to override existing options.
         hr = pfd->GetOptions(&dwFlags);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->GetOptions failed");
         }

         // In this case, get shell items only for file system items.
         hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->SetOptions failed");
         }

         // Set the file types to display only. 
         // Notice that this is a 1-based array.
         /*
         hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->SetFileTypes failed");
         }
         */

         /*
         // Set the selected file type index to Word Docs for this example.
         hr = pfd->SetFileTypeIndex(INDEX_WORDDOC);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->SetFileTypeIndex failed");
         }
         */

         /*
         // Set the default extension to be ".doc" file.
         hr = pfd->SetDefaultExtension(L"doc;docx");
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->SetDefaultExtension failed");
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
            Throw(hr, L"pfd->Show failed");
         }

         // Obtain the result once the user clicks 
         // the 'Open' button.
         // The result is an IShellItem object.
         ComObject<IShellItem> psiResult;
         hr = pfd->GetResult(&psiResult);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->GetResult");
         }

         LPWSTR _name = nullptr;
         hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &_name);
         if (FAILED(hr))
         {
            Throw(hr, L"pfd->GetResult");
         }
         
         SafeCopy(name, capacity, _name, _TRUNCATE);

         CoTaskMemFree(_name);

         // Unhook the event handler.
         pfd->Unadvise(dwCookie);

         return true;
      }

      void EndDirectoryWithSlash(wchar_t* pathname, size_t capacity)
      {
         const wchar_t* finalChar = GetFinalNull(pathname);

         if (pathname == nullptr || pathname == finalChar)
         {
            Throw(0, L"Invalid pathname in call to EndDirectoryWithSlash");
         }

         bool isSlashed = finalChar[-1] == L'\\' || finalChar[-1] == L'/';
         if (!isSlashed)
         {
            if (finalChar >= (pathname + capacity - 1))
            {
               Throw(0, L"Insufficient room in directory buffer to trail with slash");
            }

            wchar_t* mutablePath = const_cast<wchar_t*>(finalChar);
            mutablePath[0] = L'/';
            mutablePath[1] = 0;
         }
      }

      void ForEachFileInDirectory(const wchar_t* directory, IEventCallback<const wchar_t*>& onFile)
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

         wchar_t fullpath[MAX_PATH];
         bool isSlashed = GetFinalNull(directory)[-1] == L'\\' || GetFinalNull(directory)[-1] == L'/';
         SafeFormat(fullpath, _TRUNCATE, L"%s%s*.*", directory, isSlashed ? L"" : L"\\");

         WIN32_FIND_DATAW findData;
         hSearch.hSearch = FindFirstFileW(fullpath, &findData);
 
         if (hSearch.hSearch == INVALID_HANDLE_VALUE)
         {
            if (GetLastError() != ERROR_FILE_NOT_FOUND)
            {
               Throw(GetLastError(), L"Could not browse directory: %s", fullpath);
            }
            return;
         }

         if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
         {
            onFile.OnEvent(findData.cFileName);
         }

         while (FindNextFile(hSearch, &findData))
         {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
               onFile.OnEvent(findData.cFileName);
            }
         }
      }
   } // IO
}
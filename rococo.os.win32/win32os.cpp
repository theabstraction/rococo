#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <windows.h>

#include <rococo.types.h>

#include <wchar.h>
#include <stdlib.h>

#include <rococo.io.h>

#include <process.h>

#include <vector>

namespace Rococo
{
	void TripDebugger()
	{
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
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

			FilePath sysPath;
			os.ConvertUnixPathToSysPath(resourcePath + 1, sysPath, _MAX_PATH);
			
			FilePath absPath;
			SecureFormat(absPath.data, L"%s%s", contentDirectory.data, sysPath.data);

			os.LoadAbsolute(absPath, buffer, maxFileLength);
		}
	};

	struct ILock
	{
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
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

	class Sync
	{
		ILock& lock;
	public:
		Sync(ILock& _lock) : lock(_lock)
		{
			lock.Lock();
		}

		~Sync()
		{
			lock.Unlock();
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
		Win32OS(HINSTANCE hAppInstance):
			hMonitorDirectory(INVALID_HANDLE_VALUE), 
			hThread(0),
			isRunning(false),
			onUnstable(nullptr)
		{
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

		virtual void EnumerateModifiedFiles(ITextCallback& cb)
		{
			while (!modifiedFiles.empty())
			{
				threadLock.Lock();
				std::wstring f = modifiedFiles.back();
				modifiedFiles.pop_back();
				threadLock.Unlock();

				cb.OnItem(f.c_str());
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
					OnModified(info.FileName);
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
			if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), L"Win32OS::LoadResource failed: CreateFile failed for <%s>", absPath);

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
	IOSSupervisor* GetWin32OS(HINSTANCE hAppInstance)
	{
		return new Win32OS(hAppInstance);
	}

	IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& os)
	{
		return new Installation(contentIndicatorName, os);
	}
}
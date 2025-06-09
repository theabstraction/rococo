#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <Psapi.h>
#include <rococo.api.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdlib.h>
#include <rococo.io.h>
#include <process.h>
#include <string>
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
#include <lmcons.h>

#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Winmm.lib")

#include <ctime>
#include <shellapi.h>
#include <rococo.strings.h>
#include <rococo.os.h>

#include <rococo.task.queue.h>
#include <list>
#include <rococo.time.h>

#include <pathcch.h>

#include <rococo.hashtable.h>

#pragma comment(lib, "Pathcch.lib")

#include <allocators/rococo.allocators.inl>

using namespace Rococo::IO;
using namespace Rococo::Strings;
using namespace Rococo::Memory;

#include <allocators/rococo.allocators.modules.h>

USE_ROCOCO_UTILS_FOR_MODULE_ALLOCATORS
DeclareAllocator(TrackingAllocator, RococoUtils, g_allocator)
//AllocatorMonitor<RococoUtils> monitor;
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

namespace Rococo
{
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

		operator HKEY() { return hKey; }
	};

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

	constexpr fstring packageprefix = "Package["_fstring;

	namespace IO
	{
		ROCOCO_API void ToSysPath(wchar_t* path)
		{
			for (wchar_t* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
			}
		}

		ROCOCO_API void ToUnixPath(wchar_t* path)
		{
			for (wchar_t* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}

		ROCOCO_API void ToSysPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
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
			DWORD flags = GetFileAttributesA(filename);
			return flags != INVALID_FILE_ATTRIBUTES;
		}

		ROCOCO_API bool IsFileExistant(const wchar_t* filename)
		{
			DWORD flags = GetFileAttributesW(filename);
			return flags != INVALID_FILE_ATTRIBUTES;
		}

		ROCOCO_API bool StripLastSubpath(char* fullpath)
		{
			int32 len = (int32)strlen(fullpath);
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

		ROCOCO_API bool StripLastSubpath(wchar_t* fullpath)
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

		ROCOCO_API void SanitizePath(wchar_t* path)
		{
			for (auto* s = path; *s != 0; ++s)
			{
				if (*s == L'/') *s = L'\\';
			}
		}

		ROCOCO_API void SanitizePath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '/') *s = '\\';
			}
		}

		ROCOCO_API bool MakeContainerDirectory(wchar_t* filename)
		{
			int len = (int)wcslen(filename);

			for (int i = len - 2; i > 0; --i)
			{
				if (filename[i] == L'\\' || filename[i] == L'/')
				{
					filename[i + 1] = 0;
					return true;
				}
			}

			return false;
		}

		ROCOCO_API bool MakeContainerDirectory(char* filename)
		{
			int len = (int)rlen(filename);

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
			return '\\';
		}

		ROCOCO_API void CreateDirectoryFolder(const WideFilePath& path)
		{
			if (!CreateDirectoryW(path, NULL))
			{
				int err = GetLastError();
				if (err == ERROR_ALREADY_EXISTS)
				{
					return;
				}

				Throw(GetLastError(), "Cannot create directory %ls", path.buf);
			}
		}

		ROCOCO_API void CreateDirectoryFolder(const U8FilePath& path)
		{
			if (!CreateDirectoryA(path, NULL))
			{
				int err = GetLastError();
				if (err == ERROR_ALREADY_EXISTS)
				{
					return;
				}

				Throw(GetLastError(), "Cannot create directory %s", path.buf);
			}
		}

		ROCOCO_API IBinaryArchive* CreateNewBinaryFile(const wchar_t* sysPath)
		{
			struct Win32BinArchive : IBinaryArchive
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

					DWORD nTotal = (DWORD)nTotalBytes;
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

		ROCOCO_API IBinarySource* ReadBinarySource(const wchar_t* sysPath)
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

				~Win32BinFile()
				{
					CloseHandle(hFile);
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

		ROCOCO_API IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(const wchar_t* sysPath)
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
						CloseHandle(hFile);
						Throw(GetLastError(), "Error creating map of file: %ls\n", sysPath);
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
					return (const char*)pMem;
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

		void NormalizePath(WideFilePath& path)
		{
			WideFilePath out;
			HRESULT hr = PathCchCanonicalizeEx(OUT out.buf, WideFilePath::CAPACITY, path, PATHCCH_NONE);
			if FAILED(hr)
			{
				Throw(hr, "Failed to normalize path for %ls", path.buf);
			}

			path = out;
		}

		ROCOCO_API void GetExeName(U8FilePath& path)
		{
			U8FilePath fullPath;
			GetExePath(OUT fullPath);

			auto fp = Substring::ToSubstring(fullPath);
			cstr slash = ReverseFind(IO::GetFileSeparator(), fp);
			if (!slash)
			{
				path = fullPath;
			}
			else
			{
				Format(path, "%s", slash + 1);
			}
		}

		ROCOCO_API void GetExePath(U8FilePath& path)
		{
			::GetModuleFileNameA(NULL, path.buf, path.CAPACITY);
		}

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
					IO::MakeContainerDirectory(path.buf);
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

				IO::MakeContainerDirectory(path.buf);

				size_t newLen = wcslen(path);
				if (newLen >= len) break;
				len = newLen;
			}

			Throw(0, "Could not find %ls below the executable folder '%ls'", contentIndicatorName, binDirectory.buf);
		}

		ROCOCO_API bool IsKeyPressed(int vkeyCode)
		{
			SHORT value = GetAsyncKeyState(vkeyCode);
			return (value & 0x8000) != 0;
		}

		ROCOCO_API void PasteFromClipboard(char* asciiBuffer, size_t capacity)
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

		ROCOCO_API void UseBufferlessStdout()
		{
			setvbuf(stdout, nullptr, _IONBF, 0);
		}

		ROCOCO_API bool TryGetFileAttributes(const wchar_t* sysPath, FileAttributes& attr)
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

	ROCOCO_API bool FileModifiedArgs::Matches(cstr resource) const
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
	class CriticalSection: public Rococo::OS::ICriticalSection
	{
	private:
		Rococo::OS::CriticalSectionMemorySource src;
		CRITICAL_SECTION cs;

	public:
		CriticalSection(Rococo::OS::CriticalSectionMemorySource _src): src(_src)
		{
			InitializeCriticalSection(&cs);
		}

		virtual ~CriticalSection()
		{
			DeleteCriticalSection(&cs);
		}

		void Free() override
		{
			if (src == Rococo::OS::CriticalSectionMemorySource::OPERATOR_NEW)
			{
				delete this;
			}
			else
			{
				free(this);
			}
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

namespace Rococo::OS
{
	ROCOCO_API void PasteStringFromClipboard(Strings::IStringPopulator& populator)
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
					populator.Populate(pData);
					GlobalUnlock(hItem);
				}
			}

			CloseClipboard();
		}
	}

	ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow)
	{
		if (isVisible)
		{
			for (int i = 0; i < 3; ++i)
			{
				int index = ShowCursor(TRUE);
				if (index >= 0)
				{
					ClipCursor(nullptr);
					SetCapture(nullptr);
					return;
				}
			}
		}
		else
		{
			for (int i = 0; i < 3; ++i)
			{
				int index = ShowCursor(FALSE);
				if (index < 0)
				{
					POINT pos;
					GetCursorPos(&pos);

					RECT rect{ pos.x - 1, pos.y - 1, pos.x + 1, pos.y + 1 };

					ClipCursor(&rect);
					SetCapture(captureWindow);
					return;
				}
			}
		}
	}

	ROCOCO_API void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath)
	{
		ShellExecuteW(window, L"open", sysPath, nullptr, nullptr, SW_SHOW);
	}

	ROCOCO_API void SleepUntilAsync(uint32 timeoutMS)
	{
		SleepEx(timeoutMS, true);
	}

	static const wchar_t* notePadPP = L"C:\\Program Files\\Notepad++\\notepad++.exe";

	static std::vector<PROCESS_INFORMATION> processes;

	void ClearProcesses()
	{
		for (auto& p : processes)
		{
			WaitForSingleObject(p.hProcess, INFINITE);
			CloseHandle(p.hProcess);
		}
	}


	// Not thread safe
	bool SpawnChildProcessAsync(const wchar_t* executable, const wchar_t* commandLineArgs)
	{
		std::vector<wchar_t> commandLine;
		commandLine.resize(32786);

		SafeFormat(commandLine.data(), commandLine.size(), L"%s", commandLineArgs);

		STARTUPINFOW info = { 0 };
		info.cb = sizeof info;
		PROCESS_INFORMATION pInfo = { 0 };
		BOOL status = CreateProcessW(executable, commandLine.data(), NULL, NULL, FALSE, 0, NULL, NULL, &info, &pInfo);
		if (status)
		{
			processes.push_back(pInfo);

			if (processes.size() == 1)
			{
				atexit(ClearProcesses);
			}
		}

		return status;
	}

	// Thread safe
	void SpawnIndependentProcess(HWND hMsgSink, const wchar_t* executable, const wchar_t* commandLine)
	{
		auto result = (INT_PTR)ShellExecuteW(hMsgSink, L"open", executable, commandLine, NULL, SW_SHOW);
		if (result < 32)
		{
			Throw(GetLastError(), "Error spawning [%s: '%s']", __FUNCTION__, executable, commandLine);
		}
	}

	bool OpenNotepadPP(HWND hWndMessageSink, cstr documentFilePath, int lineNumber)
	{
		wchar_t commandLine[1024];
		SafeFormat(commandLine, L"-n%d -titleAdd=\" (via Sexy Studio)\" \"%hs\"", lineNumber, documentFilePath);

		try
		{
			SpawnIndependentProcess(hWndMessageSink, notePadPP, commandLine);
			return true;
		}
		catch (IException&)
		{
			return false;
		}
	}

	ROCOCO_API void ShellOpenDocument(Windows::IWindow& parent, cstr caption, cstr documentFilePath, int lineNumber)
	{
		WideFilePath wTarget;
		Assign(wTarget, documentFilePath);

		if (!IsFileExistant(wTarget))
		{
			char msg[320];
			SafeFormat(msg, "File not found: %s", documentFilePath);

			HWND hRoot = GetAncestor(parent, GA_ROOT);
			MessageBoxA(hRoot, msg, caption, MB_ICONINFORMATION);
			return;
		}

		if (lineNumber > 0)
		{
			if (IsFileExistant(notePadPP))
			{
				if (OpenNotepadPP(parent, documentFilePath, lineNumber))
				{
					return;
				}
			}
		}

		auto result = (INT_PTR) ShellExecuteA(NULL, "open", documentFilePath, NULL, NULL, SW_SHOW);
		if (result < 32)
		{
			Throw(GetLastError(), "%s: '%s'", __FUNCTION__, documentFilePath);
		}
	}

	ROCOCO_API void WakeUp(IThreadControl& thread)
	{
		struct ANON
		{
			static void WakeUp(void* context)
			{
				UNUSED(context);
			}
		};
		thread.QueueAPC(ANON::WakeUp, nullptr);
	}

	ROCOCO_API IdThread GetCurrentThreadIdentifier()
	{
		return (IdThread) GetCurrentThreadId();
	}

	ROCOCO_API ICriticalSection* CreateCriticalSection(CriticalSectionMemorySource src)
	{
		if (src == CriticalSectionMemorySource::OPERATOR_NEW)
		{
			return new CriticalSection(src);
		}
		else
		{
			void* buffer = malloc(sizeof CriticalSection);
			return new (buffer) CriticalSection(src);
		}
	}

	ROCOCO_API IThreadSupervisor* CreateRococoThread(IThreadJob* job, uint32 stacksize)
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
				UNUSED(data);
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

	struct ThreadErrorDesc
	{
		int errorCode;
		HString message;
		IdThread threadId;
	};

	bool library_initialized = false;
	CRITICAL_SECTION sync = { 0 };

	std::vector<ThreadErrorDesc> threadErrors;

	ROCOCO_API void InitRococoOS()
	{
		InitializeCriticalSection(&sync);
		library_initialized = true;
	}

	ROCOCO_API void TerminateRococoOS()
	{
		DeleteCriticalSection(&sync);
	}

	ROCOCO_API void AddThreadError(int errorCode, cstr format, ...)
	{
		va_list arglist;
		va_start(arglist, format);
		char line[4096];
		SafeVFormat(line, sizeof(line), format, arglist);
		ThreadErrorDesc desc;
		desc.errorCode = errorCode;
		desc.message = line;
		desc.threadId = OS::GetCurrentThreadIdentifier();

		if (library_initialized)
		{
			EnterCriticalSection(&sync);
			threadErrors.push_back(desc);
			LeaveCriticalSection(&sync);
		}
	}

	ROCOCO_API void ThrowOnThreadError()
	{
		if (!library_initialized)
		{
			Throw(0, "API implementor must call Rococo::OS::InitRococoOS to use %s", __FUNCTION__);
		}

		ThreadErrorDesc desc;
		EnterCriticalSection(&sync);

		if (!threadErrors.empty())
		{
			desc = threadErrors[0];
			threadErrors.clear();
		}

		LeaveCriticalSection(&sync);

		if (desc.message.length() > 0)
		{
			Throw(desc.errorCode, "%s", desc.message.c_str());
		}
	}

	ROCOCO_API cstr GetAsciiCommandLine()
	{
		auto line =  GetCommandLineA();
		return line;
	}

	ROCOCO_API void PollKeys(uint8 scanArray[256])
	{
		if (!GetKeyboardState(scanArray))
		{
			ZeroMemory(scanArray, 256);
		}
	}

	ROCOCO_API void SaveClipBoardText(cstr text, Windows::IWindow& window)
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

	ROCOCO_API void UILoop(uint32 milliseconds)
	{
		MSG msg;

		Time::ticks count = Time::TickCount();
		Time::ticks target = count + (Time::TickHz() * milliseconds / 1000);

		while (target < Time::TickCount())
		{
			MsgWaitForMultipleObjects(0, nullptr, FALSE, 25, QS_ALLINPUT);

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	ROCOCO_API void Format_C_Error(int errorCode, char* buffer, size_t capacity)
	{
		if (0 == FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, buffer, (DWORD)capacity, NULL))
		{
			SafeFormat(buffer, capacity, "Unknown error code (%d)", errorCode);
		}
	}

	ROCOCO_API void TripDebugger()
	{
		if (IsDebuggerPresent())
		{
			__debugbreak();
		}
	}

	ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode)
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

	ROCOCO_API bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window)
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

	ROCOCO_API bool IsDebugging()
	{
		return IsDebuggerPresent() ? true : false;
	}

	ROCOCO_API void* AllocBoundedMemory(size_t nBytes)
	{
		auto* pMem = malloc(nBytes);
		if (pMem == nullptr)
		{
			Throw(0, "Could not allocate %ull bytes of memory", nBytes);
		}
		return pMem;
	}

	ROCOCO_API void FreeBoundedMemory(void* pMemory)
	{
		free(pMemory);
	}
} // Rococo::OS

namespace Rococo
{
	ROCOCO_API MemoryUsage ProcessMemory()
	{
		PROCESS_MEMORY_COUNTERS counters = { 0 };
		counters.cb = sizeof(counters);
		GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters));
		return{ counters.PagefileUsage, counters.PeakPagefileUsage };
	}
}

namespace WIN32_ANON
{
	using namespace Rococo;

	struct FilePath
	{
		enum { CAPACITY = 260 };
		char data[CAPACITY];
		operator char*() { return data; }
		operator cstr() const { return data; }
	};

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

			if (!IO::IsDirectory(contentDirectory))
			{
				Throw(0, "No such content directory: %ls", contentDirectory);
			}
		}

		Installation(IOS& _os, const wchar_t* contentPath) : os(_os)
		{
			if (!IO::IsDirectory(contentPath))
			{
				Throw(0, "No such content directory: %ls", contentPath);
			}

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
			Rococo::IO::StripLastSubpath(sysPath.buf);
			size_t len = StringLength(sysPath);
			SecureFormat(sysPath.buf + len, sysPath.CAPACITY - len, L"packages/%hs/%hs", packName, dir);
			IO::ToSysPath(sysPath.buf);
		}

		void ConvertPingPathToSysPath(cstr pingPath, U8FilePath& sysPath) const override
		{
			WideFilePath wPath;
			ConvertPingPathToSysPath(pingPath, wPath);
			Assign(sysPath, wPath);
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
				Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): abspath not found and/or unknown prefix. Expecting ! or #", pingPath);
			}

			if (strstr(pingPath, "..") != nullptr)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
			}

			IO::ToSysPath(sysPath.buf);
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

			IO::ToUnixPath(pingPath.buf);
		}

		void ConvertSysPathToPingPath(const char* sysPath, U8FilePath& pingPath) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			WideFilePath wSysPath;
			Assign(OUT REF wSysPath, sysPath);

			ConvertSysPathToPingPath(wSysPath, OUT REF pingPath);
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

		void LoadResource(cstr pingPath, ILoadEventsCallback& cb) override
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

			os.LoadAbsolute(absPath, cb);
		}

		bool TryLoadResource(cstr pingPath, ILoadEventsCallback& cb, OUT ErrorCode& errorCode) override
		{
			if (pingPath == nullptr || rlen(pingPath) < 2)
			{
				errorCode = (ErrorCode)E_INVALIDARG;
				return false;
			}

			WideFilePath absPath;
			if (pingPath[0] == '!' || pingPath[0] == '#')
			{
				ConvertPingPathToSysPath(pingPath, absPath);
			}
			else
			{
				Assign(absPath, pingPath);
			}

			return os.TryLoadAbsolute(absPath, cb, OUT errorCode);
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
			IO::ToUnixPath(pingRoot.buf);
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

		std::unordered_map<std::wstring, Rococo::Time::ticks> lastModifiedList;

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
			IO::MakeContainerDirectory(binDirectory);
		}

		~Win32OS()
		{
			if (isRunning)
			{
				isRunning = false;
				struct wake 
				{ 
					static VOID CALLBACK me(ULONG_PTR param) 
					{
						UNUSED(param);
					} 
				};
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
			int64 timeoutInSeconds = 1;
			auto timeout = Rococo::Time::TickHz() * timeoutInSeconds;

			while (!modifiedFiles.empty())
			{
				threadLock.Lock();
				std::wstring f = modifiedFiles.back();
				modifiedFiles.pop_back();
				threadLock.Unlock();

				// N.B the Windows API for scanning file changes will typically send multiple events
				// for the same file change, so we ignore the superfluous notifications in the same period

				auto i = lastModifiedList.insert(std::pair<const std::wstring, Rococo::Time::ticks>(f, Rococo::Time::TickCount() ) );
				if (i.second == false)
				{
					// already in list

					auto insertionTime = i.first->second;
					if (Rococo::Time::TickCount() - insertionTime > timeout)
					{
						i.first->second = Rococo::Time::TickCount();
					}
					else
					{
						continue;
					}
				}

				FileModifiedArgs args{ f.c_str() };
				cb.OnEvent(args);
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
			OS::Sync sync(threadLock);

			enum { MAX_MODIFIED_QUEUE_LENGTH = 200 };
			if (modifiedFiles.size() < MAX_MODIFIED_QUEUE_LENGTH)
			{
				modifiedFiles.push_back(filename);
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
						WideFilePath nullTerminatedFilename;
						for (DWORD j = 0; j < nChars; ++j)
						{
							nullTerminatedFilename.buf[j] = info.FileName[j];
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
				std::vector<char> raw;
				OVERLAPPED ovl = { 0 };
				Win32OS* os = nullptr;
				DWORD bytesReturned = 0;
				HANDLE hMonitorDirectory = 0;
				int exitCode = 0;

				void OnScan()
				{
					auto& info = *(FILE_NOTIFY_INFORMATION*)raw.data();
					os->OnScan(info);
					exitCode = QueueScan();
				}

				static void WINAPI OnScan(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
				{
					UNUSED(dwErrorCode);
					UNUSED(dwNumberOfBytesTransfered);
					Context* This = (Context*)lpOverlapped->hEvent;
					This->OnScan();
				}

				int QueueScan()
				{
					DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
					if (!ReadDirectoryChangesW(hMonitorDirectory, raw.data(), (DWORD) raw.size(), TRUE, dwNotifyFilter, &bytesReturned, &ovl, OnScan))
					{
						return GetLastError();
					}
					else
					{
						return NO_ERROR;
					}
				}

				Context()
				{
					raw.resize(32768);
					std::fill(raw.begin(), raw.end(), '\0');
				}
			} c;

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

		bool IsFileExistant(cstr absPath) const override
		{
			return INVALID_FILE_ATTRIBUTES != GetFileAttributesA(absPath);
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
			if (hFile == INVALID_HANDLE_VALUE) Throw(HRESULT_FROM_WIN32(GetLastError()), "Win32OS::LoadResource failed: Error opening file %ls", absPath);

			LARGE_INTEGER len;
			GetFileSizeEx(hFile, &len);

			if (len.QuadPart == 0)
			{
				Throw(0, "Win32OS::LoadResource failed: %ws was blank", absPath);
			}

			if (maxFileLength > 0 && len.QuadPart > maxFileLength)
			{
				Throw(0, "Win32OS::LoadResource failed: File <%s> was too large at over %lld bytes", absPath, maxFileLength);
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

		bool TryLoadAbsolute(const wchar_t* absPath, ILoadEventsCallback& cb, ErrorCode& sysErrorCode) const override
		{
			sysErrorCode = (ErrorCode) 0;

			FileHandle hFile = CreateFileW(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				sysErrorCode = (ErrorCode)HRESULT_FROM_WIN32(GetLastError());
				return false; 
			}

			try
			{
				LARGE_INTEGER len;
				if (!GetFileSizeEx(hFile, &len))
				{

				}

				cb.OnFileOpen(len.QuadPart);

				struct LoadError : IException
				{
					int code;

					LoadError(int _code): code(_code)
					{

					}

					cstr Message() const override
					{
						return __FUNCTION__;
					}

					int32 ErrorCode() const override
					{
						return (int32) code;
					}

					Debugging::IStackFrameEnumerator* StackFrames() override
					{
						return nullptr;
					}
				};

				struct Reader : ILoadEventReader
				{
					HANDLE hFile;
					const wchar_t* absPath;

					Reader(HANDLE _hFile, const wchar_t* _absPath) : hFile(_hFile), absPath(_absPath) {}

					void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
					{
						DWORD bytesReadAPI;
						if (!ReadFile(hFile, buffer, capacity, &bytesReadAPI, nullptr))
						{
							throw LoadError(HRESULT_FROM_WIN32(GetLastError()));
						}

						bytesRead = bytesReadAPI;
					}
				} reader(hFile, absPath);

				cb.OnDataAvailable(reader);

				return true;
			}
			catch (IException& ex)
			{
				sysErrorCode = (ErrorCode)ex.ErrorCode();
				return false;
			}
		}

		void LoadAbsolute(const wchar_t* absPath, ILoadEventsCallback& cb) const override
		{
			FileHandle hFile = CreateFileW(absPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
			if (hFile == INVALID_HANDLE_VALUE) Throw(HRESULT_FROM_WIN32(GetLastError()), "Win32OS::LoadResource failed: Error opening file %ls", absPath);

			LARGE_INTEGER len;
			GetFileSizeEx(hFile, &len);

			cb.OnFileOpen(len.QuadPart);

			struct Reader : ILoadEventReader
			{
				HANDLE hFile;
				const wchar_t* absPath;

				Reader(HANDLE _hFile, const wchar_t* _absPath) : hFile(_hFile), absPath(_absPath) {}

				void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
				{
					DWORD bytesReadAPI;
					if (!ReadFile(hFile, buffer, capacity, &bytesReadAPI, nullptr))
					{
						Throw(HRESULT_FROM_WIN32(GetLastError()), "Error reading file <%s>", absPath);
					}

					bytesRead = bytesReadAPI;
				}
			} reader(hFile, absPath);

			cb.OnDataAvailable(reader);
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

namespace Rococo::IO
{
	ROCOCO_API IOSSupervisor* GetIOS()
	{
		return new WIN32_ANON::Win32OS();
	}

	ROCOCO_API IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& os)
	{
		return new WIN32_ANON::Installation(contentIndicatorName, os);
	}

	ROCOCO_API IInstallationSupervisor* CreateInstallationDirect(const wchar_t* contentDirectory, IOS& os)
	{
		wchar_t slash[2] = { 0 };
		slash[0] = Rococo::IO::GetFileSeparator();

		if (!Rococo::Strings::EndsWith(contentDirectory, slash))
		{
			Throw(0, "Content %ls did not end with %ls", contentDirectory, slash);
		}

		bool assumeAbsolute = false;

		size_t len = wcslen(contentDirectory);
		if (len > 2)
		{
			if (contentDirectory[1] == L':' && IsCapital((char)contentDirectory[0]))
			{
				assumeAbsolute = true;
			}
		}

		if (contentDirectory[0] == IO::GetFileSeparator())
		{
			assumeAbsolute = true;
		}

		if (!assumeAbsolute)
		{
			U8FilePath currentDir;
			IO::GetCurrentDirectoryPath(currentDir);

			WideFilePath absPath;
			Format(absPath, L"%hs\\%ls", currentDir.buf, contentDirectory);

			IO::NormalizePath(absPath);
			return new WIN32_ANON::Installation(os, absPath);
		}
		else
		{
			return new WIN32_ANON::Installation(os, contentDirectory);
		}
	}
}

namespace Rococo::OS
{
	ROCOCO_API ThreadLock::ThreadLock()
	{
		static_assert(sizeof(CRITICAL_SECTION) <= sizeof(implementation), "ThreadLock too small. Increase opaque data");
		InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	ROCOCO_API ThreadLock::~ThreadLock()
	{
		DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	ROCOCO_API void ThreadLock::Lock()
	{
		EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	ROCOCO_API void ThreadLock::Unlock()
	{
		LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this->implementation));
	}

	ROCOCO_API IAppControlSupervisor* CreateAppControl()
	{
		struct AppControl : public IAppControlSupervisor, public Rococo::Tasks::ITaskQueue
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

			std::list<Rococo::Function<void()>> tasks;

			Rococo::Tasks::ITaskQueue& MainThreadQueue() override
			{
				return *this;
			}

			void AddTask(Rococo::Function<void()> lambda) override
			{
				tasks.push_back(lambda);
			}

			bool ExecuteNext() override
			{
				if (tasks.empty())
				{
					AdvanceSysMonitors();
					return false;
				}

				tasks.front().Invoke();
				tasks.pop_front();
				return true;
			}

			void AdvanceSysMonitors() override
			{
				for (auto* s : sysMonitors)
				{
					s->DoHousekeeping();
				}
			}

			bool isRunning = true;

			std::vector<ISysMonitor*> sysMonitors;

			void AddSysMonitor(IO::ISysMonitor& monitor) override
			{
				auto i = std::find(sysMonitors.begin(), sysMonitors.end(), &monitor);
				if (i == sysMonitors.end())
				{
					sysMonitors.push_back(&monitor);
				}
			}
		};

		return new AppControl();
	}

	ROCOCO_API void BeepWarning()
	{
		MessageBeep(MB_ICONWARNING);
	}

	ROCOCO_API void PrintDebug(const char* format, ...)
	{
#if _DEBUG
		va_list arglist;
		va_start(arglist, format);
		char line[4096];
		SafeVFormat(line, sizeof(line), format, arglist);
		OutputDebugStringA(line);
#else
		UNUSED(format);
#endif
	}

	ROCOCO_API cstr GetCommandLineText()
	{
		return GetCommandLineA();
	}

	ROCOCO_API void GetCurrentUserName(Strings::IStringPopulator& populator)
	{
		char username[UNLEN + 1];
		DWORD cb = sizeof(username);
		if (GetUserNameA(username, &cb))
		{
			populator.Populate(username);
		}
		else
		{
			populator.Populate("<unknown-user>");
		}
		memset(username, 0, UNLEN + 1);
	}

	ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack)
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
					s.AppendFormat("#%-2u %-48.48s ", sf.depth, sf.functionName);
					if (*sf.sourceFile)
					{
						s.AppendFormat("Line #%4u of %-64s ", sf.lineNumber, sf.sourceFile);
					}
					else
					{
						s.AppendFormat("%-79.79s", "");
					}
					s.AppendFormat("%-64s ", sf.moduleName);
					s.AppendFormat("%4.4u:%016.16llX", sf.address.segment, sf.address.offset);
					s << "\n";
				}
			} formatter;
			formatter.sb = &sb;

			stackFrames->FormatEachStackFrame(formatter);
		}
	}

	ROCOCO_API void CopyExceptionToClipboard(IException& ex, Windows::IWindow& clipboardOwner)
	{
		std::vector<char> buffer;
		buffer.resize(128_kilobytes);
		BuildExceptionString(buffer.data(), buffer.size(), ex, true);
		SaveClipBoardText(buffer.data(), clipboardOwner);
	}
}

namespace Rococo::IO
{
	ROCOCO_API void EnsureUserDocumentFolderExists(const wchar_t* subdirectory)
	{
		if (subdirectory == nullptr || *subdirectory == 0)
		{
			Throw(0, "%s: subdirectory argument was blank", __FUNCTION__);
		}

		if (StrStrW(subdirectory, L".") != nullptr)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '.'", __FUNCTION__, subdirectory);
		}

		if (StrStrW(subdirectory, L"%") != nullptr)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '%'", __FUNCTION__, subdirectory);
		}

		if (StrStrW(subdirectory, L"$") != nullptr)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '$'", __FUNCTION__, subdirectory);
		}

		if (subdirectory[0] == L'\\')
		{
			Throw(0, "%s: subdirectory %ls must not begin with a slash character '\\'", __FUNCTION__, subdirectory);
		}

		PWSTR path;
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path);
		if (FAILED(hr) || path == nullptr)
		{
			Throw(hr, "Failed to identify user documents folder. Win32 issue?");
		}
		
		std::vector<wchar_t> fullPath;
		fullPath.resize(wcslen(path) + 2 + wcslen(subdirectory));
		wnsprintfW(fullPath.data(), MAX_PATH, L"%ws\\%ws", path, subdirectory);

		int len = wnsprintfW(fullPath.data(), MAX_PATH, L"%s\\%s", path, subdirectory);
		if (len >= MAX_PATH)
		{
			Throw(hr, "%s: path too long: %ls", __FUNCTION__, fullPath.data());
		}

		if (!CreateDirectoryW(fullPath.data(), nullptr))
		{
			hr = GetLastError();
			if (hr == ERROR_ALREADY_EXISTS)
			{
				// We ensured the directory exists
				return;
			}
			Throw(hr, "%s: could not create subdirectory %ls", __FUNCTION__, fullPath.data());
		}
	}

	void GetFullPathFromTarget(TargetDirectory target, const wchar_t* relativePath, OUT std::vector<wchar_t>& absPath)
	{
		switch (target)
		{
		case TargetDirectory_UserDocuments:
		{
			PWSTR path; // This could potentially be a long longer than MAX_PATH, as windows allows 32k characters in a path
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path); // A really badly designed API, would have been much better for MSoft to have used a string populator callback, so that freeing memory was not the job of the caller
			if (FAILED(hr) || path == nullptr)
			{
				Throw(hr, "Failed to identify user documents folder. Win32 issue?");
			}

			try
			{
				absPath.resize(wcslen(path) + 2 + wcslen(relativePath));
				wnsprintfW(absPath.data(), MAX_PATH, L"%ls\\%ls", path, relativePath);
				CoTaskMemFree(path);
			}
			catch (...)
			{
				CoTaskMemFree(path);
				Throw(0, "Error allocating memory for SaveAsciiTextFile");
			}
		}
		break;
		case TargetDirectory_Root:
			absPath.resize(MAX_PATH);
			memcpy(absPath.data(), relativePath, wcslen(relativePath) * sizeof(wchar_t) + 2);
			break;
		default:
			Throw(0, "Rococo::IO::SaveAsciiTextFile(... %ls): Unrecognized target directory", relativePath);
			break;
		}
	}

	ROCOCO_API void SaveAsciiTextFileIfDifferent(TargetDirectory target, const wchar_t* filename, const fstring& text)
	{
		std::vector<wchar_t> fullPath;
		GetFullPathFromTarget(target, filename, OUT fullPath);

		struct Populator : IStringPopulator
		{
			fstring specimen;
			bool match = false;

			Populator(const fstring& _specimen) : specimen(_specimen)
			{

			}

			void Populate(const char* text) override
			{
				size_t len = strlen(text);
				if (len >= 2_gigabytes)
				{
					return;
				}

				if (len != (size_t)specimen.length)
				{
					return;
				}

				if (strcmp(text, specimen) == 0)
				{
					match = true;
				}
			}
		} onLoad(text);

		try
		{
			LoadAsciiTextFile(onLoad, fullPath.data());
		}
		catch (...)
		{
			onLoad.match = false;
		}

		if (!onLoad.match)
		{
			SaveAsciiTextFile(target, fullPath.data(), text);
		}
	}

	ROCOCO_API void SaveAsciiTextFileIfDifferent(TargetDirectory target, const char* filename, const fstring& text)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		SaveAsciiTextFileIfDifferent(target, wPath, text);
	}

	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text)
	{
		if (text.length > 1024_megabytes)
		{
			Throw(0, "Rococo::IO::SaveAsciiTextFile(%ls): Sanity check. String was > 1 gigabyte in length", filename);
		}

		HANDLE hFile = INVALID_HANDLE_VALUE;

		switch (target)
		{
		case TargetDirectory_UserDocuments:
		{
			PWSTR path; // This could potentially be a long longer than MAX_PATH, as windows allows 32k characters in a path
			HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path); // A really badly designed API, would have been much better for MSoft to have used a string populator callback, so that freeing memory was not the job of the caller
			if (FAILED(hr) || path == nullptr)
			{
				Throw(hr, "Failed to identify user documents folder. Win32 issue?");
			}

			std::vector<wchar_t> fullPath;

			try
			{
				fullPath.resize(wcslen(path) + 2 + wcslen(filename));
				wnsprintfW(fullPath.data(), MAX_PATH, L"%ls\\%ls", path, filename);
				CoTaskMemFree(path);
			}
			catch (...)
			{
				CoTaskMemFree(path);
				Throw(0, "Error allocating memory for SaveAsciiTextFile");
			}

			hFile = CreateFileW(fullPath.data(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Cannot create file %ls", fullPath.data());
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
			Throw(0, "Rococo::IO::SaveAsciiTextFile(... %ls): Unrecognized target directory", filename);
			break;
		}

		DWORD bytesWritten;
		bool status = WriteFile(hFile, text.buffer, (DWORD)text.length, &bytesWritten, NULL);
		int err = GetLastError();
		CloseHandle(hFile);

		if (!status)
		{
			Throw(err, "Rococo::IO::SaveAsciiTextFile(%ls) : failed to write text to file", filename);
		}
	}

	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, cstr filename, const fstring& text)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		SaveAsciiTextFile(target, wPath, text);
	}

	ROCOCO_API void GetUserPath(wchar_t* fullpath, size_t capacity, cstr shortname)
	{
		wchar_t* userDocPath;
		SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &userDocPath);

		size_t nChars = wcslen(userDocPath) + 2 + strlen(shortname);

		if (nChars > capacity) Throw(0, "Rococo::IO::GetUserPath -> Insufficient capacity in result buffer");

		_snwprintf_s(fullpath, capacity, capacity, L"%ls\\%hs", userDocPath, shortname);

		CoTaskMemFree(userDocPath);
	}

	ROCOCO_API void DeleteUserFile(cstr filename)
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
		if (!success)
		{
			UNUSED(hr);
		}
	}

	ROCOCO_API void SaveUserFile(cstr filename, cstr s)
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

	ROCOCO_API bool IsDirectory(const wchar_t* filename)
	{
		DWORD flags = GetFileAttributesW(filename);
		return (flags != INVALID_FILE_ATTRIBUTES && flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	ROCOCO_API bool IsDirectory(cstr filename)
	{
		DWORD flags = GetFileAttributesA(filename);
		return (flags != INVALID_FILE_ATTRIBUTES && flags & FILE_ATTRIBUTE_DIRECTORY) != 0;
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

	ROCOCO_API bool ChooseDirectory(char* name, size_t capacity)
	{
		return ChooseDirectory(name, capacity, "Rococo App");
	}

	ROCOCO_API bool ChooseDirectory(char* name, size_t capacity, cstr title)
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
			IFACEMETHODIMP OnFileOk(IFileDialog*) { return S_OK; };
			IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; };
			IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; };
			IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; };
			IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; };
			IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; };
			IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; };
			IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; };

			// IFileDialogControlEvents methods
			IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) { return S_OK; };
			IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) { return S_OK; };
			IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD, BOOL) { return S_OK; };
			IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) { return S_OK; };

			static HRESULT CreateInstance(REFIID riid, void** ppv)
			{
				*ppv = NULL;
				DialogEventHandler* pDialogEventHandler = new (std::nothrow) DialogEventHandler();
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

		WideFilePath wTitle;
		Assign(wTitle, title);
		pfd->SetTitle(wTitle);

		if (name)
		{
			WideFilePath wName;
			Assign(wName, name);
			pfd->SetFileName(wName);
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
		hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
		if (FAILED(hr))
		{
			Throw(hr, "pfd->SetOptions failed");
		}

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

	bool TrySwapExtension(U8FilePath& path, cstr expectedExtension, cstr newExtension)
	{
		using namespace Strings;

		cstr ext = GetFileExtension(path);

		char* mext = (char*)ext;

		if (expectedExtension == nullptr && ext == nullptr)
		{
			// Celebrating the 42nd anniversay of BBC Basic
			goto stripExtensionAndCatNew;
		}
		else if (expectedExtension == nullptr && ext != nullptr)
		{
			// Any extension should be stripped
			*mext = 0;
			goto stripExtensionAndCatNew;
		}
		else if (expectedExtension && ext)
		{
			if (!EqI(expectedExtension, ext))
			{
				return false;
			}

			*mext = 0;
			goto stripExtensionAndCatNew;
		}
		else // expectedExtension && !ext
		{
			return false;
		}

	stripExtensionAndCatNew:
		if (newExtension) StringCat(path.buf, newExtension, U8FilePath::CAPACITY);
		return true;
	}

	ROCOCO_API void EndDirectoryWithSlash(char* pathname, size_t capacity)
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

	ROCOCO_API void EndDirectoryWithSlash(wchar_t* pathname, size_t capacity)
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

	ROCOCO_API bool IsDirectory(const WIN32_FIND_DATAW& fd)
	{
		return (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	ROCOCO_API void* RouteSearchResult(const WIN32_FIND_DATAW& findResult, const wchar_t* root, const wchar_t* containerRelRoot, void* containerContext, IEventCallback<FileItemData>& onFile)
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

	ROCOCO_API void SearchSubdirectoryAndRecurse(const wchar_t* root, const wchar_t* containerDirectory, const wchar_t* subdirectory, void* subContext, IEventCallback<FileItemData>& onFile);

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
				IO::MakeContainerDirectory(rootDirectory);
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

	class U8SearchObject
	{
	private:
		HANDLE hSearch = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAW rootFindData;
		wchar_t fullSearchFilter[_MAX_PATH];
		wchar_t rootDirectory[_MAX_PATH];

	public:
		U8SearchObject(cstr filter)
		{
			if (filter == nullptr || filter[0] == 0)
			{
				Throw(0, "%s: <filter> was blank.", __FUNCTION__);
			}

			auto finalChar = GetFinalNull(filter)[-1];
			bool isSlashed = finalChar == L'\\' || finalChar == L'/';

			DWORD status = GetFileAttributesA(filter);

			if (status != INVALID_FILE_ATTRIBUTES && ((status & FILE_ATTRIBUTE_DIRECTORY) != 0))
			{
				SafeFormat(fullSearchFilter, L"%hs%s*.*", filter, isSlashed ? L"" : L"\\");
				SafeFormat(rootDirectory, L"%hs%s", filter, isSlashed ? L"" : L"\\");
			}
			else // Assume we have <dir>/*.ext or something similar
			{
				SafeFormat(fullSearchFilter, L"%hs", filter);
				SafeFormat(rootDirectory, L"%hs", filter);
				IO::MakeContainerDirectory(rootDirectory);
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

		void RouteSearchResults(const wchar_t* root, IEventCallback<U8FileItemData>& onFileU8, void* containerContext, bool recurse)
		{
			struct ANON : IEventCallback<FileItemData>
			{
				IEventCallback<U8FileItemData>* onFileU8;
				void OnEvent(FileItemData& itemW) override
				{
					U8FilePath u8FullPath;
					U8FilePath u8ContainerRelRoot;
					U8FilePath u8ItemRelContainer;
					Assign(u8FullPath, itemW.fullPath);
					Assign(u8ContainerRelRoot, itemW.containerRelRoot);
					Assign(u8ItemRelContainer, itemW.itemRelContainer);

					U8FileItemData u8Context;
					u8Context.containerContext = itemW.containerContext;
					u8Context.containerRelRoot = u8ContainerRelRoot;
					u8Context.fullPath = u8FullPath;
					u8Context.isDirectory = itemW.isDirectory;
					u8Context.outContext = itemW.outContext;

					onFileU8->OnEvent(u8Context);
					itemW.outContext = u8Context.outContext;
				}
			} onFile;
			onFile.onFileU8 = &onFileU8;

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

		~U8SearchObject()
		{
			if (hSearch != INVALID_HANDLE_VALUE)
			{
				FindClose(hSearch);
			}
		}
	};

	ROCOCO_API void SearchSubdirectoryAndRecurse(const wchar_t* root, const wchar_t* containerDirectory, const wchar_t* subdirectory, void* subContext, IEventCallback<FileItemData>& onFile)
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
				item.containerRelRoot = i.containerRelRoot;
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

	ROCOCO_API void GetCurrentDirectoryPath(U8FilePath& path)
	{
		GetCurrentDirectoryA(path.CAPACITY, path.buf);
	}

	ROCOCO_API void ForEachFileInDirectory(const wchar_t* filter, IEventCallback<FileItemData>& onFile, bool recurse, void* containerContext)
	{
		SearchObject searchObj(filter);
		searchObj.RouteSearchResults(nullptr, onFile, containerContext, recurse);
	}

	ROCOCO_API void ForEachFileInDirectory(cstr filter, IEventCallback<U8FileItemData>& onFile, bool recurse, void* containerContext)
	{
		U8SearchObject searchObj(filter);
		searchObj.RouteSearchResults(nullptr, onFile, containerContext, recurse);
	}

	struct PathCache : IPathCacheSupervisor
	{
		stringmap<int> files;
		mutable std::vector<cstr> results; // These point to the persistent strings in the stringmap, so will be valid for the lifetime of the file map

		void AddPathsFromDirectory(const char* directory, bool recurse) override
		{
			struct ANON : IEventCallback<U8FileItemData>
			{
				stringmap<int>* files;
				void OnEvent(U8FileItemData& item)
				{
					if (!item.isDirectory)
					{
						files->insert(item.fullPath, 0);
					}
				}
			} onFile;
			onFile.files = &files;

			ForEachFileInDirectory(directory, onFile, recurse, nullptr);

			results.clear();
		}

		void PopulateResults() const
		{
			if (results.empty())
			{
				for (auto f : files)
				{
					results.push_back(f.first);
				}
			}
		}

		void Sort() override
		{
			PopulateResults();

			std::sort(results.begin(), results.end(), 
				[](cstr a, cstr b)
				{
					return strcmp(a, b) < 0;
				}
			);
		}

		size_t NumberOfFiles() const override
		{
			PopulateResults();
			return results.size();
		}

		cstr GetFileName(size_t index) const override
		{
			return results[index];
		}

		void Free() override
		{
			delete this;
		}
	};


	ROCOCO_API IPathCacheSupervisor* CreatePathCache()
	{
		return new PathCache();
	}

	ROCOCO_API void LoadBinaryFile(IBinaryFileLoader& loader, const char* filename, uint64 maxLength)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		LoadBinaryFile(loader, wPath, maxLength);
	}

	ROCOCO_API void LoadBinaryFile(IBinaryFileLoader& loader, const wchar_t* filename, uint64 maxLength)
	{
		if (maxLength > 2_gigabytes)
		{
			Throw(0, "Max file length is 2 gigabytes");
		}


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

			if (maxLength != 0 && len.QuadPart >= (LONGLONG) maxLength)
			{
				Throw(GetLastError(), "LoadBinaryFile: File too large - length must be less than %llu bytes.\n%ls", filename);
			}

			uint8* startOfBuffer = loader.LockWriter(len.QuadPart);

			try
			{
				f.ReadBuffer((DWORD)len.QuadPart, (char*) startOfBuffer);
				loader.Unlock();
			}
			catch (IException& ex)
			{
				loader.Unlock();
				Throw(ex.ErrorCode(), "IBinaryFileLoader: %s.\n%ls", ex.Message(), filename);
			}

		} // File is no longer locked
	}
	
	ROCOCO_API void LoadAsciiTextFile(Strings::IStringPopulator& onLoad, const wchar_t* filename)
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

		onLoad.Populate(asciiData.data());
	}

	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename)
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

		if (len.QuadPart >= (int64)capacity)
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

	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, cstr filename)
	{
		WideFilePath wPath;
		Assign(wPath, filename);

		return LoadAsciiTextFile(data, capacity, wPath);
	}

	ROCOCO_API void SaveBinaryFile(const wchar_t* targetPath, const uint8* buffer, size_t nBytes)
	{
		if (nBytes > 2_gigabytes)
		{
			Throw(0, "Cannot open %ls for writing. Buffer length > 2 gigs", targetPath);
		}

		AutoFile hFile(CreateFileW(targetPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
		if (hFile == INVALID_HANDLE_VALUE)
		{
			Throw(GetLastError(), "Cannot open %ls for writing", targetPath);
		}

		DWORD bytesWritten;
		if (!WriteFile(hFile, buffer, (DWORD)nBytes, &bytesWritten, NULL))
		{
			Throw(GetLastError(), "Cannot write to file %ls", targetPath);
		}

		if (bytesWritten != (DWORD)nBytes)
		{
			Throw(GetLastError(), "Only partial write to %ls", targetPath);
		}
	}

	ROCOCO_API void SaveBinaryFile(cstr targetPath, const uint8* buffer, size_t nBytes)
	{
		WideFilePath wPath;
		Assign(wPath, targetPath);
		SaveBinaryFile(wPath, buffer, nBytes);
	}
} // Rococo::IO

namespace Rococo::Windows
{
	ROCOCO_API IWindow& NoParent()
	{
		class : public IWindow
		{
			virtual operator HWND () const
			{
				return nullptr;
			}
		} static noParent;

		return noParent;
	}

	ROCOCO_API void AddColumns(int col, int width, const char* text, HWND hReportView)
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

	ROCOCO_API void SetStackViewColumns(HWND hStackView, const int columnWidths[5])
	{
		AddColumns(0, columnWidths[0], "#", hStackView);
		AddColumns(1, columnWidths[1], "Function", hStackView);
		AddColumns(2, columnWidths[2], "Source", hStackView);
		AddColumns(3, columnWidths[3], "Module", hStackView);
		AddColumns(4, columnWidths[4], "Address", hStackView);
	}

	ROCOCO_API void PopulateStackView(HWND hStackView, Rococo::IException& ex)
	{
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
				int index = (int)SendMessage(hStackView, LVM_INSERTITEMA, 0, (LPARAM)&item);

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

namespace Rococo::Debugging
{
	void FormatStackFrames_WithDepthTarget(IStackFrameFormatter& formatter, int depthTarget = -1)
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

			if (depthTarget > 0 && depth > depthTarget)
			{
				return;
			}

			if (depthTarget > 0 && depth != depthTarget)
			{
				continue;
			}

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

	ROCOCO_API void FormatStackFrames(IStackFrameFormatter& formatter)
	{
		FormatStackFrames_WithDepthTarget(formatter, -1);
	}

	ROCOCO_API void FormatStackFrame(char* buffer, size_t capacity, StackFrame::Address address)
	{
		if (capacity && buffer) *buffer = 0;
		else return;

		HANDLE hProcess = GetCurrentProcess();

		SymInitialize(hProcess, NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES);

		STACKFRAME64 frame = { 0 };
		frame.AddrPC.Offset = address.offset;
		frame.AddrPC.Segment = address.segment;
		frame.AddrPC.Mode = AddrModeFlat;

		char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
		PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
		symbol->MaxNameLength = 254;

		char functionName[256];
		*functionName = 0;

		if (SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, NULL, symbol))
		{
			Strings::SafeFormat(functionName, symbol->Name, _TRUNCATE);
		}

		DWORD  offset = 0;
		IMAGEHLP_LINE line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

		if (SymGetLineFromAddr(hProcess, frame.AddrPC.Offset, &offset, &line))
		{
			SafeFormat(buffer, capacity, "%s: %s #%d", functionName, line.FileName, line.LineNumber);
		}
	}

	ROCOCO_API StackFrame::Address FormatStackFrame(char* buffer, size_t capacity, int depth)
	{
		if (buffer && capacity) *buffer = 0;

		struct ANON : IStackFrameFormatter
		{
			char* writePos = nullptr;
			size_t capacity = 0;
			StackFrame::Address result = { 0 };

			void Format(const StackFrame& sf) override
			{
				if (writePos != nullptr && capacity != 0)
				{
					SafeFormat(writePos, capacity, "%s: %s #%d", sf.functionName, sf.sourceFile, sf.lineNumber);
				}
				result = sf.address;
			}
		} formatter;
		formatter.writePos = buffer;
		formatter.capacity = capacity;

		FormatStackFrames_WithDepthTarget(formatter, depth);

		return formatter.result;
	}
}

using namespace Rococo;

namespace Rococo::OS
{
	template<class T> void RunInConfig(ConfigRootName root, cstr organization, T& t)
	{
		AutoHKEY hKeySoftware;
		auto status = RegOpenKeyA(HKEY_CURRENT_USER, "Software", &hKeySoftware);
		if (status != ERROR_SUCCESS)
		{
			Throw(status, "%s: Cannot open registry Software section", __FUNCTION__);
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
			Throw(status, "%s: Cannot open or create registry 'Software/%s' section", __FUNCTION__, organization);
		}

		AutoHKEY hKeyRoot;
		status = RegOpenKeyA(hKeyOrganization, root.rootName, &hKeyRoot);

		if (status != ERROR_SUCCESS)
		{
			status = RegCreateKeyA(hKeyOrganization, root.rootName, &hKeyRoot);

		}

		if (status != ERROR_SUCCESS)
		{
			Throw(status, "%s: Cannot open or create registry 'Software/%s/%s' section", __FUNCTION__, organization, root.rootName);
		}

		t(hKeyRoot);
	}

	ROCOCO_API void GetConfigVariable(char* textBuffer, size_t lenBytes, cstr defaultValue, ConfigSection section, ConfigRootName root, cstr organization, bool throwOnError)
	{
		SecureFormat(textBuffer, lenBytes, "%s", defaultValue);

		if (organization == nullptr)
		{
			organization = "Rococo - 19th Century Software";
		}

		auto readValue = [textBuffer, lenBytes, section, organization, root](HKEY hConfigRoot)
		{
			DWORD dwType = REG_SZ;
			DWORD sizeofBuffer = (DWORD)lenBytes;
			LSTATUS status = RegGetValueA(hConfigRoot, NULL, section.sectionName, RRF_RT_REG_SZ, &dwType, textBuffer, &sizeofBuffer);
			if (status != ERROR_SUCCESS)
			{
				Throw(status, "Cannot open or get registry value 'Software/%s/%s' section", organization, root.rootName);
			}
		};

		try
		{
			RunInConfig(root, organization, readValue);
		}
		catch (IException& ex)
		{
			if (throwOnError)
			{
				Throw(ex.ErrorCode(), "%s", ex.Message());
			}
		}
	}

	ROCOCO_API void SetConfigVariable(cstr value, ConfigSection section, ConfigRootName root, cstr organization)
	{
		auto writeValue = [value, section](HKEY hConfigRoot)
		{
			size_t len = strlen(value) + 1;
			if (len < 1_megabytes)
			{
				LSTATUS status = RegSetKeyValueA(hConfigRoot, NULL, section.sectionName, REG_SZ, value, (DWORD)len);
				if (status != ERROR_SUCCESS)
				{
					Throw(status, "%s: RegSetValueA(..., %s, ...) returned an error code", __FUNCTION__, section.sectionName);
				}
			}
			else
			{
				Throw(0, "%s: maximum string length is 1 megabytes", __FUNCTION__);
			}
		};

		RunInConfig(root, organization, writeValue);
	}
} // Rococo::OS

namespace Rococo::Windows
{
	ROCOCO_API int32 WheelDeltaToScrollLines(int32 wheelDelta, bool& scrollByPage)
	{
		unsigned long scrollLines = 1;
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
		if (scrollLines == WHEEL_PAGESCROLL)
		{
			scrollByPage = true;
		}
		else
		{
			scrollByPage = false;
		}

		if (wheelDelta != 0 && (wheelDelta % 120) == 0)
		{
			// Assume a windows stanard wheel, in which deltas are in units of 120
			wheelDelta = wheelDelta / 120;
		}

		return wheelDelta * (int32) scrollLines;
	}

	ROCOCO_API void MinimizeApp(IWindow& window)
	{
		ShowWindow(window, SW_MINIMIZE);
	}

	ROCOCO_API void RestoreApp(IWindow& window)
	{
		WINDOWPLACEMENT p;
		p = { 0 };
		p.length = sizeof p;

		if (GetWindowPlacement(window, &p))
		{
			if (p.showCmd == SW_SHOWNORMAL)
			{
				ShowWindow(window, SW_MAXIMIZE);
			}
			else if (p.showCmd == SW_SHOWMAXIMIZED)
			{
				ShowWindow(window, SW_SHOWNORMAL);
			}
			else if (p.showCmd == SW_SHOWMINIMIZED)
			{
				ShowWindow(window, SW_SHOWNORMAL);
			}
		}
	}

	ROCOCO_API void SendCloseEvent(IWindow& window)
	{
		SendMessage(window, WM_CLOSE, 0, 0);
	}

	ROCOCO_API Vec2i GetDesktopSpan()
	{
		HWND hWnd = GetDesktopWindow();

		RECT rect;
		GetWindowRect(hWnd, &rect);

		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	ROCOCO_API Vec2i GetDesktopWorkArea()
	{
		RECT rect;
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}
}

namespace Rococo::Strings::CLI
{
	ROCOCO_API int GetClampedCommandLineOption(const CommandLineOptionInt32& option)
	{
		const auto cmd = GetCommandLineA();
		const auto fullArg = strstr(cmd, option.spec.prefix);

		int value = option.defaultValue;

		if (fullArg)
		{
			const auto rhs = fullArg + option.spec.prefix.length;
			value = atoi(rhs);
		}

		return clamp(value, option.minValue, option.maxValue);
	}

	bool HasSwitch(const CommandLineOption& option)
	{
		const auto cmd = GetCommandLineA();
		const auto fullArg = strstr(cmd, option.prefix);

		if (!fullArg)
		{
			return false;
		}

		const char lastChar = fullArg[option.prefix.length];
		switch(lastChar)
		{
		case 0:
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			return true;
		default:
			return false;
		}
	}
}

namespace Rococo::Time
{
	ROCOCO_API ticks TickCount()
	{
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);
		return ticks.QuadPart;
	}

	ROCOCO_API double ToMilliseconds(ticks dt)
	{
		double seconds = dt / (double) TickHz();
		return seconds * 1000.0;
	}

	ROCOCO_API ticks TickHz()
	{
		LARGE_INTEGER hz;
		QueryPerformanceFrequency(&hz);
		return hz.QuadPart;
	}

	ROCOCO_API ticks UTCTime()
	{
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		return *(ticks*)&ft;
	}

	ROCOCO_API void FormatTime(ticks utcTime, char* buffer, size_t nBytes)
	{
		SYSTEMTIME st;
		char localDate[255], localTime[255];

		FileTimeToLocalFileTime((FILETIME*)&utcTime, (FILETIME*)&utcTime);
		FileTimeToSystemTime((FILETIME*)&utcTime, &st);

		GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, localDate, 255);
		GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, localTime, 255);
		SafeFormat(buffer, nBytes, "%s %s", localTime, localDate);
	}

	ROCOCO_API void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

	ROCOCO_API Timer::Timer(const char * const _name):
		name(_name), start(0), end(0)
	{

	}

	ROCOCO_API void Timer::Start()
	{
		end = 0;
		start = TickCount();
	}

	ROCOCO_API void Timer::End()
	{
		end = TickCount();
	}

	ROCOCO_API Time::ticks Timer::ExpiredTime()
	{
		return TickCount() - start;
	}

	ROCOCO_API void Timer::FormatMillisecondsWithName(char buffer[Timer::FORMAT_CAPACITY])
	{
		SafeFormat(buffer, 256, "%.128s: %.3f", name, Time::ToMilliseconds(end - start));
	}

} // Rococo::Time

namespace Rococo::Debugging
{
	ROCOCO_API int Log(cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char output[1024];
		int len = Strings::SafeVFormat(output, sizeof output, format, args);
		OutputDebugStringA(output);

		vprintf_s(format, args);

		return len;
	}
}

#ifdef _DEBUG
namespace Rococo::Strings
{
	ROCOCO_API int PrintD(const char* format, ...)
	{
		char message[2048];

		va_list args;
		va_start(args, format);
		int len = SafeVFormat(message, sizeof message, format, args);

		OutputDebugStringA(message);

		return len;

	}
}
#endif
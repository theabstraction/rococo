#define ROCOCO_API 

#include <CoreMinimal.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.task.queue.h>
#include <rococo.debugging.h>
#include <rococo.time.h>
#include <time.h>
#include <Misc/Paths.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <GenericPlatform/GenericPlatformMisc.h>
#include <GenericPlatform/GenericPlatformApplicationMisc.h>
#include <Logging/LogMacros.h>
#include <Misc/CommandLine.h>

using namespace Rococo::Strings;

namespace Rococo
{
	ROCOCO_API void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

	void ConvertFStringToUint8Buffer(TArray<uint8>& buffer, const FString& src)
	{
		int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*src, src.Len());

		TArray<uint8> buffer;
		buffer.SetNumUninitialized(nElements);

		FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer.GetData()), buffer.Num(), *src, nElements);
	}

	void SecureCopyStringToBuffer(wchar_t* destination, size_t capacity, const FString& s)
	{
		if (s.Len() >= capacity)
		{
			Throw(FString::Printf(TEXT("SecureCopyStringToBuffer(%s) failed. Insufficient capacity"), *s));
		}

		if constexpr (sizeof(TCHAR) == sizeof(wchar_t))
		{
			for (size_t i = 0; i < s.Len(); ++i)
			{
				destination[i] = s[i];
			}

			destination[s.Len()] = 0;
		}
	}

	void Populate(WideFilePath& path, const FString& src)
	{
		if (src.Len() >= path.CAPACITY)
		{
			Throw(FString::Printf(TEXT("Cannot populate UnrealFilePath. Source name too long: %s"), *src));
		}

		FMemory::Memcpy(path.buf, *src, sizeof(TCHAR) * (src.Len() + 1));
	}
	
	void Populate(U8FilePath& path, const FString& src)
	{
		if (src.Len() >= path.CAPACITY)
		{
			Throw(FString::Printf(TEXT("Cannot populate UnrealFilePath. Source name too long: %s"), *src));
		}

		if constexpr ((sizeof TCHAR) > 1)
		{
			FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(path.buf), path.CAPACITY, (const TCHAR*) *src, src.Len());
		}
		else
		{
			Strings::CopyString(path.buf, path.CAPACITY, (const char*) *src);
		}
	}



	void Throw(const FString& msg)
	{	
		if constexpr ((sizeof TCHAR) > 1)
		{
			TArray<uint8> buffer;
			ConvertFStringToUint8Buffer(buffer, msg);
			Throw(0, "%s", buffer.GetData());
		}
		else
		{
			Throw(0, "%s", *msg);
		}
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
					Throw(FString::Printf(TEXT("Error creating file: %s"), sysPath));
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

	ROCOCO_API IBinarySource* ReadBinarySource(const TCHAR* sysPath)
	{
		struct BinFile : IBinarySource
		{
			IFileHandle* file = nullptr;
			FString filename;

			BinFile(const TCHAR* sysPath)
			{
				if (sysPath == nullptr) Throw(0, "%s: null sysPath", __FUNCTION__);

				file = IPlatformFile::GetPlatformPhysical().OpenRead(sysPath);
				if (file == nullptr)
				{
					Throw(FString::Printf(TEXT("Error opening file for read: %s"), sysPath));
				}
			}

			~BinFile()
			{
				delete file;
			}

			uint32 Read(uint32 capacity, void* pElements) override
			{
				DWORD bytesRead = 0;

				if (!file->Read((uint8*)pElements, (int64)capacity))
				{
					Throw(FString::Printf(TEXT("Error reading file: %s"), *filename));
				}
				
				return bytesRead;
			}

			void Free() override
			{
				delete this;
			}
		};

		return new BinFile(sysPath);
	}

	struct AutoFile
	{
		IFileHandle* file;

		AutoFile(IFileHandle* _file): file(_file)
		{
		}

		~AutoFile()
		{
			delete file;
		}

		IFileHandle* operator -> ()
		{
			if (!file)
			{
				Throw(0, __FUNCTION__ ": file handle nullptr");
			}
			return file;
		}
	};

	ROCOCO_API IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(const wchar_t* sysPath)
	{
		// On Unreal we don't do file mapping as not all platforms may support it, so just open the whole file into RAM
		struct BinMapping : IReadOnlyBinaryMapping
		{
			TArray<uint8> dataMap;

			BinMapping(const wchar_t* sysPath)
			{
				AutoFile file(IPlatformFile::GetPlatformPhysical().OpenRead(sysPath));
				if (file.file == nullptr)
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": Error opening file for read %s"), sysPath));
				}
			
				int64 len = file->Size();

				if (len > 4_gigabytes)
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": Error opening file for read %s. File > 4GB"), sysPath));
				}

				dataMap.SetNum(len, true);

				if (!file->Read(dataMap.GetData(), len))
				{
					Throw(FString::Printf(TEXT(__FUNCTION__) TEXT(": Error reading data from %s"), sysPath));
				}
			}

			~BinMapping()
			{
			}

			const char* Data() const override
			{
				return (const char*)dataMap.GetData();
			}

			const uint64 Length() const override
			{
				return dataMap.Num();
			}

			void Free() override
			{
				delete this;
			}
		};

		return new BinMapping(sysPath);
	}

	void NormalizePath(WideFilePath& path)
	{
		FString sPath(path.buf);
		FPaths::NormalizeFilename(sPath);
		Populate(path, sPath);
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
		FString exeName(FGenericPlatformProcess::ExecutablePath());

		TArray<uint8> buffer;
		ConvertFStringToUint8Buffer(buffer, exeName);

		if (buffer.Num() >= path.CAPACITY)
		{
			Throw(FString::Printf(TEXT("Cannot GetExePath - filename '%s' too long"), *exeName));
		}

		FMemory::Memcpy(path.buf, buffer.GetData(), buffer.NumBytes());
	}

	ROCOCO_API void GetExePath(WideFilePath& path)
	{
		FString exeName(FGenericPlatformProcess::ExecutablePath());
		Strings::Format(OUT path, TEXT("%s"), *exeName);
	}

	void GetContentDirectory(const TCHAR* contentIndicatorName, WideFilePath& path, IOS& os)
	{
		WideFilePath binDirectory;
		os.GetBinDirectoryAbsolute(binDirectory);

		path = binDirectory;

		FString contentIndicator(contentIndicatorName);

		if (contentIndicator.Find(TEXT("\\")) > 0 || contentIndicator.Find(TEXT("/")) > 0)
		{
			// The indicator is part of a path
			if (os.IsFileExistant(contentIndicatorName))
			{
				Populate(path, contentIndicator);
				IO::MakeContainerDirectory(path.buf);
				return;
			}
		}

		size_t len = Strings::StringLength(path);

		while (len > 0)
		{
			FString indicator = FString::Printf(TEXT("%s%s"), path, contentIndicatorName);
			if (os.IsFileExistant(*indicator))
			{
				indicator = FString::Printf(TEXT("%scontent\\"), path);
				Populate(path, indicator);
				return;
			}

			IO::MakeContainerDirectory(path.buf);

			size_t newLen = Strings::StringLength(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(FString::Printf(TEXT("Could not find %s below the executable folder '%s'"), contentIndicatorName, binDirectory.buf));
	}

	/*
	ROCOCO_API bool IsKeyPressed(int vkeyCode)
	{
		Throw(0, "IO::IsKeyPressed not supported by Rococo for Unreal Engine");
	}
	*/

	ROCOCO_API void CopyToClipboard(cstr asciiText)
	{
		if constexpr (sizeof TCHAR > 1)
		{
			FString tcharText(asciiText);
			FGenericPlatformApplicationMisc::ClipboardCopy(*tcharText);
		}
		else
		{
			FGenericPlatformApplicationMisc::ClipboardCopy((const TCHAR*) asciiText);
		}
		
	}

	ROCOCO_API void PasteFromClipboard(char* asciiBuffer, size_t capacity)
	{
		FString pastedText;
		FGenericPlatformApplicationMisc::ClipboardPaste(OUT pastedText);

		if constexpr (sizeof TCHAR > 1)
		{
			FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(asciiBuffer), capacity, *pastedText, pastedText.Len());
		}
		else
		{
			Strings::CopyString(asciiBuffer, capacity, (const char*)*pastedText);
		}
	}

	ROCOCO_API void UseBufferlessStdout()
	{
		setvbuf(stdout, nullptr, _IONBF, 0);
	}

	ROCOCO_API bool TryGetFileAttributes(const TCHAR* sysPath, FileAttributes& attr)
	{
		if (sysPath == nullptr) Throw(0, "Rococo::IO::GetFileLength: sysPath was null");

		auto& P = IPlatformFile::GetPlatformPhysical();

		int64 len = P.FileSize(sysPath);
		if (len == -1)
		{
			attr.fileLength = -1;
			attr.timestamp[0] = 0;
			return false;
		}

		attr.fileLength = len;

		FDateTime t = P.GetTimeStamp(sysPath);

		SafeFormat(attr.timestamp, sizeof(attr.timestamp), "%2.2d:%2.2d:%2.2d %2.2d/%2.2d/%4.4d",
			t.GetHour(), t.GetMinute(), t.GetSecond(), t.GetDay(), t.GetMonth(), t.GetYear());

		return true;
	}
}

namespace Rococo
{
	ROCOCO_API bool FileModifiedArgs::Matches(cstr resource) const
	{
		const wchar_t* a = this->sysPath;
		cstr b = resource;
		if (*b == TEXT('!')) b++;

		while (*a != 0)
		{
			if (*a == TEXT('\\'))
			{
				if (*b == TEXT('\\') || *b == TEXT('/'))
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

namespace
{
	class CriticalSection : public Rococo::OS::ICriticalSection
	{
	private:
		Rococo::OS::CriticalSectionMemorySource src;
		FCriticalSection cs;

	public:
		CriticalSection(Rococo::OS::CriticalSectionMemorySource _src) : src(_src)
		{
			
		}

		virtual ~CriticalSection()
		{
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
			cs.Lock();
		}

		void Unlock() override
		{
			cs.Unlock();
		}
	};
}

namespace Rococo::OS
{
	ROCOCO_API void PasteStringFromClipboard(Strings::IStringPopulator& populator)
	{
		FString pastedText;
		FGenericPlatformApplicationMisc::ClipboardPaste(OUT pastedText);

		if constexpr (sizeof TCHAR > 1)
		{
			TArray<uint8> buffer;
			ConvertFStringToUint8Buffer(OUT buffer, pastedText);
			populator.Populate((cstr) buffer.GetData());
		}
		else
		{
			populator.Populate((cstr)*pastedText);
		}
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

	/*
	ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow)
	{
		Throw(0, "OS::SetCursorVisibility not supported by Rococo for Unreal Engine");
	}

	ROCOCO_API void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath)
	{
		Throw(0, "OS::EditImageFile not supported by Rococo for Unreal Engine");
	}

	ROCOCO_API void SleepUntilAsync(uint32 timeoutMS)
	{
		Throw(0, "OS::SleepUntilAsync not supported by Rococo for Unreal Engine");
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


	*/

	ROCOCO_API bool IsDebugging()
	{
		return FGenericPlatformMisc::IsDebuggerPresent();
	}

	ROCOCO_API void TripDebugger()
	{
		FGenericPlatformMisc::DebugBreak();
	}

	ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode)
	{
		if (errorCode != 0)
		{
			snprintf(message, sizeofBuffer, "Error %d (0x%X)", errorCode, errorCode);
		}
		else
		{
			snprintf(message, sizeofBuffer, "No error code");
		}
	}
}

namespace Rococo
{
	ROCOCO_API MemoryUsage ProcessMemory()
	{
		auto& constants = FGenericPlatformMemory::GetConstants();
		auto stats = FGenericPlatformMemory::GetStats();

		return{ stats.UsedPhysical, stats.PeakUsedPhysical };
	}
}

using namespace Rococo;
using namespace Rococo::IO;

namespace UE5_ANON
{
	struct FilePath
	{
		enum { CAPACITY = 260 };
		char data[CAPACITY];
		operator char* () { return data; }
		operator cstr() const { return data; }
	};

	constexpr fstring packageprefix = "Package["_fstring;

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
			len = (int32)Strings::StringLength(contentDirectory);

			if (!IO::IsDirectory(contentDirectory))
			{
				Throw(FString::Printf(TEXT("No such content directory: %s"), contentDirectory.buf));
			}
		}

		Installation(IOS& _os, const TCHAR* contentPath) : os(_os)
		{
			if (!IO::IsDirectory(contentPath))
			{
				Throw(FString::Printf(TEXT("No such content directory: %s"), contentPath));
			}

			len = Format(contentDirectory, TEXT("%s"), contentPath);
		}

		void Free()  override
		{
			delete this;
		}

		IOS& OS()  override
		{
			return os;
		}

		const TCHAR* Content() const  override
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

				return Strings::Eq(sysPathA, sysPathB);
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
				if (*p == '/' || *p =='\\')
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
			if (pingPath == nullptr || Strings::StringLength(pingPath) < 2) Throw(0, "LoadResource failed: <resourcePath> was blank");

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
			if (pingPath == nullptr || Strings::StringLength(pingPath) < 2)
			{
				errorCode = (ErrorCode)0;
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
			if (pingPath == nullptr || rlen(pingPath) < 2) Throw(0, "LoadResource failed: <resourcePath> was blank");

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
			if (pingPath == nullptr || rlen(pingPath) < 2) Throw(0, "TryLoadResource failed: <resourcePath> was blank");

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

	class UE5OS : public IOSSupervisor
	{
		WideFilePath binDirectory;
		IEventCallback<SysUnstableArgs>* onUnstable;

	public:
		UE5OS() :
			onUnstable(nullptr)
		{
			IO::GetExePath(OUT binDirectory);
		}

		~UE5OS()
		{
		}

		void Free() override
		{
			delete this;
		}

		void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs>& cb) override
		{
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
		}

		bool IsFileExistant(cstr absPath) const override
		{
			return IO::IsFileExistant(absPath);
		}

		bool IsFileExistant(const wchar_t* absPath) const override
		{
			return IO::IsFileExistant(absPath);
		}

		void ConvertUnixPathToSysPath(const wchar_t* unixPath, WideFilePath& sysPath) const override
		{
			if (unixPath == nullptr) Throw(0, "Blank path in call to os.ConvertUnixPathToSysPath");
			if (wcslen(unixPath) >= sysPath.CAPACITY)
			{
				Throw(0, "Path too long in call to os.ConvertUnixPathToSysPath");
			}

			size_t len = wcslen(unixPath);

			size_t i = 0;
			for (; i < len; ++i)
			{
				wchar_t c = unixPath[i];

				if (c == '\\') Throw(0, "Illegal backslash '\\' in unixPath in call to os.ConvertUnixPathToSysPath");

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
			AutoFile file(IPlatformFile::GetPlatformPhysical().OpenRead(absPath));
			if (file.file == nullptr)
			{
				Throw(FString::Printf(TEXT("LoadAbsolute failed: Error opening file %s"), absPath));
			}
			
			int64 fileSize = file->Size();

			if (fileSize == 0)
			{
				Throw(FString::Printf(TEXT("LoadAbsolute failed: %s was blank"), absPath));
			}

			if (maxFileLength > 0 && fileSize > maxFileLength)
			{
				Throw(FString::Printf(TEXT("LoadAbsolute failed: File <%s> was too large at over %lld bytes"), absPath, maxFileLength));
			}

			buffer.Resize(maxFileLength + 1); // This gives us space for a nul terminating character
			buffer.Resize(maxFileLength);

			int64 bytesLeft = maxFileLength;
			ptrdiff_t offset = 0;

			uint8* data = (uint8*)buffer.GetData();
			data[fileSize] = 0;

			while (bytesLeft > 0)
			{
				DWORD chunk = (DWORD)(int32)min(bytesLeft, 65536LL);
				DWORD bytesRead = 0;
				if (!file->Read(data + offset, chunk))
				{
					Throw(FString::Printf(TEXT("LoadAbsolute failed: Error reading file <%s>"), absPath));
				}

				if (bytesRead != chunk)
				{
					Throw(FString::Printf(TEXT("LoadAbsolute failed:  Error reading file <%s>. Failed to read chunk"), absPath));
				}

				offset += (ptrdiff_t)chunk;
				bytesLeft -= (int64)chunk;
			}
		}

		bool TryLoadAbsolute(const wchar_t* absPath, ILoadEventsCallback& cb, ErrorCode& sysErrorCode) const override
		{
			sysErrorCode = (ErrorCode)0;

			AutoFile file(IPlatformFile::GetPlatformPhysical().OpenRead(absPath));
			if (file.file == nullptr)
			{
				return false;
			}

			try
			{
				int len = file->Size();

				cb.OnFileOpen(len);

				struct LoadError : IException
				{
					int code;

					LoadError(int _code) : code(_code)
					{

					}

					cstr Message() const override
					{
						return __FUNCTION__;
					}

					int32 ErrorCode() const override
					{
						return (int32)code;
					}

					Debugging::IStackFrameEnumerator* StackFrames() override
					{
						return nullptr;
					}
				};

				struct Reader : ILoadEventReader
				{
					IFileHandle* hFile;
					const wchar_t* absPath;

					Reader(IFileHandle* _hFile, const wchar_t* _absPath) : hFile(_hFile), absPath(_absPath) {}

					void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
					{
						int64 prePos = hFile->Tell();

						if (!hFile->Read((uint8*)buffer, (int64)bytesRead))
						{
							Throw(FString::Printf(TEXT("TryLoadAbsolute(%s) - read failed"), absPath));
						}

						int64 postPos = hFile->Tell();

						int64 delta = postPos - prePos;
						if (delta < 0 || delta > 0x00000000FFFFFFFF)
						{
							Throw(FString::Printf(TEXT("TryLoadAbsolute(%s) - unexpected file position"), absPath));
						}

						bytesRead = (uint32)delta;
					}
				} reader(file.file, absPath);

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
			AutoFile hFile = IPlatformFile::GetPlatformPhysical().OpenRead(absPath);
			if (hFile.file == nullptr) Throw(FString::Printf(TEXT("LoadAbsolute(%ls) failed: Error opening file"), absPath));

			int64 fileLength = hFile->Size();

			cb.OnFileOpen(fileLength);

			struct Reader : ILoadEventReader
			{
				IFileHandle* hFile;
				const wchar_t* absPath;

				Reader(IFileHandle* _hFile, const wchar_t* _absPath) : hFile(_hFile), absPath(_absPath) {}

				void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
				{
					int64 prePos = hFile->Tell();

					if (!hFile->Read((uint8*)buffer, (int64)bytesRead))
					{
						Throw(FString::Printf(TEXT("TryLoadAbsolute(%s) - read failed"), absPath));
					}

					int64 postPos = hFile->Tell();

					int64 delta = postPos - prePos;
					if (delta < 0 || delta > 0x00000000FFFFFFFF)
					{
						Throw(FString::Printf(TEXT("TryLoadAbsolute(%s) - unexpected file position"), absPath));
					}

					bytesRead = (uint32)delta;
				}
			} reader(hFile.file, absPath);

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

		void Monitor(const wchar_t* /* absPath */) override
		{
			Throw(0, "Not implemented");
		}
	};
}

namespace Rococo::IO
{
	ROCOCO_API IOSSupervisor* GetIOS()
	{
		return new UE5_ANON::UE5OS();
	}

	ROCOCO_API IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& os)
	{
		return new UE5_ANON::Installation(contentIndicatorName, os);
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
			return new UE5_ANON::Installation(os, absPath);
		}
		else
		{
			return new UE5_ANON::Installation(os, contentDirectory);
		}
	}
}

namespace Rococo::OS
{
	ROCOCO_API IAppControlSupervisor* CreateAppControl()
	{
		struct AppControl : public IAppControlSupervisor, public Rococo::Tasks::ITaskQueue
		{
			void ShutdownApp() override
			{
				isRunning = false;
				FGenericPlatformMisc::RequestExit(false, TEXT(__FUNCTION__));
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
		
	}

	FLogCategoryName RococoLog = "RococoLog";

	ROCOCO_API void PrintDebug(const char* format, ...)
	{
		va_list arglist;
		va_start(arglist, format);

		char buffer[1024];
		Strings::SafeVFormat(buffer, sizeof buffer, format, arglist);

		FString tcharBuffer(buffer);

		FMsg::Logf(__FILE__, __LINE__, RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
	}

	ROCOCO_API cstr GetCommandLineText()
	{
		static TArray<uint8> utf8buffer;
		if (utf8buffer.Num() == 0)
		{
			FString cmdLine = FCommandLine::Get();
			ConvertFStringToUint8Buffer(OUT utf8buffer, cmdLine);
		}

		return (cstr)utf8buffer.GetData();
	}

	ROCOCO_API void GetCurrentUserName(Strings::IStringPopulator& populator)
	{
		FString user = FPlatformProcess::UserName();
		if (user.Len() == 0)
		{
			user = FString(TEXT("<Unknown user>"));
		}

		TArray<uint8> utf8buffer;
		ConvertFStringToUint8Buffer(OUT utf8buffer, user);

		populator.Populate((cstr) utf8buffer.GetData());
	}

	ROCOCO_API void CopyStringToClipboard(cstr text)
	{
		CopyToClipboard(text);
	}

	ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack)
	{
		StackStringBuilder sb(buffer, capacity);

		if (ex.ErrorCode() != 0)
		{
			sb.AppendFormat(" Error code: %d (0x%8.8X)\n", ex.ErrorCode(), ex.ErrorCode());
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

	ROCOCO_API void CopyExceptionToClipboard(IException& ex)
	{
		TArray<char> buffer;
		buffer.SetNum(128_kilobytes);
		BuildExceptionString(buffer.GetData(), buffer.Num(), ex, true);
		CopyStringToClipboard(buffer.GetData());
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

		FString sSubDirectory(subdirectory);

		if (sSubDirectory.Find(TEXT(".")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '.'", __FUNCTION__, subdirectory);
		}

		if (sSubDirectory.Find(TEXT("%")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '%'", __FUNCTION__, subdirectory);
		}

		if (sSubDirectory.Find(TEXT("$")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '$'", __FUNCTION__, subdirectory);
		}

		if (subdirectory[0] == '\\' || subdirectory[0] == '/')
		{
			Throw(0, "%s: subdirectory %ls must not begin with a slash character '\\'", __FUNCTION__, subdirectory);
		}
		
		FString userDocs = FPlatformProcess::UserDir();

		FString fullPath = FPaths::Combine(userDocs, subdirectory);

		if (fullPath.Len() > _MAX_PATH)
		{
			Throw(FString::Printf(TEXT("Path too long: %s"), *fullPath));
		}

		IO::CreateDirectoryFolder(*fullPath);
	}

	FString GetFullPathFromTarget(TargetDirectory target, const wchar_t* relativePath)
	{
		switch (target)
		{
		case TargetDirectory_UserDocuments:
		{
			FString userDocs = FPlatformProcess::UserDir();	
			return FPaths::Combine(userDocs, relativePath);
		}
		break;
		case TargetDirectory_Root:
			return relativePath;
		default:
			Throw(0, "Rococo::IO::GetFullPathFromTarget(... %ls): Unrecognized target directory", relativePath);
			break;
		}
	}

	ROCOCO_API void SaveAsciiTextFileIfDifferent(TargetDirectory target, const wchar_t* filename, const fstring& text)
	{
		FString fullPath = GetFullPathFromTarget(target, filename);

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
			LoadAsciiTextFile(onLoad, *fullPath);
		}
		catch (...)
		{
			onLoad.match = false;
		}

		if (!onLoad.match)
		{
			SaveAsciiTextFile(target, *fullPath, text);
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

		FString fullPath = GetFullPathFromTarget(target, filename);

		AutoFile file(IPlatformFile::GetPlatformPhysical().OpenWrite(*fullPath));
		if (!file.file)
		{
			Throw(FString::Printf(TEXT("Cannot create file %ls"), filename));
		}
		
		if (!file->Write((const uint8*)text.buffer, (int64)text.length))
		{
			Throw(0, "Rococo::IO::SaveAsciiTextFile(%ls): write failed", filename);
		}
	}

	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, cstr filename, const fstring& text)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		SaveAsciiTextFile(target, wPath, text);
	}

	ROCOCO_API void GetUserPath(wchar_t* result, size_t capacity, cstr shortname)
	{
		FString path =  FPlatformProcess::UserDir();		
		FString fullPath = FPaths::Combine(path, shortname);
		SecureCopyStringToBuffer(result, capacity, fullPath);
	}

	ROCOCO_API void DeleteUserFile(cstr filename)
	{
		FString userDocs = FPlatformProcess::UserDir();
		FString fullPath = FPaths::Combine(userDocs, filename);
		IPlatformFile::GetPlatformPhysical().DeleteFile(*fullPath);
	}

	ROCOCO_API void SaveUserFile(cstr filename, cstr s)
	{
		FString userDocs = FPlatformProcess::UserDir();
		FString fullPath = FPaths::Combine(userDocs, filename);

		AutoFile file(IPlatformFile::GetPlatformPhysical().OpenWrite(*fullPath));
		if (!file.file)
		{
			Throw(FString::Printf(TEXT("Cannot write to %s"), *fullPath));
		}

		file->Write((const uint8*)s, strlen(s));
	}

	ROCOCO_API bool IsDirectory(const wchar_t* filename)
	{
		FString sFilename(filename);
		IPlatformFile::GetPlatformPhysical().DirectoryExists(*sFilename);
	}

	ROCOCO_API bool IsDirectory(cstr filename)
	{
		FString sFilename(filename);
		IPlatformFile::GetPlatformPhysical().DirectoryExists(*sFilename);
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
			AutoFile f( IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(FString::Printf(TEXT("LoadBinaryFile: Cannot open file %ls"), filename));
			}

			int64 fileLength = f->Size();

			if (maxLength != 0 && fileLength >= (LONGLONG)maxLength)
			{
				Throw(FString::Printf(TEXT("LoadBinaryFile: Cannot open file %ls. File length was %lld bytes. Max length is %llu bytes"), filename, fileLength, maxLength));
			}

			uint8* startOfBuffer = loader.LockWriter(fileLength);

			try
			{
				f->Read(startOfBuffer, fileLength);
				loader.Unlock();
			}
			catch (IException& ex)
			{
				loader.Unlock();
				Throw(FString::Printf(TEXT("LoadBinaryFile %s.\n%ls"), ex.Message(), filename));
			}

		} // File is no longer locked
	}

	ROCOCO_API void LoadAsciiTextFile(Strings::IStringPopulator& onLoad, const wchar_t* filename)
	{
		TArray<char> asciiData;

		{ // File is locked in this codesection
			AutoFile f(IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile: Cannot open file %ls"), filename));
			}

			int64 fileLength = f->Size();

			if (fileLength >= (int64)2048_megabytes)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile(%ls): Cannot open file length must be less than 2GB.\n"), filename));
			}

			asciiData.SetNum(fileLength + 1);

			if (!f->Read((uint8*)asciiData.GetData(), fileLength))
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile: Cannot read file %ls"), filename));
			}

		} // File is no longer locked

		onLoad.Populate(asciiData.GetData());
	}

	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename)
	{
		if (capacity >= 2048_megabytes)
		{
			Throw(FString::Printf(TEXT("LoadAsciiTextFile: capacity must be less than 2GB.\n%ls"), filename));
		}

		{ // File is locked in this codesection
			AutoFile f(IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile: Cannot open file %ls"), filename));
			}

			int64 fileLength = f->Size();

			if (fileLength >= capacity)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile(%ls): Cannot open file length must be less than %llu bytes.\n"), filename, capacity));
			}

			if (!f->Read((uint8*)data, fileLength))
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile: Cannot read file %ls"), filename));
			}

			data[fileLength] = 0;

		} // File is no longer locked
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
			Throw(0, "Cannot open %ls for writing. Buffer length > 2GB", targetPath);
		}

		AutoFile f(IPlatformFile::GetPlatformPhysical().OpenWrite(targetPath));
		if (f.file == nullptr)
		{
			Throw(FString::Printf(TEXT("SaveBinaryFile: Cannot create file %ls"), targetPath));
		}

		if (!f->Write(buffer, nBytes))
		{
			Throw(0, "Cannot write to file %ls", targetPath);
		}
	}

	ROCOCO_API void SaveBinaryFile(cstr targetPath, const uint8* buffer, size_t nBytes)
	{
		WideFilePath wPath;
		Assign(wPath, targetPath);
		SaveBinaryFile(wPath, buffer, nBytes);
	}
} // Rococo::IO

namespace Rococo::Debugging
{
	void FormatStackFrames_WithDepthTarget(IStackFrameFormatter& formatter, int depthTarget = -1)
	{	
	}

	ROCOCO_API void FormatStackFrames(IStackFrameFormatter& formatter)
	{
		
	}

	ROCOCO_API void FormatStackFrame(char* buffer, size_t capacity, StackFrame::Address address)
	{
		if (capacity && buffer) *buffer = 0;	return;

	}

	ROCOCO_API StackFrame::Address FormatStackFrame(char* buffer, size_t capacity, int depth)
	{
		if (buffer && capacity) *buffer = 0;
		return { 0,0 };
	}
} // Rococo::Debugging

namespace Rococo::Strings::CLI
{
	ROCOCO_API int GetClampedCommandLineOption(const CommandLineOptionInt32& option)
	{
		const auto cmd = OS::GetCommandLineText();
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
		const auto cmd = OS::GetCommandLineText();
		const auto fullArg = strstr(cmd, option.prefix);

		if (!fullArg)
		{
			return false;
		}

		const char lastChar = fullArg[option.prefix.length];
		switch (lastChar)
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
} // Rococo::Strings::CLI

namespace Rococo::Time
{
	ROCOCO_API ticks TickCount()
	{
		return (ticks) FPlatformTime::Cycles64();
	}

	ROCOCO_API double ToMilliseconds(ticks dt)
	{
		return FPlatformTime::GetSecondsPerCycle64() * dt * 1000.0;
	}

	ROCOCO_API ticks TickHz()
	{
		double f = FPlatformTime::GetSecondsPerCycle64();
		return f <= 0.0 ? 1000000.0 : 1.0 / f;
	}

	ROCOCO_API ticks UTCTime()
	{
		return FDateTime::UtcNow().GetTicks();
	}

	ROCOCO_API void FormatTime(ticks utcTime, char* buffer, size_t nBytes)
	{
		FDateTime t(utcTime);
		FString s = t.ToString();

		int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*s, s.Len());

		if (nElements >= nBytes)
		{
			Throw(0, "Insufficient buffer. Requires %d bytes. Capacity is %llu bytes", nElements, nBytes);
		}

		FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer), nBytes, *s, nElements);
	}

	ROCOCO_API Timer::Timer(const char* const _name) :
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

		char message[1024];
		int len = Strings::SafeVFormat(message, sizeof message, format, args);
		
		FString tcharBuffer(message);
		FMsg::Logf(__FILE__, __LINE__, OS::RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
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

		FString tcharBuffer(message);
		FMsg::Logf(__FILE__, __LINE__, OS::RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
		return len;
	}
}
#endif

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
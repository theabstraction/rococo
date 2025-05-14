#define ROCOCO_API 

#include <CoreMinimal.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <time.h>
#include <Misc/Paths.h>
#include <rococo.strings.h>
#include <GenericPlatform/GenericPlatformMisc.h>
#include <GenericPlatform/GenericPlatformApplicationMisc.h>

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


	*/
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
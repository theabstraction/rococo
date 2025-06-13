#include "rococo.os.UE5.h"

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
#include <HAL/PlatformMisc.h>
#include <HAL/PlatformApplicationMisc.h>
#include <Logging/LogMacros.h>
#include <Misc/CommandLine.h>
#include <rococo.hashtable.h>
#include <vector>

DECLARE_LOG_CATEGORY_EXTERN(RococoException, Error, All);
DEFINE_LOG_CATEGORY(RococoException);

using namespace Rococo::Strings;

#ifdef _WIN32
struct HWND__;
#endif

namespace Rococo
{
#ifdef _WIN32
	typedef unsigned long WindowsDWORD;
	typedef int WindowsBOOL;
	typedef unsigned int WindowsUINT;
	typedef HWND__* HWND;
	extern "C" __declspec(dllimport) HWND GetActiveWindow();
	extern "C" __declspec(dllimport) WindowsBOOL SystemParametersInfoW(WindowsUINT uiAction, WindowsUINT uiParam, void* param, WindowsUINT fWinIni);
	extern "C" _declspec(dllimport) WindowsDWORD GetLastError();
	extern "C" _declspec(dllimport) void SetLastError(WindowsDWORD err);
	extern "C" _declspec(dllimport)  WindowsDWORD FormatMessageA(
		WindowsDWORD flags,
		const void* source,
		WindowsDWORD messageId,
		WindowsDWORD languageId,
		char* buffer,
		WindowsDWORD bufferCapacity,
		va_list* Arguments
	);

#else
	void SetLastError(int errorCode)
	{
		UNUSED(errorCode);
	}

	int GetLastError()
	{
		return 0;
	}
#endif

	void LogException(Rococo::IException& ex, cstr prelude, cstr postlude, bool isFatal)
	{
		TArray<char> err;
		err.SetNum(8192);
		StackStringBuilder sb(err.GetData(), err.Num(), StringBuilder::CursorState::BUILD_EXISTING);
		if (prelude) sb << prelude;
		if (ex.ErrorCode() != 0)
		{
			sb.AppendFormat("Error code: %d (0x%8.8X). ", ex.ErrorCode(), ex.ErrorCode());
		}

		sb << ex.Message();

		if (postlude) sb << postlude;

		FString msg(err.GetData());

		if (isFatal)
		{
			UE_LOG(RococoException, Fatal, TEXT("%s"), *msg);
		}
		else
		{
			UE_LOG(RococoException, Error, TEXT("%s"), *msg);
		}
	}

	ROCOCO_API void LogExceptionAndQuit(Rococo::IException& ex, cstr prelude, cstr postlude)
	{
		LogException(ex, prelude, postlude, true);
	}

	ROCOCO_API void LogExceptionAndContinue(Rococo::IException& ex, cstr prelude, cstr postlude)
	{
		LogException(ex, prelude, postlude, false);
	}

	[[noreturn]] void Throw(const FString& msg);

	void SecureCopyStringToBuffer(char* buffer, size_t capacity, const FString& s)
	{
		int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*s, s.Len());

		if (nElements >= capacity)
		{
			Throw(FString::Printf(TEXT("Insufficient buffer to convert string to UTF8. Requires %d bytes. Capacity is %llu bytes\r\nString: %s"), nElements, capacity, *s));
		}

		FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer), capacity, *s, nElements);
	}

	void ConvertFStringToUTF8Buffer(TArray<uint8>& buffer, const FString& src)
	{
		int32 nElements = FTCHARToUTF8_Convert::ConvertedLength(*src, src.Len());
		buffer.SetNumUninitialized(nElements);
		FTCHARToUTF8_Convert::Convert(reinterpret_cast<UTF8CHAR*>(buffer.GetData()), buffer.Num(), *src, nElements);
	}

	[[noreturn]] void Throw(int errorCode, const FString& msg)
	{
		if constexpr ((sizeof (TCHAR)) > 1)
		{
			TArray<uint8> buffer;
			ConvertFStringToUTF8Buffer(buffer, msg);
			Throw(errorCode, "%s", buffer.GetData());
		}
		else
		{
			Throw(errorCode, "%s", *msg);
		}
	}

	[[noreturn]] void Throw(const FString& msg)
	{
		Throw(0, msg);
	}

	void SecureCopyStringToBuffer(WIDECHAR* destination, size_t capacity, const FString& s)
	{
		if (s.Len() >= capacity)
		{
			Throw(FString::Printf(TEXT("SecureCopyStringToBuffer(%s) failed. Insufficient capacity"), *s));
		}

		for (size_t i = 0; i < s.Len(); ++i)
		{
			destination[i] = s[i];
		}

		destination[s.Len()] = 0;
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
		SecureCopyStringToBuffer(path.buf, U8FilePath::CAPACITY, src);
	}
}

namespace Rococo::IO
{
	ROCOCO_API void GetCurrentDirectoryPath(U8FilePath& path)
	{
		FString launchDir = FPaths::LaunchDir();
		Rococo::Populate(path, launchDir);
	}

	ROCOCO_API void ToSysPath(char* path)
	{
		char directorySeparator = DirectorySeparatorChar();

		for (char* s = path; *s != 0; ++s)
		{
			if (*s == '/' || *s == '\\') *s = directorySeparator;
		}
	}

	ROCOCO_API void ToSysPath(WIDECHAR* path)
	{
		WIDECHAR directorySeparator = DirectorySeparatorChar();

		for (auto* s = path; *s != 0; ++s)
		{
			if (*s == '/' || *s == '\\') *s = directorySeparator;
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

	ROCOCO_API bool IsFileExistant(crwstr filename)
	{
		FString filePath(filename);
		return FPaths::FileExists(filePath);
	}

	ROCOCO_API bool StripLastSubpath(char* fullpath)
	{
		int32 len = (int32)Strings::StringLength(fullpath);
		for (int i = len - 2; i > 0; --i)
		{
			if (fullpath[i] == '/' || fullpath[i] =='\\')
			{
				fullpath[i + 1] = 0;
				return true;
			}
		}

		return false;
	}

	ROCOCO_API bool StripLastSubpath(WIDECHAR* fullpath)
	{
		int32 len = (int32)Strings::StringLength(fullpath);
		for (int i = len - 2; i > 0; --i)
		{
			if (fullpath[i] == '/' || fullpath[i] == '\\')
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

	ROCOCO_API bool MakeContainerDirectory(WIDECHAR* filename)
	{
		int len = (int)Strings::StringLength(filename);

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
		return *FPlatformMisc::GetDefaultPathSeparator();
	}

	void CreateDirectoryFolder(const FString& path)
	{
		IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Directory Exists?
		if (!platformFile.DirectoryExists(*path))
		{
			SetLastError(0);
			if (!platformFile.CreateDirectory(*path))
			{
				Throw(GetLastError(), FString::Printf(TEXT("Cannot create directory %s"), *path));
			}
		}
	}

	ROCOCO_API void CreateDirectoryFolder(const WideFilePath& path)
	{
		FString sPath(path.buf);
		CreateDirectoryFolder(sPath);
	}

	ROCOCO_API void CreateDirectoryFolder(const U8FilePath& path)
	{
		FString sPath(path.buf);
		CreateDirectoryFolder(sPath);
	}

	ROCOCO_API IBinaryArchive* CreateNewBinaryFile(const TCHAR* sysPath)
	{
		struct BinArchive : IBinaryArchive
		{		
			IFileHandle* file = nullptr;
			FString filename;

			BinArchive(const TCHAR* sysPath): filename(sysPath)
			{
				if (sysPath == nullptr) Throw(0, "%s: null sysPath", __ROCOCO_FUNCTION__);

				SetLastError(0);
				file = IPlatformFile::GetPlatformPhysical().OpenWrite(sysPath);
				if (file == nullptr)
				{
					Throw(GetLastError(), FString::Printf(TEXT("Error creating file: %s"), sysPath));
				}
			}

			virtual ~BinArchive()
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
				SetLastError(0);
				if (!file->Seek((int64)position))
				{
					Throw(GetLastError(), FString::Printf(TEXT("%hs: %s - Seek failed"), __ROCOCO_FUNCTION__, *filename));
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
					Throw(FString::Printf(TEXT("%hs: %s - Could not write data. It was greater than 4GB in length"), __ROCOCO_FUNCTION__, *filename));
				}

				SetLastError(0);
				if (!file->Write((const uint8*)pElements, (int64) nTotalBytes))
				{
					Throw(GetLastError(), FString::Printf(TEXT("%hs: %s - Could not write data. Disk full?"), __ROCOCO_FUNCTION__, *filename));
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
				if (sysPath == nullptr) Throw(0, "%s: null sysPath", __ROCOCO_FUNCTION__);

				SetLastError(0);
				file = IPlatformFile::GetPlatformPhysical().OpenRead(sysPath);
				if (file == nullptr)
				{
					Throw(GetLastError(), FString::Printf(TEXT("Error opening file for read: %s"), sysPath));
				}
			}

			virtual ~BinFile()
			{
				delete file;
			}

			uint32 Read(uint32 bytesToRead, void* pElements) override
			{
				SetLastError(0);
				if (!file->Read((uint8*)pElements, (int64)bytesToRead))
				{
					Throw(GetLastError(), FString::Printf(TEXT("Error reading file: %s"), *filename));
				}
				
				return bytesToRead;
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
				Throw(0, "%hs: file handle nullptr", __ROCOCO_FUNCTION__);
			}
			return file;
		}
	};

	ROCOCO_API IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(crwstr sysPath)
	{
		// On Unreal we don't do file mapping as not all platforms may support it, so just open the whole file into RAM
		struct BinMapping : IReadOnlyBinaryMapping
		{
			TArray<uint8> dataMap;

			BinMapping(crwstr sysPath)
			{
				SetLastError(0);
				AutoFile file(IPlatformFile::GetPlatformPhysical().OpenRead(sysPath));
				if (file.file == nullptr)
				{
					Throw(GetLastError(), FString::Printf(TEXT("%hs: Error opening file for read %s"), __ROCOCO_FUNCTION__, sysPath));
				}
			
				int64 len = file->Size();

				if (len > (int64) 4_gigabytes)
				{
					Throw(GetLastError(), FString::Printf(TEXT("%hs: Error opening file for read %s. File > 4GB"), __ROCOCO_FUNCTION__, sysPath));
				}

				dataMap.SetNum(len, EAllowShrinking::Yes);

				if (!file->Read(dataMap.GetData(), len))
				{
					Throw(GetLastError(), FString::Printf(TEXT("%hs: Error reading data from %s"), __ROCOCO_FUNCTION__, sysPath));
				}
			}

			virtual ~BinMapping()
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
			Rococo::Strings::Format(path, "%s", slash + 1);
		}
	}

	ROCOCO_API void GetExePath(U8FilePath& path)
	{
		FString exeName(FPlatformProcess::ExecutablePath());

		TArray<uint8> buffer;
		ConvertFStringToUTF8Buffer(buffer, exeName);

		if (buffer.Num() >= path.CAPACITY)
		{
			Throw(FString::Printf(TEXT("Cannot GetExePath - filename '%s' too long"), *exeName));
		}

		FMemory::Memcpy(path.buf, buffer.GetData(), buffer.NumBytes());
	}

	ROCOCO_API void GetExePath(WideFilePath& path)
	{
		FString exeName(FPlatformProcess::ExecutablePath());
		Strings::Format(OUT path, TEXT("%s"), *exeName);
	}

	void GetContentDirectory(const TCHAR* contentIndicatorName, WideFilePath& path, IOS& os)
	{
		FString gameRelativeDir = FPaths::ProjectContentDir();
		FString gameDir = FPaths::ConvertRelativePathToFull(gameRelativeDir);

		static_assert(sizeof(TCHAR) == sizeof(WIDECHAR));

		Strings::SecureFormat(path.buf, TEXT("%s"), *gameDir);

		FString contentIndicator(contentIndicatorName);

		if (contentIndicator.Find(TEXT("\\")) > 0 || contentIndicator.Find(TEXT("/")) > 0)
		{
			// The indicator is part of a path
			if (os.IsFileExistant(contentIndicatorName))
			{
				Populate(path, contentIndicator);
				IO::MakeContainerDirectory(path.buf);
				IO::EndDirectoryWithSlash(path.buf, WideFilePath::CAPACITY);
				IO::NormalizePath(path);
				return;
			}
		}

		size_t len = Strings::StringLength(path);

		while (len > 0)
		{
			FString indicator = FString::Printf(TEXT("%s%s"), path.buf, contentIndicatorName);
			if (os.IsFileExistant(*indicator))
			{
				U8FilePath subDir;
				IO::LoadAsciiTextFile(subDir.buf, subDir.CAPACITY, *indicator);

				FString sSubDir(subDir);
				FString rootDir(path.buf);

				FString fullDir = FPaths::Combine(rootDir, sSubDir);

				if (!FPaths::DirectoryExists(fullDir))
				{
					Throw(FString::Printf(TEXT("RococoOS: Could not find %s. File path was specified in %s"), *fullDir, *indicator));
				}

				Populate(path, fullDir);
				IO::EndDirectoryWithSlash(path.buf, WideFilePath::CAPACITY);
				IO::NormalizePath(path);
				return;
			}

			IO::MakeContainerDirectory(path.buf);

			size_t newLen = Strings::StringLength(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(FString::Printf(TEXT("Could not find %s at or below the game content folder '%s'"), contentIndicatorName, *gameDir));
	}

	/*
	ROCOCO_API bool IsKeyPressed(int vkeyCode)
	{
		Throw(0, "IO::IsKeyPressed not supported by Rococo for Unreal Engine");
	}
	*/

	ROCOCO_API void PasteFromClipboard(char* asciiBuffer, size_t capacity)
	{
		FString pastedText;
		FPlatformApplicationMisc::ClipboardPaste(OUT pastedText);

		if constexpr (sizeof(TCHAR) > 1)
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
		crwstr a = this->sysPath;
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
		FPlatformApplicationMisc::ClipboardPaste(OUT pastedText);

		if constexpr (sizeof(TCHAR) > 1)
		{
			TArray<uint8> buffer;
			ConvertFStringToUTF8Buffer(OUT buffer, pastedText);
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
			void* buffer = malloc(sizeof(CriticalSection));
			return new (buffer) CriticalSection(src);
		}
	}

	ROCOCO_API IdThread GetCurrentThreadIdentifier()
	{
		return (IdThread)FPlatformTLS::GetCurrentThreadId();
	}

	/*
	ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow)
	{
		Throw(0, "OS::SetCursorVisibility not supported by Rococo for Unreal Engine");
	}

	ROCOCO_API void EditImageFile(Rococo::Windows::IWindow& window, crwstr sysPath)
	{
		Throw(0, "OS::EditImageFile not supported by Rococo for Unreal Engine");
	}

	ROCOCO_API void SleepUntilAsync(uint32 timeoutMS)
	{
		Throw(0, "OS::SleepUntilAsync not supported by Rococo for Unreal Engine");
	}

	static crwstr notePadPP = L"C:\\Program Files\\Notepad++\\notepad++.exe";

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
	bool SpawnChildProcessAsync(crwstr executable, crwstr commandLineArgs)
	{
		std::vector<WIDECHAR> commandLine;
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
	void SpawnIndependentProcess(HWND hMsgSink, crwstr executable, crwstr commandLine)
	{
		auto result = (INT_PTR)ShellExecuteW(hMsgSink, L"open", executable, commandLine, NULL, SW_SHOW);
		if (result < 32)
		{
			Throw(GetLastError(), "Error spawning [%s: '%s']", __ROCOCO_FUNCTION__, executable, commandLine);
		}
	}

	bool OpenNotepadPP(HWND hWndMessageSink, cstr documentFilePath, int lineNumber)
	{
		WIDECHAR commandLine[1024];
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
			Throw(GetLastError(), "%s: '%s'", __ROCOCO_FUNCTION__, documentFilePath);
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
		return FPlatformMisc::IsDebuggerPresent();
	}

	ROCOCO_API void TripDebugger()
	{
		UE_DEBUG_BREAK();
	}

#ifdef _WIN32
	ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode)
	{
		enum { EFORMAT_MESSAGE_FROM_SYSTEM = 0x00001000 };

		int nChars = snprintf(message, sizeofBuffer, "Error %d (0x%X): ", errorCode, errorCode);

		if (nChars < sizeofBuffer)
		{
			DWORD code = FormatMessageA(EFORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, message + nChars, sizeofBuffer - nChars, nullptr);
			if (code == 0)
			{
				snprintf(message, sizeofBuffer, "Error %d (0x%X)", errorCode, errorCode);
			}
		}
	}
#else
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
#endif
}

namespace Rococo
{
	ROCOCO_API MemoryUsage ProcessMemory()
	{
		auto& constants = FPlatformMemory::GetConstants();
		auto stats = FPlatformMemory::GetStats();

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
		stringmap<HString> macroToSubdir;
	public:
		Installation(crwstr contentIndicatorName, IOS& _os) : os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);

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

			Rococo::Strings::Format(contentDirectory, TEXT("%s"), contentPath);
		}

		virtual ~Installation()
		{

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
				HString macro;
				HString subpath;

				bool operator < (const MacroToSubpath& other) const
				{
					return other.macro.length() - other.subpath.length() > macro.length() - subpath.length();
				}
			};

			std::vector<MacroToSubpath> macros;
			for (auto& i : macroToSubdir)
			{
				macros.push_back({ (cstr) i.first, i.second });
			}

			std::sort(macros.begin(), macros.end()); // macros is now sorted in order of macro length

			for (auto& m : macros)
			{
				if (StartsWith(pingPath, m.subpath.c_str()))
				{
					Rococo::Strings::Format(compressedPath, "%s/%s", m.macro.c_str(), pingPath + m.subpath.length());
					return;
				}
			}

			Rococo::Strings::Format(compressedPath, "%s", pingPath);
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
			size_t sysPathLength = StringLength(sysPath);
			SecureFormat(sysPath.buf + sysPathLength, sysPath.CAPACITY - sysPathLength, TEXT("packages/%hs/%hs"), packName, dir);
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

				Rococo::Strings::Format(sysPath, TEXT("%ls%hs"), contentDirectory, subdir);
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

				Rococo::Strings::Format(sysPath, TEXT("%ls%hs%hs"), contentDirectory, macroDir + 1, subdir);
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

		void ConvertSysPathToMacroPath(crwstr sysPath, U8FilePath& pingPath, cstr macro) const override
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

			Rococo::Strings::Format(pingPath, "%s/%s", macro, fullPingPath.buf + i->second.length());
		}

		void ConvertSysPathToPingPath(crwstr sysPath, U8FilePath& pingPath) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			size_t contentDirLength = StringLength(contentDirectory);

			if (!Strings::Compare(sysPath, contentDirectory.buf, contentDirLength))
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' did not begin with the content folder %ls", sysPath, contentDirectory.buf);
			}

			if (FindSubstring(sysPath, TEXT("..")) != nullptr)
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' - Illegal sequence in ping path: '..'", sysPath);
			}

			Rococo::Strings::Format(pingPath, "!%ls", sysPath + contentDirLength);

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

			Rococo::Strings::Format(expandedPath, "%s%s", i->second.c_str(), subdir);
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
			int len = Rococo::Strings::Format(pingRoot, "%s", pingFolder);
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

		virtual ~UE5OS()
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

		void OnModified(crwstr filename)
		{
		}

		bool IsFileExistant(cstr absPath) const override
		{
			return IO::IsFileExistant(absPath);
		}

		bool IsFileExistant(crwstr absPath) const override
		{
			return IO::IsFileExistant(absPath);
		}

		void ConvertUnixPathToSysPath(crwstr unixPath, WideFilePath& sysPath) const override
		{
			if (unixPath == nullptr) Throw(0, "Blank path in call to os.ConvertUnixPathToSysPath");

			size_t len = StringLength(unixPath);

			if (len >= sysPath.CAPACITY)
			{
				Throw(0, "Path too long in call to os.ConvertUnixPathToSysPath");
			}

			size_t i = 0;
			for (; i < len; ++i)
			{
				WIDECHAR c = unixPath[i];

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

		void LoadAbsolute(crwstr absPath, IExpandingBuffer& buffer, int64 maxFileLength) const override
		{
			SetLastError(0);
			AutoFile file(IPlatformFile::GetPlatformPhysical().OpenRead(absPath));
			if (file.file == nullptr)
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAbsolute failed: Error opening file %s"), absPath));
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

			buffer.Resize(fileSize + 1); // This gives us space for a nul terminating character
			buffer.Resize(fileSize);

			uint8* data = (uint8*)buffer.GetData();
			data[fileSize] = 0;

			if (!file->Read(data, fileSize))
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAbsolute failed: Error attempting to read %lld bytes from file <%s>"), fileSize, absPath));
			}
		}

		bool TryLoadAbsolute(crwstr absPath, ILoadEventsCallback& cb, ErrorCode& sysErrorCode) const override
		{
			sysErrorCode = (ErrorCode)0;

			SetLastError(0);

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
						return __ROCOCO_FUNCTION__;
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
					crwstr absPath;

					Reader(IFileHandle* _hFile, crwstr _absPath) : hFile(_hFile), absPath(_absPath) {}

					void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
					{
						int64 prePos = hFile->Tell();

						if (!hFile->Read((uint8*)buffer, (int64)bytesRead))
						{
							Throw(GetLastError(), FString::Printf(TEXT("TryLoadAbsolute(%s) - read failed"), absPath));
						}

						int64 postPos = hFile->Tell();

						int64 delta = postPos - prePos;
						if (delta < 0 || delta > 0x00000000FFFFFFFF)
						{
							Throw(GetLastError(), FString::Printf(TEXT("TryLoadAbsolute(%s) - unexpected file position"), absPath));
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

		void LoadAbsolute(crwstr absPath, ILoadEventsCallback& cb) const override
		{
			SetLastError(0);
			AutoFile hFile = IPlatformFile::GetPlatformPhysical().OpenRead(absPath);
			if (hFile.file == nullptr) Throw(GetLastError(), FString::Printf(TEXT("LoadAbsolute(%ls) failed: Error opening file"), absPath));

			int64 fileLength = hFile->Size();

			cb.OnFileOpen(fileLength);

			struct Reader : ILoadEventReader
			{
				IFileHandle* hFile;
				crwstr absPath;

				Reader(IFileHandle* _hFile, crwstr _absPath) : hFile(_hFile), absPath(_absPath) {}

				void ReadData(void* buffer, uint32 capacity, uint32& bytesRead) override
				{
					int64 prePos = hFile->Tell();

					if (!hFile->Read((uint8*)buffer, (int64)capacity))
					{
						Throw(GetLastError(), FString::Printf(TEXT("TryLoadAbsolute(%s) - read failed"), absPath));
					}

					int64 postPos = hFile->Tell();

					int64 delta = postPos - prePos;
					if (delta < 0 || delta > 0x00000000FFFFFFFF)
					{
						Throw(GetLastError(), FString::Printf(TEXT("TryLoadAbsolute(%s) - unexpected file position"), absPath));
					}

					bytesRead = (uint32)delta;
				}
			} reader(hFile.file, absPath);

			cb.OnDataAvailable(reader);
		}

		void GetBinDirectoryAbsolute(WideFilePath& directory) const override
		{
			Rococo::Strings::Format(directory, TEXT("%s"), binDirectory);
		}

		size_t MaxPath() const override
		{
			return _MAX_PATH;
		}

		void Monitor(crwstr /* absPath */) override
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

	ROCOCO_API IInstallationSupervisor* CreateInstallation(crwstr contentIndicatorName, IOS& os)
	{
		return new UE5_ANON::Installation(contentIndicatorName, os);
	}

	ROCOCO_API IInstallationSupervisor* CreateInstallationDirect(crwstr contentDirectory, IOS& os)
	{
		WIDECHAR slash[2] = { 0 };
		slash[0] = Rococo::IO::GetFileSeparator();

		if (!Rococo::Strings::EndsWith(contentDirectory, slash))
		{
			Throw(0, "Content %ls did not end with %ls", contentDirectory, slash);
		}

		bool assumeAbsolute = false;

		size_t len = StringLength(contentDirectory);
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
			Rococo::Strings::Format(absPath, TEXT("%hs\\%ls"), currentDir.buf, contentDirectory);

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
			virtual ~AppControl()
			{

			}

			void ShutdownApp() override
			{
				isRunning = false;
				FPlatformMisc::RequestExit(false, TEXT("AppControl::ShutdownApp"));
			}

			bool IsRunning() const
			{
				return isRunning;
			}

			void Free() override
			{
				delete this;
			}

			Rococo::Tasks::ITaskQueue& MainThreadQueue() override
			{
				return *this;
			}

			void AddTask(Rococo::Function<void()> lambda) override
			{
				Throw(0, "Not implemented for UE5");
			}

			bool ExecuteNext() override
			{
				return false;
			}

			void AdvanceSysMonitors() override
			{
			}

			bool isRunning = true;

			void AddSysMonitor(IO::ISysMonitor& monitor) override
			{
				Throw(0, "Not implemented for UE5");
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
		Strings::SafeVFormat(buffer, sizeof(buffer), format, arglist);

		FString tcharBuffer(buffer);

		FMsg::Logf(__FILE__, __LINE__, RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
	}

	ROCOCO_API cstr GetCommandLineText()
	{
		static TArray<uint8> utf8buffer;
		if (utf8buffer.Num() == 0)
		{
			FString cmdLine = FCommandLine::Get();
			ConvertFStringToUTF8Buffer(OUT utf8buffer, cmdLine);
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
		ConvertFStringToUTF8Buffer(OUT utf8buffer, user);

		populator.Populate((cstr) utf8buffer.GetData());
	}

	ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack)
	{
		StackStringBuilder sb(buffer, capacity);

		if (ex.ErrorCode() != 0)
		{
			char errMsg[256];
			FormatErrorMessage(errMsg, sizeof(errMsg), ex.ErrorCode());
			sb.AppendFormat("%s\n", errMsg);
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

	ROCOCO_API void SaveClipBoardText(cstr text, Rococo::Windows::IWindow& window)
	{
		// The UE5 API uses a global window reference for clipboard Copy, so ignores the argument
		UNUSED(window);

		FString sText(text);
		FPlatformApplicationMisc::ClipboardCopy(*sText);
	}

	ROCOCO_API void CopyExceptionToClipboard(IException& ex, Rococo::Windows::IWindow& window)
	{
		TArray<char> buffer;
		buffer.SetNum(128_kilobytes);
		BuildExceptionString(buffer.GetData(), buffer.Num(), ex, true);
		SaveClipBoardText(buffer.GetData(), window);
	}
}

namespace Rococo::IO
{
	ROCOCO_API void EnsureUserDocumentFolderExists(crwstr subdirectory)
	{
		if (subdirectory == nullptr || *subdirectory == 0)
		{
			Throw(0, "%s: subdirectory argument was blank", __ROCOCO_FUNCTION__);
		}

		FString sSubDirectory(subdirectory);

		if (sSubDirectory.Find(TEXT(".")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '.'", __ROCOCO_FUNCTION__, subdirectory);
		}

		if (sSubDirectory.Find(TEXT("%")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '%'", __ROCOCO_FUNCTION__, subdirectory);
		}

		if (sSubDirectory.Find(TEXT("$")) > 0)
		{
			Throw(0, "%s: subdirectory %ls contained an illegal character '$'", __ROCOCO_FUNCTION__, subdirectory);
		}

		if (subdirectory[0] == '\\' || subdirectory[0] == '/')
		{
			Throw(0, "%s: subdirectory %ls must not begin with a slash character '\\'", __ROCOCO_FUNCTION__, subdirectory);
		}
		
		FString userDocs = FPlatformProcess::UserDir();

		FString fullPath = FPaths::Combine(userDocs, subdirectory);

		if (fullPath.Len() > _MAX_PATH)
		{
			Throw(FString::Printf(TEXT("Path too long: %s"), *fullPath));
		}

		IO::CreateDirectoryFolder(*fullPath);
	}

	FString GetFullPathFromTarget(TargetDirectory target, crwstr relativePath)
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

	ROCOCO_API void SaveAsciiTextFileIfDifferent(TargetDirectory target, crwstr filename, const fstring& text)
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

	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, crwstr filename, const fstring& text)
	{
		if (text.length > 1024_megabytes)
		{
			Throw(0, "Rococo::IO::SaveAsciiTextFile(%ls): Sanity check. String was > 1 gigabyte in length", filename);
		}

		FString fullPath = GetFullPathFromTarget(target, filename);

		SetLastError(0);

		AutoFile file(IPlatformFile::GetPlatformPhysical().OpenWrite(*fullPath));
		if (!file.file)
		{
			Throw(GetLastError(), FString::Printf(TEXT("Cannot create file %ls"), filename));
		}
		
		if (!file->Write((const uint8*)text.buffer, (int64)text.length))
		{
			Throw(GetLastError(), "Rococo::IO::SaveAsciiTextFile(%ls): write failed", filename);
		}
	}

	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, cstr filename, const fstring& text)
	{
		WideFilePath wPath;
		Assign(wPath, filename);
		SaveAsciiTextFile(target, wPath, text);
	}

	ROCOCO_API void GetUserPath(WIDECHAR* result, size_t capacity, cstr shortname)
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

		SetLastError(0);
		AutoFile file(IPlatformFile::GetPlatformPhysical().OpenWrite(*fullPath));
		if (!file.file)
		{
			Throw(GetLastError(), FString::Printf(TEXT("Cannot write to %s"), *fullPath));
		}

		file->Write((const uint8*)s, strlen(s));
	}

	ROCOCO_API bool IsDirectory(crwstr filename)
	{
		FString sFilename(filename);
		return IPlatformFile::GetPlatformPhysical().DirectoryExists(*sFilename);
	}

	ROCOCO_API bool IsDirectory(cstr filename)
	{
		FString sFilename(filename);
		return IPlatformFile::GetPlatformPhysical().DirectoryExists(*sFilename);
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

	ROCOCO_API void EndDirectoryWithSlash(WIDECHAR* pathname, size_t capacity)
	{
		crwstr finalChar = GetFinalNull(pathname);

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

			WIDECHAR* mutablePath = const_cast<WIDECHAR*>(finalChar);
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

	ROCOCO_API void LoadBinaryFile(IBinaryFileLoader& loader, crwstr filename, uint64 maxLength)
	{
		if (maxLength > 2_gigabytes)
		{
			Throw(0, "Max file length is 2 gigabytes");
		}


		{ // File is locked in this codesection
			SetLastError(0);
			AutoFile f( IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadBinaryFile: Cannot open file %ls"), filename));
			}

			uint64 fileLength = (uint64) f->Size();

			if (maxLength != 0 && fileLength >= maxLength)
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
				Throw(ex.ErrorCode(), FString::Printf(TEXT("LoadBinaryFile %hs.\n%ls"), ex.Message(), filename));
			}

		} // File is no longer locked
	}

	ROCOCO_API void LoadAsciiTextFile(Strings::IStringPopulator& onLoad, crwstr filename)
	{
		TArray<char> asciiData;

		{ // File is locked in this codesection
			SetLastError(0);
			AutoFile f(IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAsciiTextFile: Cannot open file %ls"), filename));
			}

			int64 fileLength = f->Size();

			if (fileLength >= (int64)2048_megabytes)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile(%ls): Cannot open file length must be less than 2GB.\n"), filename));
			}

			asciiData.SetNum(fileLength + 1);

			if (!f->Read((uint8*)asciiData.GetData(), fileLength))
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAsciiTextFile: Cannot read file %ls"), filename));
			}

		} // File is no longer locked

		onLoad.Populate(asciiData.GetData());
	}

	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, crwstr filename)
	{
		if (capacity >= 2048_megabytes)
		{
			Throw(FString::Printf(TEXT("LoadAsciiTextFile: capacity must be less than 2GB.\n%ls"), filename));
		}

		{ // File is locked in this codesection
			SetLastError(0);
			AutoFile f(IPlatformFile::GetPlatformPhysical().OpenRead(filename));
			if (f.file == nullptr)
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAsciiTextFile: Cannot open file %ls"), filename));
			}

			int64 fileLength = f->Size();

			if (fileLength >= (int64) capacity)
			{
				Throw(FString::Printf(TEXT("LoadAsciiTextFile(%ls): Cannot open file length must be less than %llu bytes.\n"), filename, capacity));
			}

			if (!f->Read((uint8*)data, fileLength))
			{
				Throw(GetLastError(), FString::Printf(TEXT("LoadAsciiTextFile: Cannot read file %ls"), filename));
			}

			data[fileLength] = 0;

			return (size_t) fileLength;
		} // File is no longer locked
	}

	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, cstr filename)
	{
		WideFilePath wPath;
		Assign(wPath, filename);

		return LoadAsciiTextFile(data, capacity, wPath);
	}

	ROCOCO_API void SaveBinaryFile(crwstr targetPath, const uint8* buffer, size_t nBytes)
	{
		if (nBytes > 2_gigabytes)
		{
			Throw(0, "Cannot open %ls for writing. Buffer length > 2GB", targetPath);
		}

		SetLastError(0);
		AutoFile f(IPlatformFile::GetPlatformPhysical().OpenWrite(targetPath));
		if (f.file == nullptr)
		{
			Throw(GetLastError(), FString::Printf(TEXT("SaveBinaryFile: Cannot create file %ls"), targetPath));
		}

		if (!f->Write(buffer, nBytes))
		{
			Throw(GetLastError(), "Cannot write to file %ls", targetPath);
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
		start(0), end(0), name(_name)
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

#ifdef _WIN32
	ROCOCO_API void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		ctime_s(str, 26, &t);
	}

#else
	enum {TIMESTAMP_LEN = 26 };

	ROCOCO_API void GetTimestamp(char str[TIMESTAMP_LEN])
	{
		time_t t;
		time(&t);
		char* timeBuffer = ctime(&t);
		SafeFormat(str, TIMESTAMP_LEN, "%s", timeBuffer);
	}
#endif
} // Rococo::Time

#ifdef _WIN32
namespace Rococo::Windows
{
	ROCOCO_API int32 WheelDeltaToScrollLines(int32 wheelDelta, bool& scrollByPage)
	{
		enum { ESPI_GETWHEELSCROLLLINES = 0x0068, EWHEEL_PAGESCROLL = 0xFFFFFFFF };
		unsigned long linesPerWheelDelta = 1;
		SystemParametersInfoW(ESPI_GETWHEELSCROLLLINES, 0, &linesPerWheelDelta, 0);
		if (linesPerWheelDelta == EWHEEL_PAGESCROLL)
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

		return wheelDelta * (int32)linesPerWheelDelta;
	}
}
#else
namespace Rococo::Windows
{
	ROCOCO_API int32 WheelDeltaToScrollLines(int32 wheelDelta, bool& scrollByPage)
	{
		scrollByPage = false;

		if (wheelDelta != 0 && (wheelDelta % 120) == 0)
		{
			// Assume a windows stanard wheel, in which deltas are in units of 120
			wheelDelta = wheelDelta / 120;
		}

		int32 linesPerWheelDelta = 1;
		return wheelDelta * linesPerWheelDelta;
	}
}
#endif

namespace Rococo::Debugging
{
	ROCOCO_API int Log(cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char message[1024];
		int len = Strings::SafeVFormat(message, sizeof(message), format, args);
		
		FString tcharBuffer(message);
		FMsg::Logf(__FILE__, __LINE__, OS::RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
		return len;
	}
}

#ifndef _WIN32
namespace Rococo::Strings
{
	ROCOCO_API int SafeVFormat(WIDECHAR* buffer, size_t capacity, const WIDECHAR* format, va_list args)
	{
		return TCString<WIDECHAR>::GetVarArgs(buffer, capacity, format, args);
	}

	ROCOCO_API bool EqI(const WIDECHAR* a, const WIDECHAR* b)
	{
		return FCStringWide::Strcmp(a, b) == 0;
	}
}
#endif

#ifdef _DEBUG
namespace Rococo::Strings
{
	ROCOCO_API int PrintD(const char* format, ...)
	{
		char message[2048];

		va_list args;
		va_start(args, format);
		int len = SafeVFormat(message, sizeof(message), format, args);

		FString tcharBuffer(message);
		FMsg::Logf(__FILE__, __LINE__, OS::RococoLog, ELogVerbosity::Type::Log, TEXT("%s"), *tcharBuffer);
		return len;
	}
}
#endif

#ifdef _WIN32
namespace Rococo::Windows
{
	ROCOCO_INTERFACE IWindow
	{
	   virtual operator HWND () const = 0;
	};
}

namespace Rococo::OS
{
	struct ActiveWindow: Windows::IWindow
	{
		operator HWND () const override
		{
			return GetActiveWindow();
		}
	} s_ActiveWindow;

	ROCOCO_API Windows::IWindow& WindowOwner()
	{
		return s_ActiveWindow;
	}
}

#else

namespace Rococo::Windows
{
	ROCOCO_INTERFACE IWindow
	{
	   virtual void Nop() const = 0;
	};
}

namespace Rococo::OS
{
	struct NoWindow : Windows::IWindow
	{
		void Nop() const override
		{

		}
	} s_noWindow;
	
	Windows::IWindow& WindowOwner()
	{
		return s_noWindow;
	}
}

#endif


using namespace Rococo;

void RunRococoOSTests()
{
	char timestamp[26];
	Time::GetTimestamp(timestamp);

	char myPath[16] = "/a/b/c";
	IO::ToSysPath(myPath);
	IO::ToUnixPath(myPath);

	if (strcmp(myPath, "/a/b/c") != 0)
	{
		check(false);
	}
}
#pragma once

#include <rococo.types.h>

namespace Rococo::Tasks
{
	struct ITaskQueue;
}

namespace Rococo::OS
{
	struct ConfigSection
	{
		cstr sectionName;
	};

	struct ConfigRootName
	{
		cstr rootName;
	};

	ROCOCO_API void PasteStringFromClipboard(IEventCallback<cstr>& populator);
	ROCOCO_API void CopyStringToClipboard(cstr text);

	// Gets a null terminated OS config string with lenBytes capacity. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	ROCOCO_API void GetConfigVariable(char* textBuffer, size_t lenBytes, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr);

	// Gets U8 file path from the OS config. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	inline void GetConfigVariable(U8FilePath& path, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr)
	{
		GetConfigVariable(path.buf, path.CAPACITY, defaultValue, section, rootName, organization);
	}

	// Sets the value of a config string in the OS. The maximum length is 1 megabyte.
	ROCOCO_API void SetConfigVariable(cstr value, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr);

	ROCOCO_INTERFACE IAppControl
	{
		// Returns the task queue for the main thread. Include <rococo.task.queue.h> for the definition of the full interface
		virtual Tasks::ITaskQueue& MainThreadQueue() = 0;
		virtual bool IsRunning() const = 0;
		virtual void ShutdownApp() = 0;
	};

	ROCOCO_INTERFACE IAppControlSupervisor : public IAppControl
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IAppControlSupervisor* CreateAppControl();

	ROCOCO_API void BeepWarning();
	ROCOCO_API void CopyExceptionToClipboard(IException& ex);
	ROCOCO_API void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath);

	struct IThreadControl;

	ROCOCO_INTERFACE IThreadJob
	{
		virtual uint32 RunThread(IThreadControl & control) = 0;
	};

	ROCOCO_INTERFACE IThreadControl : public ILock
	{
		typedef void (*FN_APC)(void* context);
		virtual void QueueAPC(FN_APC apc, void* context) = 0;
		virtual bool IsRunning() const = 0;
		virtual void Resume() = 0;
		virtual void SetRealTimePriority() = 0;
		virtual void SleepUntilAysncEvent(uint32 milliseconds) = 0;
		virtual cstr GetErrorMessage(int& err) const = 0;
	};

	using IdThread = int64;
	ROCOCO_API IdThread GetCurrentThreadIdentifier();

	ROCOCO_API void WakeUp(IThreadControl& thread);

	ROCOCO_INTERFACE ICriticalSection
	{
		virtual void Free() = 0;
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
	};

	ROCOCO_INTERFACE IThreadSupervisor : public IThreadControl
	{
		virtual ICriticalSection * CreateCriticalSection() = 0;
		virtual void Free() = 0;
	};

	ROCOCO_API [[nodiscard]] IThreadSupervisor* CreateRococoThread(IThreadJob* thread, uint32 stacksize);


	ROCOCO_API [[nodiscard]] void* AllocBoundedMemory(size_t nBytes);
	ROCOCO_API void FreeBoundedMemory(void* pMemory);

	enum TargetDirectory
	{
		TargetDirectory_UserDocuments = 0,
		TargetDirectory_Root
	};

	ROCOCO_API void EnsureUserDocumentFolderExists(const wchar_t* subdirectory);
	ROCOCO_API void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text);

	ROCOCO_API void FormatTime(ticks utcTime, char* buffer, size_t nBytes);
	ROCOCO_API bool StripLastSubpath(wchar_t* fullpath);
	ROCOCO_API bool IsFileExistant(const wchar_t* path);
	ROCOCO_API void Format_C_Error(int errorCode, char* buffer, size_t capacity);
	ROCOCO_API [[nodiscard]] int OpenForAppend(void** fp, cstr name);
	ROCOCO_API [[nodiscard]] int OpenForRead(void** fp, cstr name);
	ROCOCO_API void UILoop(uint32 milliseconds);
	ROCOCO_API void ToSysPath(wchar_t* path);
	ROCOCO_API void ToUnixPath(wchar_t* path);
	ROCOCO_API void ToSysPath(char* path);
	ROCOCO_API void ToUnixPath(char* path);
	ROCOCO_API void SanitizePath(char* path);
	ROCOCO_API void SanitizePath(wchar_t* path);
	ROCOCO_API void SaveClipBoardText(cstr text, Windows::IWindow& window);
	ROCOCO_API bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window);
	ROCOCO_API cstr GetAsciiCommandLine();

	template<typename Type, typename IDType = typename Type::IDType>
	class Mappings;

	// Open a file and fit into buffer. In the case of a truncation an IException is thrown. The function returns the number of bytes copied to the buffer.
	ROCOCO_API size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename);
	ROCOCO_API void LoadAsciiTextFile(IEventCallback<cstr>& callback, const wchar_t* filename);
	ROCOCO_API void PollKeys(uint8 scanArray[256]);
	ROCOCO_API void MakeContainerDirectory(char* filename);
	ROCOCO_API void MakeContainerDirectory(wchar_t* filename);

	class Lock
	{
	private:
		ICriticalSection* sync;
	public:
		Lock(ICriticalSection* pSync): sync(pSync)
		{
			pSync->Lock();
		}

		~Lock()
		{
			sync->Unlock();
		}
	};

	ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow);
	ROCOCO_API void ShellOpenDocument(cstr path);
	ROCOCO_API void TripDebugger();
	ROCOCO_API void PrintDebug(const char* format, ...);	
	ROCOCO_API [[nodiscard]] bool IsDebugging();
	ROCOCO_API void BreakOnThrow(BreakFlag flag);
	ROCOCO_API void SetBreakPoints(int flags);
	ROCOCO_API [[nodiscard]] ticks CpuTicks();
	ROCOCO_API [[nodiscard]] ticks CpuHz();
	ROCOCO_API [[nodiscard]] ticks UTCTime();
	ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode);
	ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack);
	ROCOCO_API cstr GetCommandLineText();
}

namespace Rococo::Windows
{
	ROCOCO_WINDOWS_API IWindow& NoParent();
	ROCOCO_WINDOWS_API void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr caption);
	ROCOCO_WINDOWS_API int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 uType);
}


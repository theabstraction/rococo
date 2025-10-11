// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo::Tasks
{
	struct ITaskQueue;
}

namespace Rococo
{
	// Interface that indicates a section of code was aborted without error
	ROCOCO_INTERFACE INoErrorException : IException
	{
	};

	// Quit a section without error
	ROCOCO_API void ThrowNoError();

	ROCOCO_API void ThrowMissingResourceFile(ErrorCode code, cstr description, cstr filename);

	ROCOCO_API void LogExceptionAndQuit(Rococo::IException& ex, cstr prelude, cstr postlude);
	ROCOCO_API void LogExceptionAndContinue(Rococo::IException& ex, cstr prelude, cstr postlude);
}

namespace Rococo::IO
{
	struct ISysMonitor;
}

namespace Rococo::OS
{
#ifdef _WIN32
	inline bool IsEndianLittle() { return true; }
#else
# if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
     defined(__BIG_ENDIAN__) || \
     defined(__ARMEB__) || \
     defined(__THUMBEB__) || \
     defined(__AARCH64EB__) || \
     defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
	// It's a big-endian target architecture
# elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
     defined(__LITTLE_ENDIAN__) || \
     defined(__ARMEL__) || \
     defined(__THUMBEL__) || \
     defined(__AARCH64EL__) || \
     defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
	// It's a little-endian target architecture
# else
#  error "Unknown endianess!"
# endif
#endif

	struct ConfigSection
	{
		cstr sectionName;
	};

	struct ConfigRootName
	{
		cstr rootName;
	};

	class Sync
	{
		ILock& lock;
	public:
		FORCE_INLINE Sync(ILock& _lock) : lock(_lock)
		{
			lock.Lock();
		}

		FORCE_INLINE ~Sync()
		{
			lock.Unlock();
		}
	};

	class ThreadLock : public ILock
	{
		int64 implementation[8];
	public:
		ROCOCO_API ThreadLock();
		ROCOCO_API virtual ~ThreadLock();

		ROCOCO_API void Lock();
		ROCOCO_API void Unlock();
	};

	ROCOCO_API void PasteStringFromClipboard(Strings::IStringPopulator& populator);
	ROCOCO_API void SaveClipBoardText(cstr text, Windows::IWindow& window);

	// Gets a null terminated OS config string with lenBytes capacity. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	ROCOCO_API void GetConfigVariable(char* textBuffer, size_t lenBytes, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr, bool throwOnError = false);

	// Gets U8 file path from the OS config. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	inline void GetConfigVariable(U8FilePath& path, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr)
	{
		GetConfigVariable(path.buf, path.CAPACITY, defaultValue, section, rootName, organization);
	}

	// Sets the value of a config string in the OS. The maximum length is 1 megabyte.
	ROCOCO_API void SetConfigVariable(cstr value, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr);

	ROCOCO_INTERFACE IAppControl
	{
		virtual void AdvanceSysMonitors() = 0;
		// Returns the task queue for the main thread. Include <rococo.task.queue.h> for the definition of the full interface
		virtual Tasks::ITaskQueue& MainThreadQueue() = 0;
		virtual bool IsRunning() const = 0;
		virtual void ShutdownApp() = 0;
	};

	ROCOCO_INTERFACE IAppControlSupervisor : public IAppControl
	{
		virtual void AddSysMonitor(IO::ISysMonitor& monitor) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_API IAppControlSupervisor* CreateAppControl();

	ROCOCO_API void BeepWarning();
	ROCOCO_API void CopyExceptionToClipboard(IException& ex, Rococo::Windows::IWindow& window);
	ROCOCO_API void EditImageFile(Rococo::Windows::IWindow& window, crwstr sysPath);

	struct IThreadControl;

	// Implements method uint32 RunThread(IThreadControl & control)
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

	template<class CONTEXT>
	void QueueAPC(IThreadControl* control, void (*callback)(CONTEXT* context), CONTEXT* context)
	{
		control->QueueAPC(reinterpret_cast<IThreadControl::FN_APC>(callback), context);
	}

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
		virtual void Free() = 0;
	};

	enum class CriticalSectionMemorySource
	{
		OPERATOR_NEW, // The default
		GLOBAL_MALLOC // Used when creating synchronization sections for allocators, in which OPERATOR_NEW would create a recursion issue
	};

	ROCOCO_API ICriticalSection* CreateCriticalSection(CriticalSectionMemorySource src = CriticalSectionMemorySource::OPERATOR_NEW);

	ROCOCO_API [[nodiscard]] IThreadSupervisor* CreateRococoThread(IThreadJob* thread, uint32 stacksize);

	ROCOCO_API void InitRococoOS();
	ROCOCO_API void TerminateRococoOS();
	
	ROCOCO_API void AddThreadError(int errorCode, cstr format, ...);
	ROCOCO_API void ThrowOnThreadError();

	ROCOCO_API [[nodiscard]] void* AllocBoundedMemory(size_t nBytes);
	ROCOCO_API void FreeBoundedMemory(void* pMemory);

	ROCOCO_API void UILoop(uint32 milliseconds);
	ROCOCO_API bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window);
	ROCOCO_API cstr GetAsciiCommandLine();

	template<typename Type, typename IDType = typename Type::IDType>
	class Mappings;

	ROCOCO_API void PollKeys(uint8 scanArray[256]);
	ROCOCO_API void SleepUntilAsync(uint32 timeoutMS);

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

	ROCOCO_API void Format_C_Error(int errorCode, char* buffer, size_t capacity);
	ROCOCO_API void SetCursorVisibility(bool isVisible, Rococo::Windows::IWindow& captureWindow);

	// Try opening a document at a given line number. If the number is negative, then use a generic handler, otherwise use the default text editor for the Rococo OS system
	ROCOCO_API void ShellOpenDocument(Windows::IWindow& parent, cstr caption, cstr path, int lineNumber);
	ROCOCO_API void TripDebugger();
	ROCOCO_API void PrintDebug(const char* format, ...);	
	ROCOCO_API [[nodiscard]] bool IsDebugging();
	ROCOCO_API void BreakOnThrow(Flags::BreakFlag flag);

	// Merge Flags::BreakFlag to provide the argument to SetBreakPoints
	ROCOCO_API void SetBreakPoints(int flags);
	ROCOCO_API void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode);
	ROCOCO_API void BuildExceptionString(char* buffer, size_t capacity, IException& ex, bool appendStack);
	ROCOCO_API cstr GetCommandLineText();
	ROCOCO_API void GetCurrentUserName(Strings::IStringPopulator& populator);
}

namespace Rococo::Windows
{
	ROCOCO_API IWindow& NoParent(); // This is in the utils lib, which reduces need to include rococo.windows, avoiding circular dependecies
	ROCOCO_WINDOWS_API void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr captionFormat, ...);
	ROCOCO_WINDOWS_API void ShowErrorBox(Windows::IWindow& parent, int errorCode, cstr msg, cstr caption);

	enum class SHOW_WINDOW_TYPE
	{
		INFO = 0x40
	};

	ROCOCO_WINDOWS_API int ShowMessageBox(IWindow& window, cstr text, cstr caption, uint32 type);
}

namespace Rococo::Memory
{
	ROCOCO_API [[nodiscard]] IAllocator& CheckedAllocator();
	ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
	ROCOCO_API void* AlignedAlloc(size_t nBytes, int32 alignment, void* allocatorFunction(size_t));
	ROCOCO_API void AlignedFree(void* buffer, void deleteFunction(void*));
}

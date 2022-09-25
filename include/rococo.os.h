#pragma once

#include <rococo.types.h>

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

	void PasteStringFromClipboard(IEventCallback<cstr>& populator);
	void CopyStringToClipboard(cstr text);

	// Gets a null terminated OS config string with lenBytes capacity. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	void GetConfigVariable(char* textBuffer, size_t lenBytes, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr);

	// Gets U8 file path from the OS config. If not found, fills with the defaultValue. If organization is null the library default name is chosen 
	inline void GetConfigVariable(U8FilePath& path, cstr defaultValue, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr)
	{
		GetConfigVariable(path.buf, path.CAPACITY, defaultValue, section, rootName, organization);
	}

	// Sets the value of a config string in the OS. The maximum length is 1 megabyte.
	void SetConfigVariable(cstr value, ConfigSection section, ConfigRootName rootName, cstr organization = nullptr);

	ROCOCO_INTERFACE IAppControl
	{
		virtual bool IsRunning() const = 0;
		virtual void ShutdownApp() = 0;
	};

	ROCOCO_INTERFACE IAppControlSupervisor : public IAppControl
	{
		virtual void Free() = 0;
	};

	IAppControlSupervisor* CreateAppControl();

	void BeepWarning();
	void CopyExceptionToClipboard(IException& ex);
	void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath);

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

	void WakeUp(IThreadControl& thread);

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

	[[nodiscard]] IThreadSupervisor* CreateRococoThread(IThreadJob* thread, uint32 stacksize);


	[[nodiscard]] void* AllocBoundedMemory(size_t nBytes);
	void FreeBoundedMemory(void* pMemory);

	enum TargetDirectory
	{
		TargetDirectory_UserDocuments = 0,
		TargetDirectory_Root
	};

	void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text);

	void FormatTime(ticks utcTime, char* buffer, size_t nBytes);
	bool StripLastSubpath(wchar_t* fullpath);
	bool IsFileExistant(const wchar_t* path);
	void Format_C_Error(int errorCode, char* buffer, size_t capacity);
	[[nodiscard]] int OpenForAppend(void** fp, cstr name);
	[[nodiscard]] int OpenForRead(void** fp, cstr name);
	void UILoop(uint32 milliseconds);
	void ToSysPath(wchar_t* path);
	void ToUnixPath(wchar_t* path);
	void ToSysPath(char* path);
	void ToUnixPath(char* path);
	void SanitizePath(char* path);
	void SanitizePath(wchar_t* path);
	void SaveClipBoardText(cstr text, Windows::IWindow& window);
	bool TryGetColourFromDialog(RGBAb& colour, Windows::IWindow& window);
	cstr GetAsciiCommandLine();

	template<typename Type, typename IDType = typename Type::IDType>
	class Mappings;

	// Open a file and fit into buffer. In the case of a truncation an IException is thrown. The function returns the number of bytes copied to the buffer.
	size_t LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename);
	void LoadAsciiTextFile(IEventCallback<cstr>& callback, const wchar_t* filename);

	void GetEnvVariable(wchar_t* data, size_t capacity, const wchar_t* envVariable);
	void PollKeys(uint8 scanArray[256]);
	void MakeContainerDirectory(char* filename);
	void MakeContainerDirectory(wchar_t* filename);

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
}


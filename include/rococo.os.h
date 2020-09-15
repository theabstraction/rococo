#pragma once

#include <rococo.types.h>

namespace Rococo::OS
{
	ROCOCOAPI IAppControl
	{
		virtual bool IsRunning() const = 0;
		virtual void ShutdownApp() = 0;
	};

	ROCOCOAPI IAppControlSupervisor : public IAppControl
	{
		virtual void Free() = 0;
	};

	IAppControlSupervisor* CreateAppControl();

	void BeepWarning();
	void CopyExceptionToClipboard(IException& ex);
	void EditImageFile(Rococo::Windows::IWindow& window, const wchar_t* sysPath);

	struct IThreadControl;

	ROCOCOAPI IThreadJob
	{
		virtual uint32 RunThread(IThreadControl & control) = 0;
	};

	ROCOCOAPI IThreadControl : public ILock
	{
		virtual bool IsRunning() const = 0;
		virtual void Resume() = 0;
		virtual void SetRealTimePriority() = 0;
		virtual void SleepUntilAysncEvent(uint32 milliseconds) = 0;
		virtual cstr GetErrorMessage() const = 0;
	};

	ROCOCOAPI IThreadSupervisor : public IThreadControl
	{
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
	void LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename);
	void LoadAsciiTextFile(IEventCallback<cstr>& onLoad, const wchar_t* filename);
	void GetEnvVariable(wchar_t* data, size_t capacity, const wchar_t* envVariable);
	void PollKeys(uint8 scanArray[256]);
	void MakeContainerDirectory(char* filename);
	void MakeContainerDirectory(wchar_t* filename);
}


#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX

#include <windows.h>

#include "resource.h"

#include <rococo.types.h>
#include <rococo.renderer.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.window.h>

#include <vector>
#include <algorithm>

#ifdef _DEBUG
# pragma comment(lib, "dx11.renderer.debug.lib")
#else
# pragma comment(lib, "dx11.renderer.lib")
#endif

using namespace Rococo;
using namespace Rococo::Windows;

namespace Dystopia
{
	IApp* CreateDystopiaApp(IRenderer& renderer, IOS& os);
}

namespace
{
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

	class Win32OS : public IOS
	{
		wchar_t contentDirectory[_MAX_PATH];
	public:
		Win32OS(const wchar_t* _contentDirectory)
		{
			memcpy_s(contentDirectory, sizeof(wchar_t) * _MAX_PATH, _contentDirectory, sizeof(wchar_t) * _MAX_PATH);
		}

		virtual void LoadResource(const wchar_t* resourcePath, IExpandingBuffer& buffer, int64 maxFileLength)
		{
			if (resourcePath == nullptr || wcslen(resourcePath) < 2) Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: <resourcePath> was blank");
			if (resourcePath[0] != '!') Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: <%s> did not begin with ping '!' character", resourcePath);

			if (wcslen(resourcePath) + wcslen(contentDirectory) >= _MAX_PATH)
			{
				Throw(E_INVALIDARG, L"Win32OS::LoadResource failed: %s%s - filename was too long", contentDirectory, resourcePath + 1);
			}

			wchar_t absPath[_MAX_PATH];
			SecureFormat(absPath, L"%s%s", contentDirectory, resourcePath + 1);

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
				DWORD chunk = (DWORD)(int32)std::min(bytesLeft, 65536LL);
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
	};

	void MakeContainerDirectory(wchar_t* filename)
	{
		int len = (int) wcslen(filename);

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

	void GetContentDirectory(HINSTANCE hInstance, FilePath& path)
	{
		FilePath binDirectory;
		GetModuleFileNameW(hInstance, binDirectory, _MAX_PATH);
		MakeContainerDirectory(binDirectory);

		path = binDirectory;

		size_t len = wcslen(path);

		while (len > 0)
		{
			FilePath indicator;
			SecureFormat(indicator.data, L"%scontent.indicator.txt", path.data);
			if (INVALID_FILE_ATTRIBUTES != GetFileAttributes(indicator))
			{
				SecureCat(path.data, L"content\\");
				return;
			}

			MakeContainerDirectory(path);

			size_t newLen = wcslen(path);
			if (newLen >= len) break;
			len = newLen;
		}

		Throw(0, L"Could not find content.indicator.txt below the executable folder '%s'", binDirectory);
	}
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HANDLE hInstanceLock = CreateEvent(nullptr, TRUE, FALSE, L"Dystopia_InstanceLock");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		SetEvent(hInstanceLock);

		if (IsDebuggerPresent())
		{
			MessageBox(nullptr, L"Dystopia is already running", L"Dystopia Exception", MB_ICONEXCLAMATION);
		}
		return -1;
	}

	try
	{
		FilePath contentDirectory;
		GetContentDirectory(_hInstance, contentDirectory);

		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain

		Win32OS os(contentDirectory);

		struct : IAppFactory
		{
			Win32OS* os;
			virtual IApp* CreateApp(IRenderer& renderer)
			{
				return Dystopia::CreateDystopiaApp(renderer, *os);
			}
		} factory;

		factory.os = &os;

		RendererMain(hInstanceLock, os, factory);
	}
	catch (IException& ex)
	{
		ShowErrorBox(ex, L"Dystopia threw an exception");
	}

	CloseHandle(hInstanceLock);

	return 0;
}



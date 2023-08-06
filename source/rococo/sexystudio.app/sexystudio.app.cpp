#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.api.h>
#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Strings;

static auto DLL_NAME = L"sexystudio.dll";

cstr ErrorCaption = "SexyStudio Standalone App - error!";

#include <rococo.sexystudio.api.h>

using namespace Rococo::SexyStudio;

void MainProtected(HINSTANCE, HMODULE hLib)
{
	FARPROC proc = GetProcAddress(hLib, "CreateSexyStudioFactory");
	if (proc == nullptr)
	{
		Throw(GetLastError(), "Could not find CreateSexyStudioFactory in %ls", DLL_NAME);
	}

	auto CreateSexyStudioFactory = (Rococo::SexyStudio::FN_CreateSexyStudioFactory) proc;

	cstr interfaceURL = "Rococo.SexyStudio.ISexyStudioFactory1";

	AutoFree<ISexyStudioFactory1> factory;
	int nErr = CreateSexyStudioFactory((void**)&factory, interfaceURL);
	if (nErr == 0)
	{
		struct ANON : ISexyStudioEventHandler
		{
			bool TryOpenEditor(cstr, int) override
			{
				return false;
			}

			EIDECloseResponse OnIDEClose(IWindow&) override
			{
				return EIDECloseResponse::Shutdown;
			}
		} eventHandler;

		AutoFree<ISexyStudioInstance1> instance = factory->CreateSexyIDE(Rococo::Windows::NoParent(), eventHandler);
		ShowWindow(instance->GetIDEFrame(), SW_SHOW);
		
		MSG msg;
		while (instance->IsRunning() && GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		Throw(nErr, "CreateSexyStudioFactory(&factory, %s) failed", interfaceURL);
	}
}

#include <rococo.allocators.h>
using namespace Rococo::Memory;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	AllocatorLogFlags flags;
	flags.LogDetailedMetrics = false;
	flags.LogLeaks = false;
	flags.LogOnModuleExit = false;
	SetAllocatorLogFlags(flags);
	// Assume that the DLL is in the same directory as the executable

	WideFilePath directory;
	if (!GetModuleFileNameW(NULL, directory.buf, directory.CAPACITY)) return GetLastError();
	Rococo::IO::StripLastSubpath(directory.buf);

	WideFilePath pathToDLL;
	Format(pathToDLL, L"%ls%ls", directory.buf, DLL_NAME);

	auto hLib = LoadLibraryW(pathToDLL);
	if (hLib == nullptr)
	{
		U8FilePath msg;
		Format(msg, "Could not load library: %ls", pathToDLL.buf);
		MessageBoxA(NULL, msg, ErrorCaption, MB_ICONERROR);
		return GetLastError();
	}

	int errorCode = 0;

	try
	{
		MainProtected(hInstance, hLib);
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		errorCode = ex.ErrorCode();
	}

	FreeLibrary(hLib);

	return errorCode;
}
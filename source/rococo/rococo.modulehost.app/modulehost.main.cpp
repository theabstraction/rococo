// modulehost.main.cpp

#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.io.h>
#include <rococo.os.h>
#include <rococo.api.h>
#include <rococo.window.h>
#include <rococo.mvc.h>

using namespace Rococo;
using namespace Rococo::MVC;
using namespace Rococo::Strings;

cstr ErrorCaption = "Rococo Module Host App - error!";

typedef IMVC_ControllerSupervisor* (*FN_Create_MVC_Controller)(IMVC_Host& host, cstr commandLine);

class MVC_Host : public IMVC_Host
{
public:
	bool isRunning = true;

	void TerminateApp()
	{
		isRunning = false;
		PostQuitMessage(0);
	}

	void DoMainloop(IMVC_Controller& controller)
	{
		MSG msg;
		while (isRunning && controller.IsRunning() && GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hInstance);

	HMODULE hLib = NULL;

	try
	{
		MVC_Host host;

		char controllerPath[MAX_PATH];
		Rococo::Strings::CLI::GetCommandLineArgument("-controller:"_fstring, commandLine, controllerPath, MAX_PATH, "");
		if (*controllerPath == 0)
		{
			Throw(0, "No -controller:<path> specified for the controller module on the command line");
		}

		hLib = LoadLibraryA(controllerPath);
		if (hLib == nullptr)
		{
			Throw(GetLastError(), "Could not load %s", controllerPath);
		}

		FN_Create_MVC_Controller mvcFactory = (FN_Create_MVC_Controller) GetProcAddress(hLib, "CreateMVCController");
		if (!mvcFactory)
		{
			Throw(GetLastError(), "GetProcAddress('CreateMVCController' in %s) failed", controllerPath);
		}

		AutoFree<IMVC_ControllerSupervisor> controller = mvcFactory(host, commandLine);
		host.DoMainloop(*controller);
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		return ex.ErrorCode();
	}

	FreeLibrary(hLib);

	return 0;
}

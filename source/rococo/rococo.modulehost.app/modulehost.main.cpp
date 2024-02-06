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

typedef IMVC_ControllerSupervisor* (*FN_Create_MVC_Controller)(IMVC_Host& host, IMVC_View& view, cstr commandLine);
typedef IMVC_ViewSupervisor* (*FN_Create_MVC_View)(IMVC_Host& host, HWND hHostWindow, HINSTANCE hAppInstance, cstr commandLine);

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

		Beep(512, 500);
	}
};

struct AutoLib
{
	HMODULE lib = NULL;

	~AutoLib()
	{
		if (lib)
		{
			FreeLibrary(lib);
		}
	}

	void Load(cstr filename)
	{
		if (lib)
		{
			Throw(0, "Library already configured. API implementation error trying to overwrite with new DLL: %s", filename);
		}

		lib = LoadLibraryA(filename);
		if (lib == nullptr)
		{
			Throw(GetLastError(), "Could not load %s", filename);
		}
	}

	operator HMODULE() 
	{
		return lib;
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hInstance);

	AutoLib hController;
	AutoLib hView;

	try
	{
		MVC_Host host;


		// Controller
		char controllerPath[MAX_PATH];
		Rococo::Strings::CLI::GetCommandLineArgument("-controller:"_fstring, commandLine, controllerPath, MAX_PATH, "");
		if (*controllerPath == 0)
		{
			Throw(0, "No -controller:<path> specified for the controller module on the command line");
		}

		hController.Load(controllerPath);		
		FN_Create_MVC_Controller controllerFactory = (FN_Create_MVC_Controller) GetProcAddress(hController, "CreateMVCController");
		if (!controllerFactory)
		{
			Throw(GetLastError(), "GetProcAddress('CreateMVCController' in %s) failed", controllerPath);
		}

		// View
		char viewPath[MAX_PATH];
		Rococo::Strings::CLI::GetCommandLineArgument("-view:"_fstring, commandLine, viewPath, MAX_PATH, "");
		if (*viewPath == 0)
		{
			Throw(0, "No -view:<path> specified for the view module on the command line");
		}

		hView.Load(viewPath);

		FN_Create_MVC_View viewFactory = (FN_Create_MVC_View)GetProcAddress(hView, "CreateMVCView");
		if (!viewFactory)
		{
			Throw(GetLastError(), "GetProcAddress('CreateMVCView' in %s) failed", viewPath);
		}

		HWND hNoHostWindow = NULL;
		AutoFree<IMVC_ViewSupervisor> view = viewFactory(host, hNoHostWindow, hInstance, commandLine);
		AutoFree<IMVC_ControllerSupervisor> controller = controllerFactory(host, *view, commandLine);

		controller->TerminateOnMainWindowClose();

		host.DoMainloop(*controller);
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "SexyStudio Standalone App error");
		return ex.ErrorCode();
	}

	return 0;
}

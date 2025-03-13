#include <rococo.types.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.os.win32.h>

using namespace Rococo;

ATOM appClassAtom = 0;

LRESULT MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

void PopulateMainClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
{
	cstr IDI_ICON1 = nullptr;
	cstr IDI_ICON2 = nullptr;

	classDef = { 0 };
	classDef.cbSize = sizeof(classDef);
	classDef.style = 0;
	classDef.cbWndExtra = 0;
	classDef.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//classDef.hIcon = LoadIconA(hInstance, (cstr)IDI_ICON1);
	//classDef.hIconSm = LoadIconA(hInstance, (cstr)IDI_ICON2);
	classDef.hInstance = hInstance;
	classDef.lpszClassName = "GR-Win32-GDI-APP";
	classDef.lpszMenuName = NULL;
	classDef.lpfnWndProc = MainProc;
}

void RunApp(HWND hWnd)
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hInstance);

	Rococo::OS::SetBreakPoints(OS::Flags::BreakFlag_All);

	try
	{
		// Controller
		char controllerPath[MAX_PATH];
		Rococo::Strings::CLI::GetCommandLineArgument("-controller:"_fstring, commandLine, controllerPath, MAX_PATH, "");
		if (*controllerPath != 0)
		{
			
		}

		WNDCLASSEXA info;
		PopulateMainClass(hInstance, info);
		appClassAtom = RegisterClassExA(&info);
		if (appClassAtom == 0)
		{
			Throw(GetLastError(), "Could not create %s class atom", info.lpszClassName);
		}

		HWND hWnd = CreateWindowExA(
			WS_EX_APPWINDOW,
			info.lpszClassName,
			"rococo.gr Win32-GDI Test Window",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, // x
			CW_USEDEFAULT, // y
			CW_USEDEFAULT, // width
			CW_USEDEFAULT, // height
			nullptr,
			nullptr,
			hInstance,
			nullptr);

		if (hWnd == nullptr)
		{
			Throw(GetLastError(), "%s: could not create overlapped window for %s", __FUNCTION__, info.lpszClassName);
		}

		RunApp(hWnd);

		if (hWnd)
		{
			DestroyWindow(hWnd);
		}
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "Rococo Module Host error");
		return ex.ErrorCode();
	}

	return 0;
}

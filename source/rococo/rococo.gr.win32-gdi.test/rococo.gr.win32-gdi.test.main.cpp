#include <rococo.types.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.os.win32.h>

using namespace Rococo;

struct GR_Win32_Host
{
	HWND hHostWindow = nullptr;

	void OnPaint()
	{
		PAINTSTRUCT ps;
		BeginPaint(hHostWindow, &ps);

		RECT rect;
		GetClientRect(hHostWindow, &rect);

		HBRUSH redBrush = CreateSolidBrush(RGB(128, 0, 0));
		auto old = SelectObject(ps.hdc, redBrush);

		FillRect(ps.hdc, &rect, redBrush);

		SelectObject(ps.hdc, old);

		DeleteObject(redBrush);

		EndPaint(hHostWindow, &ps);
	}
};

LRESULT HostProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto* host = reinterpret_cast<GR_Win32_Host*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

	switch (msg)
	{
	case WM_PAINT:
		host->OnPaint();
		return 0L;
	}
	return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static cstr clientClassName = "GR-Win32-GDI-CLIENT";

BOOL OnParentResized(HWND hWnd, LPARAM /* lParam */)
{
	char className[256];
	if (RealGetWindowClassA(hWnd, className, (UINT)sizeof className) != 0)
	{
		if (strcmp(className, clientClassName) == 0)
		{
			HWND hParent = GetParent(hWnd);
			RECT clientRect;
			GetClientRect(hParent, &clientRect);
			MoveWindow(hWnd, 0, 0, clientRect.right, clientRect.bottom, TRUE);
		}
	}

	return TRUE; /* unused according to Microsoft Docs */
}

LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
{	
	EnumChildWindows(hWnd, OnParentResized, 0);
	return DefWindowProc(hWnd, WM_SIZE, wParam, lParam);
}

LRESULT MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_GETMINMAXINFO:
		{
			auto* m = (MINMAXINFO*)lParam;
			m->ptMinTrackSize = POINT{ 640, 480 };
		}
	return 0L;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		return OnSize(hWnd, wParam, lParam);
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
	classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
	classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
	//classDef.hIcon = LoadIconA(hInstance, (cstr)IDI_ICON1);
	//classDef.hIconSm = LoadIconA(hInstance, (cstr)IDI_ICON2);
	classDef.hInstance = hInstance;
	classDef.lpszClassName = "GR-Win32-GDI-APP";
	classDef.lpszMenuName = NULL;
	classDef.lpfnWndProc = MainProc;
}

void PopulateClientClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
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
	classDef.lpszClassName = clientClassName;
	classDef.lpszMenuName = NULL;
	classDef.lpfnWndProc = DefWindowProcA;
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

	GR_Win32_Host host;

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
		ATOM appClassAtom = RegisterClassExA(&info);
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

		WNDCLASSEXA clientInfo;
		PopulateClientClass(hInstance, clientInfo);
		ATOM clientClassAtom = RegisterClassExA(&clientInfo);
		if (clientClassAtom == 0)
		{
			Throw(GetLastError(), "Could not create %s class atom", clientInfo.lpszClassName);
		}

		RECT clientRect;
		GetClientRect(hWnd, &clientRect);

		HWND hWndClient = CreateWindowExA(
			0,
			clientInfo.lpszClassName,
			"rococo.gr Win32-GDI Test Window",
			WS_CHILD | WS_VISIBLE,
			0, // x
			0, // y
			clientRect.right, // width
			clientRect.bottom, // height
			hWnd,
			nullptr,
			hInstance,
			nullptr);

		if (hWndClient == nullptr)
		{
			Throw(GetLastError(), "%s: could not create client window for %s", __FUNCTION__, clientInfo.lpszClassName);
		}

		host.hHostWindow = hWndClient;

		SetWindowLongPtrA(hWndClient, GWLP_USERDATA, (LONG_PTR) &host);
		SetWindowLongPtrA(hWndClient, GWLP_WNDPROC, (LONG_PTR)HostProc);

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

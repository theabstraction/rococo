#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include "resource.h"

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	void PopulateMainClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
	{
		classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)BLACK_BRUSH;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = LoadIconA(hInstance, (cstr)IDI_ICON1);
		classDef.hIconSm = LoadIconA(hInstance, (cstr)IDI_ICON2);
		classDef.hInstance = hInstance;
		classDef.lpszClassName = "SexyStudioMain";
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;
	}

	static ATOM mainClassAtom = 0;
	static HINSTANCE s_hInstance = nullptr;

	static int lastError = 0;
	char lastErrorMessage[2048] = { 0 };

	UINT s_WM_IDE_RESIZE = 0;
}

namespace Rococo::SexyStudio
{
	HINSTANCE GetMainInstance()
	{
		if (s_hInstance == nullptr)
		{
			Throw(0, "Call InitStudioWindows first");
		}
		return s_hInstance;
	}

	void SetLastMessageError(IException& ex)
	{
		lastError = ex.ErrorCode();
		SafeFormat(lastErrorMessage, "%s", ex.Message());
	}

	void AssertNoMessageError()
	{
		if (*lastErrorMessage)
		{
			Throw(lastError, "%s", lastErrorMessage);
		}
	}

	void InitStudioWindows(HINSTANCE hInstance, cstr iconLarge, cstr iconSmall)
	{
		s_hInstance = hInstance;

		HICON hLargeIcon = LoadIconA(hInstance, iconLarge);
		HICON hSmallIcon = LoadIconA(hInstance, iconSmall);

		if (hLargeIcon == nullptr)
		{
			Throw(GetLastError(), "Could not find large icon");
		}

		if (hSmallIcon == nullptr)
		{
			Throw(GetLastError(), "Could not find small icon");
		}

		Windows::InitRococoWindows(hInstance, hLargeIcon, hSmallIcon, nullptr, nullptr);
	}

	Win32TopLevelWindow::Win32TopLevelWindow(DWORD exStyle, DWORD style, IWin32WindowMessageLoopHandler& _handler) : handler(_handler)
	{
		if (mainClassAtom == 0)
		{
			WNDCLASSEXA info;
			PopulateMainClass(GetMainInstance(), info);
			mainClassAtom = RegisterClassExA(&info);
			if (mainClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create main window class atom");
			}
		}

		HMENU hMenu = nullptr;
		HWND hParent = nullptr;

		hWnd = CreateWindowExA(
			exStyle,
			"SexyStudioMain",
			"",
			style,
			0, // x
			0, // y
			0, // width
			0, // height
			hParent,
			hMenu,
			GetMainInstance(),
			nullptr);

		if (hWnd == nullptr)
		{
			Throw(GetLastError(), "%s: could not create overlapped window", __FUNCTION__);
		}

		struct ANON
		{
			static LRESULT WINAPI RouteMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
			{
				auto* handler = (IWindowMessageHandler*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

				try
				{
					return handler->ProcessMessage(msg, wParam, lParam);
				}
				catch (IException& ex)
				{
					Rococo::SexyStudio::SetLastMessageError(ex);
					return DefWindowProc(hWnd, msg, wParam, lParam);
				}
			}

		};

		SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)&handler);
		SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)ANON::RouteMessage);
	}

	Win32TopLevelWindow::~Win32TopLevelWindow()
	{
		DestroyWindow(hWnd);
	}
} // Rococo::SexyStudio
#include "sexystudio.impl.h"
#include "resource.h"

using namespace Rococo;

namespace Rococo::SexyStudio
{
	const char* GetChildClassName()
	{
		return  "SexyStudioChild";
	}

	void PopulatChildClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
	{
		classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(255, 0, 0));
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = nullptr;
		classDef.hIconSm = nullptr;
		classDef.hInstance = hInstance;
		classDef.lpszClassName = GetChildClassName();
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;
	}

	static ATOM childClassAtom = 0;

	Win32ChildWindow::Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& _handler):
		handler(_handler)
	{
		if (childClassAtom == 0)
		{
			WNDCLASSEXA childInfo;
			PopulatChildClass(GetMainInstance(), childInfo);
			childClassAtom = RegisterClassExA(&childInfo);
			if (childClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create child window class atom");
			}
		}

		DWORD exStyle = 0;
		DWORD style = WS_CHILD;
		HMENU hMenu = nullptr;
	
		hWnd = CreateWindowExA(
			exStyle,
			"SexyStudioChild",
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
			Throw(0, "%s: could not create child window", __FUNCTION__);
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
		SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR) ANON::RouteMessage);
	}

	Win32ChildWindow::~Win32ChildWindow()
	{
		DestroyWindow(hWnd);
	}
}
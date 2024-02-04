#include <rococo.os.win32.h>

#include <rococo.abstract.editor.win32.h>
#include "resource.h"

using namespace Rococo;

namespace ANON
{
	struct AbeditMainWindow: Rococo::Abedit::IAbeditMainWindow
	{
		HWND hMainWindow = NULL;

		~AbeditMainWindow()
		{
			if (hMainWindow)
			{
				CloseWindow(hMainWindow);
			}
		}

		void Free() override
		{
			delete this;
		}

		bool IsVisible() const override
		{
			return IsWindowVisible(hMainWindow);
		}

		static ATOM CreateCustomAtom(HINSTANCE dllInstance)
		{
			WNDCLASSEXA classDef = { 0 };
			classDef.cbSize = sizeof(classDef);
			classDef.style = 0;
			classDef.cbWndExtra = 0;
			classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
			classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
			classDef.hIcon = LoadIconA(dllInstance, MAKEINTRESOURCEA(IDI_ICON_LARGE));
			classDef.hIconSm = LoadIconA(dllInstance, MAKEINTRESOURCEA(IDI_ICON_LARGE));
			classDef.hInstance = (HINSTANCE) GetModuleHandleA(NULL);
			classDef.lpszClassName = "AbEditMainWindow_1_0";
			classDef.lpszMenuName = NULL;
			classDef.lpfnWndProc = DefWindowProcA;

			auto atom = RegisterClassExA(&classDef);

			if (atom == 0)
			{
				int err = GetLastError();
				if (err != ERROR_CLASS_ALREADY_EXISTS)
				{
					Throw(err, "Error creating custom atom. Bad hIcon/hInstance maybe?");
				}
			}

			return atom;
		}

		void Create(HWND hOwner)
		{
			DWORD exStyle = 0;
			DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
			int x = 0;
			int y = 0;
			int dx = 800;
			int dy = 600;
			cstr title = "Rococo Abstract Editor";
			HINSTANCE hInstance = NULL;
			hMainWindow = CreateWindowExA(exStyle, "AbEditMainWindow_1_0", title, style, x, y, dx, dy, hOwner, NULL, hInstance, NULL);
			if (!hMainWindow)
			{
				Throw(GetLastError(), "%s: Failed to create a window of class AbEditMainWindow_1_0", __FUNCTION__);
			}

			ShowWindow(hOwner, SW_SHOW);
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IAbeditMainWindow* CreateMainWindow(HWND hParent, HINSTANCE dllInstance)
	{
		static ATOM atom = ANON::AbeditMainWindow::CreateCustomAtom(dllInstance);
		if (!atom)
		{
			Throw(GetLastError(), "%s: could not create main window atom", __FUNCTION__);
		}

		AutoFree<ANON::AbeditMainWindow> window = new ANON::AbeditMainWindow();
		window->Create(hParent);
		return window.Release();
	}
}
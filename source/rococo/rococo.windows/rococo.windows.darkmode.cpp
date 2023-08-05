#include <rococo.os.win32.h>

namespace Rococo::Windows
{
	ROCOCO_WINDOWS_API bool IsDarkmode()
	{
		DWORD len = 4;
		DWORD dwValue;
        auto res = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, (LPVOID) & dwValue, &len);
		if (res != ERROR_SUCCESS)
		{
			return false;
		}

		return dwValue == 0;
	}
}

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define  DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#pragma comment(lib, "Dwmapi.lib")

namespace Rococo::Windows
{
	ROCOCO_WINDOWS_API void SetDarkWindow(HWND hWnd)
	{
		BOOL value = TRUE;
		::DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	}
}
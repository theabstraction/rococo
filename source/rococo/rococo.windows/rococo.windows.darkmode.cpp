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
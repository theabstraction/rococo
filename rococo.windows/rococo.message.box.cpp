#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.types.h>
#include <rococo.window.h>
#include <wchar.h>

namespace
{
	using namespace Rococo::Windows;

	
}

namespace Rococo
{
	namespace Windows
	{
		int ShowMessageBox(IWindow& window, const wchar_t* text, const wchar_t* caption, uint32 uType)
		{
			return MessageBox(window, text, caption, uType);
		}

		IWindow& NoParent()
		{
			class: public IWindow
			{
				virtual operator HWND () const
				{
					return nullptr;
				}
			} static noParent;

			return noParent;
		}
	}

	void ShowErrorBox(IWindow& parent, IException& ex, const wchar_t* caption)
	{
		if (ex.ErrorCode() == 0)
		{
			ShowMessageBox(parent, ex.Message(), caption, MB_ICONERROR);
		}
		else
		{
			wchar_t codeMsg[512];
			wchar_t bigMsg[512];
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ex.ErrorCode(), 0, codeMsg, 512, nullptr) <= 0)
			{
				SafeFormat(bigMsg, _TRUNCATE, L"%s. Code 0x%x", ex.Message(), ex.ErrorCode());
			}
			else
			{
				SafeFormat(bigMsg, _TRUNCATE, L"%s\nCode 0x%x: %s", ex.Message(), ex.ErrorCode(), codeMsg);
			}

			ShowMessageBox(parent, bigMsg, caption, MB_ICONERROR);
		}
	}
}
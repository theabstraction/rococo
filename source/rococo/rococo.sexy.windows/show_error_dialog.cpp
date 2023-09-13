#include "sexy.windows.internal.h"

#include "resource.h"

#include <richedit.h>
#include <stdio.h>

namespace ANON
{
	using namespace Rococo::SexyWindows;

	class ErrorEditorContext
	{
	public:
		const Rococo::SexyWindows::ErrorDialogSpec& error;
		HFONT hFont;
		ErrorEditorContext(const ErrorDialogSpec& _error) : error(_error)
		{
			hFont = CreateFontA(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
		}

		~ErrorEditorContext()
		{
			DeleteObject(hFont);
		}
	};

	INT_PTR CALLBACK ErrorDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_INITDIALOG)
		{
			SetWindowLongPtrA(hDlg, GWLP_USERDATA, lParam);

			ErrorEditorContext& context = *(ErrorEditorContext*)lParam;
			auto& e = context.error;

			SetWindowTextA(hDlg, e.title);

			size_t numberOfResponses = 0;
			for (size_t i = 0; e.responseButtons.pArray[i] != nullptr && i < 4; ++i)
			{
				numberOfResponses++;
				SetDlgItemTextA(hDlg, (int)(RESPONSE_0 + i), e.responseButtons.pArray[i]);
			}

			for (size_t j = numberOfResponses; j < 4; j++)
			{
				ShowWindow(GetDlgItem(hDlg, (int)(RESPONSE_0 + j)), SW_HIDE);
			}

			HWND hEditor = GetDlgItem(hDlg, IDC_OUTPUT);

			SendMessageA(hEditor, WM_SETFONT, (WPARAM)context.hFont, FALSE);
			for (size_t k = 0; e.errorMessages.pArray[k] != nullptr; ++k)
			{
				COLORREF back = RGB(255, 255, 255);
				COLORREF fore = (k % 2) == 0 ? RGB(0, 0, 0) : RGB(128, 128, 128);

				if (k == 0) fore = RGB(255, 0, 0);

				AppendText_ToEditor(hEditor, "\r\n ", back, fore);
				AppendText_ToEditor(hEditor, e.errorMessages.pArray[k], back, fore);
			}

			if (e.systemError != 0)
			{
				HRESULT hr = e.systemError;
				char sysErr[256];
				DWORD status = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, sysErr, sizeof(sysErr), nullptr);
				if (status == 0)
				{
					_snprintf_s(sysErr, _TRUNCATE, "%d, 0x%08.8X", hr, hr);
				}

				AppendText_ToEditor(hEditor, "\r\n\r\n System error: ", RGB(255, 255, 255), RGB(0, 192, 0));
				AppendText_ToEditor(hEditor, sysErr, RGB(255, 255, 255), RGB(0, 128, 0));
			}

			return TRUE;
		}
		else
		{
			ErrorEditorContext& context = *(ErrorEditorContext*)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
			switch (uMsg)
			{
			case WM_CLOSE:
				EndDialog(hDlg, RESPONSE_0);
				return TRUE;
			case WM_COMMAND:
			{
				HWND hControl = (HWND)lParam;
				if (hControl != nullptr)
				{
					WORD notifyCode = HIWORD(wParam);
					WORD id = LOWORD(wParam);
					if (notifyCode == BN_CLICKED)
					{
						EndDialog(hDlg, id);
						return TRUE;
					}
				}
			}
			}
		}
		return 0;
	}
}

namespace Rococo
{
	namespace SexyWindows
	{
		DialogResult ShowErrorDialog(const ErrorDialogSpec& e, HINSTANCE dllInstance)
		{
			ANON::ErrorEditorContext context(e);
			INT_PTR result = DialogBoxParamA(dllInstance, (LPCSTR)IDD_ERROR_DIALOG, e.parent, ANON::ErrorDialogProc, (LPARAM)&context);
			if (result == 0 || result == -1)
			{
				return DialogResult{ nullptr, (size_t) result };
			}
			else
			{
				size_t index = result - RESPONSE_0;
				for (size_t i = 0; e.responseButtons.pArray[i] != nullptr; ++i)
				{
					if (i == index)
					{
						return DialogResult{ e.responseButtons.pArray[i], (size_t) result };
					}
				}

				return DialogResult{ nullptr,  (size_t)index };
			}
		}

		void AppendText_ToEditor(HWND hEditor, const char* text, COLORREF back, COLORREF fore)
		{
			CHARFORMAT2 c;
			memset(&c, 0, sizeof(c));
			c.cbSize = sizeof(c);
			c.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			c.crBackColor = back;
			c.crTextColor = fore;
			SendMessageA(hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&c);

			CHARRANGE cr;
			cr.cpMin = -1;
			cr.cpMax = -1;

			// hwnd = rich edit hwnd
			SendMessageA(hEditor, EM_EXSETSEL, 0, (LPARAM)&cr);
			SendMessageA(hEditor, EM_REPLACESEL, 0, (LPARAM)text);
		}
	}
}
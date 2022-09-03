#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <stdio.h>
#include <richedit.h>
#include <rococo.os.h>

using namespace Rococo::Strings;

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	class ExceptionEditorContext
	{
	public:
		HFONT hFont;
		Rococo::IException* ex;
		const ExceptionDialogSpec* spec;
		Vec2i buttonSpan;
		int stackDefaultHeight = 0;

		ExceptionEditorContext()
		{
			hFont = CreateFontA(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
		}

		~ExceptionEditorContext()
		{
			DeleteObject(hFont);
		}
	};

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

	void PopulateRichEditView(HWND hRichEditor, cstr message, int errorCode)
	{
		if (errorCode != 0)
		{
			char sysMessage[256];
			cstr sep;
			DWORD code = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, 0, sysMessage, 256, nullptr);
			if (code == 0)
			{
				sep = "";
				sysMessage[0] = 0;
			}
			else
			{
				sep = " - ";
			}

			char numericErrorLine[256];
			SafeFormat(numericErrorLine, sizeof(numericErrorLine), " %s%sError code: %d (0x%8.8X)\n", sysMessage, sep, errorCode, errorCode);
			AppendText_ToEditor(hRichEditor, numericErrorLine, RGB(255, 255, 255), RGB(255, 0, 0));
		}
		AppendText_ToEditor(hRichEditor, message, RGB(255, 255, 255), RGB(0, 0, 0));
	}

	INT_PTR CALLBACK ExceptionDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_INITDIALOG)
		{
			SetWindowLongPtrA(hDlg, GWLP_USERDATA, lParam);

			ExceptionEditorContext& context = *(ExceptionEditorContext*)lParam;

			SetWindowTextA(hDlg, context.spec->title);

			auto stackView = GetDlgItem(hDlg, context.spec->stackViewId);
			if (context.ex->StackFrames())
			{
				SendMessageA(stackView, WM_SETFONT, (WPARAM)context.hFont, FALSE);

				RECT stackRect;
				GetClientRect(stackView, &stackRect);

				context.stackDefaultHeight = stackRect.bottom - stackRect.top;

				Rococo::Windows::SetStackViewColumns(stackView, context.spec->widths);
				Rococo::Windows::PopulateStackView(stackView, *context.ex);
			}
			else
			{
				context.stackDefaultHeight = 0;
				ShowWindow(stackView, SW_HIDE);
			}

			auto hButtonClose = GetDlgItem(hDlg, IDCANCEL);
			RECT buttonRect;
			GetClientRect(hButtonClose, &buttonRect);

			context.buttonSpan.x = buttonRect.right - buttonRect.left;
			context.buttonSpan.y = buttonRect.bottom - buttonRect.top;

			auto logView = GetDlgItem(hDlg, context.spec->logViewId);

			if (context.stackDefaultHeight == 0)
			{
				RECT clientRect;
				GetClientRect(hDlg, &clientRect);
				MoveWindow(logView, 7, 7, clientRect.right - context.buttonSpan.x - 35, clientRect.bottom - 14, TRUE);
				InvalidateRect(hDlg, nullptr, TRUE);
			}

			SendMessageA(logView, WM_SETFONT, (WPARAM)context.hFont, FALSE);
			PopulateRichEditView(logView, context.ex->Message(), context.ex->ErrorCode());
			return TRUE;
		}
		else
		{
			ExceptionEditorContext& context = *(ExceptionEditorContext*)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
			switch (uMsg)
			{
			case WM_CLOSE:
				EndDialog(hDlg, IDCANCEL);
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
						if (id == IDCANCEL)
						{
							EndDialog(hDlg, IDCANCEL);
						}
						else if (id == IDOK)
						{
							Rococo::OS::CopyExceptionToClipboard(*context.ex);
						}
						return TRUE;
					}
				}

				return FALSE;
			}
			case WM_GETMINMAXINFO:
			{
				auto* m = (MINMAXINFO*)lParam;
				m->ptMinTrackSize.x = 800;
				m->ptMinTrackSize.y = 600;
				m->ptMaxTrackSize.x = 1280;
				m->ptMaxTrackSize.y = 768;
				return TRUE;
			}
			case WM_SIZE:
			{
				auto dx = LOWORD(lParam);
				auto dy = HIWORD(lParam);

				int buttonWidth = context.buttonSpan.x;

				auto hLogView = GetDlgItem(hDlg, context.spec->logViewId);

				if (context.stackDefaultHeight == 0)
				{
					MoveWindow(hLogView, 7, 7, dx - 28 - buttonWidth, dy - 14, TRUE);
				}
				else
				{
					auto hStackView = GetDlgItem(hDlg, context.spec->stackViewId);
					MoveWindow(hStackView, 7, 7, dx - 14, context.stackDefaultHeight, TRUE);
					MoveWindow(hLogView, 7, context.stackDefaultHeight + 14, dx - 28 - buttonWidth, dy - 21 - context.stackDefaultHeight, TRUE);
				}

				auto hCopyButton = GetDlgItem(hDlg, IDOK);
				MoveWindow(hCopyButton, dx - buttonWidth - 14, dy - 2 * context.buttonSpan.y - 14, buttonWidth, context.buttonSpan.y, TRUE);

				auto hCloseButton = GetDlgItem(hDlg, IDCANCEL);
				MoveWindow(hCloseButton, dx - buttonWidth - 14, dy - context.buttonSpan.y - 7, buttonWidth, context.buttonSpan.y, TRUE);

				InvalidateRect(hDlg, nullptr, TRUE);
				return TRUE;
			}
			}
		}
		return 0;
	}
}

namespace Rococo
{
	namespace Windows
	{
		void ShowExceptionDialog(const ExceptionDialogSpec& spec, HWND parent, IException& ex)
		{
			ANON::ExceptionEditorContext context;
			context.spec = &spec;
			context.ex = &ex;

			INT_PTR result = DialogBoxParamA(spec.dllInstance, spec.dialogTemplate, parent, ANON::ExceptionDialogProc, (LPARAM)&context);
			if (result == 0 || result == -1)
			{
				HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
				if (hr != HRESULT_FROM_WIN32(NO_ERROR))
				{
					char err[256];
					SafeFormat(err, sizeof(err), "ShowExceptionDialog: error 0x%8.8x:\n%s", hr, ex.Message());

					char file[256];
					GetModuleFileNameA(nullptr, file, sizeof(file));
					MessageBoxA(parent, err, file, MB_ICONERROR);
				}
			}
		}
	}
}
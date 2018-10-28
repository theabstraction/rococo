#include "sexy.windows.internal.h"

#include "resource.h"

#include <richedit.h>
#include <stdio.h>

#include <commctrl.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

namespace ANON
{
	using namespace Rococo::SexyWindows;

	class ExceptionEditorContext
	{
	public:
		HFONT hFont;
		CONTEXT context = { 0 };
		ExceptionEditorContext()
		{
			hFont = CreateFontA(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier New");
		}

		~ExceptionEditorContext()
		{
			DeleteObject(hFont);
		}
	};

	void AddColumns(int col, int width, const char* text, HWND hReportView)
	{
		LV_COLUMNA c = { 0 };
		c.cx = width;
		c.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_MINWIDTH;
		c.cxMin = width / 2;

		char buf[16];
		_snprintf_s(buf, _TRUNCATE, "%s", text);

		c.pszText = buf;

		ListView_InsertColumn(hReportView, col, &c);
	}

	void PopulateStackView(HWND hStackView, ExceptionEditorContext& c)
	{
		HANDLE hProcess = GetCurrentProcess();

		SymInitialize(hProcess, NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES);
		
		STACKFRAME64 frame = {0};
		frame.AddrPC.Offset = c.context.Rip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = c.context.Rbp;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Offset = c.context.Rsp;
		frame.AddrStack.Mode = AddrModeFlat;

		int index = 0;

		while (StackWalk(
			IMAGE_FILE_MACHINE_AMD64,
			hProcess,
			GetCurrentThread(),
			&frame,
			&c.context,
			nullptr,
			SymFunctionTableAccess64,
			SymGetModuleBase,
			nullptr
			))
		{
			if (index++ < 2)
			{
				continue; // Ignore first two stack frames -> they are our stack analysis functions!
			}
			// extract data
			LVITEMA item = { 0 };
			item.mask = LVIF_TEXT;
			char text[16];
			_snprintf_s(text, _TRUNCATE, "%d", index - 2);
			item.pszText = text;
			item.iItem = index;
			int index = ListView_InsertItem(hStackView, &item);

			LVITEMA address = { 0 };
			address.iItem = index;
			address.iSubItem = 1;
			address.mask = LVIF_TEXT;
			char addresstext[16];
			_snprintf_s(addresstext, _TRUNCATE, "%04.4X:%016llX", frame.AddrPC.Segment, frame.AddrPC.Offset);
			address.pszText = addresstext;
			ListView_SetItem(hStackView, &address);

			char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
			PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
			symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
			symbol->MaxNameLength = 254;

			DWORD64 moduleBase = SymGetModuleBase64(hProcess, frame.AddrPC.Offset);
			char moduleBuff[MAX_PATH];
			if (moduleBase && GetModuleFileNameA((HINSTANCE)moduleBase, moduleBuff, MAX_PATH))
			{
				LVITEMA mname = { 0 };
				mname.iItem = index;
				mname.iSubItem = 2;
				mname.mask = LVIF_TEXT;
				mname.pszText = (char*)moduleBuff;
				ListView_SetItem(hStackView, &mname);
			}

			if (SymGetSymFromAddr(hProcess, frame.AddrPC.Offset, NULL, symbol))
			{
				LVITEMA fname = { 0 };
				fname.iItem = index;
				fname.iSubItem = 3;
				fname.mask = LVIF_TEXT;
				fname.pszText = (char*)symbol->Name;
				ListView_SetItem(hStackView, &fname);
			}

			DWORD  offset = 0;
			IMAGEHLP_LINE line;
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

			if (SymGetLineFromAddr(hProcess, frame.AddrPC.Offset, &offset, &line))
			{
				LVITEMA lineCol = { 0 };
				lineCol.iItem = index;
				lineCol.iSubItem = 4;
				lineCol.mask = LVIF_TEXT;

				char src[256];
				_snprintf_s(src, _TRUNCATE, "%s #%d", line.FileName, line.LineNumber);
				lineCol.pszText = src;
				ListView_SetItem(hStackView, &lineCol);
			}
		}
	}

	INT_PTR CALLBACK ExceptionDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (uMsg == WM_INITDIALOG)
		{
			SetWindowLongPtrA(hDlg, GWLP_USERDATA, lParam);

			ExceptionEditorContext& context = *(ExceptionEditorContext*)lParam;

			SetWindowTextA(hDlg, "Wow");

			auto stackView = GetDlgItem(hDlg, IDC_STACKVIEW);

			SendMessageA(stackView, WM_SETFONT, (WPARAM) context.hFont, FALSE);

			AddColumns(0, 30, "#", stackView);
			AddColumns(1, 120, "Address", stackView);
			AddColumns(2, 160, "Module", stackView);
			AddColumns(3, 400, "Function", stackView);
			AddColumns(4, 400, "Source", stackView);

			PopulateStackView(stackView, context);

			return TRUE;
		}
		else
		{
			ExceptionEditorContext& context = *(ExceptionEditorContext*)GetWindowLongPtrA(hDlg, GWLP_USERDATA);
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

				return FALSE;
			}
			case WM_GETMINMAXINFO:
			{
				auto* m = (MINMAXINFO*)lParam;
				m->ptMinTrackSize.x = 400;
				m->ptMinTrackSize.y = 400;
				m->ptMaxTrackSize.x = 1280;
				m->ptMaxTrackSize.y = 768;
				return TRUE;
			}
			case WM_SIZE:
			{
				auto dx = LOWORD(lParam);
				auto dy = HIWORD(lParam);
				auto hStackView = GetDlgItem(hDlg, IDC_STACKVIEW);
				MoveWindow(hStackView, 7, 7, dx - 14, dy - 44, TRUE);

				auto hCopyButton = GetDlgItem(hDlg, IDOK);
				MoveWindow(hCopyButton, dx - 200, dy - 35, 80, 24, TRUE);

				auto hCloseButton = GetDlgItem(hDlg, IDCANCEL);
				MoveWindow(hCloseButton, dx - 100, dy - 35, 80, 24, TRUE);
				return TRUE;
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
		void ShowExceptionDialog(HINSTANCE dllInstance, HWND parent)
		{
			ANON::ExceptionEditorContext context;
			context.context.ContextFlags = CONTEXT_FULL;
			RtlCaptureContext(&context.context);
			INT_PTR result = DialogBoxParamA(dllInstance, (LPCSTR)IDD_EXCEPTION_DIALOG, parent, ANON::ExceptionDialogProc, (LPARAM)&context);
		}
	}
}
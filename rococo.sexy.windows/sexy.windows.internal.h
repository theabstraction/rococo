#pragma once

#include <rococo.os.win32.h>
#include <sexy.windows.h>

namespace Rococo
{
	struct IException;

	namespace SexyWindows
	{
		void AppendText_ToEditor(HWND hEditor, const char* text, COLORREF back, COLORREF fore);
		DialogResult ShowErrorDialog(const ErrorDialogSpec& e, HINSTANCE dllInstance);
		DialogResult ShowScriptedDialog(const ScriptedDialogSpec& info, HINSTANCE hDllInstance);
		void ShowExceptionDialog(HINSTANCE hInstance, HWND parent, IException& ex);

		struct Message
		{
			HWND hDlg;
			UINT uMsg;
			WPARAM wParam;
			LPARAM lParam;
			INT_PTR result;
		};

		struct IMessageHandler
		{
			virtual void Handle(Message& message) = 0;
			virtual void Free() = 0;
		};

		namespace Handler
		{
			IMessageHandler* NewEndDialog(INT_PTR id);
		} // Handler
	} // SexyWindows
} // Rococo

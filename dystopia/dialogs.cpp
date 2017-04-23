#include "dystopia.h"

#include <windows.h>
#include <rococo.window.h>

#include "dystopia.h"
#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <commctrl.h>


using namespace Rococo;
using namespace Rococo::Windows;
using namespace Dystopia;

namespace
{
	class ContinueDialog : public StandardWindowHandler
	{
	private:
		ModalDialogHandler modalHandler;
		IDialogSupervisor* dialogWindow;

		IButton* retryButton;
		IButton* ignoreButton;
		IButton* exitButton;
		IWindowSupervisor* label;

		ContinueDialog() : dialogWindow(nullptr)
		{
		}

		~ContinueDialog()
		{
			Rococo::Free(dialogWindow);
		}

		virtual void OnClose(HWND hWnd)
		{
			modalHandler.TerminateDialog(CMD_ID_RETRY);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			modalHandler.TerminateDialog(id);
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 520, 350 }, SW_SHOW, nullptr, L"Confirm Title", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
			dialogWindow = Windows::CreateDialogWindow(config, this);

			RECT rect;
			GetClientRect(*dialogWindow, &rect);
			label = AddLabel(*dialogWindow, GuiRect(rect.left + 10, rect.top + 10, rect.right - 10, rect.bottom - 100), L"Hint", 0, WS_BORDER);
			
			retryButton = AddPushButton(*dialogWindow, GuiRect(210, rect.bottom - 40, 290, rect.bottom - 10), L"Retry", CMD_ID_RETRY, BS_DEFPUSHBUTTON);
			ignoreButton = AddPushButton(*dialogWindow, GuiRect(310, rect.bottom - 40, 390, rect.bottom - 10), L"Ignore", CMD_ID_IGNORE, 0);
			exitButton = AddPushButton(*dialogWindow, GuiRect(410, rect.bottom - 40, 490, rect.bottom - 10), L"Exit", CMD_ID_EXIT, 0);

			SetControlFont(*label);
		}

	public:
		static ContinueDialog* Create()
		{
			auto m = new ContinueDialog();
			m->PostConstruct();
			return m;
		}

		DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, cstr title, cstr hint)
		{
			SetWindowText(*dialogWindow, title);
			SetWindowText(*label, hint);
			return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
		}

		void Free()
		{
			delete this;
		}

		virtual void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO& info)
		{
			info.ptMaxSize = POINT{ 520, 350 };
			info.ptMaxTrackSize = POINT{ 520, 350 };
			info.ptMinTrackSize = POINT{ 520, 350 };
		}
	};
}

namespace Dystopia
{
	CMD_ID ShowContinueBox(IWindow& renderWindow, cstr message)
	{
		AutoFree<ContinueDialog> dialog = ContinueDialog::Create();
		return (CMD_ID) dialog->DoModal(renderWindow, L"Error", message);
	}
}
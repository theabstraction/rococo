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

		DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, LPCWSTR title, LPCWSTR hint)
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

	enum
	{
		MENU_EXECUTE_NEXT = 1001,
		MENU_EXECUTE_OVER,
		MENU_EXECUTE_OUT,
		MENU_EXECUTE_NEXT_SYMBOL,
		MENU_EXECUTE_CONTINUE,
		MENU_SYS_EXIT
	};

	void PopulateDebuggingMenu(Windows::IWin32Menu& menu)
	{
		auto& debuggingMenu = menu.AddPopup(L"&Sys");
		debuggingMenu.AddString(L"E&xit", MENU_SYS_EXIT);
		auto& executionMenu = menu.AddPopup(L"&Execution");
		executionMenu.AddString(L"Step Next", MENU_EXECUTE_NEXT, L"F11");
		executionMenu.AddString(L"Step Over", MENU_EXECUTE_OVER, L"Shift + F10");
		executionMenu.AddString(L"Step Out", MENU_EXECUTE_OUT, L"Shift + F11");
		executionMenu.AddString(L"Step Next Symbol", MENU_EXECUTE_NEXT_SYMBOL, L"F10");
		executionMenu.AddString(L"Continue", MENU_EXECUTE_CONTINUE, L"F5");
	}

	class DebuggerWindowHandler : public StandardWindowHandler, public IDebuggerWindow
	{
		struct LogListEventHandler : IListViewEvents
		{
			virtual void OnDrawItem(DRAWITEMSTRUCT& dis)
			{

			}

			virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis)
			{

			}
		} listEventHandler;

		struct TreeEventHandler : ITreeControlHandler
		{
			virtual void OnDrawItem(DRAWITEMSTRUCT& dis)
			{

			}

			virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis)
			{

			}
		} treeEventHandler;
	private:
		IParentWindowSupervisor* window;
		IListViewSupervisor* logLines;
		IListViewSupervisor* disassemblyView;
		ITreeControlSupervisor* stackItemWindow;
		IListViewSupervisor* registerWindow;
		AutoFree<Windows::IWin32Menu> menu;
		IDebugControl* debugControl;

		bool isVisible;

		DebuggerWindowHandler() :
			window(nullptr),
			isVisible(false),
			logLines(nullptr),
			disassemblyView(nullptr),
			debugControl(nullptr),
			menu(Windows::CreateMenu())
		{
			PopulateDebuggingMenu(*menu);
		}

		~DebuggerWindowHandler()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		void PostConstruct(IWindow* parent)
		{
			WindowConfig config;
			HWND hParentWnd = parent ? (HWND) *parent : nullptr;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, hParentWnd, L"Dystopia Script Debugger", WS_OVERLAPPEDWINDOW, 0, *menu);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			logLines = Windows::AddListView(*window, GuiRect(0,0,0,0), L"", listEventHandler, LVS_REPORT, WS_BORDER, 0);
			disassemblyView = Windows::AddListView(*window, GuiRect(0, 0, 0, 0), L"", listEventHandler, LVS_REPORT, WS_BORDER, 0);

			stackItemWindow = Windows::AddTree(*window, GuiRect(1, 1, 2, 2), L"Stack View", 1008, treeEventHandler, WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_BORDER);
			registerWindow = Windows::AddListView(*window, GuiRect(1, 1, 2, 2), L"Registers", listEventHandler, LVS_REPORT | LVS_NOCOLUMNHEADER, WS_BORDER, 0);

			const wchar_t* colnames[] = { L"Name", L"Value", nullptr };
			int widths[] = { 50, 345 };
			registerWindow->UIList().SetColumns(colnames, widths);

			LayoutChildren();

			{
				const wchar_t* columns[] = { L"Log Entry", nullptr };
				int widths[] = { 4096, -1 };
				logLines->UIList().SetColumns(columns, widths);
			}

			{
				const wchar_t* columns[] = { L"Disassembly", nullptr };
				int widths[] = { 1000, -1 };
				disassemblyView->UIList().SetColumns(columns, widths);
			}
		}

		void LayoutChildren()
		{
			RECT rect;
			GetClientRect(*window, &rect);
			MoveWindow(*logLines, rect.left + 2, rect.bottom - 100,rect.right - 4, 98, TRUE);
			MoveWindow(*disassemblyView, rect.left + 2, rect.top + 2, 600, rect.bottom - 104, TRUE);
			MoveWindow(*stackItemWindow, 604, rect.top + 2, rect.right - 606, 400, TRUE);
			MoveWindow(*registerWindow, 604, rect.top + 404, rect.right - 606, rect.bottom - 506, TRUE);
		}
	public:
		static DebuggerWindowHandler* Create(IWindow* parent)
		{
			auto m = new DebuggerWindowHandler();
			m->PostConstruct(parent);
			return m;
		}

		void Free()
		{
			delete this;
		}

		void AddLogLine(const wchar_t* line)
		{
			enum { MAX_LOG_LINES = 100 };
			const wchar_t* row[2] = { line, nullptr };
			logLines->UIList().AddRow(row);
			if (logLines->UIList().NumberOfRows() > MAX_LOG_LINES)
			{
				logLines->UIList().DeleteRow(0);
			}
		}

		virtual void AddDisassembly(bool clearFirst, const wchar_t* text)
		{
			if (clearFirst)
			{
				disassemblyView->UIList().ClearRows();
			}
			
			if (text)
			{
				const wchar_t* row[2] = { text, nullptr };
				disassemblyView->UIList().AddRow(row);
			}
		}

		virtual Visitors::IUITree& StackTree()
		{
			return stackItemWindow->Tree();
		}

		virtual Visitors::IUIList& RegisterList()
		{
			return registerWindow->UIList();
		}

		virtual int Log(const wchar_t* format, ...)
		{
			va_list arglist;
			va_start(arglist, format);
			wchar_t text[4096];
			int nChars = SafeVFormat(text, _TRUNCATE, format, arglist);

			wchar_t* next = nullptr;
			const wchar_t* token = wcstok_s(text, L"\n", &next);

			while (token != nullptr)
			{
				AddLogLine(token);
				token = wcstok_s(nullptr, L"\n", &next);
			}

			return nChars;
		}

		virtual Windows::IWindow& GetDebuggerWindowControl()
		{
			return *window;
		}

		virtual bool IsVisible() const
		{
			return isVisible;
		}

		virtual void ShowWindow(bool show, IDebugControl* debugControl)
		{
			if (isVisible && !show)
			{
				::ShowWindow(*window, SW_HIDE);
			}
			else if (!isVisible && show)
			{
				::ShowWindow(*window, SW_SHOW);
			}
			isVisible = show;

			this->debugControl = debugControl;

			EnableMenuItem(*menu, MENU_EXECUTE_CONTINUE, debugControl ? MF_ENABLED : MF_GRAYED);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			switch (id)
			{
			case MENU_SYS_EXIT:
				OnClose(hWnd);
				break;
			}

			if (debugControl)
			{
				switch(id)
				{
				case MENU_EXECUTE_CONTINUE:
					debugControl->Continue();
					break;
				case MENU_EXECUTE_OUT:
					debugControl->StepOut();
					break;
				case MENU_EXECUTE_OVER:
					debugControl->StepOver();
					break;
				case MENU_EXECUTE_NEXT:
					debugControl->StepNext();
					break;
				case MENU_EXECUTE_NEXT_SYMBOL:
					debugControl->StepNextSymbol();
					break;
				}
			}
		}

		virtual void OnClose(HWND hWnd)
		{
			ShowWindow(false, nullptr);
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			LayoutChildren();
		}

		virtual IParentWindowSupervisor& Window()
		{
			return *window;
		}
	};
}

namespace Dystopia
{
	CMD_ID ShowContinueBox(IWindow& renderWindow, const wchar_t* message)
	{
		AutoFree<ContinueDialog> dialog = ContinueDialog::Create();
		return (CMD_ID) dialog->DoModal(renderWindow, L"Error", message);
	}

	IDebuggerWindow* CreateDebuggerWindow(IWindow* parent)
	{
		return DebuggerWindowHandler::Create(parent);
	}
}
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.types.h>
#include <rococo.window.h>
#include <rococo.maths.h>
#include <rococo.visitors.h>

#include <malloc.h>
#include <wchar.h>
#include <vector>
#include <unordered_map>
#include <string>

#include <CommCtrl.h>
#include <Windowsx.h>

#include <Richedit.h>

namespace
{
	HINSTANCE hThisInstance = nullptr;
	HICON hLargeIcon = nullptr;
	HICON hSmallIcon = nullptr;
	HINSTANCE hRichEditor = nullptr;
	HFONT hTitleFont = nullptr;
	HFONT hControlFont = nullptr;

	void Cleanup()
	{
		if (hTitleFont)
		{
			DeleteObject(hTitleFont);
			hTitleFont = nullptr;
		}

		if (hControlFont)
		{
			DeleteObject(hControlFont);
			hControlFont = nullptr;
		}
	}
}

namespace Rococo
{
	namespace Windows
	{
		void SetControlFont(HWND hControlWindow)
		{
			SendMessage(hControlWindow, WM_SETFONT, (WPARAM)hControlFont, 0);
		}

		void SetTitleFont(HWND hTitleBar)
		{
			SendMessage(hTitleBar, WM_SETFONT, (WPARAM)hTitleFont, 0);
		}

		void InitRococoWindows(HINSTANCE _hInstance, HICON _hLargeIcon, HICON _hSmallIcon, const LOGFONTW* titleFont, const LOGFONTW* controlFont)
		{
			if (_hInstance == nullptr)
			{
				Throw(0, L"Rococo::Windows::InitRococoWindows(...): _hInstance was null");
			}

			hThisInstance = _hInstance;
			hLargeIcon = _hLargeIcon;
			hSmallIcon = _hSmallIcon;

			LOGFONT defaultTitleFont = { 0 };
			SecureCopy(defaultTitleFont.lfFaceName, L"Courier New");
			defaultTitleFont.lfHeight = -11;

			hTitleFont = CreateFontIndirect(titleFont ? titleFont : & defaultTitleFont);
			if (hTitleFont == nullptr)
			{
				Throw(GetLastError(), L"Rococo::Windows::InitRococoWindows(...): CreateFontIndirect(&titlefont) returned null");
			}

			LOGFONT defaultControlFont = { 0 };
			SecureCopy(defaultControlFont.lfFaceName, L"Courier New");
			defaultControlFont.lfHeight = -11;

			hControlFont = CreateFontIndirect(controlFont ? controlFont : &defaultControlFont);
			if (hControlFont == nullptr)
			{
				Throw(GetLastError(), L"Rococo::Windows::InitRococoWindows(...): CreateFontIndirect(&controlFont) returned null");
			}

			atexit(Cleanup);
		}

		void ValidateInit()
		{
			if (hThisInstance == nullptr)
			{
				Throw(0, L"Rococo::Windows::InitRococoWindows(...) should first be called before using functions in the Rococo.windows library.");
			}
		}

		GuiRect ClientArea(HWND hWnd)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			return FromRECT(rect);
		}

		GuiRect WindowArea(HWND hWnd)
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);
			return FromRECT(rect);
		}

		void SetChildWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, LPCWSTR name, DWORD style, DWORD exStyle)
		{
			config.style = style | WS_CHILD | WS_VISIBLE;
			config.exStyle = exStyle;
			config.hMenu = nullptr;
			config.hWndParent = hWndParent;
			config.left = rect.left;
			config.top = rect.top;
			config.width = Width(rect);
			config.height = Height(rect);
			config.windowName = name;
		}

		void SetPopupWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, LPCWSTR name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
		{
			config.style = style | WS_POPUP;
			config.exStyle = exStyle;
			config.hMenu = hPopupMenu;
			config.hWndParent = hWndParent;
			config.left = rect.left;
			config.top = rect.top;
			config.width = Width(rect);
			config.height = Height(rect);
			config.windowName = name;
		}

		void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& span, int32 showWindowCommand, HWND hWndOwner, LPCWSTR name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
		{
			config.style = style | WS_OVERLAPPED;
			config.exStyle = exStyle;
			config.hMenu = hPopupMenu;
			config.hWndParent = hWndOwner;
			config.left = CW_USEDEFAULT;
			config.top = showWindowCommand;
			config.width = span.x;
			config.height = span.y;
			config.windowName = name;
		}

		void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& topLeft, const Vec2i& span, HWND hWndOwner, LPCWSTR name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
		{
			config.style = style | WS_OVERLAPPED;
			config.exStyle = exStyle;
			config.hMenu = hPopupMenu;
			config.hWndParent = hWndOwner;
			config.left = topLeft.x;
			config.top = topLeft.y;
			config.width = span.x;
			config.height = span.y;
			config.windowName = name;
		}
	}
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	ATOM customAtom = 0;

	LRESULT CALLBACK DefCustomWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		IWindowHandler* handler = (IWindowHandler*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		try
		{
			return handler->OnMessage(hWnd, uMsg, wParam, lParam);
		}
		catch (IException& ex)
		{
			ShowErrorBox(ex, L"Rococo Window Exception!");
			PostQuitMessage(ex.ErrorCode());
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	LPCWSTR customClassName = L"Rococo_WINDOW";

	ATOM CreateCustomAtom()
	{
		ValidateInit();

		WNDCLASSEX classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = hLargeIcon;
		classDef.hIconSm = hSmallIcon;
		classDef.hInstance = hThisInstance;
		classDef.lpszClassName = customClassName;
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProc; // DefCustomWindowProc;

		return RegisterClassExW(&classDef);
	}

	typedef std::vector<IWindowSupervisor*> TWindows;

	void DeleteAll(TWindows& windows)
	{
		for (auto i : windows)
		{
			i->Free();
		}

		windows.clear();
	}

	void StandardResizeControlWithTitleBar(HWND hContainerWindow, HWND hControlWindow, HWND hTitleWindow, int titleHeight = 16)
	{
		if (!hContainerWindow) return;

		GuiRect rect = ClientArea(hContainerWindow);
		if (hTitleWindow)
		{
			MoveWindow(hTitleWindow, 0, 0, Width(rect), titleHeight, TRUE);
			MoveWindow(hControlWindow, 0, titleHeight, Width(rect), Height(rect) - titleHeight, TRUE);
		}
		else
		{
			MoveWindow(hControlWindow, 0, 0, Width(rect), Height(rect), TRUE);
		}
	}
}

#include "button.control.inl"
#include "child.inl"
#include "dialog.inl"
#include "wired.handler.inl"
#include "listbox.inl"
#include "rich.editor.inl"
#include "tab.control.inl"
#include "tree.control.inl"
#include "combo.control.inl"
#include "trackbar.control.inl"
#include "menu.inl"
#include "listview.inl"

namespace Rococo
{
	namespace Windows
	{
		HWND CreateWindowIndirect(LPCWSTR className, const WindowConfig& c, IWindowHandler* _handler)
		{
			ValidateInit();

			HWND hWnd = CreateWindowExW(c.exStyle, className, c.windowName, c.style, c.left, c.top, c.width, c.height, c.hWndParent, c.hMenu, hThisInstance, _handler);
			if (hWnd == nullptr)
			{
				Throw(GetLastError(), L"Rococo::CreateWindowIndirect(...): CreateWindowExW failed");
			}

			if (className == (LPCWSTR)customAtom || className == customClassName)
			{
				SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)(VOID*)DefCustomWindowProc);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) static_cast<IWindowHandler*>(_handler));
			}

			return hWnd;
		}

		IDialogSupervisor* CreateDialogWindow(const WindowConfig& config, IWindowHandler* modelessHandler)
		{
			DialogWindowImpl* w = DialogWindowImpl::Create(config, true, modelessHandler);
			return w;
		}

		IParentWindowSupervisor* CreateChildWindow(const WindowConfig& config, IWindowHandler* handler)
		{
			DialogWindowImpl* w = DialogWindowImpl::Create(config, true, handler);
			return w;
		}

		ModalDialogHandler::ModalDialogHandler() : isRunning(false)
		{
		}

		IWiredWindowHandler* CreateWiredHandler()
		{
			return new WiredHandler();
		}

		void ModalDialogHandler::OnEnterModal()
		{
			isRunning = true;
		}

		DWORD ModalDialogHandler::OnExitModal()
		{
			return exitCode;
		}

		bool ModalDialogHandler::IsRunning()
		{
			return isRunning;
		}

		DWORD ModalDialogHandler::OnIdle()
		{
			return wiredHandler->OnIdle();
		}

		void ModalDialogHandler::TerminateDialog(DWORD exitCode)
		{
			this->exitCode = exitCode;
			this->isRunning = false;
		}

		IDialogSupervisor* ModalDialogHandler::CreateDialogWindow(const WindowConfig& config)
		{
			return Rococo::Windows::CreateDialogWindow(config, wiredHandler);
		}

		DWORD ModalDialogHandler::BlockModal(IDialogSupervisor& window, HWND hWndOwner)
		{
			return window.BlockModal(*this, hWndOwner, wiredHandler);
		}

		IButton* AddPushButton(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD styleEx)
		{
			DWORD trueStyle = style == 0 ? BS_PUSHBUTTON : style;
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, parent, name, trueStyle, styleEx);
			IButton* button = ButtonSupervisor::Create(childConfig, parent);
			SetDlgCtrlID(*button, id);
			return button;
		}

		ICheckbox* AddCheckBox(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD styleEx)
		{
			DWORD trueStyle = style == 0 ? BS_CHECKBOX : style;
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, parent, name, trueStyle, styleEx);
			ICheckbox* button = ButtonSupervisor::Create(childConfig, parent);
			SetDlgCtrlID(*button, id);
			return button;
		}

		IWindowSupervisor* AddLabel(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			return parent.AddChild(childConfig, L"STATIC", id);
		}

		ITabControl* AddTabs(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, ITabControlEvents& eventHandler, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			TabControlSupervisor* t = TabControlSupervisor::Create(childConfig, parent, eventHandler);
			return t;
		}

		IWindowSupervisor* AddEditor(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			return parent.AddChild(childConfig, L"EDIT", id);
		}

		ITreeControlSupervisor* AddTree(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, ITreeControlHandler& eventHandler, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			TreeControlSupervisor* t = TreeControlSupervisor::Create(childConfig, parent, id, eventHandler);
			return t;
		}

		IListViewSupervisor* AddListView(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, IListViewEvents& eventHandler, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, containerStyleEx);
			ListViewSupervisor* t = ListViewSupervisor::Create(childConfig, parent, eventHandler, containerStyle);
			return t;
		}

		IRichEditor* AddRichEditor(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, IRichEditorEvents& eventHandler, DWORD style, DWORD styleEx)
		{
			if (hRichEditor == nullptr)
			{
				hRichEditor = LoadLibrary(L"Riched20.dll");
			}

			if (hRichEditor == nullptr)
			{
				Throw(GetLastError(), L"Error loading Riched20.dll");
			}

			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);

			RichEditor* editor = RichEditor::Create(childConfig, parent, eventHandler);
			return editor;
		}

		IListWindowSupervisor* AddListbox(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, IListItemHandler& listRenderer, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, 0);
			ListBoxSupervisor* w = ListBoxSupervisor::Create(config, parent, listRenderer, containerStyle, containerStyleEx);
			return w;
		}

		IComboBoxSupervisor* AddComboBox(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, 0);
			ComboBoxSupervisor* w = ComboBoxSupervisor::Create(config, parent, nullptr, containerStyle, containerStyleEx);
			return w;
		}

		ITrackBarSupervisor* AddTrackbar(IParentWindowSupervisor& parent, const GuiRect& rect, LPCWSTR name, ControlId id, DWORD style, DWORD styleEx, ITrackBarHandler& handler)
		{
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, styleEx);
			TrackBarSupervisor* w = TrackBarSupervisor::Create(config, parent, handler);
			return w;
		}

		void SetText(HWND hWnd, size_t capacity, const wchar_t* format, ...)
		{
			if (hWnd == nullptr) Throw(0, L"SetText failed. hWnd was null.");

			va_list args;
			va_start(args, format);

			wchar_t* text = (wchar_t*)_malloca(capacity * sizeof(wchar_t));
			SafeVFormat(text, capacity, _TRUNCATE, format, args);

			SetWindowTextW(hWnd, text);

			_freea(text);
		}

		void SetDlgCtrlID(HWND hWnd, DWORD id)
		{
			if (hWnd == nullptr)
			{
				Throw(0, L"SetDlgCtrlID: The window handle was null");
			}
			SetWindowLongPtr(hWnd, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(id)));
		}

		bool ModalQuery(HWND hWnd, LPCWSTR caption, LPCWSTR question)
		{
			return MessageBox(hWnd, question, caption, MB_ICONQUESTION | MB_YESNO) == IDYES;
		}

		void StandardWindowHandler::OnDestroy(HWND hWnd)
		{
			DefWindowProc(hWnd, WM_DESTROY, 0, 0);
		}

		LRESULT StandardWindowHandler::OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			return DefWindowProc(hWnd, WM_TIMER, wParam, lParam);
		}

		LRESULT StandardWindowHandler::OnInput(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
		}

		LRESULT StandardWindowHandler::OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_COMMAND:
				return OnCommand(hWnd, wParam, lParam);
			case WM_CLOSE:
				OnClose(hWnd);
				return 0L;
			case WM_DESTROY:
				OnDestroy(hWnd);
				return 0;
			case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hWnd, &ps);

					OnPaint(hWnd, ps, hdc);

					EndPaint(hWnd, &ps);
					return 0L;
				}
			case WM_SIZE:
				OnSize(hWnd, Vec2i(LOWORD(lParam), HIWORD(lParam)), (RESIZE_TYPE)wParam);
				return 0L;
			case WM_GETMINMAXINFO:
				OnGetMinMaxInfo(hWnd, *reinterpret_cast<MINMAXINFO*>(lParam));
				return 0;
			case WM_ERASEBKGND:
				OnEraseBackground(hWnd, (HDC)wParam);
				return 0;
			case WM_INPUT:
				return OnInput(hWnd, wParam, lParam);
			case WM_TIMER:
				return OnTimer(hWnd, wParam, lParam);	
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT StandardWindowHandler::OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
		{
			return DefWindowProc(hWnd, WM_COMMAND, id | (notificationCode << 16), (LPARAM)hControlCode);
		}

		StandardWindowHandler::StandardWindowHandler()
		{
			backgroundColour = RGB(255, 255, 255);
		}

		void StandardWindowHandler::OnPaint(HWND hWnd, PAINTSTRUCT& ps, HDC hdc) {}
		void StandardWindowHandler::OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) {}
		void StandardWindowHandler::OnMenuCommand(HWND hWnd, DWORD id) {}
		void StandardWindowHandler::OnAcceleratorCommand(HWND hWnd, DWORD id) {}
		void StandardWindowHandler::OnClose(HWND hWnd) {}

		COLORREF StandardWindowHandler::GetBackgroundColour() { return backgroundColour; }
		void StandardWindowHandler::SetBackgroundColour(COLORREF bkColour) { backgroundColour = bkColour; }

		LRESULT StandardWindowHandler::OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DWORD src = HIWORD(wParam);
			DWORD id = LOWORD(wParam);

			if (src == 0)
			{
				OnMenuCommand(hWnd, id);
				return 0L;
			}
			else if (src == 1 && lParam == 0)
			{
				OnAcceleratorCommand(hWnd, id);
				return 0L;
			}
			else
			{
				return OnControlCommand(hWnd, src, id, (HWND)lParam);
			}
		}

		void StandardWindowHandler::OnEraseBackground(HWND hWnd, HDC dc)
		{
			HBRUSH br = CreateSolidBrush(GetBackgroundColour());
			RECT rect;
			GetClientRect(hWnd, &rect);
			FillRect(dc, &rect, br);
			DeleteObject(br);
		}

		void StandardWindowHandler::OnGetMinMaxInfo(HWND hWnd, MINMAXINFO& info)
		{
			enum { DEFAULT_MIN_WIDTH = 800, DEFAULT_MIN_HEIGHT = 600 };

			info.ptMinTrackSize.x = DEFAULT_MIN_WIDTH;
			info.ptMinTrackSize.y = DEFAULT_MIN_HEIGHT;
		}

		IWin32Menu* CreateMenu()
		{
			return new Menu();
		}
	} // Windows
} // Rococo
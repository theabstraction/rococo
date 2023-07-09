#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.api.h>
#include <rococo.os.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.window.h>
#include <rococo.maths.h>


#include <malloc.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <string>

#include <CommCtrl.h>
#include <Windowsx.h>

#include <Richedit.h>

#include <Commdlg.h>

#pragma comment(lib, "ComCtl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
	HINSTANCE hThisInstance = nullptr;
	HICON hLargeIcon = nullptr;
	HICON hSmallIcon = nullptr;
	HINSTANCE hRichEditor = nullptr;
	HFONT hTitleFont = nullptr;
	HFONT hControlFont = nullptr;
}

using namespace Rococo::Strings;

namespace Rococo
{
	namespace Windows
	{
		void SetControlFont(HWND hControlWindow)
		{
			SendMessage(hControlWindow, WM_SETFONT, (WPARAM)hControlFont, TRUE);
		}

      void SetTitleFont(HWND hTitleBar)
      {
         SendMessage(hTitleBar, WM_SETFONT, (WPARAM)hTitleFont, 0);
      }

      IWindow& NullParent()
      {
         class NullParentClass : public IWindow
         {
         public:
            virtual operator HWND () const
            {
               return nullptr;
            }
         };

         static NullParentClass s_null;
         return s_null;
      }

      bool OpenChooseFontBox(HWND hParent, LOGFONTA& output)
      {
         CHOOSEFONTA font = { 0 };
         font.lStructSize = sizeof(font);
         font.hInstance = hThisInstance;
         font.hwndOwner = hParent;
         
         LOGFONTA f = { 0 };
         StackStringBuilder sb(f.lfFaceName, sizeof(f.lfFaceName));
         sb << "Courier New";

         font.lpLogFont = &f;
         font.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT;
         
         if (ChooseFontA(&font))
         {
            output = f;
            return true;
         }
         else
         {
            output = LOGFONTA{ 0 };
            return false;
         }
      }

	  void InitRococoWindows(HINSTANCE _hInstance, HICON _hLargeIcon, HICON _hSmallIcon, const LOGFONTA* titleFont, const LOGFONTA* controlFont)
	  {
		  /*
		  if (_hInstance == nullptr)
		  {
			  Throw(0, "Rococo::Windows::InitRococoWindows(...): _hInstance was nul");
		  }
		  */

		  hThisInstance = _hInstance;
		  hLargeIcon = _hLargeIcon;
		  hSmallIcon = _hSmallIcon;

		  LOGFONTA defaultTitleFont = { 0 };
		  SafeFormat(defaultTitleFont.lfFaceName, sizeof(defaultTitleFont.lfFaceName), "Consolas");
		  defaultTitleFont.lfHeight = 14;

		  hTitleFont = CreateFontIndirectA(titleFont ? titleFont : &defaultTitleFont);
		  if (hTitleFont == nullptr)
		  {
			  Throw(GetLastError(), "Rococo::Windows::InitRococoWindows(...): CreateFontIndirect(&titlefont) returned nul");
		  }

		  LOGFONTA defaultControlFont = { 0 };
		  SafeFormat(defaultControlFont.lfFaceName, sizeof(defaultControlFont.lfFaceName), "Consolas");
		  defaultControlFont.lfHeight = 14;

		  hControlFont = CreateFontIndirectA(controlFont ? controlFont : &defaultControlFont);
		  if (hControlFont == nullptr)
		  {
			  Throw(GetLastError(), "Rococo::Windows::InitRococoWindows(...): CreateFontIndirect(&controlFont) returned nul");
		  }

		  BOOL isTrue = TRUE, isFalse = FALSE;
		  SystemParametersInfo(SPI_SETMENUANIMATION, 0, &isTrue, 0);
		  SystemParametersInfo(SPI_SETMENUFADE, 0, &isFalse, 0);

		  InitCommonControls();

		  struct ANON
		  {
			  static void Cleanup()
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
		  };

		  atexit(ANON::Cleanup);
	  }

		void ValidateInit()
		{
			/*
			if (hThisInstance == nullptr)
			{
				Throw(0, "Rococo::Windows::InitRococoWindows(...) should first be called before using functions in the Rococo.windows library.");
			}
			*/
		}

		ROCOCO_WINDOWS_API GuiRect ClientArea(HWND hWnd)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			return FromRECT(rect);
		}

		ROCOCO_WINDOWS_API GuiRect WindowArea(HWND hWnd)
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);
			return FromRECT(rect);
		}

		ROCOCO_WINDOWS_API void SetChildWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle)
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

		ROCOCO_WINDOWS_API void SetPopupWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
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

		ROCOCO_WINDOWS_API void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& span, int32 showWindowCommand, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
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

		ROCOCO_WINDOWS_API void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& topLeft, const Vec2i& span, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu)
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
			struct : public IWindow
			{
				HWND hWnd;
				virtual operator HWND () const
				{
					return nullptr;
				}
			} local;

			local.hWnd = hWnd;

			Windows::ShowErrorBox(local, ex, "Rococo Window Exception!");
			PostQuitMessage(ex.ErrorCode());
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	}

	cstr customClassName = "Rococo_WINDOW";

	ATOM CreateCustomAtom()
	{
		ValidateInit();

		WNDCLASSEXA classDef = { 0 };
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
		classDef.lpfnWndProc = DefWindowProcA;

		auto atom = RegisterClassExA(&classDef);

      if (atom == 0)
      {
         int err = GetLastError();
		 if (err != ERROR_CLASS_ALREADY_EXISTS)
		 {
			 Throw(err, "Error creating custom atom. Bad hIcon/hInstance maybe?");
		 }
      }

      return atom;
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
		HWND CreateWindowIndirect(cstr className, const WindowConfig& c, IWindowHandler* _handler)
		{
			ValidateInit();

			HWND hWnd = CreateWindowExA(c.exStyle, className, c.windowName, c.style, c.left, c.top, c.width, c.height, c.hWndParent, c.hMenu, hThisInstance, _handler);
			if (hWnd == nullptr)
			{
				Throw(GetLastError(), "Rococo::CreateWindowIndirect(...): CreateWindowExW failed");
			}

			if (className == (cstr)customAtom || Eq(className, customClassName))
			{
				SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)(VOID*)DefCustomWindowProc);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) static_cast<IWindowHandler*>(_handler));
			}

			return hWnd;
		}

		IDialogSupervisor* CreateDialogWindow(const WindowConfig& config, IWindowHandler* modelessHandler)
		{
			DialogWindowImpl* w = DialogWindowImpl::Create(config, modelessHandler);
			return w;
		}

		IParentWindowSupervisor* CreateChildWindow(const WindowConfig& config, IWindowHandler* handler)
		{
			DialogWindowImpl* w = DialogWindowImpl::Create(config, handler);
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

		IButton* AddPushButton(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx)
		{
			DWORD trueStyle = style == 0 ? BS_PUSHBUTTON : style;
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, parent, name, trueStyle, styleEx);
			IButton* button = ButtonSupervisor::Create(childConfig, parent);
			SetDlgCtrlID(*button, id);
			return button;
		}

		ICheckbox* AddCheckBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx)
		{
			DWORD trueStyle = style == 0 ? BS_CHECKBOX : style;
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, parent, name, trueStyle, styleEx);
			ICheckbox* button = ButtonSupervisor::Create(childConfig, parent);
			SetDlgCtrlID(*button, id);
			return button;
		}

		IWindowSupervisor* AddLabel(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			return parent.AddChild(childConfig, "STATIC", id);
		}

		ITabControl* AddTabs(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITabControlEvents& eventHandler, DWORD style, DWORD styleEx)
		{
			UNUSED(id);
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			TabControlSupervisor* t = TabControlSupervisor::Create(childConfig, parent, eventHandler);
			return t;
		}

		IWindowSupervisor* AddEditor(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			return parent.AddChild(childConfig, "EDIT", id);
		}

		ITreeControlSupervisor* AddTree(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITreeControlHandler& eventHandler, DWORD style, DWORD styleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);
			TreeControlSupervisor* t = TreeControlSupervisor::Create(childConfig, parent, id, eventHandler);
			return t;
		}

		IListViewSupervisor* AddListView(IWindow& parent, const GuiRect& rect, cstr name, IListViewEvents& eventHandler, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, containerStyleEx);
			ListViewSupervisor* t = ListViewSupervisor::Create(childConfig, parent, eventHandler, containerStyle);
			return t;
		}

		IRichEditor* AddRichEditor(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, IRichEditorEvents& eventHandler, DWORD style, DWORD styleEx)
		{
			UNUSED(id);

			if (hRichEditor == nullptr)
			{
				hRichEditor = LoadLibraryA("Riched20.dll");
			}

			if (hRichEditor == nullptr)
			{
				Throw(GetLastError(), "Error loading Riched20.dll");
			}

			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, rect, nullptr, name, style, styleEx);

			RichEditor* editor = RichEditor::Create(childConfig, parent, eventHandler);
			return editor;
		}

		IListWindowSupervisor* AddListbox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, IListItemHandler& listRenderer, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, 0);
			ListBoxSupervisor* w = ListBoxSupervisor::Create(config, parent, listRenderer, containerStyle, containerStyleEx);
			return w;
		}

		IComboBoxSupervisor* AddComboBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD containerStyle, DWORD containerStyleEx)
		{
			UNUSED(id);
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, 0);
			ComboBoxSupervisor* w = ComboBoxSupervisor::Create(config, parent, nullptr, containerStyle, containerStyleEx);
			return w;
		}

		ITrackBarSupervisor* AddTrackbar(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx, ITrackBarHandler& handler)
		{
			UNUSED(id);
			WindowConfig config;
			Windows::SetChildWindowConfig(config, rect, parent, name, style, styleEx);
			TrackBarSupervisor* w = TrackBarSupervisor::Create(config, parent, handler);
			return w;
		}

		void SetText(HWND hWnd, size_t capacity, cstr format, ...)
		{
			if (hWnd == nullptr) Throw(0, "SetText failed. hWnd was null.");

			va_list args;
			va_start(args, format);

			char* text = (char*)_malloca(capacity * sizeof(char));
			Rococo::Strings::SafeVFormat(text, capacity, format, args);

			SetWindowTextA(hWnd, text);

			_freea(text);
		}

		void SetDlgCtrlID(HWND hWnd, DWORD id)
		{
			if (hWnd == nullptr)
			{
				Throw(0, "SetDlgCtrlID: The window handle was nul");
			}
			SetWindowLongPtr(hWnd, GWLP_ID, static_cast<LONG_PTR>(static_cast<DWORD_PTR>(id)));
		}

		bool ModalQuery(HWND hWnd, cstr caption, cstr question)
		{
			return MessageBoxA(hWnd, question, caption, MB_ICONQUESTION | MB_YESNO) == IDYES;
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

      LRESULT StandardWindowHandler::OnKeydown(HWND hWnd, WPARAM wParam, LPARAM lParam)
      {
         return DefWindowProc(hWnd, WM_KEYDOWN, wParam, lParam);
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
				OnSize(hWnd, Vec2i{ LOWORD(lParam), HIWORD(lParam) }, (RESIZE_TYPE)wParam);
				return 0L;
			case WM_GETMINMAXINFO:
				OnGetMinMaxInfo(hWnd, *reinterpret_cast<MINMAXINFO*>(lParam));
				return 0;
			case WM_ERASEBKGND:
				OnEraseBackground(hWnd, (HDC)wParam);
				return 0;
			case WM_SETCURSOR:
				return OnSetCursor(hWnd, wParam, lParam);
			case WM_INPUT:
				return OnInput(hWnd, wParam, lParam);
			case WM_TIMER:
				return OnTimer(hWnd, wParam, lParam);	
			case WM_KEYDOWN:
				return OnKeydown(hWnd, wParam, lParam);
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

      LRESULT StandardWindowHandler::OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam)
      {
         return DefWindowProc(hWnd, WM_SETCURSOR, wParam, lParam);
      }

		LRESULT StandardWindowHandler::OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
		{
			return DefWindowProc(hWnd, WM_COMMAND, id | (notificationCode << 16), (LPARAM)hControlCode);
		}

		StandardWindowHandler::StandardWindowHandler()
		{
			backgroundColour = RGB(255, 255, 255);
		}

		ROCOCO_WINDOWS_API void StandardWindowHandler::OnPaint(HWND, PAINTSTRUCT&, HDC) { }
		ROCOCO_WINDOWS_API void StandardWindowHandler::OnSize(HWND, const Vec2i& span, RESIZE_TYPE) { UNUSED(span); }
		ROCOCO_WINDOWS_API void StandardWindowHandler::OnMenuCommand(HWND, DWORD id) { UNUSED(id); }
		ROCOCO_WINDOWS_API void StandardWindowHandler::OnAcceleratorCommand(HWND, DWORD id) { UNUSED(id); }
		ROCOCO_WINDOWS_API void StandardWindowHandler::OnClose(HWND) {}

		COLORREF StandardWindowHandler::GetBackgroundColour() { return backgroundColour; }
		ROCOCO_WINDOWS_API void StandardWindowHandler::SetBackgroundColour(COLORREF bkColour) { backgroundColour = bkColour; }

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

		ROCOCO_WINDOWS_API void StandardWindowHandler::OnEraseBackground(HWND hWnd, HDC dc)
		{
			HBRUSH br = CreateSolidBrush(GetBackgroundColour());
			RECT rect;
			GetClientRect(hWnd, &rect);
			FillRect(dc, &rect, br);
			DeleteObject(br);
		}

		ROCOCO_WINDOWS_API void StandardWindowHandler::OnPretranslateMessage(MSG&)
		{

		}

		ROCOCO_WINDOWS_API void StandardWindowHandler::OnGetMinMaxInfo(HWND, MINMAXINFO& info)
		{
			enum { DEFAULT_MIN_WIDTH = 800, DEFAULT_MIN_HEIGHT = 600 };

			info.ptMinTrackSize.x = DEFAULT_MIN_WIDTH;
			info.ptMinTrackSize.y = DEFAULT_MIN_HEIGHT;
		}

		ROCOCO_WINDOWS_API IWin32Menu* CreateMenu(bool contextMenu)
		{
			return new Menu(contextMenu);
		}
	} // Windows
} // Rococo

#include <sexy.lib.util.h>
#include <sexy.lib.script.h>
#include <sexy.lib.sexy-util.h>
#include <sexy.lib.s-parser.h>
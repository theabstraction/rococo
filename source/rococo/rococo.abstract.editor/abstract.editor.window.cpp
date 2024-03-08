#include <rococo.os.win32.h>
#include <rococo.editors.h>
#include <rococo.abstract.editor.win32.h>
#include "resource.h"
#include <rococo.os.h>
#include <rococo.window.h>
#include <windowsx.h>

using namespace Rococo;
using namespace Rococo::Abedit;
using namespace Rococo::Windows;
using namespace Rococo::Editors;

namespace ANON
{
	static const char* const abEditClassName = "AbEditMainWindow_1_0";

	class AbeditPropertiesWindow : public StandardWindowHandler
	{
	private:
		IParentWindowSupervisor* window;
		AutoFree<IUIPropertiesEditorSupervisor> properties;

		HBRUSH hFocusBrush = nullptr;
		COLORREF focusColour;

		HBRUSH hButtonFocusBrush = nullptr;
		COLORREF buttonFocusColour;

		AbeditPropertiesWindow() : window(nullptr)
		{
			focusColour = RGB(255, 240, 240);
			hFocusBrush = CreateSolidBrush(focusColour);

			buttonFocusColour = RGB(192, 128, 128);
			hButtonFocusBrush = CreateSolidBrush(buttonFocusColour);
		}

		~AbeditPropertiesWindow()
		{
			Rococo::Free(window);

			DeleteObject(hFocusBrush);
		}

		void PostConstruct(IWindow* parent)
		{
			GuiRect tabRect = { 0,0, 200, 20 };

			Windows::WindowConfig config;
			DWORD style = WS_VISIBLE | WS_CHILD;
			SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, "Blank", style, 0);
			window = Windows::CreateChildWindow(config, this);

			properties = Windows::CreatePropertiesEditor(*window);
		}

		void OnPaint()
		{
			PAINTSTRUCT ps;
			BeginPaint(*window, &ps);

			HPEN hPenOld;

			HPEN hLinePen;
			COLORREF qLineColor = RGB(128, 128, 128);
			hLinePen = CreatePen(PS_SOLID, 1, qLineColor);
			hPenOld = (HPEN)SelectObject(ps.hdc, hLinePen);

			RECT rect;
			GetClientRect(*window, &rect);

			MoveToEx(ps.hdc, rect.left, rect.top, NULL);
			LineTo(ps.hdc, rect.left, rect.bottom - 1);
			LineTo(ps.hdc, rect.right - 1, rect.bottom - 1);

			SelectObject(ps.hdc, hPenOld);
			DeleteObject(hLinePen);

			EndPaint(*window, &ps);
		}

		LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam) override
		{
			auto id = LOWORD(wParam);
			auto command = HIWORD(wParam);
			switch (command)
			{
				case EN_CHANGE:
					properties->OnEditorChanged(UI::SysWidgetId{ id });
					return 0L;
				case EN_KILLFOCUS:	
					properties->OnEditorLostKeyboardFocus(UI::SysWidgetId{ id });
					return 0L;
				case BN_CLICKED:
					properties->OnButtonClicked(UI::SysWidgetId{ id });
					return 0L;
			}

			return StandardWindowHandler::OnCommand(hWnd, wParam, lParam);
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_CTLCOLOREDIT:
			{
				HWND hEditor = (HWND)lParam;

				HDC hdc = (HDC)wParam;

				if (GetFocus() == hEditor)
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, focusColour);
					return (LRESULT) hFocusBrush;
				}
				else
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
					return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
				}
			}
			case WM_CTLCOLORBTN:
			{
				HWND hButton = (HWND)lParam;

				HDC hdc = (HDC)wParam;

				if (GetFocus() == hButton)
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, buttonFocusColour);
					return (LRESULT)hButtonFocusBrush;
				}
				else
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
					return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
				}
			}
			case WM_PAINT:
				OnPaint();
				return 0L;
			case WM_NAVIGATE_BY_TAB:
				{
					UI::SysWidgetId id{ (uint16) wParam };
					properties->NavigateByTabFrom(id, (int) lParam);
					return 0L;
				}
			case WM_ADVANCE_COMBO_LIST:
			{
				UI::SysWidgetId id{ (uint16)wParam };
				properties->NavigateByTabFrom(id, (int)lParam);
				return 0L;
			}
			case WM_ADVANCE_SELECTION:
				{
				UI::SysWidgetId id{ (uint16)wParam };
				properties->AdvanceSelection(id);
				return 0L;
				}
			case WM_DRAWITEM:
				{
					DRAWITEMSTRUCT* d = (DRAWITEMSTRUCT*)lParam;
					if (d->CtlType == ODT_BUTTON)
					{
						auto* item = (IOwnerDrawItem*) GetWindowLongPtrA(d->hwndItem, GWLP_USERDATA);
						if (item)
						{
							item->DrawItem(*d);
							return 0L;
						}
					}
				}
				break;
			}

			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void LayoutChildren()
		{
			properties->LayouVertically();
		}

		void OnSize(HWND, const Vec2i& span, RESIZE_TYPE) override
		{
			UNUSED(span);
			LayoutChildren();
		}

		LRESULT OnSetCursor(HWND, WPARAM, LPARAM)
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));
			return TRUE;
		}

		ColourScheme scheme;
	public:
		static AbeditPropertiesWindow* Create(IWindow* parent)
		{
			auto node = new AbeditPropertiesWindow();
			node->SetBackgroundColour(RGB(192, 192, 192));
			node->PostConstruct(parent);
			return node;
		}

		void SetColourSchemeRecursive(const ColourScheme& scheme)
		{
			this->scheme = scheme;
			SetBackgroundColour(ToCOLORREF(scheme.backColour));
		}

		void Free()
		{
			delete this;
		}

		IWindow& GetWindow()
		{
			return *window;
		}

		IParentWindowSupervisor& Supervisor()
		{
			return *window;
		}

		IUIPropertiesEditor& Properties()
		{
			return *properties;
		}
	};

	enum
	{
		WM_PAINT_SPLITTER_PREVIEW = WM_USER + 1,
		WM_SET_HORZ_SPLITTER
	};

	class AbeditSplitter: public StandardWindowHandler
	{
	private:
		IDialogSupervisor* window = nullptr;

		AbeditSplitter()
		{

		}

		~AbeditSplitter()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		int dragStart = -1;
		int dragNew = -1;

		RECT previousGuideRect{ -1,-1,-1,-1 };
	public:
		operator HWND()
		{
			return *window;
		}

		void OnErase(HDC dc)
		{
			HBRUSH hBrush = (HBRUSH) GetStockObject(LTGRAY_BRUSH);
			RECT rect;
			GetClientRect(*window, &rect);
			FillRect(dc, &rect, hBrush);
		}

		void PaintGuide()
		{
			HWND hParent = GetParent(*window);
			HDC dc = GetDC(hParent);

			RECT guideRect = GetGuideRect();

			FillRect(dc, &guideRect, (HBRUSH)GetStockObject(GRAY_BRUSH));
			
			ReleaseDC(hParent, dc);

			ValidateRect(hParent, &guideRect);

			if (previousGuideRect.left > -1)
			{
				InvalidateRect(hParent, &previousGuideRect, TRUE);
			}

			previousGuideRect = guideRect;
		}

		RECT GetGuideRect()
		{
			HWND hParent = GetParent(*window);

			RECT windowScreenRect;
			GetWindowRect(*window, &windowScreenRect);

			POINT windowParentRect{ windowScreenRect.left, windowScreenRect.top };
			ScreenToClient(hParent, &windowParentRect);

			POINT p, q;
			p.x = windowParentRect.x + dragNew;
			p.y = windowParentRect.y;
			q.x = p.x;
			q.y = windowScreenRect.bottom - windowScreenRect.top;

			return RECT{ p.x - 1,  p.y, q.x + 1, q.y };
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				OnErase((HDC) wParam);
				return 0L;
			case WM_SETCURSOR:
				SetCursor(LoadCursor(NULL, IDC_SIZEWE));
				return 0L;
			case WM_LBUTTONDOWN:
				dragStart = GET_X_LPARAM(lParam);
				dragNew = dragStart;
				SetCapture(*window);
				return 0L;
			case WM_MOUSEMOVE:
				if (dragStart >= 0)
				{
					dragNew = GET_X_LPARAM(lParam);
					RECT guideRect = GetGuideRect();
					InvalidateRect(GetParent(hWnd), &guideRect, TRUE);
				}
				return 0L;
			case WM_LBUTTONUP:
				if (dragStart != -1)
				{
					SetCapture(NULL);

					RECT screenRect;
					GetWindowRect(hWnd, &screenRect);

					POINT topLeft = { screenRect.left, screenRect.top };
					ScreenToClient(GetParent(hWnd), &topLeft);

					SendMessage(GetParent(hWnd), WM_SET_HORZ_SPLITTER, topLeft.x + dragNew - dragStart, 0);
					dragStart = -1;
					dragNew = -1;
				}

				return 0L;
			case WM_PAINT_SPLITTER_PREVIEW:
				if (dragStart != -1)
				{
					PaintGuide();
				}
				return 0L;
			}
			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void PostConstruct(HINSTANCE hDll, HWND hParentWnd, Vec2i topLeft, Vec2i span)
		{
			UNUSED(hDll);

			GuiRect rect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

			WindowConfig config;
			Rococo::Windows::SetChildWindowConfig(config, rect, hParentWnd, "Splitter", WS_CHILD | WS_VISIBLE, 0);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			Layout();
		}

		void Layout()
		{
		}

		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static AbeditSplitter* Create(HINSTANCE hDll, HWND hParentWnd, Vec2i topLeft, Vec2i span)
		{
			auto m = new AbeditSplitter();
			m->PostConstruct(hDll, hParentWnd, topLeft, span);
			return m;
		}

		void Free()
		{
			delete this;
		}
	};

	class AbeditSlate: public StandardWindowHandler
	{
	private:
		AutoFree<IParentWindowSupervisor> window;
		HBRUSH hBrush{ nullptr };

		void PostConstruct(HWND hParent)
		{
			WindowConfig config;
			Rococo::Windows::SetChildWindowConfig(config, GuiRect{ 0,0,0,0 }, hParent, "Slate", WS_CHILD | WS_VISIBLE, 0);
			hBrush = CreateSolidBrush(RGB(0, 255, 0));
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
		}

		~AbeditSlate()
		{
			DeleteObject(hBrush);
		}
	public:
		void OnEraseBackground(HDC dc)
		{
			RECT rect;
			GetClientRect(*window, &rect);
			FillRect(dc, &rect, hBrush);
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				OnEraseBackground((HDC)wParam);
				return 1L;
			}
			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		static AbeditSlate* Create(HWND hParent)
		{
			auto node = new AbeditSlate();
			node->SetBackgroundColour(RGB(192, 192, 192));
			node->PostConstruct(hParent);
			return node;
		}

		IParentWindowSupervisor& Window()
		{
			return *window;
		}

		void Free()
		{
			delete this;
		}
	};

	class AbeditMainWindow : public StandardWindowHandler, public IAbeditMainWindowSupervisor
	{
	private:
		IAbstractEditorMainWindowEventHandler& eventHandler;
		AutoFree<IParentWindowSupervisor> window;
		AutoFree<AbeditPropertiesWindow> propertiesPanel;
		AutoFree<AbeditSplitter> rhsSplitter;
		AutoFree<AbeditSlate> slate;
		AutoFree<IWin32Menu> mainMenu;

		enum class MenuItem: uint16
		{
			New = 3500,
			Load,
			Save,
			SaveAs,
			Exit
		};

		AbeditMainWindow(IAbstractEditorMainWindowEventHandler& _eventHandler) : eventHandler(_eventHandler), window(nullptr), propertiesPanel(nullptr)
		{
			mainMenu = Windows::CreateMenu(false);
		}

		~AbeditMainWindow()
		{
		}
	public:
		IParentWindowSupervisor& PropertiesPanel() override
		{
			return propertiesPanel->Supervisor();
		}

		Rococo::Editors::IUIPropertiesEditor& Properties() override
		{
			return propertiesPanel->Properties();
		}

		IParentWindowSupervisor& SlateWindow() override
		{
			return slate->Window();
		}

		void PostConstruct(HINSTANCE hDll, HWND hParentWnd, const EditorSessionConfig& sessionConfig)
		{
			UNUSED(hDll);

			Vec2i span{ 1366, 640 };
			span.x = sessionConfig.defaultWidth > 0 ? sessionConfig.defaultWidth : span.x;
			span.y = sessionConfig.defaultHeight > 0 ? sessionConfig.defaultHeight : span.y;

			Vec2i topLeft{ 32, 32 };
			if (sessionConfig.defaultPosLeft < 0) topLeft.x = CW_USEDEFAULT;
			if (sessionConfig.defaultPosTop < 0) topLeft.y = CW_USEDEFAULT;

			auto& filePopup = mainMenu->AddPopup("&File");
			filePopup.AddString("&New", (int32)MenuItem::New);
			filePopup.AddString("&Load...", (int32)MenuItem::Load);
			filePopup.AddString("&Save", (int32)MenuItem::Save);
			filePopup.AddString("&Save As...", (int32)MenuItem::SaveAs);
			filePopup.AddString("E&xit", (int32)MenuItem::Exit);

			WindowConfig config;
			Rococo::Windows::SetOverlappedWindowConfig(config, topLeft, span, hParentWnd, "Rococo Abstract Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, sessionConfig.slateHasMenu ? *mainMenu : (HMENU) nullptr);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			rhsSplitter = AbeditSplitter::Create(hDll, *window, { 0,0 }, { 0,0 });
			propertiesPanel = AbeditPropertiesWindow::Create(window);
			slate = AbeditSlate::Create(*window);

			Layout();
		}

		int propertiesWidth = 600;
		int splitterSpan = 4;

		void Layout()
		{
			RECT rect;
			GetClientRect(*window, &rect);

			int splitterLeft = rect.right - propertiesWidth - splitterSpan;

			MoveWindow(slate->Window(), 0, 0, splitterLeft, rect.bottom, TRUE);
			MoveWindow(*rhsSplitter, splitterLeft, 0, splitterSpan, rect.bottom, TRUE);
			MoveWindow(propertiesPanel->GetWindow(), rect.right - propertiesWidth, 0, propertiesWidth, rect.bottom, TRUE);

			eventHandler.OnSlateResized();
		}

		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static AbeditMainWindow* Create(HINSTANCE hDll, HWND hParentWnd, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler)
		{
			auto m = new AbeditMainWindow(eventHandler);
			m->PostConstruct(hDll, hParentWnd, config);
			return m;
		}

		void Free()
		{
			delete this;
		}

		void OnMenuSelected(uint16 id)
		{
			auto item = static_cast<MenuItem>(id);
			switch (item)
			{
			case MenuItem::Load:
				eventHandler.OnSelectFileToLoad(*this);
				break;
			case MenuItem::Save:
				eventHandler.OnSelectSave(*this);
			case MenuItem::Exit:
				eventHandler.OnRequestToClose(*this);
				break;
			}
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_COMMAND:
				if (HIWORD(wParam) == 0)
				{
					OnMenuSelected(LOWORD(wParam));
					return 0L;
				}
			case WM_CLOSE:
				break;
			case WM_PAINT:
				SendMessage(*rhsSplitter, WM_PAINT_SPLITTER_PREVIEW, 0, 0);
				break;
			case WM_SET_HORZ_SPLITTER:
				{
					int newSplitterPos =  (int) wParam;
					RECT rect;
					GetClientRect(hWnd, &rect);
					propertiesWidth = rect.right - (newSplitterPos + splitterSpan);

					propertiesWidth = Rococo::max(splitterSpan, propertiesWidth);
					propertiesWidth = Rococo::min((int) rect.right - 20, propertiesWidth);

					Layout();
				}
				break;
			}
			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void OnMenuCommand(HWND hWnd, DWORD id) override
		{
			UNUSED(hWnd);
			UNUSED(id);
		}

		void OnClose(HWND hWnd) override
		{
			UNUSED(hWnd);
			eventHandler.OnRequestToClose(*this);
		}

		void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) override
		{
			UNUSED(span);
			UNUSED(type);

			Layout();

			InvalidateRect(hWnd, NULL, TRUE);
		}

		void Hide() override
		{
			ShowWindow(*window, SW_MINIMIZE);
		}

		bool IsVisible() const override
		{
			return IsWindowVisible(*window);
		}
	};
}

namespace Rococo::Abedit::Internal
{
	IAbeditMainWindowSupervisor* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler)
	{
		AutoFree<ANON::AbeditMainWindow> window = ANON::AbeditMainWindow::Create(dllInstance, hParent, config, eventHandler);
		return window.Release();
	}
}
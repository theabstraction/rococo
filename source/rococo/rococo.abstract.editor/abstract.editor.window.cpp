#include <rococo.os.win32.h>

#include <rococo.abstract.editor.win32.h>
#include "resource.h"
#include <rococo.os.h>
#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Abedit;
using namespace Rococo::Windows;

namespace ANON
{
	static const char* const abEditClassName = "AbEditMainWindow_1_0";

	class AbeditPropertiesWindow : public StandardWindowHandler
	{
	private:
		IParentWindowSupervisor* window;

		AbeditPropertiesWindow() : window(nullptr)
		{
		}

		~AbeditPropertiesWindow()
		{
			Rococo::Free(window);
		}

		void PostConstruct(IWindow* parent)
		{
			GuiRect tabRect = { 0,0, 200, 20 };

			Windows::WindowConfig config;
			DWORD style = WS_VISIBLE | WS_CHILD;
			SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, "Blank", style, 0);
			window = Windows::CreateChildWindow(config, this);
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

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				OnPaint();
				return 0L;
			}

			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void LayoutChildren()
		{
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
	};

	class AbeditMainWindow : public StandardWindowHandler, public IAbeditMainWindowSupervisor
	{
	private:
		IAbstractEditorMainWindowEventHandler& eventHandler;
		IDialogSupervisor* window;
		AbeditPropertiesWindow* propertiesPanel;

		AbeditMainWindow(IAbstractEditorMainWindowEventHandler& _eventHandler) : eventHandler(_eventHandler), window(nullptr), propertiesPanel(nullptr)
		{

		}

		~AbeditMainWindow()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

	public:
		Rococo::Windows::IParentWindowSupervisor& PropertiesPanel()
		{
			return propertiesPanel->Supervisor();
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

			WindowConfig config;
			Rococo::Windows::SetOverlappedWindowConfig(config, topLeft, span, hParentWnd, "Rococo Abstract Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, NULL);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			propertiesPanel = AbeditPropertiesWindow::Create(window);

			Layout();
		}

		void Layout()
		{
			RECT rect;
			GetClientRect(*window, &rect);

			int propertiesWidth = 200;

			MoveWindow(propertiesPanel->GetWindow(), rect.right - propertiesWidth, 0, propertiesWidth, rect.bottom, TRUE);
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

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_CLOSE:

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
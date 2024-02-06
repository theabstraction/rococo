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

	class AbeditMainWindow : public StandardWindowHandler, public IAbeditMainWindow
	{
	private:
		IAbstractEditorMainWindowEventHandler& eventHandler;
		IDialogSupervisor* window;

		AbeditMainWindow(IAbstractEditorMainWindowEventHandler& _eventHandler) : eventHandler(_eventHandler), window(nullptr)
		{

		}

		~AbeditMainWindow()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

	public:
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
			Rococo::Windows::SetOverlappedWindowConfig(config, topLeft, span, hParentWnd, "DX11 64-bit Rococo API Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, NULL);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
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
			eventHandler.OnRequestToClose(*this);
		}

		void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) override
		{
			UNUSED(hWnd);
			UNUSED(span);
			UNUSED(type);
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
	IAbeditMainWindow* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler)
	{
		AutoFree<ANON::AbeditMainWindow> window = ANON::AbeditMainWindow::Create(dllInstance, hParent, config, eventHandler);
		return window.Release();
	}
}
#include "windows.test.h"
#include "resource.h"

#include <rococo.types.h>

#ifdef _WIN32
#pragma comment(lib, "rococo.windows.lib")
#pragma comment(lib, "rococo.util.lib")
#endif

namespace
{
	using namespace Rococo;

	struct TestException : public IException
	{
		char msg[256];
		int32 errorCode;

		virtual cstr Message() const
		{
			return msg;
		}

		virtual int32 ErrorCode() const
		{
			return errorCode;
		}

		Debugging::IStackFrameEnumerator* StackFrames() override { return nullptr; }
	};
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	enum { IDSHOWMODAL = 1001 };

	class TestModalDialog : public StandardWindowHandler
	{
	private:
		ModalDialogHandler modalHandler;
		IDialogSupervisor* dialogWindow;

		TestModalDialog() : dialogWindow(nullptr)
		{
		}

		~TestModalDialog()
		{
			Rococo::Free(dialogWindow);
		}

		virtual void OnClose(HWND hWnd)
		{
			modalHandler.TerminateDialog(IDCANCEL);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			modalHandler.TerminateDialog(IDOK);
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOW, nullptr, "Test Modal Dialog", WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU, 0);
			dialogWindow = Windows::CreateDialogWindow(config, this);
			AddPushButton(*dialogWindow, GuiRect(10, 10, 200, 34), "OK", IDOK, 0);
		}

	public:
		static TestModalDialog* Create()
		{
			auto m = new TestModalDialog();
			m->PostConstruct();
			return m;
		}

		DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */)
		{
			return dialogWindow->BlockModal(modalHandler.ModalControl(), owner, this);
		}

		void Free()
		{
			delete this;
		}
	};

	class MainWindowHandler : public StandardWindowHandler
	{
	private:
		IDialogSupervisor* window;

		MainWindowHandler() : window(nullptr)
		{

		}

		~MainWindowHandler()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, nullptr, "64-bit Rococo API Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);

			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
			AddPushButton(*window, GuiRect(10, 10, 200, 34), "Close", IDCANCEL, 0); // Child window is auto freed when the parent is deleted
			AddPushButton(*window, GuiRect(10, 40, 200, 64), "Moda", IDSHOWMODAL, 0); // Child window is auto freed when the parent is deleted
		}
	public:
		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static MainWindowHandler* Create()
		{
			auto m = new MainWindowHandler();
			m->PostConstruct();
			return m;
		}

		void Free()
		{
			delete this;
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			if (id == IDCANCEL)
			{
				PostQuitMessage(0);
			}
			else if (id == IDSHOWMODAL)
			{
				AutoFree<TestModalDialog> modalDialog(TestModalDialog::Create());
				switch (modalDialog->DoModal(*window))
				{
				case IDOK:
					ShowMessageBox(Windows::NullParent(), "The OK button was pressed", "Test", MB_ICONINFORMATION);
					break;
				case IDCANCEL:
               ShowMessageBox(Windows::NullParent(), "The window was closed", "Test", MB_ICONINFORMATION);
					break;
				}
			}
		}

		virtual void OnClose(HWND hWnd)
		{
			PostQuitMessage(0);
		}
	};
}


int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	try
	{
		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain
		AutoFree<MainWindowHandler> mainWindowHandler(MainWindowHandler::Create());

		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	catch (IException& ex)
	{
		OS::ShowErrorBox(NoParent(), ex, "Test threw an exception");
	}

	return 0;
}
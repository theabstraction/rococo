#include <rococo.mvc.h>
#include <rococo.os.win32.h>
#include <rococo.abstract.editor.win32.h>
#include <rococo.strings.h>
#include <rococo.properties.h>
#include <rococo.editors.h>
#include <rococo.window.h>

using namespace Rococo;
using namespace Rococo::Abedit;
using namespace Rococo::MVC;
using namespace Rococo::Reflection;
using namespace Rococo::Editors;

HINSTANCE GetDllInstance();

namespace ANON
{
	struct AbstractEditor : Rococo::Abedit::IWin32AbstractEditorSupervisor
	{
		IMVC_Host& host;
		HWND hHostWindow;

		AutoFree<IAbeditMainWindowSupervisor> mainWindow;
		AutoFree<IUIPaletteSupervisor> palette;

		AbstractEditor(IMVC_Host& _host, HWND _hHostWindow, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler): host(_host), hHostWindow(_hHostWindow)
		{
			mainWindow = Internal::CreateMainWindow(_hHostWindow, GetDllInstance(), IN config, IN eventHandler);

			// TODO - create a child or mainwindow from the host module that yield a HWND
			// Create three children, palette, properties and slate that have subwindows of HWND
			// Pass subwindow context to palette, properties and slate
			// Define and implement palette properties and slate
			palette = Internal::CreatePalette();
		}

		virtual ~AbstractEditor()
		{

		}

		void BringToFront() override
		{
			BringWindowToTop(GetParent(mainWindow->SlateWindow()));
		}

		cstr Implementation() const override
		{
			return IMPLEMENTATION_TYPE_WIN32_HWND;
		}

		void Free() override
		{
			delete this;
		}

		void HideWindow() override
		{
			mainWindow->Hide();
		}

		bool IsVisible() const override
		{
			return mainWindow->IsVisible();
		}

		IUIPalette& Palette() override
		{
			return *palette;
		}

		IUIPropertiesEditor& Properties() override
		{
			return mainWindow->Properties();
		}

		Windows::IParentWindowSupervisor& Slate() override
		{
			return mainWindow->SlateWindow();
		}

		void SetTitleWithPath(const wchar_t* mainTitle, const wchar_t* filePath) override
		{
			if (hHostWindow)
			{
				enum { WM_SET_TITLE_WITH_PATH = WM_USER + 9000 };
				SendMessageW(hHostWindow, WM_SET_TITLE_WITH_PATH, (WPARAM)mainTitle, (LPARAM)filePath);
			}
			else
			{
				if (filePath != nullptr)
				{
					wchar_t fullTitle[sizeof(U8FilePath) + 128];
					Strings::SafeFormat(fullTitle, L"%ls: %ls", mainTitle, filePath);
					SetWindowTextW(GetParent(mainWindow->SlateWindow()), fullTitle);
				}
				else
				{
					SetWindowTextW(GetParent(mainWindow->SlateWindow()), mainTitle);
				}
			}
		}
	};

	struct AbstractEditor_MVC_View : IMVC_ViewSupervisor, IAbstractEditorFactory
	{
		IMVC_Host& host;
		HWND hHostWindow;

		AbstractEditor_MVC_View(IMVC_Host& _host, HWND _hHostWindow, HINSTANCE _hInstance, cstr _commandLine):
			host(_host), hHostWindow(_hHostWindow)
		{
			UNUSED(_commandLine);
			UNUSED(_hInstance);
		}

		void Free() override
		{
			delete this;
		}

		void Cast(void** ppInterface, cstr interfaceId) override
		{
			if (interfaceId == nullptr || *interfaceId == 0)
			{
				Throw(0, "%s: Cannot Cast. interfaceId was blank", __FUNCTION__);
			}

			cstr onlyKnownInterface = "Rococo::Abedit::IAbstractEditorFactory";

			if (ppInterface == nullptr)
			{
				Throw(0, "%s: Cannot Cast to %s. ppInterface was null. Only known interface is %s", __FUNCTION__, interfaceId, onlyKnownInterface);
			}

			if (Strings::Eq(interfaceId, onlyKnownInterface))
			{
				*ppInterface = static_cast<IAbstractEditorFactory*>(this);
				return;
			}

			Throw(0, "%s: Cannot Cast to %s. Only known interface is %s", __FUNCTION__, interfaceId, onlyKnownInterface);
		}

		IAbstractEditorSupervisor* CreateAbstractEditor(const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler) override
		{
			return new AbstractEditor(host, hHostWindow, config, eventHandler);
		}
	};
}

// Abstract Editor
namespace Rococo::Abedit
{
	IMVC_ViewSupervisor* CreateAbstractEditor(IMVC_Host& host, HWND hHostWindow, HINSTANCE hInstance, cstr commandLine)
	{
		return new ANON::AbstractEditor_MVC_View(host, hHostWindow, hInstance, commandLine);
	}
}
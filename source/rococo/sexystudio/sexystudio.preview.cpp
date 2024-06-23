#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.sexystudio.api.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;

enum class Ids
{
	PATH_BAR = 7000,
	GOTO_BUTTON,
	BACK_BUTTON
};

class MainWindowHandler : public StandardWindowHandler, public IModalControl
{
	struct PathEvents : Windows::IRichEditorEvents
	{
		LRESULT RichEditor_OnCommand(HWND, UINT, WPARAM, LPARAM, IRichEditor&) override
		{
			return 0L;
		}

		void RichEditor_OnRightButtonUp(const Vec2i&, IRichEditor&) override
		{
		}
	};
private:
	IDialogSupervisor* window = nullptr;
	IRichEditor* pathPreview = nullptr;

	HFONT hCodeFont = nullptr;
	LOGFONTA codeLogFont{ 0 };

	bool isRunning = true;

	PathEvents pathControlEvents;

	HString filename;
	int lineNumber = 0;

	Rococo::SexyStudio::IPreviewEventHandler& eventHandler;

	MainWindowHandler(Rococo::SexyStudio::IPreviewEventHandler& _eventHandler): eventHandler(_eventHandler)
	{

	}

	~MainWindowHandler()
	{
		Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
	}

	LRESULT OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode) override
	{
		return StandardWindowHandler::OnControlCommand(hWnd, notificationCode, id, hControlCode);
	}

	void OnMenuCommand(HWND hWnd, DWORD id) override
	{
		UNUSED(hWnd);

		if (id == (ControlId)Ids::GOTO_BUTTON)
		{
			if (filename.length() > 0)
			{
				if (eventHandler.OnJumpToCode(filename, lineNumber))
				{
					HWND hBackButton = GetDlgItem(*window, (int)Ids::BACK_BUTTON);
					if (!IsWindowVisible(hBackButton))
					{
						ShowWindow(hBackButton, SW_SHOW);
					}
				}
			}
		}
		else if (id == (ControlId)Ids::BACK_BUTTON)
		{
			eventHandler.OnBackButtonClicked();
		}
	}

	LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
	{
		return StandardWindowHandler::OnMessage(hWnd, uMsg, wParam, lParam);
	}

	void PostConstruct()
	{
		WindowConfig config;
		SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWNORMAL, nullptr, "SexyStudio Preview Definition...", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);

		window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

		RECT clientRect;
		GetClientRect(*window, &clientRect);

		GuiRect pathRect;
		pathRect.left = clientRect.left + 10;
		pathRect.top = clientRect.top + 10;
		pathRect.right = clientRect.right - 40;
		pathRect.bottom = clientRect.top + 40;

		pathPreview = Rococo::Windows::AddRichEditor(*window, pathRect, "", (ControlId)Ids::PATH_BAR, pathControlEvents, WS_BORDER);
		
		GuiRect moveToRect;
		moveToRect.left = clientRect.right - 30;
		moveToRect.top = clientRect.top + 10;
		moveToRect.right = clientRect.right - 10;
		moveToRect.bottom = clientRect.top + 40;
		Rococo::Windows::AddPushButton(*window, moveToRect, "->", (ControlId)Ids::GOTO_BUTTON, WS_BORDER);

		GuiRect backButtonRect;
		backButtonRect.left = clientRect.right - 52;
		backButtonRect.top = clientRect.top + 10;
		backButtonRect.right = clientRect.right - 32;
		backButtonRect.bottom = clientRect.top + 40;
		auto* backButton = Rococo::Windows::AddPushButton(*window, backButtonRect, "<-", (ControlId)Ids::BACK_BUTTON, WS_BORDER);

		ShowWindow(*backButton, SW_HIDE);

		SetBackgroundColour(RGB(0, 0, 32));
	}
public:
	// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
	static MainWindowHandler* Create(Rococo::SexyStudio::IPreviewEventHandler& eventHandler)
	{
		auto m = new MainWindowHandler(eventHandler);
		m->PostConstruct();
		return m;
	}

	void Free()
	{
		delete this;
	}

	void OnGetMinMaxInfo(HWND, MINMAXINFO& info) override
	{
		info.ptMinTrackSize = { 800,600 };
		info.ptMaxTrackSize = { 800,600 };
	}

	void OnClose(HWND) override
	{
		isRunning = false;
	}

	void DoModal(HWND hOwner)
	{
		window->BlockModal(*this, hOwner, this);
	}

	void OnEnterModal() override
	{

	}

	DWORD OnExitModal() override
	{
		return 0;
	}

	bool IsRunning() override
	{
		return isRunning;
	}

	DWORD OnIdle() override
	{
		return 10;
	}

	void SyncCodeFont(int height)
	{
		if (codeLogFont.lfHeight != height)
		{
			DeleteObject(hCodeFont);
			hCodeFont = nullptr;
		}

		SafeFormat(codeLogFont.lfFaceName, "Consolas");
		codeLogFont.lfHeight = height;

		hCodeFont = CreateFontIndirectA(&codeLogFont);
	}

	void SetPreviewTarget(const char* token, const char* path, int lineNumber)
	{
		this->filename = path;
		this->lineNumber = lineNumber;

		char fullTitle[256];
		SafeFormat(fullTitle, "SexyStudio Preview Definition...[%s]", token);

		SetWindowTextA(*window, fullTitle);

		const int height = 18;
		SyncCodeFont(height);

		HDC dc = GetDC(*window);

		HFONT oldFont = (HFONT) SelectObject(dc, hCodeFont);

		TEXTMETRICA metrics;
		GetTextMetricsA(dc, &metrics);

		int fontHeightPixels = metrics.tmHeight + metrics.tmInternalLeading + metrics.tmExternalLeading;

		RECT clientRect;
		GetClientRect(*window, &clientRect);

		int pathWidth = clientRect.right - 60;


		SIZE extent;

		cstr activePath = path;
		
		for (int i = 0; i < 100; i++)
		{
			char fullPathAndLine[MAX_PATH + 128];
			SafeFormat(fullPathAndLine, "%s%s line %d", activePath != path ? "...." : "", activePath, lineNumber);
			GetTextExtentPointA(dc, fullPathAndLine, (int)strlen(fullPathAndLine), &extent);

			if (extent.cx >= pathWidth - 4)
			{
				activePath = activePath + 4;
				if (activePath > path + strlen(path))
				{
					activePath = "....";
					break;
				}
			}
			else
			{
				break;
			}
		}

		SelectObject(dc, oldFont);

		ReleaseDC(*window, dc);

		SendMessage(pathPreview->EditorHandle(), WM_SETFONT, (WPARAM)hCodeFont, 0);

		GuiRect pathRect;
		pathRect.left = clientRect.left + 10;
		pathRect.top = clientRect.top + 10;
		pathRect.right = clientRect.left + 10 + pathWidth;
		pathRect.bottom = clientRect.top + 40;
		MoveWindow(*pathPreview, pathRect.left, pathRect.top, pathWidth, fontHeightPixels + 4, TRUE);

		ColourScheme scheme;
		scheme.backColour = RGBAb(0, 0, 0, 255);
		pathPreview->SetColourSchemeRecursive(scheme);

		pathPreview->ResetContent();

		if (activePath != path)
		{
			pathPreview->AppendText(RGB(192, 192, 192), RGB(0, 0, 0), "....");
		}
		pathPreview->AppendText(RGB(192, 192, 192), RGB(0,0,0), activePath);
		pathPreview->AppendText(RGB(224, 224, 224), RGB(0, 0, 0), " line ");

		char line[64];
		SafeFormat(line, "%d", lineNumber);

		pathPreview->AppendText(RGB(255, 255, 0), RGB(0, 0, 0), line);
	}
};

void ShowPreviewPopup(Rococo::Windows::IWindow& hParent, const char* token, const char* path, int lineNumber, Rococo::SexyStudio::IPreviewEventHandler& eventHandler)
{
	if (!IsWindow(hParent))
	{
		return;
	}

	HWND hRoot = GetAncestor(hParent, GA_ROOTOWNER);

	ShowWindow(hRoot, SW_SHOW);

	BringWindowToTop(hRoot);

	AutoFree<MainWindowHandler> mainWindowHandler(MainWindowHandler::Create(eventHandler));

	mainWindowHandler->SetPreviewTarget(token, path, lineNumber);

	mainWindowHandler->DoModal(hRoot);
}
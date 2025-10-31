#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <Windows.h>
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

	enum { BackButtonRHSOffsetToLHS = 52 };

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
		backButtonRect.left = clientRect.right - BackButtonRHSOffsetToLHS;
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


	// Configure the preview dialog, with the [searchToken] appended to the title, and the goto box populated with the [path] and [lineNumber]
	void SetPreviewTarget(const char* searchToken, const char* path, int lineNumber)
	{
		this->filename = path;
		this->lineNumber = lineNumber;

		char fullTitle[256] = { 0 };
		SafeFormat(fullTitle, "SexyStudio Preview Definition...[%s]", searchToken);

		SetWindowTextA(*window, fullTitle);

		const int height = 18;
		SyncCodeFont(height);

		HDC dc = GetDC(*window);

		HFONT oldFont = (HFONT) SelectObject(dc, hCodeFont);

		TEXTMETRICA codeFontMetrics;
		GetTextMetricsA(dc, &codeFontMetrics);

		int fontHeightPixels = codeFontMetrics.tmHeight + codeFontMetrics.tmInternalLeading + codeFontMetrics.tmExternalLeading;

		RECT clientRect;
		GetClientRect(*window, &clientRect);

		enum { PathToBackButtonPadding = 8 };
		enum { ButtonPairWidth = BackButtonRHSOffsetToLHS + PathToBackButtonPadding };

		enum { PATH_EDITOR_PADDING = 10 };

		int pathWidth = clientRect.right - ButtonPairWidth - PATH_EDITOR_PADDING;


		SIZE extent;

		cstr visiblePath = path;
		
		// Strip characters from the left hand side of the path until it fits in the designated rich text editor space
		for (;;)
		{
			char fullPathAndLine[MAX_PATH + 128] = { 0 };
			SafeFormat(fullPathAndLine, "%s%s line %d", visiblePath != path ? "...." : "", visiblePath, lineNumber);
			GetTextExtentPointA(dc, fullPathAndLine, (int)strlen(fullPathAndLine), &extent);

			enum { PathRHS_SafeZone = 4};

			if (extent.cx >= pathWidth - PathRHS_SafeZone)
			{
				enum { NCharsToStrip = 4 };
				visiblePath += NCharsToStrip;
				if (visiblePath >= path + strlen(path))
				{
					visiblePath = "...";
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

		int xPos = clientRect.left + PATH_EDITOR_PADDING;
		int yPos = clientRect.top + PATH_EDITOR_PADDING;

		enum { EDITOR_VDELTA_SAFEZONE = 4};
		MoveWindow(*pathPreview, xPos, yPos, pathWidth, fontHeightPixels + EDITOR_VDELTA_SAFEZONE, TRUE);

		ColourScheme scheme;
		scheme.backColour = RGBAb(0, 0, 0, 255);
		pathPreview->SetColourSchemeRecursive(scheme);

		pathPreview->ResetContent();

		if (visiblePath != path)
		{
			pathPreview->AppendText(RGB(192, 192, 192), RGB(0, 0, 0), "...");
		}
		pathPreview->AppendText(RGB(192, 192, 192), RGB(0,0,0), visiblePath);
		pathPreview->AppendText(RGB(224, 224, 224), RGB(0, 0, 0), " line ");

		char line[64];
		SafeFormat(line, "%d", lineNumber);

		pathPreview->AppendText(RGB(255, 255, 0), RGB(0, 0, 0), line);

		HWND hBackButton = GetDlgItem(*window, static_cast<int>(Ids::BACK_BUTTON));

		enum { ButtonWidth = 20 };
		MoveWindow(hBackButton, xPos + pathWidth + PathToBackButtonPadding, yPos, ButtonWidth, fontHeightPixels + EDITOR_VDELTA_SAFEZONE, TRUE);

		HWND hGotoButton = GetDlgItem(*window, static_cast<int>(Ids::GOTO_BUTTON));

		enum { ButtonToButtonHPadding = 4 };

		MoveWindow(hGotoButton, xPos + pathWidth + PathToBackButtonPadding + ButtonToButtonHPadding + ButtonWidth, yPos, ButtonWidth, fontHeightPixels + EDITOR_VDELTA_SAFEZONE, TRUE);
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
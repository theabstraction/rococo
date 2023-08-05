#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>

#include <windows.h>
#include <rococo.window.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.io.h>
#include <rococo.ide.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::Windows::IDE;
using namespace Rococo::Visitors;


using namespace Rococo::Sex;
using namespace Rococo::VM;

namespace Rococo::Windows::IDE
{
	IDebuggerEventHandler* CreateDebuggerEventHandler(IO::IInstallation& installation, IDebuggerEventHandlerData& data);
}

namespace
{
	enum EPaneId
	{
		EPaneId_ID_DISASSEMBLER,
		EPaneId_ID_SOURCE,
		EPaneId_ID_STACK,
		EPaneId_ID_MEMBERS,
		EPaneId_ID_CALLSTACK,
		EPaneId_ID_REGISTER,
		EPaneId_ID_LOG,
		EPaneId_ID_API,
		EPaneId_ID_Count
	};

	const IDEPANE_ID IDEPANE_ID_DISASSEMBLER(EPaneId_ID_DISASSEMBLER);
	const IDEPANE_ID IDEPANE_ID_SOURCE(EPaneId_ID_SOURCE);
	const IDEPANE_ID IDEPANE_ID_STACK(EPaneId_ID_STACK);
	const IDEPANE_ID IDEPANE_ID_CALLSTACK(EPaneId_ID_CALLSTACK);
	const IDEPANE_ID IDEPANE_ID_MEMBERS(EPaneId_ID_MEMBERS);
	const IDEPANE_ID IDEPANE_ID_REGISTER(EPaneId_ID_REGISTER);
	const IDEPANE_ID IDEPANE_ID_API(EPaneId_ID_API);
	const IDEPANE_ID IDEPANE_ID_LOG(EPaneId_ID_LOG);

	const std::vector<IDEPANE_ID> defaultPaneSet =
	{
	   IDEPANE_ID_DISASSEMBLER,
	   IDEPANE_ID_SOURCE,
	   IDEPANE_ID_STACK,
	   IDEPANE_ID_CALLSTACK,
	   IDEPANE_ID_MEMBERS,
	   IDEPANE_ID_REGISTER,
	   IDEPANE_ID_API,
	   IDEPANE_ID_LOG
	};

	enum : uintptr_t
	{
		MENU_SPACER = 0,
		MENU_SYS_EXIT = 1000,
		MENU_SYSFONT,
		MENU_SYSRESET,
		MENU_SYS_LOADLAYOUT,
		MENU_SYS_SAVELAYOUT,
		MENU_SYS_TOGGLE_DARKMODE,
		MENU_EXECUTE_NEXT,
		MENU_EXECUTE_OVER,
		MENU_EXECUTE_OUT,
		MENU_EXECUTE_NEXT_SYMBOL,
		MENU_EXECUTE_CONTINUE
	};

	NOT_INLINE void LV_DrawItem(const ColourScheme& scheme, DRAWITEMSTRUCT& d)
	{
		if (!HasFlag(ODA_DRAWENTIRE, d.itemAction))
		{
			return;
		}

		bool isSelected = false;
		bool isFocused = false;
		
		LVITEMA item = { 0 };
		item.mask = LVIF_STATE;
		item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
		item.iItem = d.itemID;
		item.iSubItem = 0;
		if (ListView_GetItem(d.hwndItem, &item))
		{
			if (HasFlag(LVIS_SELECTED, item.state))
			{
				isSelected = true;
			}
			if (HasFlag(LVIS_FOCUSED, item.state))
			{
				isFocused = true;
			}
		}

		char text[256];

		int x = 4;
		int x1 = 0;

		RGBAb backColour = (d.itemID % 2) == 0 ? scheme.evenRowBackColour : scheme.oddRowBackColour;
		RGBAb textColour = scheme.foreColour;

		if (isSelected)
		{
			backColour = scheme.rowSelectBackColour;
			textColour = scheme.foreSelectColour;
		}

		COLORREF originalColour = SetTextColor(d.hDC, ToCOLORREF(textColour));
	
		if (backColour.alpha > 0)
		{
			HBRUSH hBackBrush = CreateSolidBrush(ToCOLORREF(backColour));
			FillRect(d.hDC, &d.rcItem, hBackBrush);
			DeleteObject((HGDIOBJ)hBackBrush);
		}

		for (int i = 0; i < 120; i++)
		{
			LVCOLUMNA col = { 0 };	
			col.mask = LVCF_WIDTH;

			if (ListView_GetColumn(d.hwndItem, i, &col))
			{
				x1 = x + col.cx;

				item = { 0 };
				item.mask = LVIF_TEXT | LVIF_STATE;
				item.iItem = d.itemID;
				item.iSubItem = i;
				item.cchTextMax = sizeof text;
				item.pszText = text;
				if (ListView_GetItem(d.hwndItem, &item))
				{
					RECT itemRect = d.rcItem;
					itemRect.left = x;
					itemRect.right = x1;

					DrawTextA(d.hDC, text, (int) strlen(text), &itemRect, DT_VCENTER | DT_LEFT);
				}

				x = x1;
			}
			else
			{
				break;
			}
		}

		if (isFocused)
		{
			DrawFocusRect(d.hDC, &d.rcItem);
		}

		SetTextColor(d.hDC, originalColour);
	}

	class TabbedDebuggerWindowHandler :
		public StandardWindowHandler,
		public IDebuggerWindow,
		public Windows::IDE::IPaneDatabase,
		public ITreeControlHandler,
		public IDebuggerEventHandlerData
	{
	private:
		AutoFree<IDebuggerEventHandler> eventHandler; // This should come first, as menuHandler is dependent upon it
		IEventCallback<MenuCommand>& menuHandler;
		IParentWindowSupervisor* dialog;
		IDebugControl* debugControl;
		Windows::IDE::ISpatialManager* spatialManager;
		AutoFree<IWin32Menu> mainMenu;
		OS::IAppControl& appControl;

		bool isVisible;
		IDEPANE_ID migratingId;

		LOGFONTA logFont;
		HFONT hFont;

		int32 requiredDepth; // Function to view according to stack depth 

		UINT WM_DEBUGGER_TABCHANGED;

		struct CallstackEventHandler : public IListViewEvents
		{
			TabbedDebuggerWindowHandler* This;
			void OnDrawItem(DRAWITEMSTRUCT& d) override
			{
				LV_DrawItem(This->scheme, d);
			}

			void OnMeasureItem(MEASUREITEMSTRUCT&) override
			{

			}

			void OnItemChanged(int index) override
			{
				This->OnCallstackChanged(index);
			}
		} callstackEventHandler;

		struct VariableEventHandler : public IListViewEvents
		{
			TabbedDebuggerWindowHandler* This;
			void OnDrawItem(DRAWITEMSTRUCT& d) override
			{
				LV_DrawItem(This->scheme, d);
			}

			void OnMeasureItem(MEASUREITEMSTRUCT&) override
			{

			}

			void OnItemChanged(int) override
			{
				
			}
		} variableEventHandler;

		struct RegisterEventHandler : public IListViewEvents
		{
			TabbedDebuggerWindowHandler* This;
			void OnDrawItem(DRAWITEMSTRUCT& d) override
			{
				LV_DrawItem(This->scheme, d);
			}

			void OnMeasureItem(MEASUREITEMSTRUCT&) override
			{

			}

			void OnItemChanged(int) override
			{

			}
		} registerEventHandler;

		void OnCallstackChanged(int depth)
		{
			if (requiredDepth != depth)
			{
				requiredDepth = depth;
				if (debugControl)
				{
					debugControl->RefreshAtDepth(requiredDepth);
				}
			}
		}

		TabbedDebuggerWindowHandler(OS::IAppControl& _appControl, IO::IInstallation& installation) :
			mainMenu(Windows::CreateMenu(false)),
			dialog(nullptr),
			isVisible(false),
			debugControl(nullptr),
			spatialManager(nullptr),
			disassemblyId(false), hFont(nullptr),
			requiredDepth(1),
			eventHandler(CreateDebuggerEventHandler(installation, *this)),
			menuHandler(eventHandler->GetMenuCallback()),
			appControl(_appControl)
		{
			WM_DEBUGGER_TABCHANGED = RegisterWindowMessageA("WM_DEBUGGER_TABCHANGED");

			auto& sysMenu = mainMenu->AddPopup("&Sys");
			auto& sysDebug = mainMenu->AddPopup("&Debug");

			sysMenu.AddString("&Font...", MENU_SYSFONT);
			sysMenu.AddString("&Reset UI", MENU_SYSRESET);
			sysMenu.AddString("&Load layout", MENU_SYS_LOADLAYOUT);
			sysMenu.AddString("&Save layout", MENU_SYS_SAVELAYOUT);
			sysMenu.AddString("Toggle Darkmode", MENU_SYS_TOGGLE_DARKMODE);
			sysMenu.AddString("E&xit", MENU_SYS_EXIT);

			sysDebug.AddString("Step Next", MENU_EXECUTE_NEXT, "F10");
			sysDebug.AddString("Step Over", MENU_EXECUTE_OVER, "F11");
			sysDebug.AddString("Step Out", MENU_EXECUTE_OUT, "Shift + F11");
			sysDebug.AddString("Step Symbol", MENU_EXECUTE_NEXT_SYMBOL, "Shift + F10");
			sysDebug.AddString("Continue", MENU_EXECUTE_CONTINUE, "F5");

			logFont = LOGFONTA{ 0 };
			StackStringBuilder sb(logFont.lfFaceName, sizeof(logFont.lfFaceName));
			sb << "Courier New";

			logFont.lfHeight = -11;

			hFont = CreateFontIndirectA(&logFont);

			callstackEventHandler.This = this;
			registerEventHandler.This = this;
			variableEventHandler.This = this;
		}

		void OnChooseFont()
		{
			LOGFONTA output;
			if (OpenChooseFontBox(*dialog, output))
			{
				DeleteObject(hFont);
				hFont = CreateFontIndirectA(&output);
				logFont = output;
				spatialManager->SetFontRecursive(hFont);
			}
		}

		void OnItemSelected(int64, ITreeControlSupervisor&) override
		{
		}

		void ResetUI()
		{
			if (MessageBoxA(*dialog, "Do you wish to reset the IDE to tabs?", "Sexy Debugger IDE", MB_YESNO) == IDYES)
			{
				spatialManager->Free();
				IO::DeleteUserFile("debugger.ide.sxy");
				spatialManager = LoadSpatialManager(*dialog, *this, &defaultPaneSet[0], defaultPaneSet.size(), IDE_FILE_VERSION, logFont, "debugger");

				DeleteObject(hFont);
				hFont = CreateFontIndirectA(&logFont);
				spatialManager->SetFontRecursive(hFont);

				LayoutChildren();
			}
		}

		bool darkMode = false;
		ColourScheme scheme;

		void SyncColourScheme()
		{
			if (darkMode)
			{
				ColourScheme dark;
				dark.backColour = RGBAb(16, 16, 16, 255);
				dark.foreColour = RGBAb(240, 240, 240, 255);
				dark.evenRowBackColour = RGBAb(48, 48, 48, 255);
				dark.oddRowBackColour = RGBAb(0, 0, 0, 255);
				dark.rowSelectBackColour = RGBAb(0, 0, 128, 255);
				dark.foreSelectColour = RGBAb(255, 255, 255, 255);
				dark.foreComment = RGBAb(0, 240, 0, 255);
				dark.pressedColour = RGBAb(64, 0, 0, 255);
				dark.edgeColour = RGBAb(128, 128, 128, 255);
				dark.pressedEdgeColour = RGBAb(192, 192, 192, 255);
				spatialManager->SetColourSchemeRecursive(dark);
				this->scheme = dark;
			}
			else
			{
				ColourScheme light;
				spatialManager->SetColourSchemeRecursive(light);
				this->scheme = light;
			}

			InvalidateRect(Window(), NULL, TRUE);
		}

		void OnMenuCommand(HWND hWnd, DWORD id) override
		{
			switch (id)
			{
			case MENU_SYS_EXIT:
				OnClose(hWnd);
				appControl.ShutdownApp();
				break;
			case MENU_SYSFONT:
				OnChooseFont();
				break;
			case MENU_SYSRESET:
				ResetUI();
				break;
			case MENU_SYS_LOADLAYOUT:
				Load();
				break;
			case MENU_SYS_SAVELAYOUT:
				Save();
				break;
			case MENU_SYS_TOGGLE_DARKMODE:
				darkMode = !darkMode;
				SyncColourScheme();
				debugControl->RefreshAtDepth(0);
				break;
			}

			if (debugControl)
			{
				switch (id)
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

		bool ProcessDebugShortcut(size_t vkCode)
		{
			if (debugControl)
			{
				bool isShift = (0x8000 & GetKeyState(VK_SHIFT)) != 0;

				switch (vkCode)
				{
				case VK_F5:
					debugControl->Continue();
					return true;
				case VK_F10:
					if (isShift)
						debugControl->StepNextSymbol();
					else
						debugControl->StepNext();
					return true;
				case VK_F11:
					if (isShift)
						debugControl->StepOut();
					else
						debugControl->StepOver();
					return true;
				default:
					return false;
				}
			}

			return false;
		}

		void OnClose(HWND)
		{
			ShowWindow(false, nullptr);
		}

		~TabbedDebuggerWindowHandler()
		{
			Rococo::Free(dialog); // top level windows created with CreateDialogWindow have to be manually freed
			DeleteObject(hFont);
		}

		enum { IDE_FILE_VERSION = 0x1002 };

		void PostConstruct(IWindow& parent)
		{
			WindowConfig config;
			HWND hParentWnd = parent ? (HWND)parent : nullptr;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, hParentWnd, "Dystopia Script Debugger", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE, 0, *mainMenu);
			dialog = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			spatialManager = LoadSpatialManager(*dialog, *this, &defaultPaneSet[0], defaultPaneSet.size(), IDE_FILE_VERSION, logFont, "debugger");

			DeleteObject(hFont);
			hFont = CreateFontIndirectA(&logFont);
			spatialManager->SetFontRecursive(hFont);

			if (IsDarkmode())
			{
				darkMode = true;
				SyncColourScheme();
			}

			LayoutChildren();
		}

		void LayoutChildren()
		{
			Vec2i workAreaSpan = GetDesktopWorkArea();
			Vec2i totalDesktopSpan = GetDesktopSpan();
			Vec2i desktopSpan{ min(workAreaSpan.x, totalDesktopSpan.x), min(workAreaSpan.y, totalDesktopSpan.y) };

			Vec2i offset = { 0,0 };

			WINDOWPLACEMENT wp = { 0 };
			GetWindowPlacement(Window(), &wp);
			wp.length = sizeof wp;

			bool isMaximized = wp.showCmd == SW_SHOWMAXIMIZED;
			
			if (!isMaximized)
			{
				int posX = GetScrollPos(Window(), SB_HORZ);
				offset.x = (posX > 0) ? -posX : 0;
			}

			if (!isMaximized)
			{
				int posY = GetScrollPos(Window(), SB_VERT);
				offset.y = (posY > 0) ? -posY : 0;
			}

			MoveWindow(*spatialManager, offset.x, offset.y, desktopSpan.x, desktopSpan.y, TRUE);
		}

		void GetName(char name[256], IDEPANE_ID id) override
		{
			static std::unordered_map<IDEPANE_ID, cstr, IDEPANE_ID> idToName(
				{
				   { IDEPANE_ID_DISASSEMBLER, "Disassembly"},
				   { IDEPANE_ID_SOURCE,       "Source"},
				   { IDEPANE_ID_STACK,        "Variables" },
				   { IDEPANE_ID_MEMBERS,      "Members" },
				   { IDEPANE_ID_CALLSTACK,    "Call Stack" },
				   { IDEPANE_ID_REGISTER,     "Registers" },
				   { IDEPANE_ID_API,          "API" },
				   { IDEPANE_ID_LOG,          "Log" }
				});

			StackStringBuilder sb(name, 256);
			sb << idToName[id];
		}

		IDEPANE_ID GetMigratingId() override
		{
			return migratingId;
		}

		void SetMigratingId(IDEPANE_ID id) override
		{
			migratingId = id;
		}

		void NotifyMigration() override
		{
			spatialManager->NotifyMigration(migratingId);
			migratingId = IDEPANE_ID::Invalid();
		}

		IIDENode* ConstructPane(IDEPANE_ID id, IParentWindowSupervisor& parent) override
		{
			PostMessage(nullptr, WM_DEBUGGER_TABCHANGED, id.value, 0);

			switch (id.value)
			{
			case EPaneId_ID_DISASSEMBLER:
			{
				auto report = Windows::IDE::CreateTextWindow(parent);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_SOURCE:
			{
				auto report = Windows::IDE::CreateTextWindow(parent);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_STACK:
			{
				auto report = CreateReportView(parent, variableEventHandler, true);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_MEMBERS:
			{
				auto report = CreateTreeView(parent, this);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_CALLSTACK:
			{
				auto report = CreateReportView(parent, callstackEventHandler, true);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_REGISTER:
			{
				auto report = CreateReportView(parent, registerEventHandler, true);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_API:
			{
				auto apiTree = CreateTreeView(parent, this);
				apiTree->SetFont(hFont);
				if (debugControl)
				{
					debugControl->PopulateAPITree(apiTree->GetTreeSupervisor().Tree());
				}
				return apiTree;
			}
			case EPaneId_ID_LOG:
			{
				auto report = Windows::IDE::CreateTextWindow(parent);
				report->SetFont(hFont);
				for (const auto& segment : logSegments)
				{
					report->AddSegment(true, segment.colour, segment.text.c_str(), segment.text.size(), RGBAb(255, 255, 255));
				}
				return report;
			}
			default:
				return nullptr;
			}
		}

		struct Hilight
		{
			std::string source;
			std::string toolTip;
			Vec2i start;
			Vec2i end;
		} hilight;

		bool isJitDebuggingActive = false;

		void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message, bool isJitComileError) override
		{
			if (isJitDebuggingActive && !isJitComileError)
			{
				// Jit errors take priority
				return;
			}

			isJitDebuggingActive = isJitComileError;

			if (strcmp(message, "!") == 0)
			{
				hilight = { source, message, start, end };
			}
			else
			{
				hilight = { source, message, start, end };
			}
		}

		void ResetJitStatus() override
		{
			isJitDebuggingActive = false;
		}
	public:
		static TabbedDebuggerWindowHandler* Create(IWindow& parent, IO::IInstallation& installation, OS::IAppControl& appControl)
		{
			auto m = new TabbedDebuggerWindowHandler(appControl, installation);
			m->PostConstruct(parent);
			return m;
		}

		IWindow& Controller() override
		{
			return this->Window();
		}

		uint32 GetLineNumber() const override
		{
			return hilight.start.y;
		}

		void Free() override
		{
			delete this;
		}

		struct ColouredTextSegment
		{
			RGBAb colour;
			std::string text;
		};

		std::vector<ColouredTextSegment> logSegments;

		void ClearLog() override
		{
			logSegments.clear();
		}

		int Log(cstr format, ...) override
		{
			char text[4096];
			va_list args;
			va_start(args, format);
			int len = SafeVFormat(text, (sizeof text) - 2, format, args);
			if (len == -1) 
				len = (sizeof text) - 2;
			text[len] = L'\n';
			text[len + 1] = 0;

			logSegments.push_back({ RGB(0,0,0), text });

			auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
			if (logPane)
			{
				static_cast<IIDETextWindow*>(logPane)->AddSegment(true, RGB(0, 0, 0), text, rlen(text) + 1, RGBAb(255, 255, 255));
			}
			else
			{
				Rococo::OS::BeepWarning();
			}

			return len;
		}

		void AddLogSection(RGBAb colour, cstr format, ...) override
		{
			char text[4096];
			va_list args;
			va_start(args, format);
			int len = SafeVFormat(text, 4094, format, args);
			UNUSED(len);

			logSegments.push_back({ colour, text });

			auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
			if (logPane)
			{
				static_cast<IIDETextWindow*>(logPane)->AddSegment(true, colour, text, rlen(text) + 1, RGBAb(255, 255, 255));
			}
		}

		int activeDisassemblyLine;
		Vec2i lastScrollPos;

		size_t disassemblyId;

		void InitDisassembly(size_t id) override
		{
			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID(IDEPANE_ID_DISASSEMBLER)));
			if (!report)
			{
				return;
			}

			if (disassemblyId == id)
			{
				SCROLLINFO info = { 0 };
				info.cbSize = sizeof(info);
				info.fMask = SIF_POS;
				GetScrollInfo(report->Editor().EditorHandle(), SB_HORZ, &info);
				lastScrollPos.x = info.nPos;
			}
			else
			{
				lastScrollPos = Vec2i{ 0,0 };
				disassemblyId = id;
			}

			::ShowWindow(report->Editor().EditorHandle(), SW_HIDE);

			report->Editor().ResetContent();
		}

		void AddDisassembly(DISASSEMBLY_TEXT_TYPE type, cstr text, bool bringToView) override
		{
			RGBAb colour = scheme.foreColour;
			RGBAb backColour = scheme.oddRowBackColour;

			switch (type)
			{
			case DISASSEMBLY_TEXT_TYPE::MAIN:
				break;
			case DISASSEMBLY_TEXT_TYPE::HEADER:
				backColour = scheme.evenRowBackColour;
				break;
			case DISASSEMBLY_TEXT_TYPE::COMMENT:
				colour = scheme.foreComment;
				break;
			case DISASSEMBLY_TEXT_TYPE::HILIGHT:
				colour = scheme.foreSelectColour;
				backColour = scheme.rowSelectBackColour;
				break;
			}

			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID(IDEPANE_ID_DISASSEMBLER)));
			if (report)
			{
				if (text)
				{
					report->AddSegment(false, colour, text, rlen(text), backColour);
				}

				if (bringToView)
				{
					activeDisassemblyLine = report->Editor().LineCount();
				}

				if (text == nullptr)
				{
					RECT rect;
					GetClientRect(report->Editor().EditorHandle(), &rect);

					POINTL p;
					p.x = rect.right - 24;
					p.y = rect.bottom - 24;

					report->Editor().ScrollTo(lastScrollPos.y);

					int characterIndex = (int)SendMessage(report->Editor().EditorHandle(), EM_CHARFROMPOS, 0, (LPARAM)&p);
					int finalLineNumber = (int)SendMessage(report->Editor().EditorHandle(), EM_LINEFROMCHAR, characterIndex, 0);

					if (activeDisassemblyLine <= lastScrollPos.y || activeDisassemblyLine >= finalLineNumber)
					{
						report->Editor().ScrollTo(activeDisassemblyLine - 2);

						SCROLLINFO info = { 0 };
						info.cbSize = sizeof(info);
						info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;

						GetScrollInfo(report->Editor().EditorHandle(), SB_VERT, &info);

						if (info.nPos + (int)info.nPage >= info.nMax)
						{
							SendMessage(report->Editor().EditorHandle(), EM_SCROLL, SB_LINEDOWN, 0);
						}

						lastScrollPos.y = report->Editor().GetFirstVisibleLine();
					}

					::ShowWindow(report->Editor().EditorHandle(), SW_SHOW);
				}
			}
		}

		Windows::IWindow& GetDebuggerWindowControl() override
		{
			return *dialog;
		}

		virtual bool IsVisible() const
		{
			return isVisible;
		}

		void ShowWindow(bool show, IDebugControl* debugControl) override
		{
			if (isVisible && !show)
			{
				::ShowWindow(*dialog, SW_HIDE);
			}
			else if (!isVisible && show)
			{
				::ShowWindow(*dialog, SW_SHOW);
			}
			isVisible = show;

			this->debugControl = debugControl;
		}

		void ClearSourceCode() override
		{
			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID_SOURCE));
			if (report)
			{
				report->Editor().ResetContent();
			}
		}

		void AddSourceCode(cstr name, cstr sourceCode) override
		{
			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID_SOURCE));
			if (report)
			{
				HWND hEditor = report->Editor().EditorHandle();
				SendMessageA(hEditor, WM_SETREDRAW, FALSE, 0);
				report->Editor().ResetContent();
				report->AddSegment(true, RGBAb(0, 0, 0), "Module: ", (size_t) -1LL, RGBAb(192, 192, 192));
				report->AddSegment(true, RGBAb(0, 0, 0), name, rlen(name) + 1, RGBAb(192, 192, 192));
				report->AddSegment(true, RGBAb(0, 0, 0), "\n", 2, RGBAb(192, 192, 192));
				report->AddSegment(true, RGBAb(0, 0, 0), sourceCode, rlen(sourceCode) + 1, RGBAb(255, 255, 255));

				if (hilight.source == name)
				{
					report->Editor().Hilight(hilight.start, hilight.end);
					report->Editor().ScrollTo(hilight.start.y > 4 ? hilight.start.y - 4 : 0);
				}

				report->SetEventCallback(&menuHandler);


				DebuggerCommandObjectFile openFile = { { DebuggerCommandObject::OPEN_SOURCE_FILE}, "" };
				SafeFormat(openFile.filename, sizeof(openFile.filename), "%s", name);
				report->AddContextMenuItem("Open file", (const uint8*)&openFile, sizeof(openFile));

				DebuggerCommandObjectFile openFolder = { { DebuggerCommandObject::OPEN_SOURCE_FOLDER}, "" };
				SafeFormat(openFolder.filename, sizeof(openFolder.filename), "%s", name);
				report->AddContextMenuItem("Open folder", (const uint8*)&openFolder, sizeof(openFolder));

				SendMessageA(hEditor, WM_SETREDRAW, TRUE, 0);
				InvalidateRect(hEditor, nullptr, TRUE);
			}
		}

		void PopulateVariableView(IListPopulator& populator) override
		{
			IIDEReportWindow* report = static_cast<IIDEReportWindow*>(spatialManager->FindPane(IDEPANE_ID_STACK));
			if (report)
			{
				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_HIDE);
				RECT rect;
				GetClientRect(report->GetListViewSupervisor(), &rect);

				cstr columns[] = { "DSF", "Address", "Loc", "Type", "Name", "Value", nullptr };
				int widths[] = { 40, 120, 60, 120, 120, 340, -1 };
				report->GetListViewSupervisor().UIList().SetColumns(columns, widths);
				report->GetListViewSupervisor().UIList().ClearRows();

				ListView_SetExtendedListViewStyle(report->GetListViewSupervisor().ListViewHandle(), LVS_EX_FULLROWSELECT);

				populator.Populate(report->GetListViewSupervisor().UIList());
				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_SHOW);
			}
		}

		void PopulateMemberView(ITreePopulator& populator) override
		{
			IIDETreeWindow* report = static_cast<IIDETreeWindow*>(spatialManager->FindPane(IDEPANE_ID_MEMBERS));
			if (report)
			{
				report->GetTreeSupervisor().Tree().ResetContent();
				populator.Populate(report->GetTreeSupervisor().Tree());
			}
		}


		void PopulateCallStackView(IListPopulator& populator) override
		{
			IIDEReportWindow* report = static_cast<IIDEReportWindow*>(spatialManager->FindPane(IDEPANE_ID_CALLSTACK));
			if (report)
			{
				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_HIDE);
				RECT rect;
				GetClientRect(report->GetListViewSupervisor(), &rect);

				int width = max(rect.right - 120, 256);
				cstr columns[] = { "Function", "Module", nullptr };
				int widths[] = { 200, width, -1 };
				ListView_SetExtendedListViewStyle(report->GetListViewSupervisor().ListViewHandle(), LVS_EX_FULLROWSELECT);
				report->GetListViewSupervisor().UIList().SetColumns(columns, widths);
				report->GetListViewSupervisor().UIList().ClearRows();
				populator.Populate(report->GetListViewSupervisor().UIList());

				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_SHOW);
				ListView_SetItemState(report->GetListViewSupervisor().ListViewHandle(), 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			}
		}

		void PopulateRegisterView(IListPopulator& populator) override
		{
			IIDEReportWindow* report = static_cast<IIDEReportWindow*>(spatialManager->FindPane(IDEPANE_ID_REGISTER));
			if (report)
			{
				RECT rect;
				GetClientRect(report->GetListViewSupervisor(), &rect);

				int width = max(rect.right - 60, 256);
				cstr columns[] = { "VM", "Values", nullptr };
				int widths[] = { 40, width, -1 };
				report->GetListViewSupervisor().UIList().SetColumns(columns, widths);

				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_HIDE);
				populator.Populate(report->GetListViewSupervisor().UIList());
				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_SHOW);
			}
		}

		void OnVScroll(HWND hWnd, WPARAM wParam)
		{
			uint32 dragIndex = HIWORD(wParam);
			switch (LOWORD(wParam))
			{
			case SB_BOTTOM:
			case SB_ENDSCROLL:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				SetScrollPos(hWnd, SB_VERT, dragIndex, TRUE);
				break;
			case SB_TOP:
				break;
			}

			LayoutChildren();
		}

		void OnHScroll(HWND hWnd, WPARAM wParam)
		{
			uint32 dragIndex = HIWORD(wParam);
			switch (LOWORD(wParam))
			{
			case SB_BOTTOM:
			case SB_ENDSCROLL:
			case SB_LINEDOWN:
			case SB_LINEUP:
			case SB_PAGEDOWN:
			case SB_PAGEUP:
				break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:
				SetScrollPos(hWnd, SB_HORZ, dragIndex, TRUE);
				break;
			case SB_TOP:
				break;
			}

			LayoutChildren();
		}

		void OnSize(HWND hMainWnd, const Vec2i& span, RESIZE_TYPE type) override
		{
			Vec2i workAreaSpan = GetDesktopWorkArea();
			Vec2i totalDesktopSpan = GetDesktopSpan();
			Vec2i desktopSpan{ min(workAreaSpan.x, totalDesktopSpan.x), min(workAreaSpan.y, totalDesktopSpan.y) };

			if (type != RESIZE_TYPE_MINIMIZED)
			{
				LONG_PTR oldStyleFlags = GetWindowLongPtrA(hMainWnd, GWL_STYLE);
				LONG_PTR newStyleFlags = oldStyleFlags;
				if (type == RESIZE_TYPE_MAXIMIZED)
				{
					newStyleFlags = oldStyleFlags & ~(WS_VSCROLL | WS_HSCROLL);			
					ShowScrollBar(hMainWnd, SB_BOTH, FALSE);
				}
				else
				{
					newStyleFlags = oldStyleFlags | (WS_VSCROLL | WS_HSCROLL);

					SCROLLINFO hInfo = { 0 };
					hInfo.cbSize = sizeof hInfo;
					hInfo.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_RANGE;
					hInfo.nMax = desktopSpan.x;
					hInfo.nPage = span.x;
					SetScrollInfo(hMainWnd, SB_HORZ, &hInfo, TRUE);

					SCROLLINFO vInfo = { 0 };
					vInfo.cbSize = sizeof vInfo;
					vInfo.fMask = SIF_PAGE | SIF_RANGE | SIF_POS | SIF_RANGE;
					vInfo.nMax = desktopSpan.y;
					vInfo.nPage = span.y;
					SetScrollInfo(hMainWnd, SB_VERT, &vInfo, TRUE);
					ShowScrollBar(hMainWnd, SB_BOTH, TRUE);
				}

				SetWindowLongPtrA(hMainWnd, GWL_STYLE, newStyleFlags);
				LayoutChildren();
			}
		}

		IParentWindowSupervisor& Window()
		{
			return *dialog;
		}

		void Load()
		{
			spatialManager->Free();
			spatialManager = nullptr;
			spatialManager = LoadSpatialManager(*dialog, *this, &defaultPaneSet[0], defaultPaneSet.size(), IDE_FILE_VERSION, logFont, "debugger");
			spatialManager->SetFontRecursive(hFont);
			spatialManager->SetColourSchemeRecursive(scheme);
			LayoutChildren();
		}

		void Save()
		{
			spatialManager->Save(logFont, IDE_FILE_VERSION);
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_VSCROLL:
				OnVScroll(hWnd, wParam);
				return 0L;
			case WM_HSCROLL:
				OnHScroll(hWnd, wParam);
				return 0L;				
			}
			return StandardWindowHandler::OnMessage(hWnd, uMsg, wParam, lParam);
		}

		void Run(IDebuggerPopulator& populator, IDebugControl& debugControl) override
		{
			ShowWindow(true, &debugControl);

			MSG msg;
			while (GetMessage(&msg, 0, 0, 0) && IsVisible())
			{
				if (msg.message == WM_DEBUGGER_TABCHANGED)
				{
					populator.Populate(*this);
					continue;
				}

				if (msg.message == WM_KEYDOWN && ProcessDebugShortcut(msg.wParam))
				{
					continue;
				}

				if (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_F10)
				{
					ProcessDebugShortcut(VK_F10);
					continue;
				}

				DispatchMessage(&msg);
			}

			auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
			if (logPane)
			{
				static_cast<IIDETextWindow*>(logPane)->Editor().ResetContent();
			}
			ClearLog();
			ShowWindow(false, nullptr);
		}
	};

	struct DebuggerEventHandler : public IDebuggerEventHandler, public IEventCallback<MenuCommand>
	{
		IO::IInstallation& installation;
		IDebuggerEventHandlerData& data;

		DebuggerEventHandler(IO::IInstallation& _installation, IDebuggerEventHandlerData& _data):
			installation(_installation), data(_data)
		{
		}

		void OnEvent(MenuCommand& mc) override
		{
			HWND hOwner = data.Controller();

			auto* obj = (DebuggerCommandObject*)mc.buffer;
			switch (obj->type)
			{
			case DebuggerCommandObject::OPEN_SOURCE_FILE:
				{
					auto* fileObj = (DebuggerCommandObjectFile*)obj;
					U8FilePath sysPath;
					installation.ConvertPingPathToSysPath(fileObj->filename, sysPath);
					if (OS::IsFileExistant(sysPath))
					{
						THIS_WINDOW thisOwner(hOwner);
						Rococo::OS::ShellOpenDocument(thisOwner, "Rococo::Sexy::IDE", sysPath, data.GetLineNumber());
					}
					else
					{
						ShellExecuteW(hOwner, L"explore", installation.Content(), nullptr, nullptr, SW_SHOW);
					}
				}
				break;
			case DebuggerCommandObject::OPEN_SOURCE_FOLDER:
				{
					auto* fileObj = (DebuggerCommandObjectFile*)obj;
					WideFilePath sysPath;
					installation.ConvertPingPathToSysPath(fileObj->filename, sysPath);
					if (OS::IsFileExistant(sysPath))
					{
						OS::StripLastSubpath(sysPath.buf);
						ShellExecuteW(hOwner, L"open", sysPath, nullptr, nullptr, SW_SHOW);
					}
					else
					{
						ShellExecuteW(hOwner, L"explore", installation.Content(), nullptr, nullptr, SW_SHOW);
					}
				}
				break;
			}
		}

		void Free() override
		{
			delete this;
		}

		IEventCallback<MenuCommand>& GetMenuCallback() override
		{
			return *this;
		}
	};
}

namespace Rococo::Windows::IDE
{
    IDebuggerWindow* CreateDebuggerWindow(IWindow& parent, OS::IAppControl& appControl, IO::IInstallation& installation)
    {
		return TabbedDebuggerWindowHandler::Create(parent, installation, appControl);
    }

	IDebuggerEventHandler* CreateDebuggerEventHandler(IO::IInstallation& installation, IDebuggerEventHandlerData& data)
	{
		return new DebuggerEventHandler(installation, data);
	}
}

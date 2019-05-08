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

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.io.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::Windows::IDE;
using namespace Rococo::Visitors;


using namespace Rococo::Sex;
using namespace Rococo::VM;

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
		MENU_EXECUTE_NEXT,
		MENU_EXECUTE_OVER,
		MENU_EXECUTE_OUT,
		MENU_EXECUTE_NEXT_SYMBOL,
		MENU_EXECUTE_CONTINUE
	};

	class TabbedDebuggerWindowHandler :
		public StandardWindowHandler,
		public IDebuggerWindow,
		public Windows::IDE::IPaneDatabase,
		public ITreeControlHandler
	{
	private:
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
			void OnDrawItem(DRAWITEMSTRUCT& dis) override
			{

			}

			void OnMeasureItem(MEASUREITEMSTRUCT& mis) override
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
			void OnDrawItem(DRAWITEMSTRUCT& dis) override
			{

			}

			void OnMeasureItem(MEASUREITEMSTRUCT& mis) override
			{

			}

			void OnItemChanged(int index) override
			{
				
			}
		} variableEventHandler;

		struct RegisterEventHandler : public IListViewEvents
		{
			TabbedDebuggerWindowHandler* This;
			void OnDrawItem(DRAWITEMSTRUCT& dis) override
			{

			}

			void OnMeasureItem(MEASUREITEMSTRUCT& mis) override
			{

			}

			void OnItemChanged(int index) override
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

		TabbedDebuggerWindowHandler(IEventCallback<MenuCommand>& _menuHandler, OS::IAppControl& _appControl) :
			mainMenu(Windows::CreateMenu(false)),
			dialog(nullptr),
			isVisible(false),
			debugControl(nullptr),
			spatialManager(nullptr),
			disassemblyId(false), hFont(nullptr),
			requiredDepth(1),
			menuHandler(_menuHandler),
			appControl(_appControl)
		{
			WM_DEBUGGER_TABCHANGED = RegisterWindowMessageA("WM_DEBUGGER_TABCHANGED");

			auto& sysMenu = mainMenu->AddPopup("&Sys");
			auto& sysDebug = mainMenu->AddPopup("&Debug");

			sysMenu.AddString("&Font...", MENU_SYSFONT);
			sysMenu.AddString("&Reset UI", MENU_SYSRESET);
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

		virtual void OnItemSelected(int64 id, ITreeControlSupervisor& tree)
		{
		}

		virtual void ResetUI()
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

				IO::DeleteUserFile("debugger.ide.sxy");
			}
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
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

		virtual void OnClose(HWND hWnd)
		{
			Save();
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

			LayoutChildren();
		}

		void LayoutChildren()
		{
			RECT rect;
			GetClientRect(*dialog, &rect);

			MoveWindow(*spatialManager, 0, 0, rect.right, rect.bottom, TRUE);
		}

		virtual void GetName(char name[256], IDEPANE_ID id)
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

		virtual IDEPANE_ID GetMigratingId()
		{
			return migratingId;
		}

		virtual void SetMigratingId(IDEPANE_ID id)
		{
			migratingId = id;
		}

		virtual void NotifyMigration()
		{
			spatialManager->NotifyMigration(migratingId);
			migratingId = IDEPANE_ID::Invalid();
		}

		virtual IIDENode* ConstructPane(IDEPANE_ID id, IParentWindowSupervisor& parent)
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
				auto report = CreateReportView(parent, variableEventHandler);
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
				auto report = CreateReportView(parent, callstackEventHandler);
				report->SetFont(hFont);
				return report;
			}
			case EPaneId_ID_REGISTER:
			{
				auto report = CreateReportView(parent, registerEventHandler);
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
					report->AddSegment(segment.colour, segment.text.c_str(), segment.text.size(), RGBAb(255, 255, 255));
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
			RGBAb foreground;
			RGBAb background;
		} hilight;

		virtual void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message)
		{
			if (strcmp(message, "!") == 0)
			{
				hilight = { source, message, start, end, RGBAb(255,255,255), RGBAb(0,0,192) };
			}
			else
			{
				hilight = { source, message, start, end, RGBAb(255,0,0), RGBAb(192,192,192) };
			}
		}
	public:
		static TabbedDebuggerWindowHandler* Create(IWindow& parent, IEventCallback<MenuCommand>& menuHandler, OS::IAppControl& appControl)
		{
			auto m = new TabbedDebuggerWindowHandler(menuHandler, appControl);
			m->PostConstruct(parent);
			return m;
		}

		void Free()
		{
			delete this;
		}

		struct ColouredTextSegment
		{
			RGBAb colour;
			std::string text;
		};

		std::vector<ColouredTextSegment> logSegments;

		virtual void ClearLog()
		{
			logSegments.clear();
		}

		virtual int Log(cstr format, ...)
		{
			char text[4096];
			va_list args;
			va_start(args, format);
			int len = SafeVFormat(text, 4094, format, args);

			if (len > 0)
			{
				text[len] = L'\n';
				text[len + 1] = 0;
			}
			else
			{
				Rococo::Throw(GetLastError(), "Bad format in log message");
			}

			logSegments.push_back({ RGB(0,0,0), text });

			auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
			if (logPane)
			{
				static_cast<IIDETextWindow*>(logPane)->AddSegment(RGB(0, 0, 0), text, rlen(text) + 1, RGBAb(255, 255, 255));
			}

			return len;
		}

		virtual void AddLogSection(RGBAb colour, cstr format, ...)
		{
			char text[4096];
			va_list args;
			va_start(args, format);
			int len = SafeVFormat(text, 4094, format, args);

			logSegments.push_back({ colour, text });

			auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
			if (logPane)
			{
				static_cast<IIDETextWindow*>(logPane)->AddSegment(colour, text, rlen(text) + 1, RGBAb(255, 255, 255));
			}
		}

		int activeDisassemblyLine;
		Vec2i lastScrollPos;

		size_t disassemblyId;

		virtual void InitDisassembly(size_t id)
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

		virtual void AddDisassembly(RGBAb colour, cstr text, RGBAb bkColor, bool bringToView)
		{
			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID(IDEPANE_ID_DISASSEMBLER)));
			if (report)
			{
				if (text)
				{
					report->AddSegment(colour, text, rlen(text), bkColor);
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

		virtual Windows::IWindow& GetDebuggerWindowControl()
		{
			return *dialog;
		}

		virtual bool IsVisible() const
		{
			return isVisible;
		}

		virtual void ShowWindow(bool show, IDebugControl* debugControl)
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

		virtual void AddSourceCode(cstr name, cstr sourceCode)
		{
			IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID_SOURCE));
			if (report)
			{
				HWND hEditor = report->Editor().EditorHandle();
				SendMessageA(hEditor, WM_SETREDRAW, FALSE, 0);
				report->Editor().ResetContent();
				report->AddSegment(RGBAb(0, 0, 0), "Module: ", -1, RGBAb(192, 192, 192));
				report->AddSegment(RGBAb(0, 0, 0), name, rlen(name) + 1, RGBAb(192, 192, 192));
				report->AddSegment(RGBAb(0, 0, 0), "\n", 2, RGBAb(192, 192, 192));
				report->AddSegment(RGBAb(0, 0, 0), sourceCode, rlen(sourceCode) + 1, RGBAb(255, 255, 255));

				if (hilight.source == name)
				{
					report->Editor().Hilight(hilight.start, hilight.end, hilight.background, hilight.foreground);
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
				int widths[] = { 40, 120, 60, 120, 120, 240, -1 };
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


		virtual void PopulateCallStackView(IListPopulator& populator)
		{
			IIDEReportWindow* report = static_cast<IIDEReportWindow*>(spatialManager->FindPane(IDEPANE_ID_CALLSTACK));
			if (report)
			{
				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_HIDE);
				RECT rect;
				GetClientRect(report->GetListViewSupervisor(), &rect);

				int width = max(rect.right - 120, 256);
				cstr columns[] = { "Function", "Module", nullptr };
				int widths[] = { 120, width, -1 };
				ListView_SetExtendedListViewStyle(report->GetListViewSupervisor().ListViewHandle(), LVS_EX_FULLROWSELECT);
				report->GetListViewSupervisor().UIList().SetColumns(columns, widths);
				report->GetListViewSupervisor().UIList().ClearRows();
				populator.Populate(report->GetListViewSupervisor().UIList());

				::ShowWindow(report->GetListViewSupervisor().ListViewHandle(), SW_SHOW);
				ListView_SetItemState(report->GetListViewSupervisor().ListViewHandle(), 0, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
			}
		}

		virtual void PopulateRegisterView(IListPopulator& populator)
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

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			if (type != RESIZE_TYPE_MINIMIZED)
			{
				LayoutChildren();
			}
		}

		virtual IParentWindowSupervisor& Window()
		{
			return *dialog;
		}

		virtual void Save()
		{
			spatialManager->Save(logFont, IDE_FILE_VERSION);
		}

		virtual void Run(IDebuggerPopulator& populator, IDebugControl& debugControl)
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
		IInstallation& installation;
		HWND hOwner;

		DebuggerEventHandler(IInstallation& _installation, HWND _hOwner):
			installation(_installation), hOwner(_hOwner)
		{
		}

		void OnEvent(MenuCommand& mc) override
		{
			auto* obj = (DebuggerCommandObject*)mc.buffer;
			switch (obj->type)
			{
			case DebuggerCommandObject::OPEN_SOURCE_FILE:
				{
					auto* fileObj = (DebuggerCommandObjectFile*)obj;
					char sysPath[256];
					installation.ConvertPingPathToSysPath(fileObj->filename, sysPath, 256);
					if (OS::IsFileExistant(sysPath))
					{
						ShellExecuteA(hOwner, "open", sysPath, nullptr, nullptr, SW_SHOW);
					}
					else
					{
						ShellExecuteA(hOwner, "explore", installation.Content(), nullptr, nullptr, SW_SHOW);
					}
				}
				break;
			case DebuggerCommandObject::OPEN_SOURCE_FOLDER:
				{
					auto* fileObj = (DebuggerCommandObjectFile*)obj;
					char sysPath[256];
					installation.ConvertPingPathToSysPath(fileObj->filename, sysPath, 256);
					if (OS::IsFileExistant(sysPath))
					{
						OS::StripLastSubpath(sysPath);
						ShellExecuteA(hOwner, "explore", sysPath, nullptr, nullptr, SW_SHOW);
					}
					else
					{
						ShellExecuteA(hOwner, "explore", installation.Content(), nullptr, nullptr, SW_SHOW);
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

namespace Rococo
{
   namespace Windows
   {
      namespace IDE
      {
         IDebuggerWindow* CreateDebuggerWindow(IWindow& parent, IEventCallback<MenuCommand>& menuHandler, OS::IAppControl& appControl)
         {
            return TabbedDebuggerWindowHandler::Create(parent, menuHandler, appControl);
         }

		 IDebuggerEventHandler* CreateDebuggerEventHandler(IInstallation& installation, IWindow& hOwner)
		 {
			 return new DebuggerEventHandler(installation, hOwner);
		 }
      }
   }
}
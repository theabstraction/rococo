#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>

#include <windows.h>
#include <rococo.window.h>

#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <commctrl.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.io.h>

using namespace Rococo;
using namespace Rococo::IDE;
using namespace Rococo::Windows;
using namespace Rococo::Windows::IDE;
using namespace Rococo::Visitors;

using namespace Sexy;
using namespace Sexy::Sex;
using namespace Sexy::VM;

namespace
{
   enum EPaneId
   {
      EPaneId_ID_DISASSEMBLER,
      EPaneId_ID_SOURCE,
      EPaneId_ID_STACK,
      EPaneId_ID_REGISTER,
      EPaneId_ID_LOG,
      EPaneId_ID_API,
      EPaneId_ID_Count
   };

   const IDEPANE_ID IDEPANE_ID_DISASSEMBLER(EPaneId_ID_DISASSEMBLER);
   const IDEPANE_ID IDEPANE_ID_SOURCE(EPaneId_ID_SOURCE);
   const IDEPANE_ID IDEPANE_ID_STACK(EPaneId_ID_STACK);
   const IDEPANE_ID IDEPANE_ID_REGISTER(EPaneId_ID_REGISTER);
   const IDEPANE_ID IDEPANE_ID_API(EPaneId_ID_API);
   const IDEPANE_ID IDEPANE_ID_LOG(EPaneId_ID_LOG);

   const std::vector<IDEPANE_ID> defaultPaneSet =
   {
      IDEPANE_ID_DISASSEMBLER,
      IDEPANE_ID_SOURCE,
      IDEPANE_ID_STACK,
      IDEPANE_ID_REGISTER,
      IDEPANE_ID_API,
      IDEPANE_ID_LOG
   };

   enum: uintptr_t
   {
      MENU_SPACER = 0,
      MENU_SYS_EXIT = 1000,
      MENU_SYSFONT,
      MENU_EXECUTE_NEXT,
      MENU_EXECUTE_OVER,
      MENU_EXECUTE_OUT,
      MENU_EXECUTE_NEXT_SYMBOL,
      MENU_EXECUTE_CONTINUE
   };

   class TabbedDebuggerWindowHandler : public StandardWindowHandler, public IDebuggerWindow, public Windows::IDE::IPaneDatabase
   {
   private:
      IParentWindowSupervisor* dialog;
      IDebugControl* debugControl;
      Windows::IDE::ISpatialManager* spatialManager;
      AutoFree<IWin32Menu> mainMenu;

      bool isVisible;
      IDEPANE_ID migratingId;

      LOGFONT logFont;
      HFONT hFont;

      UINT WM_DEBUGGER_TABCHANGED;

      TabbedDebuggerWindowHandler() :
         mainMenu(Windows::CreateMenu(false)),
         dialog(nullptr),
         isVisible(false),
         debugControl(nullptr),
         spatialManager(nullptr),
         disassemblyId(false), hFont(nullptr)
      {
         WM_DEBUGGER_TABCHANGED = RegisterWindowMessage(L"WM_DEBUGGER_TABCHANGED");

         auto& sysMenu = mainMenu->AddPopup(L"&Sys");
         auto& sysDebug = mainMenu->AddPopup(L"&Debug");
         
         sysMenu.AddString(L"&Font...", MENU_SYSFONT);
         sysMenu.AddString(L"E&xit", MENU_SYS_EXIT);
         
         sysDebug.AddString(L"Step Next", MENU_EXECUTE_NEXT, L"F10");
         sysDebug.AddString(L"Step Over", MENU_EXECUTE_OVER, L"F11");
         sysDebug.AddString(L"Step Out", MENU_EXECUTE_OUT, L"Shift + F11");
         sysDebug.AddString(L"Step Symbol", MENU_EXECUTE_NEXT_SYMBOL, L"Shift + F10");
         sysDebug.AddString(L"Continue", MENU_EXECUTE_CONTINUE, L"F5");

         logFont = LOGFONT{ 0 };
         SafeCopy(logFont.lfFaceName, L"Courier New", _TRUNCATE);
         logFont.lfHeight = -11;

         hFont = CreateFontIndirect(&logFont);
      }

      void OnChooseFont()
      {
         LOGFONT output;
         if (OpenChooseFontBox(*dialog, output))
         {
            DeleteObject(hFont);
            hFont = CreateFontIndirect(&output);
            logFont = output;
            spatialManager->SetFontRecursive(hFont);
         }
      }

      virtual void OnMenuCommand(HWND hWnd, DWORD id)
      {
         switch (id)
         {
         case MENU_SYS_EXIT:
            OnClose(hWnd);
            break;
         case MENU_SYSFONT:
            OnChooseFont();
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

      void PostConstruct(IWindow* parent)
      {
         WindowConfig config;
         HWND hParentWnd = parent ? (HWND)*parent : nullptr;
         SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, hParentWnd, L"Dystopia Script Debugger", WS_OVERLAPPEDWINDOW | WS_MAXIMIZE, 0, *mainMenu);    
         dialog = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
        
         spatialManager = LoadSpatialManager(*dialog, *this, &defaultPaneSet[0], defaultPaneSet.size(), IDE_FILE_VERSION, logFont);

         DeleteObject(hFont);
         hFont = CreateFontIndirectW(&logFont);
         spatialManager->SetFontRecursive(hFont);

         LayoutChildren();
      }

      void LayoutChildren()
      {
         RECT rect;
         GetClientRect(*dialog, &rect);

         MoveWindow(*spatialManager, 0, 0, rect.right, rect.bottom, TRUE);
      }

      virtual void GetName(wchar_t name[256], IDEPANE_ID id)
      {
         std::unordered_map<IDEPANE_ID, const wchar_t*, IDEPANE_ID> idToName (
         {
            { IDEPANE_ID_DISASSEMBLER, L"Disassembly"},
            { IDEPANE_ID_SOURCE,       L"Source"},
            { IDEPANE_ID_STACK,        L"Stack" },
            { IDEPANE_ID_REGISTER,     L"Registers" },
            { IDEPANE_ID_API,          L"API" },
            { IDEPANE_ID_LOG,          L"Log" }
         });
         SafeCopy(name, 256, idToName[id], _TRUNCATE);
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
               auto report = CreateTreeView(parent);
               report->SetFont(hFont);
               return report;
            }
            case EPaneId_ID_REGISTER:
            {
               auto report = CreateReportView(parent);
               report->SetFont(hFont);
               return report;
            }
            case EPaneId_ID_API:
            {
               auto apiTree = CreateTreeView(parent);
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
                  report->AddSegment(segment.colour, segment.text.c_str(), segment.text.size(), RGBAb(255,255,255));
               }
               return report;
            }
            default:
               return nullptr;
         }  
      }

      struct Hilight
      {
         std::wstring source;
         std::wstring toolTip;
         Vec2i start;
         Vec2i end;
         RGBAb foreground;
         RGBAb background;
      } hilight;

      virtual void SetCodeHilight(const wchar_t* source, const Vec2i& start, const Vec2i& end, const wchar_t* message)
      {
         if (wcscmp(message, L"!") == 0)
         {
            hilight = { source, message, start, end, RGBAb(255,255,255), RGBAb(0,0,192) };
         }
         else
         {
            hilight = { source, message, start, end, RGBAb(255,0,0), RGBAb(192,192,192) };
         }
      }
   public:
      static TabbedDebuggerWindowHandler* Create(IWindow* parent)
      {
         auto m = new TabbedDebuggerWindowHandler();
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
         std::wstring text;
      };

      std::vector<ColouredTextSegment> logSegments;

      virtual void ClearLog()
      {
         logSegments.clear();
      }

      virtual int Log(const wchar_t* format, ...)
      {
         wchar_t text[4096];
         va_list args;
         va_start(args, format);
         int len = SafeVFormat(text, 4094,  _TRUNCATE, format, args);

         if (len > 0)
         {
            text[len] = L'\n';
            text[len + 1] = 0;
         }
         else
         {
            Throw(GetLastError(), L"Bad format in log message");
         }

         logSegments.push_back({ RGB(0,0,0), text  });

         return len;
      }

      virtual void AddLogSection(RGBAb colour, const wchar_t* format, ...)
      {
         wchar_t text[4096];
         va_list args;
         va_start(args, format);
         int len = SafeVFormat(text, 4094, _TRUNCATE, format, args);

         logSegments.push_back({ colour, text });

         auto* logPane = spatialManager->FindPane(IDEPANE_ID_LOG);
         if (logPane)
         {
            static_cast<IIDETextWindow*>(logPane)->AddSegment(colour, text, wcslen(text) + 1, RGBAb(255,255,255));
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

      virtual void AddDisassembly(RGBAb colour, const wchar_t* text, RGBAb bkColor, bool bringToView)
      {
         IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID(IDEPANE_ID_DISASSEMBLER)));
         if (report)
         {
            if (text)
            {
               report->AddSegment(colour, text, wcslen(text), bkColor);
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

               int characterIndex = (int) SendMessage(report->Editor().EditorHandle(), EM_CHARFROMPOS, 0, (LPARAM) &p);
               int finalLineNumber = (int) SendMessage(report->Editor().EditorHandle(), EM_LINEFROMCHAR, characterIndex, 0);

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

      virtual void AddSourceCode(const wchar_t* name, const wchar_t* sourceCode)
      {
         IIDETextWindow* report = static_cast<IIDETextWindow*>(spatialManager->FindPane(IDEPANE_ID_SOURCE));
         if (report)
         {
            report->Editor().ResetContent();
            report->AddSegment(RGBAb(255, 255, 255), L"Module: ", -1, RGBAb(0, 0, 255));
            report->AddSegment(RGBAb(255, 255, 255), name, wcslen(name) + 1, RGBAb(0, 0, 255));
            report->AddSegment(RGBAb(0, 0, 64), L"\n", 2, RGBAb(255, 255, 255));
            report->AddSegment(RGBAb(0, 0, 0), sourceCode, wcslen(sourceCode) + 1, RGBAb(255, 255, 255));

            if (hilight.source == name)
            {
               report->Editor().Hilight(hilight.start, hilight.end, hilight.background, hilight.foreground);
            }
         }
      }

      virtual void PopulateStackView(ITreePopulator& populator)
      {
         IIDETreeWindow* report = static_cast<IIDETreeWindow*>(spatialManager->FindPane(IDEPANE_ID_STACK));
         if (report) 
         {        
            populator.Populate(report->GetTreeSupervisor().Tree());
         }
      }

      virtual void BeginStackUpdate()
      {
         IIDETreeWindow* report = static_cast<IIDETreeWindow*>(spatialManager->FindPane(IDEPANE_ID_STACK));
         if (report)
         {
            ::ShowWindow(report->GetTreeSupervisor(), SW_HIDE);
            report->GetTreeSupervisor().Tree().ResetContent();
         }
      }

      virtual void EndStackUpdate()
      {
         IIDETreeWindow* report = static_cast<IIDETreeWindow*>(spatialManager->FindPane(IDEPANE_ID_STACK));
         if (report) ::ShowWindow(report->GetTreeSupervisor(), SW_SHOW);
      }

      virtual void PopulateRegisterView(IListPopulator& populator)
      {
         IIDEReportWindow* report = static_cast<IIDEReportWindow*>(spatialManager->FindPane(IDEPANE_ID_REGISTER));
         if (report)
         {
            RECT rect;
            GetClientRect(report->GetListViewSupervisor(), &rect);

            int width = max(rect.right - 60, 256);
            const wchar_t* columns[] = { L"VM", L"Values", nullptr };
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
         spatialManager->Save(L"debugger.ide.sxy", logFont, IDE_FILE_VERSION);
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

         ClearLog();
         ShowWindow(false, nullptr);
      }
   };
}

namespace Rococo
{
   namespace IDE
   {
      IDebuggerWindow* CreateDebuggerWindow(IWindow* parent)
      {
         return TabbedDebuggerWindowHandler::Create(parent);
      }
   }
}
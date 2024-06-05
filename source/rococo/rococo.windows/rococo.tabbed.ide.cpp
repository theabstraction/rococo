#include <rococo.os.h>
#include <sexy.types.h>
#include <sexy.s-parser.h> // The S-Parser is used by the tabbed IDE to de-serialize the tree structure from a file
#include <sexy.debug.types.h>
#include <sexy.compiler.public.h>
#include <sexy.script.h>
#include <windows.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <rococo.io.h>
#include <rococo.parse.h>
#include <sexy.strings.h>
#include <rococo.ide.h>
#include <rococo.sexy.api.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::Visitors;
using namespace Rococo::Windows::IDE;


using namespace Rococo::Sex;

namespace
{
   UINT WM_SPLITTER_DRAGGED = 0;
   UINT WM_IDEPANE_MIGRATED = 0;

   template<class T> struct IIterator
   {
      virtual void Begin() = 0;
      virtual bool IsEnd() const = 0;
      virtual const T& Next() = 0;
      virtual size_t Count() const = 0;
   };

   template<class OUTPUT_TYPE, class INPUT_TYPE> struct Converter
   {
      static const OUTPUT_TYPE& Convert(const INPUT_TYPE& value)
      {
          
      }
   };

   template<class T, class Element> class StdEnumerator : public IIterator<Element>
   {
   public:
      const T& t;

      typedef typename T::const_iterator iterator;

      iterator end;
      iterator current;

      StdEnumerator(const T& _t) : t(_t)
      {
         current = t.begin();
         end = t.end();
      }

      virtual void Begin()
      {
         current = t.begin();
      }

      virtual bool IsEnd() const
      {
         return current == end;
      }

      virtual const Element& Next()
      {
         return *current++;
      }

      virtual size_t Count() const
      {
         return t.size();
      }
   };

   struct IIDEWriter
   {
      virtual void WriteText(cstr propName, cstr value) = 0;
      virtual void WriteText(cstr propName, const wchar_t* value) = 0;
      virtual void WriteInt(cstr propName, int32 value) = 0;
      virtual void WriteSetOfIds(cstr propName, IIterator<IDEPANE_ID>& container) = 0;
      virtual void PushChild() = 0;
      virtual void PopChild() = 0;
   };

   class IDEWriterViaSexy : public IIDEWriter
   {
      int depth = 0;

      AutoFree<IDynamicStringBuilder> sb;
   public:
      IDEWriterViaSexy()
      {
          sb = CreateDynamicStringBuilder(4096);
      }

      auto& SB()
      {
          return sb->Builder();
      }

      void Commit(cstr filename)
      {
         IO::SaveUserFile(filename, *sb->Builder());
      }

      void AppendDepth()
      {
         for (int i = 0; i < depth; ++i)
         {
             SB().AppendFormat("  ");
         }
      }

      void WriteText(cstr propName, cstr value) override // TODO->escape sequences
      {
         AppendDepth();
         SB().AppendFormat("(%s string \"%s\")\n", propName, value);
      }

      void WriteText(cstr propName, const wchar_t* value) override // TODO->escape sequences
      {
          AppendDepth();
          SB().AppendFormat("(%s string \"%ls\")\n", propName, value);
      }

      void WriteInt(cstr propName, int32 value) override
      {
         AppendDepth();
         SB().AppendFormat("(%s int32 0x%X)\n", propName, value);
      }

      void WriteSetOfIds(cstr propName, IIterator<IDEPANE_ID>& container) override
      {
         AppendDepth();
         SB().AppendFormat("(%s array IDEPANE_ID ", propName);

         container.Begin();
         while (!container.IsEnd())
         {
            auto id = container.Next();
            SB().AppendFormat(" 0x%X", id.value);
         }

         SB().AppendFormat(")\n");
      }

      void PushChild() override
      {
         AppendDepth();
         SB().AppendFormat("(child \n");
         depth++;
      }

      void PopChild() override
      {
         depth--;
         AppendDepth();
         SB().AppendFormat(")\n");
      }
   };

   class IDESplitterWindow : public StandardWindowHandler
   {
   private:
      IParentWindowSupervisor* window;

      POINT dragStart;

      IDESplitterWindow() : dragStart{ -1,-1 }, window(nullptr)
      {
      }

      ~IDESplitterWindow()
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
		  LineTo(ps.hdc, rect.left, rect.bottom-1);
		  LineTo(ps.hdc, rect.right-1, rect.bottom-1);

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
         case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
            {
               if (dragStart.x >= 0 && (HWND)lParam != hWnd)
               {
                  dragStart = { -1,-1 };
                  ClipCursor(NULL);
               }
            }
            break;
         case WM_LBUTTONDOWN:
         {
            auto hRoot = GetAncestor(hWnd, GA_ROOT);
            if (SetForegroundWindow(hRoot))
            {
               GetCursorPos(&dragStart);

               RECT rect;
               GetClientRect(GetParent(hWnd), &rect);

			   RECT splitterRect;
			   GetClientRect(hWnd, &splitterRect);
			   HCURSOR hCursor;
			   if (splitterRect.bottom > splitterRect.right)
			   {
				   // Vertical
				   hCursor = LoadCursorA(nullptr, IDC_SIZEWE);
			   }
			   else
			   {
				   hCursor = LoadCursorA(nullptr, IDC_SIZENS);
			   }
			   
			   SetCursor(hCursor);

               POINT origin{ 0,0 };
               ClientToScreen(GetParent(hWnd), &origin);

               rect.left += origin.x;
               rect.right += origin.x;
               rect.top += origin.y;
               rect.bottom += origin.y;

               ClipCursor(&rect);

			  

               SetCapture(hWnd);
            }
         }
         break;
         case WM_LBUTTONUP:
            if (dragStart.x >= 0)
            {
               POINT dragEnd;
               GetCursorPos(&dragEnd);

               PostMessage(GetParent(hWnd), WM_SPLITTER_DRAGGED, dragEnd.x - dragStart.x, dragEnd.y - dragStart.y);
            }
            dragStart = { -1,-1 };
            ReleaseCapture();
            ClipCursor(NULL);
            return 0L;
         default:
            break;
         }

         return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
      }

      void LayoutChildren()
      {
      }

      virtual void OnSize(HWND, const Vec2i& span, RESIZE_TYPE)
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
      static IDESplitterWindow* Create(IWindow* parent)
      {
         auto node = new IDESplitterWindow();
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

      virtual IWindow& GetWindow()
      {
         return *window;
      }

      virtual Windows::IWindow& IDEHandle()
      {
         return *window;
      }
   };

   class IDEBlankWindow : public StandardWindowHandler, public IIDENode
   {
   private:
      IParentWindowSupervisor* window;

      IDEBlankWindow()
      {

      }

      ~IDEBlankWindow()
      {
      }

      void SetColourSchemeRecursive(const ColourScheme& scheme) override
      {
          SetBackgroundColour(ToCOLORREF(scheme.backColour));
      }

      void SetFont(HFONT hFont) override
      {
         SendMessage(*window, WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      void PostConstruct(IWindow* parent)
      {
         GuiRect tabRect = { 0,0, 200, 20 };

         Windows::WindowConfig config;
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, "Blank", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      void LayoutChildren()
      {
      }

      virtual void OnSize(HWND, const Vec2i&, RESIZE_TYPE)
      {
         LayoutChildren();
      }

   public:
      static IDEBlankWindow* Create(IWindow* parent)
      {
         auto node = new IDEBlankWindow();
         node->PostConstruct(parent);
         return node;
      }

      void Free()
      {
         delete this;
      }

      virtual IWindow& GetWindow()
      {
         return *window;
      }

      virtual operator HWND () const
      {
         return *window;
      }
   };

   enum ELayout
   {
      ELayout_Tabbed = 0x4001,
      ELayout_Horizontal,
      ELayout_Vertical
   };

   enum EMenuCommand
   {
      EMenuCommand_MigrateFrom = 0x4101,
      EMenuCommand_MigrateTo = 0x4102
   };

   struct TabContextMenuItem
   {
      cstr text;
      int32 menuId;
   };

   uint64 g_nextIDEWindowId = 1;

   class IDESpatialManager : public StandardWindowHandler, private ITabControlEvents, public ISpatialManager
   {
   private:
      AutoFree<IParentWindowSupervisor> window;
      ITabControl* tabView;
      IDESplitterWindow* splitterControl;
      std::vector<IDEPANE_ID> paneIds;
      AutoFree<IWin32Menu> contextMenu;
      IPaneDatabase& database;
      IIDENode* currentTab;

      int32 splitPosition;
      IDESpatialManager* sectionA;
      IDESpatialManager* sectionB;

      int32 moveTabIndex;

      IDEPANE_ID lastMigratedId;

      ELayout layout;

      IDESpatialManager* root;
      IDESpatialManager* parent;

      std::string savename;
      uint64 IDEWindowId = 0;

      IDESpatialManager(IPaneDatabase& _database) :
          layout(ELayout_Tabbed), tabView(nullptr),
          window(nullptr),
          splitterControl(nullptr),
          database(_database),
          currentTab(nullptr),
          moveTabIndex(-1),
          sectionA(nullptr),
          sectionB(nullptr),
          splitPosition(0),
          root(nullptr),
          parent(nullptr)
      {
          IDEWindowId = g_nextIDEWindowId++;
          root = this;

          contextMenu = Windows::CreateMenu(true);

          if (WM_SPLITTER_DRAGGED == 0)
          {
              WM_SPLITTER_DRAGGED = RegisterWindowMessageA("WM_SPLITTER_DRAGGED");
          }

          if (WM_IDEPANE_MIGRATED == 0)
          {
              WM_IDEPANE_MIGRATED = RegisterWindowMessageA("WM_IDEPANE_MIGRATED");
          }
      }

      ~IDESpatialManager()
      {
      }

      ColourScheme scheme;

      void SetColourSchemeRecursive(const ColourScheme& scheme) override
      {
          StandardWindowHandler::SetBackgroundColour(ToCOLORREF(scheme.backColour));
          this->scheme = scheme;

          if (currentTab)
          {
              currentTab->SetColourSchemeRecursive(scheme);
          }

          if (sectionA)
          {
              sectionA->SetColourSchemeRecursive(scheme);
          }

          if (sectionB)
          {
              sectionB->SetColourSchemeRecursive(scheme);
          }

          if (tabView)
          {
              tabView->SetColourSchemeRecursive(scheme);
          }
      }

      void PostConstruct(IWindow& parent)
      {
         GuiRect tabRect = { 0,0, 200, 20 };

         Windows::WindowConfig config;
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, parent, "Host", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      void LayoutTabs()
      {
         RECT wrect;
         GetClientRect(*window, &wrect);
         GuiRect rect{ wrect.left, wrect.top, wrect.right, wrect.bottom };

         if (tabView == nullptr)
         {
			DWORD style = TCS_BUTTONS;
            tabView = Windows::AddTabs(*window, rect, "Tabbed Control", 0x41000000, *this, style, 0);
            tabView->SetColourSchemeRecursive(scheme);
         }
         else
         {
            MoveWindow(*tabView, 0, 0, rect.right, rect.bottom, TRUE);
         }

         tabView->ResetContent();

         for (auto id : paneIds)
         {
            char name[256];
            database.GetName(name, id);
            tabView->AddTab(name, "");
         }

         OnSelectionChanged(0);
      }

      void LayoutHorizontal(const RECT& rect)
      {
         int splitWidth = 4;

         if (splitPosition > rect.right - 6) splitPosition = rect.right - splitWidth;

         int x0 = 0;
         int x1 = splitPosition + 1;
         int x2 = splitPosition + splitWidth + 2;

         int dx0 = splitPosition;
         int dx1 = splitWidth;
         int dx2 = rect.right - splitPosition - splitWidth;

         MoveWindow(sectionA->IDEHandle(), x0, 0, dx0, rect.bottom, TRUE);
         MoveWindow(splitterControl->IDEHandle(), x1, 0, dx1, rect.bottom, TRUE);
         MoveWindow(sectionB->IDEHandle(), x2, 0, dx2, rect.bottom, TRUE);
      }

      void LayoutVertical(const RECT& rect)
      {
         int splitWidth = 4;

         if (splitPosition > rect.bottom - 6) splitPosition = rect.bottom - splitWidth;

         int y0 = 0;
         int y1 = splitPosition + 1;
         int y2 = splitPosition + splitWidth + 2;

         int dy0 = splitPosition;
         int dy1 = splitWidth;
         int dy2 = rect.bottom - splitPosition - splitWidth;

         MoveWindow(sectionA->IDEHandle(), 0, y0, rect.right, dy0, TRUE);
         MoveWindow(splitterControl->IDEHandle(), 0, y1, rect.right, dy1, TRUE);
         MoveWindow(sectionB->IDEHandle(), 0, y2, rect.right, dy2, TRUE);
      }

      void LayoutChildren()
      {
         RECT rect;
         GetClientRect(*window, &rect);

         switch (layout)
         {
         case ELayout_Tabbed:
            LayoutTabs();
            break;
         case ELayout_Horizontal:
            LayoutHorizontal(rect);
            break;
         case ELayout_Vertical:
            LayoutVertical(rect);
            break;
         }
      }

      void Split(int moveTabIndex)
      {
         if (moveTabIndex < 0 || moveTabIndex >= paneIds.size() || paneIds.size() < 2)
         {
            Rococo::Throw(0, "Unexpected tab index in IDE::Split");
         }

         sectionA = IDESpatialManager::Create(*window, this, database, root);
         sectionB = IDESpatialManager::Create(*window, this, database, root);
         splitterControl = IDESplitterWindow::Create(window);

         if (paneIds.size() == 2)
         {
            sectionA->AddPane(paneIds[0]);
            sectionB->AddPane(paneIds[1]);
         }
         else
         {
            if (moveTabIndex == (int32)paneIds.size() - 1)
            {
               moveTabIndex--;
            }

            for (int32 i = 0; i <= moveTabIndex; ++i)
            {
               sectionA->AddPane(paneIds[i]);
            }

            for (int32 i = moveTabIndex+1; i < paneIds.size(); ++i)
            {
               sectionB->AddPane(paneIds[i]);
            }
         }

         splitterControl->SetColourSchemeRecursive(scheme);
         if (sectionA) sectionA->SetColourSchemeRecursive(scheme);
         if (sectionB) sectionB->SetColourSchemeRecursive(scheme);

         RECT rect;
         GetClientRect(*window, &rect);

         splitPosition = layout == ELayout_Horizontal ? rect.right >> 1 : rect.bottom >> 1;

         paneIds.clear();

         Rococo::Free(currentTab);
         currentTab = nullptr;

         Rococo::Free(tabView);
         tabView = nullptr;
      }

      void Migrate(int tabIndex)
      {
         lastMigratedId = paneIds[tabIndex];
         database.SetMigratingId(lastMigratedId);
      }

      void RemoveId(IDEPANE_ID id)
      {
         bool idFound = false;

         // We need to remove the given panel, but preserve the order of the array
         for (size_t i = 0; i < paneIds.size(); ++i)
         {
            if (paneIds[i] == id)
            {
               idFound = true;
               for (size_t j = i; j < paneIds.size() - 1; ++j)
               {
                  paneIds[j] = paneIds[j + 1];
               }
               break;
            }
         }

         if (idFound)
         {
            paneIds.pop_back();
         }
      }

      void OnMenuCommand(HWND, DWORD controlId) override
      {
         switch (controlId)
         {
         case ELayout_Tabbed:
            layout = ELayout_Tabbed;
            break;
         case ELayout_Horizontal:
            layout = ELayout_Horizontal;
            Split(moveTabIndex);
            break;
         case ELayout_Vertical:
            layout = ELayout_Vertical;
            Split(moveTabIndex);
            break;
         case EMenuCommand_MigrateFrom:
            Migrate(moveTabIndex);
            break;
         case EMenuCommand_MigrateTo:
         {
            auto id = database.GetMigratingId();
            if (id != IDEPANE_ID::Invalid())
            {
                size_t len = paneIds.size();
                auto f = std::find(paneIds.begin(), paneIds.end(), id);
                if (f != paneIds.end())
                {
                    lastMigratedId = IDEPANE_ID::Invalid();
                    RemoveId(id);
                }

                if (moveTabIndex < 0 || moveTabIndex >= (int32)paneIds.size())
                {
                    paneIds.push_back(id);
                }
                else
                {
                    auto it = paneIds.begin();
                    std::advance(it, moveTabIndex);
                    paneIds.insert(it, id);
                }

                LayoutTabs();

                if (paneIds.size() != len)
                {
                    IDESpatialManager* affectedParent = nullptr;
                    EMigrationState state = root->OnMigrationUpdateRemovedTab(id, IDEWindowId, &affectedParent);
                    if (state == EMigrationState::TABS_REDUCED_TO_ZERO)
                    {
                        PostMessage(*affectedParent, WM_IDEPANE_MIGRATED, 0, 0);
                    }
                }

                return;
            }
         }
         break;
         }

         LayoutChildren();
      }

      enum class EMigrationState
      {
          NOTHING_CHANGED,
          TABS_REDUCED_BUT_FINITE,
          TABS_REDUCED_TO_ZERO
      };

      EMigrationState OnMigrationUpdateRemovedTab(IDEPANE_ID id, uint64 insertedWindowId, IDESpatialManager** ppAffectedParent)
      {             
          if (insertedWindowId == IDEWindowId)
          {
              return EMigrationState::NOTHING_CHANGED;
          }

          if (!paneIds.empty())
          {
              auto i = std::remove(paneIds.begin(), paneIds.end(), id);
              if (i != paneIds.end())
              {
                  paneIds.erase(i, paneIds.end());
                  LayoutTabs();
                  *ppAffectedParent = parent;
                  return paneIds.empty() ? EMigrationState::TABS_REDUCED_TO_ZERO : EMigrationState::TABS_REDUCED_BUT_FINITE;
              }

              return EMigrationState::NOTHING_CHANGED;
          }

          // The message reached the root, now send it to the children
          if (sectionA)
          {
              auto result = sectionA->OnMigrationUpdateRemovedTab(id, insertedWindowId, ppAffectedParent);
              if (result != EMigrationState::NOTHING_CHANGED)
              {
                  return result;
              }
          }

          if (sectionB)
          {
              auto result = sectionB->OnMigrationUpdateRemovedTab(id, insertedWindowId, ppAffectedParent);
              if (result != EMigrationState::NOTHING_CHANGED)
              {
                  return result;
              }
          }

          return EMigrationState::NOTHING_CHANGED;
      }

      LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
      {
         if (msg == WM_SPLITTER_DRAGGED)
         {
            if (layout == ELayout_Horizontal)
            {
               int dx = (int)wParam;
               splitPosition += dx;
            }
            else if (layout == ELayout_Vertical)
            {
               int dy = (int)lParam;
               splitPosition += dy;
            }

            LayoutChildren();
            return 0L;
         }
         else if (msg == WM_IDEPANE_MIGRATED)
         {
             ConsolidateEmptyWindows();
             LayoutChildren();
             SetColourSchemeRecursive(scheme);
             return 0L;
         }
         else if (msg == WM_INITMENUPOPUP)
         {
            HMENU hMenu = (HMENU)wParam;
            UINT originPos = LOWORD(lParam);
            BOOL isWindowMenu = HIWORD(lParam);
            UNUSED(isWindowMenu);
            UNUSED(originPos);

            if (hMenu == *contextMenu)
            {
               while (GetMenuItemCount(hMenu))
               {
                  DeleteMenu(hMenu, 0, MF_BYPOSITION);
               }

               char name[40];
               MENUITEMINFOA item = { 0 };
               item.cbSize = sizeof(item);
               item.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
               item.dwTypeData = name;
               item.fState = MFS_ENABLED;

               UINT pos = 0;

               StackStringBuilder sb_name(name, sizeof(name));

               if (moveTabIndex >= 0)
               {
                  if (paneIds.size() > 1)
                  {
                     item.wID = ELayout_Horizontal;
                     sb_name << "Split Horiztonally";
                     InsertMenuItemA(hMenu, pos++, MF_BYPOSITION, &item);
                     sb_name.Clear();

                     item.wID = ELayout_Vertical;
                     sb_name << "Split Vertically";
                     InsertMenuItemA(hMenu, pos++, MF_BYPOSITION, &item);
                     sb_name.Clear();
                  }

                  item.wID = EMenuCommand_MigrateFrom;
                  sb_name << "Migrate From";
                  InsertMenuItemA(hMenu, pos++, MF_BYPOSITION, &item);
                  sb_name.Clear();
               }

               if (database.GetMigratingId() == IDEPANE_ID::Invalid())
               {
                  item.fState = MF_DISABLED;
               }
               item.wID = EMenuCommand_MigrateTo;
               sb_name << "Migrate To";
               InsertMenuItemA(hMenu, pos++, MF_BYPOSITION, &item);
            }
         }

         return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
      }

      void OnSize(HWND, const Vec2i&, RESIZE_TYPE) override
      {
         LayoutChildren();
      }

      void OnSelectionChanged(int index) override
      {
         if (tabView != nullptr)
         {
            Rococo::Free(currentTab);
            currentTab = nullptr;

            if (index >= 0 && index < paneIds.size())
            {
               currentTab = database.ConstructPane(paneIds[index], tabView->ClientSpace());
               currentTab->SetColourSchemeRecursive(scheme);
               RECT rect;
               GetClientRect(tabView->ClientSpace(), &rect);
               MoveWindow(*currentTab, 0, 0, rect.right, rect.bottom, TRUE);
            }
         }
      }

      void OnTabRightClicked(int index, const POINT& screenPos) override
      {
         moveTabIndex = index;
         TrackPopupMenu(*contextMenu, TPM_VERNEGANIMATION | TPM_TOPALIGN | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, *window, NULL);
      }
   public:
      static IDESpatialManager* Create(IWindow& parent, IDESpatialManager* parentManager, IPaneDatabase& database, IDESpatialManager* root, cstr savename = nullptr)
      {
         auto node = new IDESpatialManager(database);
         node->PostConstruct(parent);
         node->SetBackgroundColour(RGB(192, 192, 192));
         node->root = (root == nullptr) ? node : root;
         node->parent = parentManager;

         if (savename) { node->savename = savename; }
         return node;
      }

      void SetFontRecursive(HFONT hFont) override
      {
         SendMessage(*window, WM_SETFONT, (WPARAM)hFont, FALSE);

         if (tabView)
         {
            currentTab->SetFont(hFont);
         }
         else
         {
            if (sectionA) sectionA->SetFontRecursive(hFont);
            if (sectionB) sectionB->SetFontRecursive(hFont);
         }
      }

      IIDENode* FindPane(IDEPANE_ID id) override
      {
         if (tabView != nullptr)
         {
            int index = tabView->GetCurrentSelection();
            if (index >= 0 && id == paneIds[index])
            {
               return currentTab;
            }
         }
         else
         {
            auto* pane = sectionA->FindPane(id);
            if (pane) return pane;

            pane = sectionB->FindPane(id);
            if (pane) return pane;
         }

         return nullptr;
      }

      void Free()
      {
         delete this;
      }

      void ConsolidateEmptyWindows()
      {
          if (sectionA != nullptr)
          {
              if (sectionA->layout == ELayout_Tabbed && sectionB->layout == ELayout_Tabbed
                  && (sectionA->paneIds.empty() || sectionB->paneIds.empty()))
              {
                  layout = ELayout_Tabbed;
                  paneIds.swap(sectionA->paneIds.empty() ? sectionB->paneIds : sectionA->paneIds);
                  Rococo::Free(sectionA);
                  Rococo::Free(sectionB);
                  Rococo::Free(splitterControl);
                  sectionA = sectionB = nullptr;
                  splitterControl = nullptr;
                  Rococo::Free(currentTab);
                  currentTab = nullptr;
                  OnSelectionChanged((int)(paneIds.size() - 1));
              }
              else if (sectionA->layout == ELayout_Tabbed && sectionA->paneIds.empty() && sectionB->layout != ELayout_Tabbed)
              {
                  // Eliminate section A and clone section B to this container window
                  Rococo::Free(sectionA);

                  layout = sectionB->layout;

                  auto* newA = IDESpatialManager::Create(*window, this, database, root);
                  newA->CloneFrom(*sectionB->sectionA);

                  auto* newB = IDESpatialManager::Create(*window, this, database, root);
                  newB->CloneFrom(*sectionB->sectionB);

                  Rococo::Free(sectionA);
                  Rococo::Free(sectionB);

                  sectionA = newA;
                  sectionB = newB;
              }
              else if (sectionB->layout == ELayout_Tabbed && sectionB->paneIds.empty() && sectionA->layout != ELayout_Tabbed)
              {
                  // Eliminate section B and clone section A to this container window

                  layout = sectionA->layout;

                  auto* newA = IDESpatialManager::Create(*window, this, database, root);
                  newA->CloneFrom(*sectionA->sectionA);

                  auto* newB = IDESpatialManager::Create(*window, this, database, root);
                  newB->CloneFrom(*sectionA->sectionB);

                  Rococo::Free(sectionA);
                  Rococo::Free(sectionB);

                  sectionA = newA;
                  sectionB = newB;
              }
          }
      }

      void CloneFrom(IDESpatialManager& other)
      {
         this->root = other.root;
         this->layout = other.layout;
         this->paneIds = other.paneIds;
         this->splitPosition = other.splitPosition;

         if (other.tabView == nullptr)
         {
            this->sectionA = IDESpatialManager::Create(*window, this, database, root);
            this->sectionB = IDESpatialManager::Create(*window, this, database, root);
            this->splitterControl = IDESplitterWindow::Create(window);

            this->sectionA->CloneFrom(*other.sectionA);
            this->sectionB->CloneFrom(*other.sectionB);
         }
      }

      int32 GetInt32(cr_sex s, int32 index, cstr helper)
      {
         if (index < 0 || index >= s.NumberOfElements())
         {
            char msg[1024];
            SafeFormat(msg, sizeof(msg), "Expression too short. Expected an int32 in position %d: %s", index, helper);
            ThrowSex(s, msg);
         }

         if (!IsAtomic(s[index]))
         {
            char msg[1024];
            SafeFormat(msg, sizeof(msg), "Expecting atomic argument in position %d: %s", index, helper);
            ThrowSex(s[index], msg);
         }

         VariantValue value;
         if (Rococo::Parse::PARSERESULT_GOOD != Rococo::Parse::TryParse(value, VARTYPE_Int32, s[index].c_str()))
         {
            char msg[1024];
            SafeFormat(msg, sizeof(msg), "Expecting int32 argument in position %d: %s", index, helper);
            ThrowSex(s[index], msg);
         }

         return value.int32Value;
      }

      void Load(cr_sex s)
      {
         for (int i = 0; i < s.NumberOfElements(); ++i)
         {
            cr_sex sdirective = s[i];
            if (IsCompound(sdirective))
            {
               if (sdirective[0] == "splitPosition")
               {
                  auto helper = "(splitPosition int32 <width>)";
                  this->splitPosition = GetInt32(sdirective, 2, helper);
               }
               else if (sdirective[0] == "layout")
               {
                  auto helper = "(layout int32 <layout-enum-value>)";
                  int ilayout = GetInt32(sdirective, 2, helper);

                  switch (ilayout)
                  {
                  case ELayout_Tabbed:
                  case ELayout_Horizontal:
                  case ELayout_Vertical:
                     layout = (ELayout)ilayout;
                     break;
                  default:
                     ThrowSex(sdirective, "Unknown layout");
                  }
               }
               else if (sdirective[0] == "paneIds")
               {
                  auto helper = "(layout array int32 id1 .... id1)";

                  for (int j = 3; j < sdirective.NumberOfElements(); ++j)
                  {
                     int32 id = GetInt32(sdirective, j, helper);
                     paneIds.push_back(IDEPANE_ID(id));
                  }
               }
            }
         }

         if (layout != ELayout_Tabbed)
         {
            int32 childCount = 0;

            for (int i = 0; i < s.NumberOfElements(); ++i)
            {
               cr_sex sdirective = s[i];
               if (IsCompound(sdirective))
               {
                  if (sdirective[0] == "child")
                  {
                     childCount++;
                     IDESpatialManager* child;
                     if (childCount == 1)
                     {
                        child = sectionA = IDESpatialManager::Create(*window, this, database, root);
                        child->Load(sdirective);
                     }
                     else if (childCount == 2)
                     {
                        child = sectionB = IDESpatialManager::Create(*window, this, database, root);
                        child->Load(sdirective);
                     }
                     else
                     {
                        ThrowSex(sdirective, "Expecting two child nodes");
                     }
                  }
               }
            }

            if (childCount != 2)
            {
               ThrowSex(s, "Expecing two child nodes");
            }
            else
            {
               splitterControl = IDESplitterWindow::Create(window);
            }
         }
      }

      virtual void Save(const LOGFONTW& logFont, int32 version, bool darkMode)
      {
         IDEWriterViaSexy writer;

         writer.PushChild();
            writer.WriteInt("Version", version);
            writer.WriteText("FontFamily", logFont.lfFaceName);
            writer.WriteInt("FontHeight", logFont.lfHeight);
            writer.WriteInt("DarkMode", darkMode ? 1 : 0);
         writer.PopChild();

         writer.PushChild();
            Save(writer);
         writer.PopChild();

         writer.Commit(savename.c_str());
      }

      void Save(IIDEWriter& writer)
      {
         StdEnumerator<std::vector<IDEPANE_ID>, IDEPANE_ID> enumerator(paneIds);

         if (paneIds.size() > 0)
         {
            writer.WriteSetOfIds("paneIds", enumerator);
         }

         if (sectionA != nullptr)
         {
            writer.WriteInt("splitPosition", splitPosition);
            writer.WriteInt("layout", (int32)layout);

            writer.PushChild();
            sectionA->Save(writer);
            writer.PopChild();

            writer.PushChild();
            sectionB->Save(writer);
            writer.PopChild();
         }
      }

      virtual operator HWND () const
      {
         return *window;
      }

      virtual IWindow& GetWindow()
      {
         return *window;
      }

      virtual Windows::IParentWindowSupervisor& IDEHandle()
      {
         return *window;
      }

      virtual void AddPane(IDEPANE_ID id)
      {
         paneIds.push_back(id);
      }
   };

   class IDESplitWindow : public StandardWindowHandler, public IIDENode
   {
      IParentWindowSupervisor* window;

      IIDENode* childA;
      IIDENode* childB;
      IIDENode* splitter;

      bool splitByHorizontalLine;
      int32 splitDelta;

      IDESplitWindow() : childA(nullptr), childB(nullptr), splitter(nullptr), splitByHorizontalLine(true), splitDelta(50)
      {

      }

      ~IDESplitWindow()
      {
         Rococo::Free(window);
         Rococo::Free(childA);
         Rococo::Free(childB);
         Rococo::Free(splitter);
      }

      void PostConstruct(IWindow* parent)
      {
         GuiRect tabRect = { 0,0, 200, 20 };

         Windows::WindowConfig config;
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, "Blank", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      void LayoutHorizontally(const RECT& rect)
      {
         if (splitDelta > rect.right - 6) splitDelta = rect.right - 6;

         int x0 = 0;
         int x1 = splitDelta;
         int x2 = splitDelta + 4;

         int dx0 = splitDelta;
         int dx1 = 4;
         int dx2 = rect.right - splitDelta - 4;

         MoveWindow(*childA, x0, 0, dx0, rect.bottom, TRUE);
         MoveWindow(*splitter, x1, 0, dx1, rect.bottom, TRUE);
         MoveWindow(*childB, x2, 0, dx2, rect.bottom, TRUE);
      }

      void LayoutVertically(const RECT& rect)
      {
         if (splitDelta > rect.bottom - 6) splitDelta = rect.bottom - 6;

         int y0 = 0;
         int y1 = splitDelta;
         int y2 = splitDelta + 4;

         int dy0 = splitDelta;
         int dy1 = 4;
         int dy2 = rect.right - splitDelta - 4;

         MoveWindow(*childA,   0, y0, rect.right, dy0, TRUE);
         MoveWindow(*splitter, 0, y1, rect.right, dy1, TRUE);
         MoveWindow(*childB,   0, y2, rect.right, dy2, TRUE);
      }

      void LayoutChildren()
      {
         RECT rect;
         GetClientRect(*window, &rect);

         if (childA == nullptr && childB == nullptr)
         {
            // No children
            return;
         }
         else if (childA != nullptr && childB != nullptr)
         {
            // Both children are defined

            if (splitByHorizontalLine)
            {
               LayoutHorizontally(rect);
            }
            else
            {
               LayoutVertically(rect);
            }
         }
         else
         {
            // Only one child defined, let it take over the client space
            MoveWindow(childA ? *childA : *childB, 0, 0, rect.right, rect.bottom, TRUE);
         }
      }

      virtual void OnSize(HWND, const Vec2i&, RESIZE_TYPE)
      {
         LayoutChildren();
      }

   public:
      static IDESplitWindow* Create(IWindow* parent, bool splitHorz)
      {
         auto node = new IDESplitWindow();
         node->PostConstruct(parent);
         node->splitByHorizontalLine = splitHorz;
         return node;
      }

      operator HWND () const override
      {
         return *window;
      }

      void Free() override
      {
         delete this;
      }

      virtual IWindow& GetWindow()
      {
         return *window;
      }

      ColourScheme scheme;

      void SetColourSchemeRecursive(const ColourScheme& scheme) override
      {
          SetBackgroundColour(ToCOLORREF(scheme.backColour));
          this->scheme = scheme;
      }

      void SetFont(HFONT hFont) override
      {
         SendMessage(*window, WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      void Split(IIDENode* nodeA, IIDENode* nodeB);
   };

   class IDETreeView : public StandardWindowHandler, public IIDETreeWindow, private Visitors::ITreeControlHandler
   {  
   private:
      IParentWindowSupervisor* treeFrame;
      ITreeControlSupervisor* treeClient;
      ITreeControlHandler* handler;

      void OnItemSelected(Visitors::TREE_NODE_ID id, IUITree& origin) override
      {
         if (handler) handler->OnItemSelected(id, origin);
      }

      void OnItemRightClicked(Visitors::TREE_NODE_ID id, IUITree& origin) override
      {
          if (handler) handler->OnItemRightClicked(id, origin);
      }

      IDETreeView(ITreeControlHandler* _handler) :
         treeFrame(nullptr),
         treeClient(nullptr),
         handler(_handler)
      {

      }

      ~IDETreeView()
      {
         Rococo::Free(treeClient);
         Rococo::Free(treeFrame);
      }

      operator HWND () const override
      {
         return *treeFrame;
      }

      void PostConstruct(IWindow& parent)
      {
         WindowConfig config;
         Windows::SetChildWindowConfig(config, GuiRect{ 1, 1, 2, 2 }, parent, nullptr, 0, 0);
         treeFrame = Windows::CreateChildWindow(config, this);
         treeClient = Windows::AddTree(*treeFrame, GuiRect(1, 1, 2, 2), "", 1008, *this, WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_BORDER);
      }

      void OnSize(HWND, const Vec2i& span, RESIZE_TYPE) override
      {
         MoveWindow(*treeFrame, 0, 0, span.x, span.y, TRUE);
         MoveWindow(*treeClient, 0, 0, span.x, span.y, TRUE);
      }

   public:
      static IDETreeView* Create(IWindow& parent, ITreeControlHandler* handler)
      {
         auto node = new IDETreeView(handler);
         node->PostConstruct(parent);
         return node;
      }

      void SetColourSchemeRecursive(const ColourScheme& scheme) override
      {
          SetBackgroundColour(ToCOLORREF(scheme.backColour));
          treeClient->SetColourSchemeRecursive(scheme);
      }

      void SetFont(HFONT hFont) override
      {
         SendMessage(treeClient->TreeHandle(), WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      ITreeControlSupervisor& GetTreeSupervisor() override
      {
         return *treeClient;
      }

      void Free() override
      {
         delete this;
      }

      IWindow& GetWindow()
      {
         return *treeFrame;
      }
   };

   class IDEReportWindow : public StandardWindowHandler, public IIDEReportWindow, private IListViewEvents
   {
   private:
	   IListViewEvents& eventHandler;

	   NOT_INLINE void OnDrawItem(DRAWITEMSTRUCT& ds) override
	   {
           UNUSED(ds);
	   }

	   void OnMeasureItem(HWND hListView, MEASUREITEMSTRUCT& ms) override
	   {
           UNUSED(ms);
           UNUSED(hListView);
	   }

	   void OnItemChanged(int index) override
	   {
		   eventHandler.OnItemChanged(index);
	   }

	   AutoFree<IListViewSupervisor> window;

	   IDEReportWindow(IListViewEvents& _eventHandler): 
		   eventHandler(_eventHandler)
	   {

	   }

	   ~IDEReportWindow()
	   {
	   }

	   void PostConstruct(IWindow& parent, bool ownerDraw)
	   {
		   window = Windows::AddListView(parent, GuiRect(0, 0, 0, 0), "", eventHandler, LVS_REPORT | LVS_NOSORTHEADER | (ownerDraw ? LVS_OWNERDRAWFIXED : 0), WS_BORDER, 0);
	   }

	   void LayoutChildren()
	   {
	   }

	   void OnSize(HWND, const Vec2i&, RESIZE_TYPE) override
	   {
		   LayoutChildren();
	   }

   public:
	   static IDEReportWindow* Create(IWindow& parent, IListViewEvents& eventHandler, bool ownerDraw)
	   {
		   auto node = new IDEReportWindow(eventHandler);
		   node->PostConstruct(parent, ownerDraw);
		   return node;
	   }

       void SetColourSchemeRecursive(const ColourScheme& scheme) override
       {
           SetBackgroundColour(ToCOLORREF(scheme.backColour));
           window->SetColourSchemeRecursive(scheme);
       }

	   void SetFont(HFONT hFont) override
	   {
		   SendMessage(window->ListViewHandle(), WM_SETFONT, (WPARAM)hFont, TRUE);
	   }

	   IListViewSupervisor& GetListViewSupervisor() override
	   {
		   return *window;
	   }

	   void Free() override
	   {
		   delete this;
	   }

	   virtual IWindow& GetWindow()
	   {
		   return *window;
	   }

	   virtual operator HWND () const override
	   {
		   return *window;
	   }
   };

   class CodeSideWindow : public IWindowSupervisor, StandardWindowHandler
   {
       AutoFree<IParentWindowSupervisor> window;
       IWindow& parent;
       IRichEditor* editor = nullptr;
   public:

       CodeSideWindow(IWindow& _parent) : parent(_parent)
       {

       }

       static CodeSideWindow* Create(IWindow& parent)
       {
           CodeSideWindow* sideWindow = new CodeSideWindow(parent);

           GuiRect initRect{ 0, 0, 100, 400 };

           WindowConfig config;
           Windows::SetChildWindowConfig(config, initRect, parent, "side-view", WS_VISIBLE | WS_CHILD, 0);
           sideWindow->window = Windows::CreateChildWindow(config, &sideWindow->Handler());

           return sideWindow;
       }

       void Free() override
       {
           delete this;
       }

       operator HWND() const override
       {
           return *window;
       }

       IWindowHandler& Handler() override
       {
           return *this;
       }

       void OnPaint(HWND hWnd, PAINTSTRUCT&, HDC hdc) override
       {
           RECT clientRect;
           GetClientRect(hWnd, &clientRect);

           HBRUSH hBackBrush = CreateSolidBrush(RGB(32, 0, 0));
           FillRect(hdc, &clientRect, hBackBrush);
           DeleteObject(hBackBrush);

           if (!editor)
           {
               return;
           }


       }

       void Monitor(IRichEditor* editor)
       {
           this->editor = editor;
       }
   };

   class CodeEditorWindow : public IWindowSupervisor, StandardWindowHandler
   {
       AutoFree<CodeSideWindow> codeSideWindow;
       AutoFree<IParentWindowSupervisor> window;
       IWindow& parent;
       IRichEditor* editor = nullptr;
       bool addSideControls = false;

       CodeEditorWindow(IWindow& _parent): parent(_parent)
       {

       }

   public:
       static CodeEditorWindow* Create(IWindow& parent, bool isSourceCode)
       {
           CodeEditorWindow* newWindow = new CodeEditorWindow(parent);
           newWindow->addSideControls = isSourceCode;

           GuiRect initRect{ 0, 0, 100, 400 };

           WindowConfig config;
           Windows::SetChildWindowConfig(config, initRect, parent, "code-lines", WS_VISIBLE | WS_CHILD, 0);
           newWindow->window = Windows::CreateChildWindow(config, &newWindow->Handler());
           newWindow->codeSideWindow = CodeSideWindow::Create(*newWindow->window);

           return newWindow;
       }

       void Monitor(IRichEditor* editor)
       {
           this->editor = editor;
           codeSideWindow->Monitor(editor);
       }

       void Free() override
       {
           delete this;
       }

       operator HWND() const override
       {
           return *window;
       }

       IWindowHandler& Handler() override
       {
           return *this;
       }

       void OnSize(HWND, const Vec2i& span, RESIZE_TYPE)
       {
           if (editor)
           {
               if (span.x > 0)
               {
                   int32 sideControlspan = 10;
                   if (span.x > sideControlspan && addSideControls)
                   {
                       MoveWindow(*codeSideWindow, 0, 0, sideControlspan, span.y, TRUE);
                       MoveWindow(*editor, sideControlspan, 0, span.x - sideControlspan, span.y, TRUE);
                   }
                   else
                   {
                       MoveWindow(*codeSideWindow, 0, 0, 0, 0, TRUE);
                       MoveWindow(*editor, 0, 0, span.x, span.y, TRUE);
                   }
               }
           }
       }
   };

   class IDETextWindow : public StandardWindowHandler, public IRichEditorEvents, public IIDETextWindow
   {
   private:
     // AutoFree<CodeEditorWindow> codeEditor;
      AutoFree<IRichEditor> editor;
	  std::unordered_map<std::string, std::vector<uint8>> mapMenuItemToCommand;

	  IEventCallback<MenuCommand>* eventCallback = nullptr;
	  enum { ID_USER_START = 10001 };

      IDETextWindow()
      {

      }

      ~IDETextWindow()
      {
      }

      void PostConstruct(IWindow& parent, bool /* isSourceCode */)
      {
         // codeEditor = CodeEditorWindow::Create(parent, isSourceCode);
          editor = Windows::AddRichEditor(parent, GuiRect{ 0,0,1,1 }, nullptr, 0x5300, *this, ES_READONLY | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0);
          //codeEditor->Monitor(editor);
      }

	  void SendCommandBody(cstr name, const uint8* data, size_t len)
	  {
		  if (eventCallback)
		  {
			  MenuCommand menuCommand{ name, data, len };
			  eventCallback->OnEvent(menuCommand);
		  }
	  }

	  void OnUserMenuClicked(UINT id)
	  {
		  UINT code = 0;
		  for (auto i : mapMenuItemToCommand)
		  {
			  if (code == id)
			  {
				  SendCommandBody(i.first.c_str(), i.second.data(), i.second.size());
			  }

			  code++;
		  }
	  }

	  LRESULT RichEditor_OnCommand(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, IRichEditor& origin) override
	  {
          UNUSED(origin);
          return DefWindowProcA(hWnd, msg, wParam, lParam);
	  }

      void RichEditor_OnRightButtonUp(const Vec2i& pos, IRichEditor& editor) override
      {
          UNUSED(editor);

		  if (mapMenuItemToCommand.empty()) return;

		  HMENU hPopupMenu = CreatePopupMenu();

		  UINT code = ID_USER_START;
		  for (auto i : mapMenuItemToCommand)
		  {
			  InsertMenuA(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, code++, i.first.c_str());
		  }

		  POINT p{ pos.x, pos.y };
		  ClientToScreen(*this, &p);
          UINT iResult = TrackPopupMenu(hPopupMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_TOPALIGN | TPM_LEFTALIGN, p.x, p.y, 0, static_cast<IWindow&>(*this), NULL);
          if (iResult == 0)
          {
              return;
          }

          code = ID_USER_START;
          for (auto i : mapMenuItemToCommand)
          {
              UINT id = code++;
              if (id == iResult)
              {
                  OnUserMenuClicked(id - ID_USER_START);
              }
          }
      }

	  void AddContextMenuItem(cstr key, const uint8* command, size_t lenOfCommand) override
	  {
		  std::vector<uint8> hardCopy(lenOfCommand);
		  hardCopy.resize(lenOfCommand);
		  memcpy(hardCopy.data(), command, lenOfCommand);
		  mapMenuItemToCommand[key] = hardCopy;
	  }

	  void SetEventCallback(IEventCallback<MenuCommand>* eventCallback) override
	  {
		  this->eventCallback = eventCallback;
	  }
   public:
      static IIDETextWindow* Create(IWindow& parent, bool isSourceCode)
      {
         auto node = new IDETextWindow();
         node->PostConstruct(parent, isSourceCode);
         return node;
      }

      ColourScheme scheme;

      void SetColourSchemeRecursive(const ColourScheme& scheme) override
      {
          this->scheme = scheme;
          SetBackgroundColour(ToCOLORREF(scheme.backColour));
          editor->SetColourSchemeRecursive(scheme);
      }

      void AddSegment(bool useColourScheme, RGBAb colour, cstr segment, size_t length, RGBAb bkColor) override
      {
          if (useColourScheme)
          {
              colour = scheme.foreColour;
              bkColor = scheme.backColour;
          }

          editor->AppendText(ToCOLORREF(colour), ToCOLORREF(bkColor), segment, length);
      }

      void Free() override
      {
         delete this;
      }

      IRichEditor& Editor() override
      {
         return *editor;
      }

      operator HWND () const override
      {
         return *editor;
      }

      void SetFont(HFONT hFont) override
      {
         SendMessage(editor->EditorHandle(), WM_SETFONT, (WPARAM)hFont, TRUE);
      }
   };

   void IDESplitWindow::Split(IIDENode* nodeA, IIDENode* nodeB)
   {
      if (childA != nullptr)
      {
         Rococo::Free(childA);
         Rococo::Free(childB);
         Rococo::Free(splitter);
      }

      childA = nodeA;
      childB = nodeB;

      splitter = IDEBlankWindow::Create(window);
      splitter->SetColourSchemeRecursive(scheme);

      LayoutChildren();
   }

   void LoadHeader(cr_sex sheader, UINT versionId, OUT LOGFONTW& logFont, OUT bool& isDarkMode)
   {
       // Todo - replace this with SEXML to eliminate the error handling

      if (sheader.NumberOfElements() != 5)
      {
         Rococo::Throw(0, "Expecting 5 elements in header");
      }

      cr_sex svid = sheader[1];
      cr_sex sfont = sheader[2];
      cr_sex sheight = sheader[3];
      cr_sex sdarkmode = sheader[4];

      LOGFONTW blank = { 0 };
      logFont = blank;

      VariantValue id;
      Parse::TryParse(id, VARTYPE_Int32, GetAtomicArg(svid, 2).c_str());
      if (GetAtomicArg(svid, 0) != "Version" || id.int32Value != (int32) versionId)
      {
         ThrowSex(svid, "Expecting (Version int32 0x%x)", versionId);
      }

      if (GetAtomicArg(sfont, 0) != "FontFamily") ThrowSex(sfont, "Expecting (FontFamily string ...)");

      if (GetAtomicArg(sheight, 0) != "FontHeight") ThrowSex(sfont, "Expecting (FontHeight int32 ...)");

      if (GetAtomicArg(sdarkmode, 0) != "DarkMode") ThrowSex(sfont, "Expecting (DarkMode int32 ...)");

      SecureFormat(logFont.lfFaceName, LF_FACESIZE, L"%hs", sfont[2].c_str());

      VariantValue height;
      Parse::TryParse(height, VARTYPE_Int32, GetAtomicArg(sheight, 2).c_str());
      logFont.lfHeight = height.int32Value;

      VariantValue vvDarkMode;
      Parse::TryParse(vvDarkMode, VARTYPE_Int32, GetAtomicArg(sdarkmode, 2).c_str());
      isDarkMode = vvDarkMode.int32Value == 1;
   }

   ISpatialManager* _LoadSpatialManager(IWindow& parent, REF LOGFONTW& logFont, OUT bool& isDarkMode, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, cstr file_prefix)
   {
	  AutoFree<IAllocatorSupervisor> allocator(Rococo::Memory::CreateBlockAllocator(16, 0, "tabbed-ide"));
      Auto<ISParser> parser(CreateSexParser_2_0(*allocator, 128));

      char loadname[_MAX_PATH];
      SafeFormat(loadname, sizeof(loadname), "%s.ide.sxy", file_prefix);

      AutoFree<IO::IOSSupervisor> io = IO::GetIOS();
      AutoFree<IO::IInstallationSupervisor> installation;

      WideFilePath fullLoadPath;

      if (*file_prefix == '!')
      {
          installation = IO::CreateInstallation(L"content.indicator.txt", *io);
          installation->ConvertPingPathToSysPath(loadname, fullLoadPath);
      }
      else
      {
          IO::GetUserPath(fullLoadPath.buf, _MAX_PATH, loadname);
      }

      Auto<Rococo::Sex::ISourceCode> src;
      Auto<Rococo::Sex::ISParserTree> tree;

      auto spatialManager = IDESpatialManager::Create(parent, nullptr, database, nullptr, "debugger.ide.sxy");

      try
      {

         src = parser->LoadSource(fullLoadPath, Vec2i{ 1, 1 });
         tree = parser->CreateTree(*src);

         if (src->SourceLength() == 0)
         {
            Rococo::Throw(0, "Missing IDE config file. Reverting to default");
         }

         cr_sex root = tree->Root();

         if (root.NumberOfElements() != 2)
         {
            Rococo::Throw(0, "Expecting header element followed by IDE nodes");
         }

         LoadHeader(root[0], versionId, logFont, OUT isDarkMode);
         spatialManager->Load(root[1]);
      }
      catch (IException&)
      {
         for (size_t i = 0; i < nPanes; i++)
         {
            spatialManager->AddPane(idArray[i]);
         }
      }

      return spatialManager;
   }
}

namespace Rococo
{
    namespace Windows
    {
        namespace IDE
        {
            IIDETextWindow* CreateTextWindow(IWindow& parent, bool isSourceCode)
            {
                return IDETextWindow::Create(parent, isSourceCode);
            }

            IIDETreeWindow* CreateTreeView(IWindow& parent, ITreeControlHandler* handler)
            {
                return IDETreeView::Create(parent, handler);
            }

            IIDEReportWindow* CreateReportView(IWindow& parent, IListViewEvents& eventHandler, bool ownerDraw)
            {
                return IDEReportWindow::Create(parent, eventHandler, ownerDraw);
            }

            ISpatialManager* CreateSpatialManager(IWindow& parent, IPaneDatabase& database)
            {
                return IDESpatialManager::Create(parent, nullptr, database, nullptr);
            }

            ISpatialManager* LoadSpatialManager(IWindow& parent, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, OUT LOGFONTW& logFont, OUT bool& isDarkMode, cstr file_prefix)
            {
                return _LoadSpatialManager(parent, OUT logFont, OUT isDarkMode, database, idArray, nPanes, versionId, file_prefix);
            }
        }
    }
}

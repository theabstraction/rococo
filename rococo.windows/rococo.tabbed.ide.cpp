#include <rococo.api.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h> // The S-Parser is used by the tabbed IDE to de-serialize the tree structure from a file
#include <sexy.debug.types.h>
#include <sexy.compiler.public.h>
#include <sexy.script.h>

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
using namespace Rococo::Windows;
using namespace Rococo::Visitors;
using namespace Rococo::Windows::IDE;

using namespace Sexy;
using namespace Sexy::Sex;

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
         return this_conversion_is_not_implemented();
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
      virtual void WriteText(const wchar_t* propName, const wchar_t* value) = 0;
      virtual void WriteInt(const wchar_t* propName, int32 value) = 0;
      virtual void WriteSetOfIds(const wchar_t* propName, IIterator<IDEPANE_ID>& container) = 0;
      virtual void PushChild() = 0;
      virtual void PopChild() = 0;
   };

   class IDEWriterViaSexy : public IIDEWriter
   {
      AutoFree<Rococo::IStringBuilder> sb;
      int depth;
   public:
      IDEWriterViaSexy() :
         depth(0), sb(CreateSafeStringBuilder(32768ULL))
      {
      }

      void Commit(const wchar_t* filename)
      {
         IO::SaveUserFile(filename, *sb);
      }

      virtual void AppendDepth()
      {
         for (int i = 0; i < depth; ++i)
         {
            sb->AppendFormat(L"  ");
         }
      }

      virtual void WriteText(const wchar_t* propName, const wchar_t* value) // TODO->escape sequences
      {
         AppendDepth();
         sb->AppendFormat(L"(%s string \"%s\")\n", propName, value);
      }

      virtual void WriteInt(const wchar_t* propName, int32 value)
      {
         AppendDepth();
         sb->AppendFormat(L"(%s int32 0x%X)\n", propName, value);
      }

      virtual void WriteSetOfIds(const wchar_t* propName, IIterator<IDEPANE_ID>& container)
      {
         AppendDepth();
         sb->AppendFormat(L"(%s array IDEPANE_ID ", propName);

         container.Begin();
         while (!container.IsEnd())
         {
            auto id = container.Next();
            sb->AppendFormat(L" 0x%X", id.value);
         }

         sb->AppendFormat(L")\n");
      }

      virtual void PushChild()
      {
         AppendDepth();
         sb->AppendFormat(L"(child \n");
         depth++;
      }

      virtual void PopChild()
      {
         depth--;
         AppendDepth();
         sb->AppendFormat(L")\n");
      }
   };

   class IDESplitterWindow : public StandardWindowHandler
   {
   private:
      IParentWindowSupervisor* window;

      POINT dragStart;

      IDESplitterWindow() : dragStart{ -1,-1 }
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
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, L"Blank", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
      {
         switch (msg)
         {
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

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
      {
         LayoutChildren();
      }

      LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam)
      {
         RECT rect;
         GetClientRect(*window, &rect);

         HCURSOR hCursor;
         if (rect.bottom > rect.right)
         {
            // Vertical
            hCursor = LoadCursor(nullptr, IDC_SIZEWE);
         }
         else
         {
            hCursor = LoadCursor(nullptr, IDC_SIZENS);
         }

         SetCursor(hCursor);

         return TRUE;
      }
   public:
      static IDESplitterWindow* Create(IWindow* parent)
      {
         auto node = new IDESplitterWindow();
         node->SetBackgroundColour(RGB(255, 0, 0));
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

      virtual void SetFont(HFONT hFont)
      {
         SendMessage(*window, WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      void PostConstruct(IWindow* parent)
      {
         GuiRect tabRect = { 0,0, 200, 20 };

         Windows::WindowConfig config;
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, L"Blank", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      void LayoutChildren()
      {
      }

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
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
      const wchar_t* text;
      int32 menuId;
   };

   const std::vector<TabContextMenuItem> tileSubMenuItems =
   {
      { L"Tabbed View",        ELayout_Tabbed },
      { L"Split Horiztonally", ELayout_Horizontal },
      { L"Split Vertically  ", ELayout_Vertical },
      { L"Migrate From",       EMenuCommand_MigrateFrom },
      { L"Migrate To",         EMenuCommand_MigrateTo }
   };

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

      bool isRoot;

      std::wstring savename;

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
         isRoot(false)
      {
         contextMenu = Windows::CreateMenu(true);

         if (WM_SPLITTER_DRAGGED == 0)
         {
            WM_SPLITTER_DRAGGED = RegisterWindowMessage(L"WM_SPLITTER_DRAGGED");
         }

         if (WM_IDEPANE_MIGRATED == 0)
         {
            WM_IDEPANE_MIGRATED = RegisterWindowMessage(L"WM_IDEPANE_MIGRATED");
         }
      }

      ~IDESpatialManager()
      {
      }

      void PostConstruct(IWindow& parent)
      {
         GuiRect tabRect = { 0,0, 200, 20 };

         Windows::WindowConfig config;
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, parent, L"Host", WS_VISIBLE | WS_CHILD, 0);
         window = Windows::CreateChildWindow(config, this);
      }

      void LayoutTabbed()
      {
         RECT wrect;
         GetClientRect(*window, &wrect);
         GuiRect rect{ wrect.left, wrect.top, wrect.right, wrect.bottom };

         if (tabView == nullptr)
         {
            GuiRect rect{ wrect.left, wrect.top, wrect.right, wrect.bottom };
            tabView = Windows::AddTabs(*window, rect, L"Tabbed Control", 0x41000000, *this, TCS_BUTTONS, 0);
         }
         else
         {
            MoveWindow(*tabView, 0, 0, rect.right, rect.bottom, TRUE);
         }

         tabView->ResetContent();

         for (auto id : paneIds)
         {
            wchar_t name[256];
            database.GetName(name, id);
            tabView->AddTab(name, L"");
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
            LayoutTabbed();
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
            Rococo::Throw(0, L"Unexpected tab index in IDE::Split");
         }

         sectionA = IDESpatialManager::Create(*window, database);
         sectionB = IDESpatialManager::Create(*window, database);
         splitterControl = IDESplitterWindow::Create(window);

         // The first panel always goes in section A
         sectionA->AddPane(paneIds[0]);

         for (int32 i = 1; i < moveTabIndex; ++i)
         {
            sectionA->AddPane(paneIds[i]);
         }

         for (int32 i = moveTabIndex; i < paneIds.size(); ++i)
         {
            sectionB->AddPane(paneIds[i]);
         }

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

      virtual void OnMenuCommand(HWND hWnd, DWORD id)
      {
         switch (id)
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

               PostMessage(*window, WM_IDEPANE_MIGRATED, 0, 0);
            }
         }
         break;
         }

         LayoutChildren();
      }

      virtual LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
            if (!isRoot)
            {
               PostMessage(GetParent(*window), WM_IDEPANE_MIGRATED, 0, 0);
            }
            else
            {
               database.NotifyMigration();
            }
         }
         else if (msg == WM_INITMENUPOPUP)
         {
            HMENU hMenu = (HMENU)wParam;
            UINT pos = LOWORD(lParam);
            BOOL isWindowMenu = HIWORD(lParam);

            if (hMenu == *contextMenu)
            {
               while (GetMenuItemCount(hMenu))
               {
                  DeleteMenu(hMenu, 0, MF_BYPOSITION);
               }

               wchar_t name[40];
               MENUITEMINFOW item = { 0 };
               item.cbSize = sizeof(item);
               item.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
               item.dwTypeData = name;
               item.fState = MFS_ENABLED;

               UINT pos = 0;

               if (moveTabIndex >= 0)
               {
                  if (paneIds.size() > 1)
                  {
                     item.wID = ELayout_Horizontal;
                     SafeCopy(name, L"Split Horiztonally", _TRUNCATE);
                     InsertMenuItem(hMenu, pos++, MF_BYPOSITION, &item);

                     item.wID = ELayout_Vertical;
                     SafeCopy(name, L"Split Vertically", _TRUNCATE);
                     InsertMenuItem(hMenu, pos++, MF_BYPOSITION, &item);
                  }

                  item.wID = EMenuCommand_MigrateFrom;
                  SafeCopy(name, L"Migrate From", _TRUNCATE);
                  InsertMenuItem(hMenu, pos++, MF_BYPOSITION, &item);
               }

               if (database.GetMigratingId() == IDEPANE_ID::Invalid())
               {
                  item.fState = MF_DISABLED;
               }
               item.wID = EMenuCommand_MigrateTo;
               SafeCopy(name, L"Migrate To", _TRUNCATE);
               InsertMenuItem(hMenu, pos++, MF_BYPOSITION, &item);
            }
         }

         return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
      }

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
      {
         LayoutChildren();
      }

      virtual void OnSelectionChanged(int index)
      {
         if (tabView != nullptr)
         {
            Rococo::Free(currentTab);
            currentTab = nullptr;

            if (index >= 0 && index < paneIds.size())
            {
               currentTab = database.ConstructPane(paneIds[index], tabView->ClientSpace());
               RECT rect;
               GetClientRect(tabView->ClientSpace(), &rect);
               MoveWindow(*currentTab, 0, 0, rect.right, rect.bottom, TRUE);
            }
         }
      }

      virtual void OnTabRightClicked(int index, const POINT& screenPos)
      {
         moveTabIndex = index;
         TrackPopupMenu(*contextMenu, TPM_VERNEGANIMATION | TPM_TOPALIGN | TPM_LEFTALIGN, screenPos.x, screenPos.y, 0, *window, NULL);
      }
   public:
      static IDESpatialManager* Create(IWindow& parent, IPaneDatabase& database, bool isRoot = false, const wchar_t* savename = nullptr)
      {
         auto node = new IDESpatialManager(database);
         node->PostConstruct(parent);
         node->SetBackgroundColour(RGB(255, 0, 0));
         node->isRoot = isRoot;

         if (savename) { node->savename = savename; }
         return node;
      }

      void SetFontRecursive(HFONT hFont)
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

      IIDENode* FindPane(IDEPANE_ID id)
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

      void NotifyMigration(IDEPANE_ID id)
      {
         if (lastMigratedId == id && tabView != nullptr)
         {
            RemoveId(id);
            lastMigratedId = IDEPANE_ID::Invalid();
         }
         else
         {
            if (sectionA != nullptr)
            {
               sectionA->NotifyMigration(id);
               sectionB->NotifyMigration(id);

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
                  LayoutChildren();
                  OnSelectionChanged((int)(paneIds.size() - 1));
               }
               else if (sectionA->layout == ELayout_Tabbed && sectionA->paneIds.empty() && sectionB->layout != ELayout_Tabbed)
               {
                  // Eliminate section A and clone section B to this container window
                  Rococo::Free(sectionA);

                  layout = sectionB->layout;

                  auto* newA = IDESpatialManager::Create(*window, database);
                  newA->CloneFrom(*sectionB->sectionA);

                  auto* newB = IDESpatialManager::Create(*window, database);
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

                  auto* newA = IDESpatialManager::Create(*window, database);
                  newA->CloneFrom(*sectionA->sectionA);

                  auto* newB = IDESpatialManager::Create(*window, database);
                  newB->CloneFrom(*sectionA->sectionB);

                  Rococo::Free(sectionA);
                  Rococo::Free(sectionB);

                  sectionA = newA;
                  sectionB = newB;
               }
            }
         }

         LayoutChildren();
      }

      void CloneFrom(IDESpatialManager& other)
      {
         this->isRoot = false;
         this->layout = other.layout;
         this->paneIds = other.paneIds;
         this->splitPosition = other.splitPosition;

         if (other.tabView == nullptr)
         {
            this->sectionA = IDESpatialManager::Create(*window, database);
            this->sectionB = IDESpatialManager::Create(*window, database);
            this->splitterControl = IDESplitterWindow::Create(window);

            this->sectionA->CloneFrom(*other.sectionA);
            this->sectionA->CloneFrom(*other.sectionB);
         }
      }

      int32 GetInt32(cr_sex s, int32 index, const wchar_t* helper)
      {
         if (index < 0 || index >= s.NumberOfElements())
         {
            wchar_t msg[1024];
            SafeFormat(msg, _TRUNCATE, L"Expression too short. Expected an int32 in position %d: %s", index, helper);
            ThrowSex(s, msg);
         }

         if (!IsAtomic(s[index]))
         {
            wchar_t msg[1024];
            SafeFormat(msg, _TRUNCATE, L"Expecting atomic argument in position %d: %s", index, helper);
            ThrowSex(s[index], msg);
         }

         VariantValue value;
         if (Sexy::Parse::PARSERESULT_GOOD != Sexy::Parse::TryParse(value, VARTYPE_Int32, s[index].String()->Buffer))
         {
            wchar_t msg[1024];
            SafeFormat(msg, _TRUNCATE, L"Expecting int32 argument in position %d: %s", index, helper);
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
               if (sdirective[0] == L"splitPosition")
               {
                  auto helper = L"(splitPosition int32 <width>)";
                  this->splitPosition = GetInt32(sdirective, 2, helper);
               }
               else if (sdirective[0] == L"layout")
               {
                  auto helper = L"(layout int32 <layout-enum-value>)";
                  int ilayout = GetInt32(sdirective, 2, helper);

                  switch (ilayout)
                  {
                  case ELayout_Tabbed:
                  case ELayout_Horizontal:
                  case ELayout_Vertical:
                     layout = (ELayout)ilayout;
                     break;
                  default:
                     ThrowSex(sdirective, L"Unknown layout");
                  }
               }
               else if (sdirective[0] == L"paneIds")
               {
                  auto helper = L"(layout array int32 id1 .... id1)";

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
                  if (sdirective[0] == L"child")
                  {
                     childCount++;
                     IDESpatialManager* child;
                     if (childCount == 1)
                     {
                        child = sectionA = IDESpatialManager::Create(*window, database);
                        child->Load(sdirective);
                     }
                     else if (childCount == 2)
                     {
                        child = sectionB = IDESpatialManager::Create(*window, database);
                        child->Load(sdirective);
                     }
                     else
                     {
                        ThrowSex(sdirective, L"Expecting two child nodes");
                     }
                  }
               }
            }

            if (childCount != 2)
            {
               ThrowSex(s, L"Expecing two child nodes");
            }
            else
            {
               splitterControl = IDESplitterWindow::Create(window);
            }
         }
      }

      virtual void Save(const LOGFONT& logFont, int32 version)
      {
         IDEWriterViaSexy writer;

         writer.PushChild();
            writer.WriteInt(L"Version", version);
            writer.WriteText(L"FontFamily", logFont.lfFaceName);
            writer.WriteInt(L"FontHeight", logFont.lfHeight);
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
            writer.WriteSetOfIds(L"paneIds", enumerator);
         }

         if (sectionA != nullptr)
         {
            writer.WriteInt(L"splitPosition", splitPosition);
            writer.WriteInt(L"layout", (int32)layout);

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
         SetChildWindowConfig(config, GuiRect{ 0, 0, 8, 8 }, *parent, L"Blank", WS_VISIBLE | WS_CHILD, 0);
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

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
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

      virtual operator HWND () const
      {
         return *window;
      }

      void Free()
      {
         delete this;
      }

      virtual IWindow& GetWindow()
      {
         return *window;
      }

      virtual void SetFont(HFONT hFont)
      {
         SendMessage(*window, WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      void Split(IIDENode* nodeA, IIDENode* nodeB);
   };

   class IDETreeView : public StandardWindowHandler, public IIDETreeWindow, private ITreeControlHandler
   {  
   private:
      IParentWindowSupervisor* treeFrame;
      ITreeControlSupervisor* treeClient;
      ITreeControlHandler* handler;

      virtual void OnItemSelected(int64 id, ITreeControlSupervisor& origin)
      {
         if (handler) handler->OnItemSelected(id, origin);
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

      virtual operator HWND () const
      {
         return *treeFrame;
      }

      void PostConstruct(IWindow& parent)
      {
         WindowConfig config;
         Windows::SetChildWindowConfig(config, GuiRect{ 1, 1, 2, 2 }, parent, nullptr, 0, 0);
         treeFrame = Windows::CreateChildWindow(config, this);
         treeClient = Windows::AddTree(*treeFrame, GuiRect(1, 1, 2, 2), L"", 1008, *this, WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | WS_BORDER);
      }

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
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

      virtual void SetFont(HFONT hFont)
      {
         SendMessage(treeClient->TreeHandle(), WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      ITreeControlSupervisor& GetTreeSupervisor()
      {
         return *treeClient;
      }

      void Free()
      {
         delete this;
      }

      virtual IWindow& GetWindow()
      {
         return *treeFrame;
      }
   };

   class IDEReportWindow : public StandardWindowHandler, public IIDEReportWindow
   {
      struct LogListEventHandler : IListViewEvents
      {
         virtual void OnDrawItem(DRAWITEMSTRUCT& dis)
         {

         }

         virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis)
         {

         }
      } listEventHandler;

   private:
      AutoFree<IListViewSupervisor> window;

      IDEReportWindow()
      {

      }

      ~IDEReportWindow()
      {
      }

      void PostConstruct(IWindow& parent)
      {
         window = Windows::AddListView(parent, GuiRect(0, 0, 0, 0), L"", listEventHandler, LVS_REPORT, WS_BORDER, 0);
      }

      void LayoutChildren()
      {
      }

      virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
      {
         LayoutChildren();
      }

   public:
      static IDEReportWindow* Create(IWindow& parent)
      {
         auto node = new IDEReportWindow();
         node->PostConstruct(parent);
         return node;
      }

      virtual void SetFont(HFONT hFont)
      {
         SendMessage(window->ListViewHandle(), WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      IListViewSupervisor& GetListViewSupervisor()
      {
         return *window;
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

   class IDETextWindow : public StandardWindowHandler, public IRichEditorEvents, public IIDETextWindow
   {
   private:
      AutoFree<IRichEditor> editor;

      IDETextWindow()
      {

      }

      ~IDETextWindow()
      {
      }

      void PostConstruct(IWindow& parent)
      {
         editor = Windows::AddRichEditor(parent, GuiRect{ 0,0,1,1 }, nullptr, 0x5300, *this, ES_READONLY | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL, 0);
      }

      virtual void OnRightButtonUp(const Vec2i& clientPosition)
      {

      }

   public:
      static IIDETextWindow* Create(IWindow& parent)
      {
         auto node = new IDETextWindow();
         node->PostConstruct(parent);
         return node;
      }

      void AddSegment(RGBAb colour, const wchar_t* segment, size_t length, RGBAb bkColor)
      {
         editor->AppendText(RGB(colour.red, colour.green, colour.blue), RGB(bkColor.red, bkColor.green, bkColor.blue), segment, length);
      }

      void Free()
      {
         delete this;
      }

      IRichEditor& Editor()
      {
         return *editor;
      }

      virtual operator HWND () const
      {
         return *editor;
      }

      virtual void SetFont(HFONT hFont)
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

      LayoutChildren();
   }

   void LoadHeader(cr_sex sheader, UINT versionId, LOGFONT& logFont)
   {
      if (sheader.NumberOfElements() != 4)
      {
         Rococo::Throw(0, L"Expecting 4 elements in header");
      }

      cr_sex svid = sheader[1];
      cr_sex sfont = sheader[2];
      cr_sex sheight = sheader[3];

      VariantValue id;
      Parse::TryParse(id, VARTYPE_Int32, GetAtomicArg(svid, 2).String()->Buffer);
      if (GetAtomicArg(svid, 0) != L"Version" || id.int32Value != versionId)
      {
         ThrowSex(svid, L"Expecting (Version int32 0x%x)", versionId);
      }

      if (GetAtomicArg(sfont, 0) != L"FontFamily") ThrowSex(sfont, L"Expecting (FontFamily string ...)");

      if (GetAtomicArg(sheight, 0) != L"FontHeight") ThrowSex(sfont, L"Expecting (FontHeight int32 ...)");

      SafeCopy(logFont.lfFaceName, sfont[2].String()->Buffer, _TRUNCATE);

      VariantValue height;
      Parse::TryParse(height, VARTYPE_Int32, GetAtomicArg(sheight, 2).String()->Buffer);
      logFont.lfHeight = height.int32Value;
   }

   ISpatialManager* _LoadSpatialManager(IWindow& parent, LOGFONT& logFont, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, const wchar_t* appName)
   {
      CSParserProxy parser;

      wchar_t savename[_MAX_PATH];
      SafeFormat(savename, _TRUNCATE, L"%s.ide.sxy", appName);
      wchar_t fullpath[_MAX_PATH];
      IO::GetUserPath(fullpath, _MAX_PATH, savename);

      Auto<Sexy::Sex::ISourceCode> src;
      Auto<Sexy::Sex::ISParserTree> tree;

      auto spatialManager = IDESpatialManager::Create(parent, database, true, savename);

      try
      {
         src = parser->LoadSource(fullpath, Vec2i{ 1, 1 });
         tree = parser->CreateTree(*src);

         if (src->SourceLength() == 0)
         {
            Rococo::Throw(0, L"Missing IDE config file. Reverting to default");
         }

         cr_sex root = tree->Root();

         if (root.NumberOfElements() != 2)
         {
            Rococo::Throw(0, L"Expecting header element followed by IDE nodes");
         }

         LoadHeader(root[0], versionId, logFont);
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
         IIDETextWindow* CreateTextWindow(IWindow& parent)
         {
            return IDETextWindow::Create(parent);
         }

         IIDETreeWindow* CreateTreeView(IWindow& parent, ITreeControlHandler* handler)
         {
            return IDETreeView::Create(parent, handler);
         }

         IIDEReportWindow* CreateReportView(IWindow& parent)
         {
            return IDEReportWindow::Create(parent);
         }

         ISpatialManager* CreateSpatialManager(IWindow& parent, IPaneDatabase& database)
         {
            return IDESpatialManager::Create(parent, database, true);
         }

         ISpatialManager* LoadSpatialManager(IWindow& parent, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, LOGFONT& logFont, const wchar_t* appName)
         {
            return _LoadSpatialManager(parent, logFont, database, idArray, nPanes, versionId, appName);
         }
      }
   }
}
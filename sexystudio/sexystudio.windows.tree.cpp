#include "sexystudio.impl.h"

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	// Is it a bird, is it a plane? No it's a tree.
	struct Tree : IGuiTree, IWin32WindowMessageLoopHandler
	{
		Win32ChildWindow eventSinkWindow;
		HWND hTreeWnd;
		AutoFree<ILayoutSet> layouts = CreateLayoutSet();

		Tree(IWidgetSet& widgets, const TreeStyle& treeStyle):
			eventSinkWindow(widgets.Parent(), *this)
		{
			DWORD exStyle = TVS_EX_AUTOHSCROLL | TVS_EX_DOUBLEBUFFER;
			DWORD win32Style = WS_CHILD | WS_VSCROLL;

			if (treeStyle.hasCheckBoxes) win32Style |= TVS_CHECKBOXES;
			if (treeStyle.hasButtons) win32Style |= TVS_HASBUTTONS | TVS_LINESATROOT;
			if (treeStyle.hasLines) win32Style |= TVS_HASLINES | TVS_LINESATROOT;

			hTreeWnd = CreateWindowExA(exStyle, WC_TREEVIEWA, "", win32Style, 0, 0, 100, 100, eventSinkWindow, NULL, NULL, NULL);
			if (!hTreeWnd)
			{
				Throw(GetLastError(), "%s: Could not create tree window", __FUNCTION__);
			}

			Theme theme = GetTheme(widgets.Context().publisher);

			TreeView_SetBkColor(hTreeWnd, ToCOLORREF(theme.normal.bkColor));
			TreeView_SetTextColor(hTreeWnd, ToCOLORREF(theme.normal.txColor));
		}

		~Tree()
		{
			DestroyWindow(hTreeWnd);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				{
					auto width = LOWORD(lParam);
					auto height = HIWORD(lParam);
					MoveWindow(hTreeWnd, 0, 0, width, height, TRUE);
				}
				break;
			}
			return DefWindowProcA(eventSinkWindow, msg, wParam, lParam);
		}

		ID_TREE_ITEM AppendItem(ID_TREE_ITEM branch) override
		{
			TV_INSERTSTRUCTA item = { 0 };
			item.hParent = (HTREEITEM)branch;
			item.hInsertAfter = TVI_LAST;
			item.itemex.mask = 0; 
			HTREEITEM hItem = (HTREEITEM)SendMessageA(hTreeWnd, TVM_INSERTITEMA, 0, (LPARAM)&item);
			if (hItem == 0)
			{
				Throw(GetLastError(), "%s: could not insert item in tree", __FUNCTION__);
			}
			return (ID_TREE_ITEM)hItem;
		}

		void EnableExpansionIcons(bool enable) override
		{
			
		}

		void SetItemText(cstr text, ID_TREE_ITEM hItem) override
		{
			TV_ITEMA item = { 0 };
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.pszText = const_cast<char*>(text);;
			item.hItem = (HTREEITEM) hItem;
			SendMessageA(hTreeWnd, TVM_SETITEMA, 0, (LPARAM)&item);
		}

		void Layout() override
		{
			layouts->Layout(*this);
		}

		void AddLayoutModifier(ILayout* preprocessor) override
		{
			layouts->Add(preprocessor);
		}

		void Free() override
		{
			delete this;
		}

		// Modify visibility of the widget
		void SetVisible(bool isVisible) override
		{
			ShowWindow(eventSinkWindow, isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(hTreeWnd, isVisible ? SW_SHOW : SW_HIDE);
		}

		//  the set of children if it can possess children, otherwise returns nullptr
		IWidgetSet* Children() override
		{
			return nullptr;
		}

		IWindow& Window() override
		{
			return eventSinkWindow;
		}

		void Clear() override
		{
			TreeView_DeleteAllItems(hTreeWnd);
		}
	};
}

namespace Rococo::SexyStudio
{
	IGuiTree* CreateTree(IWidgetSet& widgets, const TreeStyle& style)
	{
		auto* tree = new Tree(widgets, style);
		widgets.Add(tree);
		return tree;
	}
}
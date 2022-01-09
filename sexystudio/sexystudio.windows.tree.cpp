#include "sexystudio.impl.h"

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	// Is it a bird, is it a plane? No it's a tree.
	struct Tree : IGuiTree, IWin32WindowMessageLoopHandler
	{
		Win32ChildWindow eventSinkWindow;
		HWNDProxy hTreeWnd;
		AutoFree<ILayoutSet> layouts = CreateLayoutSet();
		IGuiTreeRenderer* customRenderer;
		HIMAGELIST hImages = nullptr;

		Tree(IWidgetSet& widgets, const TreeStyle& treeStyle, IGuiTreeRenderer* _customRenderer):
			eventSinkWindow(widgets.Parent(), *this),
			customRenderer(_customRenderer)
		{
			enum { BITMAP_SPAN = 24 };
			hImages = ImageList_Create(BITMAP_SPAN, BITMAP_SPAN, ILC_COLOR32 | ILC_MASK, 3, 1);

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
			if (hImages) ImageList_Destroy(hImages);
		}

		virtual Windows::IWindow& TreeWindow()
		{
			return hTreeWnd;
		}

		void OnExpansionChanged(NMTREEVIEW& tv)
		{
			if (customRenderer)
			{
				int imageIndex = (int)(tv.action == TVE_EXPAND ? EFolderIcon::FOLDER_OPEN : EFolderIcon::FOLDER_CLOSED);
				SetItemImage((ID_TREE_ITEM) tv.itemNew.hItem, imageIndex);
			}
		}

		LRESULT OnNotifyFromTreeView(NMHDR* header, bool& callDefProc)
		{
			callDefProc = true;;

			switch (header->code)
			{
			case TVN_ITEMEXPANDED:
			case TVN_ITEMEXPANDING:
				OnExpansionChanged((NMTREEVIEW&)*header);
				return 0L;
			}

			return 0L;
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
			case WM_NOTIFY:
				{
					auto* pHeader = (NMHDR*)(lParam);
					if (pHeader->hwndFrom == hTreeWnd)
					{
						bool callDefProc = false;
						LRESULT retValue = OnNotifyFromTreeView(pHeader, callDefProc);
						if (callDefProc)
						{
							break;
						}
						else
						{
							return retValue;
						}
					}
				}
			}

			return DefWindowProcA(eventSinkWindow, msg, wParam, lParam);
		}

		ID_TREE_ITEM AppendItem(ID_TREE_ITEM branch) override
		{
			TV_INSERTSTRUCTA item = { 0 };
			item.hParent = (HTREEITEM)branch;
			item.hInsertAfter = TVI_LAST;

			HTREEITEM hItem = (HTREEITEM)SendMessageA(hTreeWnd, TVM_INSERTITEMA, 0, (LPARAM)&item);
			if (hItem == 0)
			{
				Throw(GetLastError(), "%s: could not insert item in tree", __FUNCTION__);
			}

			return (ID_TREE_ITEM)hItem;
		}

		void CollapseBranchAndRecurse(HTREEITEM hItem)
		{
			auto state = TreeView_GetItemState(hTreeWnd, hItem, 0xFFFFFFFF);
			TreeView_SetItemState(hTreeWnd, hItem, state & ~TVIS_EXPANDEDONCE, 0xFFFFFFFF);
			auto hChild = TreeView_GetChild(hTreeWnd, hItem);
			while (hChild)
			{
				CollapseBranchAndRecurse(hChild);
				hChild = TreeView_GetNextSibling(hTreeWnd, hChild);
			}
		}

		void Collapse() override
		{
			auto root = TreeView_GetRoot(hTreeWnd);
			CollapseBranchAndRecurse(root);
		}

		void ExpandAt(ID_TREE_ITEM id) override
		{
			auto hItem = (HTREEITEM) id;
			TreeView_SelectItem(hTreeWnd, hItem);
		}
 
		void SetImageList(uint32 nItems, ...)
		{
			if (ImageList_GetImageCount(hImages) != 0)
			{
				Throw(0, "%s: the image list has already been defined", __FUNCTION__);
			}

			HINSTANCE hInstance = GetMainInstance();

			va_list args;
			va_start(args, nItems);

			ImageList_SetBkColor(hImages, TreeView_GetBkColor(hTreeWnd));

			for (uint32 i = 0; i < nItems; ++i)
			{
				uint32 id = va_arg(args, uint32);
				HBITMAP hBitmap = (HBITMAP) LoadImageW(hInstance, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, 0);
				if (hBitmap == nullptr)
				{
					Throw(GetLastError(), "%s: LoadBitmap failed", __FUNCTION__);
				}
				ImageList_AddMasked(hImages, hBitmap, RGB(255,255,255));
				DeleteObject(hBitmap);
			}

			va_end(args);

			// Fail if not all of the images were added. 
			if (ImageList_GetImageCount(hImages) != nItems)
			{
				Throw(0, "%s: ImageList failed to add all bitmaps", __FUNCTION__);
			}

			// Associate the image list with the tree-view control. 
			TreeView_SetImageList(hTreeWnd, hImages, TVSIL_NORMAL);
		}

		void EnableExpansionIcons(bool enable) override
		{
			
		}

		void SetContext(ID_TREE_ITEM hItem, uint64 contextId) override
		{
			TV_ITEMA item = { 0 };
			item.mask = TVIF_PARAM | TVIF_HANDLE;
			item.lParam = (LPARAM)contextId;
			item.hItem = (HTREEITEM)hItem;
			SendMessageA(hTreeWnd, TVM_SETITEMA, 0, (LPARAM)&item);
		}

		void SetItemText(cstr text, ID_TREE_ITEM hItem) override
		{
			TV_ITEMA item = { 0 };
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.pszText = const_cast<char*>(text);;
			item.hItem = (HTREEITEM) hItem;
			SendMessageA(hTreeWnd, TVM_SETITEMA, 0, (LPARAM)&item);
		}

		void SetItemImage(ID_TREE_ITEM hItem, int imageIndex) override
		{
			TV_ITEMA item = { 0 };
			item.mask = TVIF_IMAGE | TVIF_HANDLE | TVIF_SELECTEDIMAGE;
			item.hItem = (HTREEITEM)hItem;
			item.iImage = imageIndex;
			item.iSelectedImage = imageIndex;
			SendMessageA(hTreeWnd, TVM_SETITEMA, 0, (LPARAM)&item);
		}

		void SetItemExpandedImage(ID_TREE_ITEM hItem, int imageIndex) override
		{
			TVITEMEXA item = { 0 };
			item.mask =  TVIF_HANDLE | TVIF_EXPANDEDIMAGE;
			item.hItem = (HTREEITEM)hItem;
			item.iExpandedImage = imageIndex;
			SendMessageA(hTreeWnd, TVM_SETITEMA, 0, (LPARAM)&item);
		}

		void Layout() override
		{
			layouts->Layout(*this);
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layouts->Add(l);
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

		void EnumerateChildren(ID_TREE_ITEM id, IEventCallback<TreeItemInfo>& cb) const override
		{
			ID_TREE_ITEM idChild = (ID_TREE_ITEM) SendMessage(hTreeWnd, TVM_GETNEXTITEM, TVGN_CHILD, id);
			if (idChild != 0)
			{
				TreeItemInfo info{ idChild };
				cb.OnEvent(info);

				for (;;)
				{
					idChild = (ID_TREE_ITEM)SendMessage(hTreeWnd, TVM_GETNEXTITEM, TVGN_NEXT, idChild);
					if (idChild != 0)
					{
						TreeItemInfo info{ idChild };
						cb.OnEvent(info);
					}
					else
					{
						break;
					}
				}
			}
		}

		void GetText(char* buffer, size_t capacity, ID_TREE_ITEM id) override
		{
			TV_ITEMA item = { 0 };
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.pszText = buffer;
			item.cchTextMax = (int) capacity;
			item.hItem = (HTREEITEM)id;
			SendMessageA(hTreeWnd, TVM_GETITEMA, 0, (LPARAM) &item);
		}
	};
}

namespace Rococo::SexyStudio
{
	IGuiTree* CreateTree(IWidgetSet& widgets, const TreeStyle& style, IGuiTreeRenderer* customRenderer)
	{
		auto* tree = new Tree(widgets, style, customRenderer);
		widgets.Add(tree);
		return tree;
	}
}
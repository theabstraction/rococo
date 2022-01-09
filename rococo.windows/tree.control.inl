namespace
{
	using namespace Rococo::Visitors;

	HTREEITEM ToHTree(TREE_NODE_ID id)
	{
		union
		{
			HTREEITEM hTreeItem;
			TREE_NODE_ID id;
		} u;

		u.id = id;
		return u.hTreeItem;
	}

	TREE_NODE_ID ToId(HTREEITEM hValue)
	{
		union
		{
			HTREEITEM hTreeItem;
			TREE_NODE_ID id;
		} u;

		u.hTreeItem = hValue;
		return u.id;
	}

	class TreeControlSupervisor : public ITreeControlSupervisor, private IWindowHandler, public IUITree
	{
	private:
		IParentWindowSupervisor* containerWindow;
		HWND hTreeWindow;
		HWND hTitle;

		ITreeControlHandler& eventHandler;

		TreeControlSupervisor(ITreeControlHandler& _eventHandler) :
			containerWindow(nullptr),
			hTreeWindow(nullptr),
			hTitle(nullptr),
			eventHandler(_eventHandler)
		{
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_NOTIFY:
				{
					NMHDR* header = (NMHDR*)lParam;
               if (header->code == TVN_SELCHANGED)
               {
                  LPNMTREEVIEW treeVieww = (LPNMTREEVIEW) header;
                  auto& i = treeVieww->itemNew;
                  eventHandler.OnItemSelected((int64)i.lParam, *this);
               }
					break;
				}
			case WM_ERASEBKGND:
				return 0L;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			switch (wParam)
			{
			case SIZE_RESTORED:
			case SIZE_MAXSHOW:
			case SIZE_MAXIMIZED:
			case SIZE_MAXHIDE:
				if (containerWindow) StandardResizeControlWithTitleBar(*containerWindow, hTreeWindow, hTitle);
				break;
			}

			return 0L;
		}

		void Construct(const WindowConfig& treeConfig, IWindow& parent, ControlId id)
		{
			WindowConfig containerConfig = treeConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;
		
         containerWindow = Windows::CreateChildWindow(containerConfig, this);

			DWORD checkedStyle = 0;

			WindowConfig treeConfigCorrected = treeConfig;
			treeConfigCorrected.hWndParent = *containerWindow;
			treeConfigCorrected.style |= WS_CHILD | WS_VISIBLE;

			if ((treeConfigCorrected.style & TVS_CHECKBOXES) != 0)
			{
				// Hack, for reasons google TVS_CHECKBOXES
				checkedStyle = treeConfigCorrected.style;
				treeConfigCorrected.style &= ~(DWORD)TVS_CHECKBOXES;
			}
		
			if (treeConfig.windowName && treeConfig.windowName[0] != 0)
			{
				WindowConfig titleConfig;
				Windows::SetChildWindowConfig(titleConfig, GuiRect(0, 0, 0, 0), *containerWindow, treeConfig.windowName, WS_CHILD | WS_VISIBLE, 0);
				hTitle = CreateWindowIndirect(WC_STATICA, titleConfig, nullptr);
			}

			hTreeWindow = CreateWindowIndirect(WC_TREEVIEWA, treeConfigCorrected, nullptr);

			if (checkedStyle)
			{
				// Hack, for reasons google TVS_CHECKBOXES
				SetWindowLongPtr(hTreeWindow, GWL_STYLE, checkedStyle);
			}

			SetDlgCtrlID(hTreeWindow, id);

			SetControlFont(hTreeWindow);
			SetTitleFont(hTitle);

			StandardResizeControlWithTitleBar(*containerWindow, hTreeWindow, hTitle);
		}

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }
	public:
		static TreeControlSupervisor* Create(const WindowConfig& config, IWindow& parent, ControlId id, ITreeControlHandler& eventHandler)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			TreeControlSupervisor* p = new TreeControlSupervisor(eventHandler);
			p->Construct(config, parent, id);
			return p;
		}

		~TreeControlSupervisor()
		{
         Rococo::Free(containerWindow);
		}

		virtual IUITree& operator()()
		{
			return *this;
		}

		virtual operator IUITree& ()
		{
			return *this;
		}

		virtual IUITree& Tree()
		{
			return *this;
		}

		virtual CheckState GetCheckState(TREE_NODE_ID id) const
		{
			int checkstate = TreeView_GetCheckState(hTreeWindow, ToHTree(id));
			switch (checkstate)
			{
			case 0:
				return CheckState_Clear;
			case 1:
				return CheckState_Ticked;
			default:
				return CheckState_NoCheckBox;
			}
		}

		virtual TREE_NODE_ID AddChild(TREE_NODE_ID parentId, cstr text, CheckState state)
		{
			TVINSERTSTRUCTA z = { 0 };
			z.hParent = ToHTree(parentId);
			z.hInsertAfter = TVI_LAST;
			z.itemex.mask = TVIF_TEXT;
			z.itemex.pszText = (char*)text;

			size_t len = rlen(text);

			if (len > INT_MAX)
			{
				Throw(0, "TreeView_SetItem failed: text length > INT_MAX");
			}

			z.itemex.cchTextMax = (int) len;
			
			TREE_NODE_ID id = ToId(TreeView_InsertItem(hTreeWindow, &z));

			TVITEMEX y = { 0 };
			y.mask = TVIF_STATE | TVIF_HANDLE;
			y.stateMask = TVIS_STATEIMAGEMASK;
			y.state = INDEXTOSTATEIMAGEMASK(state);
			y.hItem = ToHTree(id);
			BOOL isOK = TreeView_SetItem(hTreeWindow, &y);
			if (!isOK) Throw(GetLastError(), "TreeView_SetItem (TEXT/STATE) failed");
			return id;
		}

      virtual void SetId(TREE_NODE_ID nodeId, int64 id)
      {
         TVITEMEX item = { 0 };
         item.mask = TVIF_PARAM;
         item.hItem = ToHTree(nodeId);
         item.lParam = (int64) id;
         if (!TreeView_SetItem(hTreeWindow, &item))
         {
            Throw(GetLastError(), "TreeView_SetItem (LPARAM) failed");
         }
      }

		virtual TREE_NODE_ID AddRootItem(cstr text, CheckState state)
		{
			TVINSERTSTRUCTA z = { 0 };
			z.hInsertAfter = TVI_LAST;
			z.itemex.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM;
			z.itemex.stateMask = TVIS_STATEIMAGEMASK;
			z.itemex.pszText = (char*) text;

			size_t len = rlen(text);

			if (len > INT_MAX)
			{
				Throw(0, "AddRootItem failed: text length > INT_MAX");
			}

			z.itemex.cchTextMax = (int)len;
			z.itemex.state = INDEXTOSTATEIMAGEMASK(state);
			TREE_NODE_ID id = ToId(TreeView_InsertItem(hTreeWindow, &z));

			TVITEMEX y = { 0 };
			y.mask = TVIF_STATE | TVIF_HANDLE;
			y.stateMask = TVIS_STATEIMAGEMASK;
			y.state = INDEXTOSTATEIMAGEMASK(state);
			y.hItem = ToHTree(id);
			BOOL isOK = TreeView_SetItem(hTreeWindow, &y);
			if (!isOK) Throw(GetLastError(), "AddRootItem failed");
			return id;
		}

		virtual void ResetContent()
		{
			TreeView_DeleteAllItems(hTreeWindow);
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return *containerWindow;
		}

		virtual HWND TreeHandle() const
		{
			return hTreeWindow;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}
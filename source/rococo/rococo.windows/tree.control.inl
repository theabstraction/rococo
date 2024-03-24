namespace Rococo::Windows
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

				switch (header->code)
				{
					case TVN_SELCHANGED:
						{
							LPNMTREEVIEW treeVieww = (LPNMTREEVIEW)header;
							auto& i = treeVieww->itemNew;
							eventHandler.OnItemSelected(ToId(i.hItem), *this);
						}
						return 0L;
					case NM_RCLICK:
						{
							DWORD dwpos = GetMessagePos();
							POINT pos{ (LONG)GET_X_LPARAM(dwpos), (LONG)GET_Y_LPARAM(dwpos) };
							POINT clientPos = pos;
							ScreenToClient(hTreeWindow, &clientPos);

							TV_HITTESTINFO data = { 0 };
							data.pt = clientPos;
							TreeView_HitTest(hTreeWindow, &data);

							if (data.flags & TVHT_ONITEM && data.hItem != 0)
							{
								eventHandler.OnItemRightClicked(ToId(data.hItem), *this);
							}
						}
						return 0L;
				}
				break;
			}
			case TVM_GETBKCOLOR:
				return ToCOLORREF(scheme.backColour);
			case WM_ERASEBKGND:
				return 0L;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			UNUSED(width);
			UNUSED(height);

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

		void OnPretranslateMessage(MSG&) override
		{

		}

		ColourScheme scheme;
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

		void SetColourSchemeRecursive(const ColourScheme& scheme) override
		{
			this->scheme = scheme;
			TreeView_SetBkColor(hTreeWindow, ToCOLORREF(scheme.backColour));
			TreeView_SetTextColor(hTreeWindow, ToCOLORREF(scheme.foreColour));
		}

		IUITree& operator()()
		{
			return *this;
		}

		operator IUITree& ()
		{
			return *this;
		}

		virtual IUITree& Tree() override
		{
			return *this;
		}

		CheckState GetCheckState(TREE_NODE_ID id) const override
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

		TREE_NODE_ID FindChild(TREE_NODE_ID parentId, cstr withText) override
		{
			HTREEITEM hFirstChild = TreeView_GetChild(hTreeWindow, ToHTree(parentId));
			HTREEITEM hItem = hFirstChild;
			while (hItem != 0)
			{
				TVITEMA item = { 0 };
				item.hItem = hItem;
				item.mask = TVIF_TEXT;
				CHAR buf[MAX_PATH];
				item.cchTextMax = MAX_PATH;
				item.pszText = buf;
				TreeView_GetItem(hTreeWindow, &item);

				if (Eq(buf, withText))
				{
					return ToId(hItem);
				}

				hItem = TreeView_GetNextSibling(hTreeWindow, hItem);
			}

			return TREE_NODE_ID{ 0 };
		}

		TREE_NODE_ID AddChild(TREE_NODE_ID parentId, cstr text, CheckState state) override
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

			z.itemex.cchTextMax = (int)len;

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

		bool Select(TREE_NODE_ID id) override
		{
			if (TreeView_SelectSetFirstVisible(hTreeWindow, ToHTree(id)))
			{
				SetFocus(hTreeWindow);
				TreeView_Select(hTreeWindow, ToHTree(id), TVGN_CARET);
				return true;
			}

			return false;
		}

		void SetId(TREE_NODE_ID nodeId, int64 id) override
		{
			TVITEMEX item = { 0 };
			item.mask = TVIF_PARAM;
			item.hItem = ToHTree(nodeId);
			item.lParam = (int64)id;
			if (!TreeView_SetItem(hTreeWindow, &item))
			{
				Throw(GetLastError(), "TreeView_SetItem (LPARAM) failed");
			}
		}

		TREE_NODE_ID AddRootItem(cstr text, CheckState state) override
		{
			TVINSERTSTRUCTA z = { 0 };
			z.hInsertAfter = TVI_LAST;
			z.itemex.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM;
			z.itemex.stateMask = TVIS_STATEIMAGEMASK;
			z.itemex.pszText = (char*)text;

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

		void ResetContent() override
		{
			TreeView_DeleteAllItems(hTreeWindow);
		}

		bool TryGetText(char* subspace, size_t sizeofSubspace, TREE_NODE_ID id) override
		{
			if (!subspace) Throw(0, "%s: null subspace argument", __FUNCTION__);

			TVITEMEX y = { 0 };
			y.mask = TVIF_TEXT | TVIF_HANDLE;
			y.hItem = ToHTree(id);
			y.pszText = subspace;
			y.cchTextMax = (int) sizeofSubspace;

			return TreeView_GetItem(hTreeWindow, &y) ? true : false;
		}

		IWindowHandler& Handler() override
		{
			return *this;
		}

		operator HWND () const override
		{
			return *containerWindow;
		}

		HWND TreeHandle() const override
		{
			return hTreeWindow;
		}

		void Free() override
		{
			delete this;
		}
	};
}
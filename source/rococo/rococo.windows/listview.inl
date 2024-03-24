namespace Rococo::Windows
{
	class ListViewSupervisor : public IListViewSupervisor, private IWindowHandler, public IUIList
	{
	private:
		HWND hWnd;
		HWND hWndListView;
		HWND hTitle;

		IListViewEvents& eventHandler;

		ListViewSupervisor(IListViewEvents& _eventHandler) :
			hWnd(nullptr),
			hWndListView(nullptr),
			hTitle(nullptr),
			eventHandler(_eventHandler)
		{
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case LVM_GETBKCOLOR:
				return ToCOLORREF(scheme.backColour);
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_DRAWITEM:
				return OnDrawItem(hWnd, wParam, lParam);
			case WM_MEASUREITEM:
				return OnMeasureItem(hWnd, wParam, lParam);
			case WM_NOTIFY:
				return OnNotify(hWnd, wParam, lParam);
				break;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			UNUSED(height);
			UNUSED(width);

			switch (wParam)
			{
			case SIZE_RESTORED:
			case SIZE_MAXSHOW:
			case SIZE_MAXIMIZED:
			case SIZE_MAXHIDE:
				StandardResizeControlWithTitleBar(hWnd, hWndListView, hTitle);
				break;
			}

			return 0L;
		}

		LRESULT OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT_PTR id = wParam;
			UNUSED(id);
			auto* header = (NMHDR*)lParam;
			if (header->code == LVN_ITEMCHANGED)
			{
				auto& n = *(LPNMLISTVIEW)header;
				eventHandler.OnItemChanged(n.iItem);
				return TRUE;
			}
			return DefWindowProc(hWnd, WM_NOTIFY, wParam, lParam);
		}

		BOOL OnDrawItem(HWND, WPARAM, LPARAM lParam)
		{
			DRAWITEMSTRUCT& dis = *LPDRAWITEMSTRUCT(lParam);
			eventHandler.OnDrawItem(dis);
			return TRUE;
		}

		BOOL OnMeasureItem(HWND, WPARAM, LPARAM lParam)
		{
			MEASUREITEMSTRUCT& mis = *LPMEASUREITEMSTRUCT(lParam);
			eventHandler.OnMeasureItem(hWndListView, mis);
			return TRUE;
		}

		void Construct(const WindowConfig& listConfig, IWindow& parent, DWORD containerStyle)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			WindowConfig containerConfig = listConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE | containerStyle;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(this));

			WindowConfig listConfigCorrected = listConfig;
			listConfigCorrected.left = 0;
			listConfigCorrected.top = 0;
			listConfigCorrected.hWndParent = hWnd;
			listConfigCorrected.style |= WS_CHILD | WS_VISIBLE;

			hWndListView = CreateWindowIndirect(WC_LISTVIEWA, listConfigCorrected, nullptr);
			SetDlgCtrlID(hWndListView, 1001);
			SetControlFont(hWndListView);

			if (listConfig.windowName != nullptr && listConfig.windowName[0] != 0)
			{
				WindowConfig titleConfig = listConfig;
				titleConfig.style = WS_CHILD | WS_VISIBLE;
				titleConfig.exStyle = 0;
				titleConfig.hWndParent = hWnd;
				hTitle = CreateWindowIndirect(WC_STATICA, titleConfig, nullptr);
				SetTitleFont(hTitle);
			}

			StandardResizeControlWithTitleBar(hWnd, hWndListView, hTitle);
		}

		void OnModal() override
		{

		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		ColourScheme scheme;
	public:
		static ListViewSupervisor* Create(const WindowConfig& listConfig, IWindow& parent, IListViewEvents& eventHandler, DWORD containerStyle)
		{
			ListViewSupervisor* p = new ListViewSupervisor(eventHandler);
			p->Construct(listConfig, parent, containerStyle);
			return p;
		}

		~ListViewSupervisor()
		{
			DestroyWindow(hWnd);
		}

		void SetColourSchemeRecursive(const ColourScheme& scheme) override
		{
			this->scheme = scheme;
			ListView_SetBkColor(hWndListView, ToCOLORREF(scheme.backColour));
			ListView_SetTextColor(hWndListView, ToCOLORREF(scheme.foreColour));
			InvalidateRect(hWndListView, NULL, TRUE);
		}

		IUIList& UIList() override
		{
			return *this;
		}

		operator IUIList& () override
		{
			return *this;
		}

		void AddRow(cstr values[]) override
		{
			if (values == nullptr || *values == nullptr)
			{
				Throw(0, "ListViewSupervisor::AddRow needs at least one value");
			}

			LV_ITEMA item = { 0 };
			item.mask = LVIF_TEXT;
			item.pszText = (char*)values[0];
			item.iItem = 0x7FFFFFFF;
			item.cchTextMax = 256;
			int index = ListView_InsertItem(hWndListView, &item);
			if (index < 0)
			{
				Throw(0, "ListViewSupervisor::AddRow failed. ListView_InsertItem returned %d", index);
			}

			int k = 1;
			for (cstr* value = values + k; *value != nullptr; ++value, ++k)
			{
				LVITEMA diddle = { 0 };
				diddle.iSubItem = k;
				diddle.pszText = (char*)*value;
				SendMessage(hWndListView, LVM_SETITEMTEXT, index, (LPARAM)&diddle);
			}
		}

		void ClearRows() override
		{
			ListView_DeleteAllItems(hWndListView);
		}

		int NumberOfRows() const override
		{
			return (int)ListView_GetItemCount(hWndListView);
		}

		void DeleteRow(int rowIndex) override
		{
			ListView_DeleteItem(hWndListView, rowIndex);
		}

		void SetColumns(cstr columnNames[], int widths[]) override
		{
			while (ListView_DeleteColumn(hWndListView, 0));

			int index = 0;
			for (cstr* col = columnNames; *col != nullptr; ++col, ++index)
			{
				LV_COLUMNA item;
				item = { 0 };
				item.mask = LVCF_TEXT | LVCF_WIDTH;
				item.pszText = (char*)*col;
				item.cx = widths[index];
				if (-1 == ListView_InsertColumn(hWndListView, 10000, &item))
				{
					Throw(0, "Error inserting item into a ListView header");
				}
			}
		}

		IWindowHandler& Handler() override
		{
			return *this;
		}

		operator HWND () const override
		{
			return hWnd;
		}

		HWND ListViewHandle() const override
		{
			return hWndListView;
		}

		void Free() override
		{
			delete this;
		}
	};
}
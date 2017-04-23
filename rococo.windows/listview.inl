namespace
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
         case WM_ERASEBKGND:
            return TRUE;
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_DRAWITEM:
				return OnDrawItem(hWnd, wParam, lParam);
			case WM_MEASUREITEM:
				return OnMeasureItem(hWnd, wParam, lParam);
			case WM_COMMAND:
			//	if (HIWORD(wParam) == LBN_SELCHANGE && (HWND)lParam == hWndListView)
			//	{
			//		itemRenderer.OnItemSelectionChanged(*this);
			//		return 0L;
			//	}
				break;
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
				StandardResizeControlWithTitleBar(hWnd, hWndListView, hTitle);
				break;
			}

			return 0L;
		}

		BOOL OnDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DRAWITEMSTRUCT& dis = *LPDRAWITEMSTRUCT(lParam);
			eventHandler.OnDrawItem(dis);
			return TRUE;
		}

		BOOL OnMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			MEASUREITEMSTRUCT& mis = *LPMEASUREITEMSTRUCT(lParam);
			eventHandler.OnMeasureItem(mis);
			return TRUE;
		}

		void Construct(const WindowConfig& listConfig, IWindow& parent, IListViewEvents& eventHandler, DWORD containerStyle)
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

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }
	public:
		static ListViewSupervisor* Create(const WindowConfig& listConfig, IWindow& parent, IListViewEvents& eventHandler, DWORD containerStyle)
		{
			ListViewSupervisor* p = new ListViewSupervisor(eventHandler);
			p->Construct(listConfig, parent, eventHandler, containerStyle);
			return p;
		}

		~ListViewSupervisor()
		{
         DestroyWindow(hWnd);
		}

		virtual IUIList& UIList()
		{
			return *this;
		}

		virtual operator IUIList& ()
		{
			return *this;
		}

		virtual void AddRow(cstr values[])
		{
			if (values == nullptr || *values == nullptr)
			{
				Throw(0, "ListViewSupervisor::AddRow needs at least one value");
			}

			LV_ITEMA item = { 0 };
			item.mask = LVIF_TEXT;
			item.pszText = (rchar*)values[0];
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
				LVITEMA item = { 0 };
				item.iSubItem = k;
				item.pszText = (rchar*) *value;
				SendMessage(hWndListView, LVM_SETITEMTEXT, index, (LPARAM)&item);
			}
		}

		virtual void ClearRows()
		{
			ListView_DeleteAllItems(hWndListView);
		}

		virtual int NumberOfRows() const
		{
			return (int) ListView_GetItemCount(hWndListView);
		}

		virtual void DeleteRow(int rowIndex)
		{
			ListView_DeleteItem(hWndListView, rowIndex);
		}

		virtual void SetColumns(cstr columnNames[], int widths[])
		{
			while (ListView_DeleteColumn(hWndListView, 0));

			int index = 0;
			for (cstr* col = columnNames; *col != nullptr; ++col, ++index)
			{
				LV_COLUMNA item;
				item = { 0 };
				item.mask = LVCF_TEXT | LVCF_WIDTH;
				item.pszText = (rchar*) *col;
				item.cx = widths[index];
				if (-1 == ListView_InsertColumn(hWndListView, 10000, &item))
				{
					Throw(0, "Error inserting item into a ListView header");
				}
			}
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual HWND ListViewHandle() const
		{
			return hWndListView;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}
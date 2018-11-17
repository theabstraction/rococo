namespace
{
	class ListBoxSupervisor : public IListWindowSupervisor, private IWindowHandler
	{
	private:
		HWND hWnd;
		HWND hWndListBox;
		HWND hTitle;
		IListItemHandler& itemRenderer;

		ListBoxSupervisor(IListItemHandler& _itemRenderer) :
			hWnd(nullptr),
			hWndListBox(nullptr),
			hTitle(nullptr),
			itemRenderer(_itemRenderer)
		{
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_DRAWITEM:
				return OnDrawItem(hWnd, wParam, lParam);
			case WM_MEASUREITEM:
				return OnMeasureItem(hWnd, wParam, lParam);
			case WM_COMMAND:
				if (HIWORD(wParam) == LBN_SELCHANGE && (HWND)lParam == hWndListBox)
				{
					itemRenderer.OnItemSelectionChanged(*this);
					return 0L;
				}
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
				StandardResizeControlWithTitleBar(hWnd, hWndListBox, hTitle);
				break;
			}

			return 0L;
		}

		BOOL OnDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			DRAWITEMSTRUCT& dis = *LPDRAWITEMSTRUCT(lParam);
			itemRenderer.OnDrawItem(dis);
			return TRUE;
		}

		BOOL OnMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			MEASUREITEMSTRUCT& mis = *LPMEASUREITEMSTRUCT(lParam);
			itemRenderer.OnMeasureItem(mis);
			return TRUE;
		}

		int AddString(cstr data)
		{
			LRESULT index = SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data); // If data is not a string, we need owner draw and no LBS_HASSTRINGS, else we crash here.
			return (int) index;
		}

		void ResetContent()
		{
			SendMessage(hWndListBox, LB_RESETCONTENT, 0, 0);
		}

		int GetCurrentSelection()
		{
			LRESULT index = SendMessage(hWndListBox, LB_GETCURSEL, 0, 0);
			return (int)index;
		}

		void SetCurrentSelection(int index)
		{
			SendMessage(hWndListBox, LB_SETCURSEL, index, 0);
		}

		LRESULT GetItemData(int index)
		{
			return SendMessage(hWndListBox, LB_GETITEMDATA, index, 0);
		}

		bool GetString(int index, char* data, size_t capacity)
		{
			LRESULT length = SendMessage(hWndListBox, LB_GETTEXTLEN, index, 0);
			if (length == LB_ERR) return false;
			char* buffer = (char*) _malloca(sizeof(char)* (length + 1));
			if (LB_ERR == SendMessage(hWndListBox, LB_GETTEXT, index, (LPARAM)buffer))
			{
				return false;
			}

         StackStringBuilder sb(data, capacity);
         sb << buffer;
			return true;
		}

		void Construct(const WindowConfig& listConfig, IParentWindowSupervisor& parent, DWORD containerStyle, DWORD containerStyleEx)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			WindowConfig containerConfig = listConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE | containerStyle;
			containerConfig.exStyle = containerStyleEx;
			containerConfig.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(this));

			WindowConfig listConfigCorrected = listConfig;
			listConfigCorrected.left = 0;
			listConfigCorrected.top = 0;
			listConfigCorrected.hWndParent = hWnd;
			listConfigCorrected.style |= WS_CHILD | WS_VISIBLE | LBS_NOTIFY;

			hWndListBox = CreateWindowIndirect("LISTBOX", listConfigCorrected, nullptr);
			SetDlgCtrlID(hWndListBox, 1001);

			if (listConfig.windowName != nullptr && listConfig.windowName[0] != 0)
			{
				WindowConfig titleConfig = listConfig;
				titleConfig.style = WS_CHILD | WS_VISIBLE;
				titleConfig.exStyle = 0;
				titleConfig.hWndParent = hWnd;
				hTitle = CreateWindowIndirect("STATIC", titleConfig, nullptr);
				SetTitleFont(hTitle);
			}

			SetControlFont(hWndListBox);

			StandardResizeControlWithTitleBar(hWnd, hWndListBox, hTitle);
		}

      virtual void OnPretranslateMessage(MSG& msg)
      {

      }
	public:
		static ListBoxSupervisor* Create(const WindowConfig& listConfig, IParentWindowSupervisor& parent, IListItemHandler& itemRenderer, DWORD containerStyle, DWORD containerStyleEx)
		{
			auto p = new ListBoxSupervisor(itemRenderer);
			p->Construct(listConfig, parent, containerStyle, containerStyleEx);
			return p;
		}

		~ListBoxSupervisor()
		{
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual HWND ListBoxHandle() const
		{
			return hWndListBox;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}
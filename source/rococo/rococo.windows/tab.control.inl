namespace Rococo::Windows
{
	class TabControlSupervisor : public ITabControl, private IWindowHandler
	{
	private:
		HWND hWnd;
		HWND hWndTabControl;
		IParentWindowSupervisor* clientSpace;
		StandardWindowHandler clientSpaceHandler;
		ITabControlEvents& eventHandler;
		std::unordered_map<size_t, std::string> tooltips;
		size_t nextIndex;

		TabControlSupervisor(ITabControlEvents& _eventHandler) :
			hWnd(nullptr),
			hWndTabControl(nullptr),
			clientSpace(nullptr),
			eventHandler(_eventHandler),
			nextIndex(0)
		{
		}

		NOT_INLINE void OnDrawTabs(DRAWITEMSTRUCT&d)
		{
			
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_DRAWITEM:
				{
					auto* d = (DRAWITEMSTRUCT*)lParam;
					OnDrawTabs(*d);
					return TRUE;
				}
				break;
			case WM_RBUTTONUP:
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);

				POINT cursorPos2 = cursorPos;
				ScreenToClient(hWndTabControl, &cursorPos2);

				TCHITTESTINFO info = { cursorPos2, 0 };
				int index = TabCtrl_HitTest(hWndTabControl, &info);

				eventHandler.OnTabRightClicked(index, cursorPos);
				return 0L;
			}
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_NOTIFY:
			{
				NMHDR* header = (NMHDR*)lParam;
				if (header->code == TCN_SELCHANGE)
				{
					eventHandler.OnSelectionChanged(GetCurrentSelection());
					return 0L;
				}
				else if (header->code == NM_RCLICK)
				{
					POINT cursorPos;
					GetCursorPos(&cursorPos);

					POINT cursorPos2 = cursorPos;
					ScreenToClient(hWndTabControl, &cursorPos2);

					TCHITTESTINFO info = { cursorPos2, 0 };
					int index = TabCtrl_HitTest(hWndTabControl, &info);

					eventHandler.OnTabRightClicked(index, cursorPos);
				}
				else if (header->code == TTN_GETDISPINFOA)
				{
					NMTTDISPINFOA* info = (NMTTDISPINFOA*)header;

					TC_ITEM item = { 0 };
					item.dwState = TCIF_PARAM;
					TabCtrl_GetItem(hWndTabControl, info->hdr.idFrom, &item);

					char text[80];
					info->lpszText = text;

					auto i = tooltips.find((size_t)item.lParam);

					cstr src = i == tooltips.end() ? "" : i->second.c_str();
					StackStringBuilder sb(text, sizeof(text));
					sb << src;

					info->hinst = nullptr;
					return TRUE;
				}
			}
			case WM_COMMAND:
				break;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			switch (wParam)
			{
			case SIZE_RESTORED:
			case SIZE_MAXSHOW:
			case SIZE_MAXIMIZED:
			case SIZE_MAXHIDE:
				if (hWndTabControl)
				{
					MoveWindow(hWndTabControl, 0, 0, width, height, TRUE);
					SyncSize();
				}
				break;
			}

			return 0L;
		}

		void SyncSize()
		{
			RECT rect;
			GetClientRect(hWndTabControl, &rect);
			TabCtrl_AdjustRect(hWndTabControl, FALSE, &rect);

			MoveWindow(*clientSpace, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}

		int AddTab(cstr data, cstr tooltip)
		{
			TCITEMA tie = { 0 };
			tie.mask = TCIF_TEXT | TCIF_IMAGE | TCIF_PARAM;
			tie.iImage = -1;
			tie.pszText = (char*)data;
			tie.lParam = tooltip ? nextIndex : -1;

			if (tooltip) tooltips[nextIndex++] = std::string(tooltip);

			int index = TabCtrl_InsertItem(hWndTabControl, 10000, &tie);

			SyncSize();

			return index;
		}

		void ResetContent()
		{
			TabCtrl_DeleteAllItems(hWndTabControl);
			tooltips.clear();
			nextIndex = 0;
		}

		int GetCurrentSelection()
		{
			return TabCtrl_GetCurSel(hWndTabControl);
		}

		void SetCurrentSelection(int index)
		{
			TabCtrl_SetCurSel(hWndTabControl, index);
		}

		int TabCount() const
		{
			return TabCtrl_GetItemCount(hWndTabControl);
		}

		bool GetTabName(int index, char* buffer, DWORD capacity) const
		{
			TC_ITEMA item = { 0 };
			item.mask = TCIF_TEXT;
			item.pszText = buffer;
			item.cchTextMax = Rococo::min((DWORD)1024, capacity);
			return TabCtrl_GetItem(hWndTabControl, index, &item) == TRUE;
		}

		void DrawItem(HDC dc, RECT& rect, cstr text, UINT state, HBRUSH hButtonBrush)
		{
			FillRect(dc, &rect, hButtonBrush);

			COLORREF old = SetTextColor(dc, ToCOLORREF(scheme.foreColour));
			COLORREF oldBk = SetBkColor(dc, ToCOLORREF(HasFlag(TCIS_BUTTONPRESSED, state) ? scheme.pressedColour : scheme.backColour));
			DrawTextA(dc, text, (int) strlen(text), &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
			SetBkColor(dc, oldBk);
			SetTextColor(dc, old);			

			HBRUSH hEdgeBrush = CreateSolidBrush(ToCOLORREF(HasFlag(TCIS_BUTTONPRESSED, state) ? scheme.pressedEdgeColour : scheme.edgeColour));
			FrameRect(dc, &rect, hEdgeBrush);
			DeleteObject((HGDIOBJ)hEdgeBrush);
		}

		static LRESULT TabSubclassMsgHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
		{
			UNUSED(uIdSubclass);

			TabControlSupervisor* This = (TabControlSupervisor*)dwRefData;

			switch (uMsg)
			{
			case WM_PAINT:
				{
					HFONT hFont = (HFONT) SendMessage(hWnd, WM_GETFONT, 0, 0);

					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hWnd, &ps);

					auto oldFont = SelectObject(hdc, (HGDIOBJ) hFont);


					HBRUSH hBrush = CreateSolidBrush(ToCOLORREF(This->scheme.evenRowBackColour));
					FillRect(hdc, &ps.rcPaint, hBrush);

					HBRUSH hButtonBrush = CreateSolidBrush(ToCOLORREF(This->scheme.oddRowBackColour));
					HBRUSH hSelectedButtonBrush = CreateSolidBrush(ToCOLORREF(This->scheme.pressedColour));

					char tabChars[32];

					int tabs = TabCtrl_GetItemCount(hWnd);
					for (int i = 0; i < tabs; ++i)
					{
						TC_ITEM item = { 0 };		
						item.mask = TCIF_TEXT | TCIF_STATE;
						item.dwStateMask = TCIS_BUTTONPRESSED | TCIS_HIGHLIGHTED;
						item.cchTextMax = sizeof tabChars;
						item.pszText = tabChars;
						TabCtrl_GetItem(hWnd, i, &item);

						RECT rect;
						TabCtrl_GetItemRect(hWnd, i, &rect);

						This->DrawItem(hdc, rect, tabChars, item.dwState, HasFlag(TCIS_BUTTONPRESSED, item.dwState) ? hSelectedButtonBrush : hButtonBrush);
					}

					SelectObject(hdc, oldFont);

					EndPaint(hWnd, &ps);

					DeleteObject((HGDIOBJ)hSelectedButtonBrush);
					DeleteObject((HGDIOBJ)hButtonBrush);
					DeleteObject((HGDIOBJ) hBrush);

				}
			case WM_ERASEBKGND:
				return TRUE;
			}

			return DefSubclassProc(hWnd, uMsg, wParam, lParam);
		};

		void Construct(const WindowConfig& tabConfig, IWindow& parent)
		{
			WindowConfig containerConfig = tabConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(this));

			WindowConfig listConfigCorrected = tabConfig;
			listConfigCorrected.left = 0;
			listConfigCorrected.top = 0;
			listConfigCorrected.hWndParent = hWnd;
			listConfigCorrected.style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_OWNERDRAWFIXED;

			hWndTabControl = CreateWindowIndirect(WC_TABCONTROLA, listConfigCorrected, nullptr);
			SetDlgCtrlID(hWndTabControl, 1001);

			SetControlFont(hWndTabControl);

			RECT rect;
			GetClientRect(hWndTabControl, &rect);
			TabCtrl_AdjustRect(hWndTabControl, TRUE, &rect);

			WindowConfig clientConfig;
			Windows::SetChildWindowConfig(clientConfig, FromRECT(rect), hWndTabControl, "", WS_CHILD | WS_VISIBLE, 0 /* WS_EX_CLIENTEDGE */);
			clientSpace = Windows::CreateChildWindow(clientConfig, &clientSpaceHandler);

			enum {SUBCLASS_ID = 57900};

			SetWindowSubclass(hWndTabControl, TabSubclassMsgHandler, SUBCLASS_ID, (DWORD_PTR) this);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static TabControlSupervisor* Create(const WindowConfig& config, IWindow& parent, ITabControlEvents& eventHandler)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			TabControlSupervisor* p = new TabControlSupervisor(eventHandler);
			p->Construct(config, parent);
			return p;
		}

		~TabControlSupervisor()
		{
			Rococo::Free(clientSpace);
			DestroyWindow(hWnd);
		}

		virtual IParentWindowSupervisor& ClientSpace()
		{
			return *clientSpace;
		}

		ColourScheme scheme;

		void SetColourSchemeRecursive(const ColourScheme& scheme) override
		{
			this->scheme = scheme;
			clientSpaceHandler.SetBackgroundColour(ToCOLORREF(scheme.backColour));
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return hWnd;
		}

		virtual HWND TabHandle() const
		{
			return hWndTabControl;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}
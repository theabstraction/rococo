namespace Rococo::Windows
{
	class SuperListBox : public IWindowSupervisor, IWindowHandler
	{
		HWND hWnd = nullptr;

		SuperListBox()
		{

		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_DRAWITEM:
			{
				auto* d = (DRAWITEMSTRUCT*)lParam;
				break;
			}
			break;
			case WM_RBUTTONUP:
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);
				break;
			}
			case WM_SIZE:
			{
				return OnSize(hWnd, wParam, lParam);
			}
			case WM_NOTIFY:
			{
				NMHDR* header = (NMHDR*)lParam;
				break;
			}
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
				break;
			}

			return 0L;
		}

		void SyncSize()
		{
		}

		void Construct(const WindowConfig& childConfig, IWindow& parent)
		{
			WindowConfig c = childConfig;
			c.style = WS_POPUP | WS_BORDER;
			c.exStyle = 0;
			c.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, c, static_cast<IWindowHandler*>(this));
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static SuperListBox* Create(const WindowConfig& childConfig, IWindow& parent)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			SuperListBox* p = new SuperListBox();
			p->Construct(childConfig, parent);
			return p;
		}

		~SuperListBox()
		{
			DestroyWindow(hWnd);
		}

		IWindowHandler& Handler() override
		{
			return *this;
		}

		operator HWND () const override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}
	};

	class SuperComboBox : public IWindowSupervisor, private IWindowHandler
	{
	private:
		HWND hWnd = nullptr;
		HWND hWndEditControl = nullptr;
		HWND hWndLeftListToggle = nullptr;
		HWND hWndRightListToggle = nullptr;
		AutoFree<SuperListBox> listBox;

		SuperComboBox()
		{
		}

		void OnToggleClick(HWND hSender)
		{
			RECT comboRect;
			GetClientRect(hWnd, &comboRect);

			HWND hWndListEditorPopup = *listBox;

			RECT popupRect;
			GetClientRect(hWndListEditorPopup, &popupRect);

			POINT absOrigin{ 0,0 };
			ClientToScreen(hSender, &absOrigin);

			RECT buttonRect;
			GetClientRect(hSender, &buttonRect);

			RECT target;

			if (popupRect.right <= comboRect.right)
			{
				// We have enough space to put the list box underneath the combo line
				if (hSender == hWndLeftListToggle)
				{
					// We want to display the drop down aligned with the left button
					target.left = absOrigin.x;
				}
				else
				{
					// We want to display the drop down aligned with the right button
					target.left = absOrigin.x - popupRect.right + buttonRect.right;
				}
			}
			else
			{
				if (hSender == hWndLeftListToggle)
				{
					// We want to display the drop down to the left of the left button
					target.left = absOrigin.x - popupRect.right;
				}
				else
				{
					// We want to display the drop down to the right
					target.left = absOrigin.x + buttonRect.right;
				}
			}

			// And below the right button
			target.top = absOrigin.y + buttonRect.bottom;

			target.right = target.left + (popupRect.right - popupRect.left);
			target.bottom = target.top + (popupRect.bottom - popupRect.top);

			MoveWindow(hWndListEditorPopup, target.left, target.top, target.right - target.left, target.bottom - target.top, FALSE);

			ShowWindow(hWndListEditorPopup, IsWindowVisible(hWndListEditorPopup) ? SW_HIDE : SW_SHOW);
		}

		LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			auto id = LOWORD(wParam);
			auto command = HIWORD(wParam);

			HWND hSender = (HWND)lParam;

			switch (command)
			{
			case BN_CLICKED:
				OnToggleClick(hSender);
				return 0L;
			}

			return DefWindowProc(hWnd, WM_COMMAND, wParam, lParam);
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_DRAWITEM:
				{
					auto* d = (DRAWITEMSTRUCT*)lParam;
					break;
				}
				break;
			case WM_RBUTTONUP:
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);
				break;
			}
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_NOTIFY:
			{
				NMHDR* header = (NMHDR*)lParam;
				break;
			}
			case WM_COMMAND:
				return OnCommand(hWnd, wParam, lParam);
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
				break;
			}

			return 0L;
		}

		void SyncSize()
		{
		}

		void Construct(const WindowConfig& childConfig, IWindow& parent)
		{
			WindowConfig c = childConfig;
			c.style = WS_CHILD | WS_VISIBLE;
			c.exStyle = 0;
			c.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, c, static_cast<IWindowHandler*>(this));

			RECT r;
			GetClientRect(hWnd, &r);

			int height = r.bottom - r.top;
			int width = r.right - r.left;

			int buttonWidth = 0;

			int editorOffset = 0;

			if (width > 2 * height)
			{
				// we have room for the toggle button on the right hand side
				buttonWidth = height;

				if (width > 4 * height)
				{
					// and we also have room for the toggle button on the left hand side
					editorOffset = height;
				}
			}

			DWORD editorExStyle = 0;
			hWndEditControl = CreateWindowExA(editorExStyle, "EDIT", "<todo>", WS_CHILD | WS_VISIBLE | ES_READONLY, editorOffset, 0, width - buttonWidth - editorOffset, height, hWnd, NULL, hThisInstance, NULL);
			SetWindowTextA(hWndEditControl, childConfig.windowName);

			if (buttonWidth > 0)
			{
				DWORD buttonStyle = WS_CHILD | WS_VISIBLE | BS_FLAT;

				hWndRightListToggle = CreateWindowExA(editorExStyle, "BUTTON", "*", buttonStyle, r.right - r.left - buttonWidth, 0, buttonWidth, height, hWnd, NULL, hThisInstance, NULL);

				if (editorOffset > 0)
				{
					hWndLeftListToggle = CreateWindowExA(editorExStyle, "BUTTON", "*", buttonStyle, 0, 0, buttonWidth, height, hWnd, NULL, hThisInstance, NULL);
				}
			}

			WindowConfig listConfig = { 0 };
			listConfig.hWndParent = hWnd;
			listConfig.width = 200;
			listConfig.height = 320;
			listBox = SuperListBox::Create(listConfig, *this);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static SuperComboBox* Create(const WindowConfig& childConfig, IWindow& parent)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			SuperComboBox* p = new SuperComboBox();
			p->Construct(childConfig, parent);
			return p;
		}

		~SuperComboBox()
		{
			DestroyWindow(hWnd);
		}

		IWindowHandler& Handler() override
		{
			return *this;
		}

		operator HWND () const override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}
	};
}
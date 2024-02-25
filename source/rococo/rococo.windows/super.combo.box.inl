#include <rococo.editors.h>

namespace Rococo::Windows
{
	class SuperListBox : public IWindowSupervisor, IWindowHandler, public ISuperListBuilder
	{
		HWND hWnd = nullptr;
		HWND hWndList = nullptr;
		ISuperListSpec& spec;

		SuperListBox(ISuperListSpec& _spec): spec(_spec)
		{
		}

		void AddColumn(cstr name, int pixelWidth) override
		{
			LV_COLUMNA item;
			item = { 0 };
			item.mask = LVCF_TEXT | LVCF_WIDTH;
			item.pszText = (char*)name;
			item.cx = pixelWidth;
			if (-1 == ListView_InsertColumn(hWndList, 10000, &item))
			{
				Throw(0, "%s: Error inserting item %s into a ListView header", __FUNCTION__, name);
			}
		}

		void AddColumnWithMaxWidth(cstr name) override
		{
			RECT rect;
			GetClientRect(hWndList, &rect);

			LV_COLUMNA item;
			item = { 0 };
			item.mask = LVCF_TEXT | LVCF_WIDTH;
			item.pszText = (char*)name;
			item.cx = rect.right;
			if (-1 == ListView_InsertColumn(hWndList, 10000, &item))
			{
				Throw(0, "%s: Error inserting item %s into a ListView header", __FUNCTION__, name);
			}
		}

		void AddKeyValue(cstr key, cstr value) override
		{
			UNUSED(key);
			LV_ITEM item = { 0 };
			item.mask = LVIF_TEXT;
			item.pszText = (char*)value;
			item.iItem = 0x7FFFFFFF;
			item.cchTextMax = 256;
			ListView_InsertItem(hWndList, &item);
		}

		int GetFirstSelection() const
		{
			for (int i = 0; i < ListView_GetItemCount(hWndList); ++i)
			{
				if (LVIS_SELECTED == ListView_GetItemState(hWndList, i, LVIS_SELECTED))
				{
					return i;
				}
			}

			return -1;
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
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
				return OnNotify(hWnd, wParam, lParam);
			}
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnNotify(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT_PTR id = wParam;
			UNUSED(id);
			auto* header = (NMHDR*)lParam;
			if (header->code == LVN_ITEMCHANGED)
			{
				auto& n = *(LPNMLISTVIEW)header;
				UNUSED(n);
				return TRUE;
			}
			else if (header->code == NM_RETURN)
			{
				int iPos = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
				if (iPos >= 0)
				{
					spec.EventHandler().OnReturnAtSelection(iPos);
					return 0L;
				}
			}
			else if (header->code == NM_DBLCLK)
			{
				NMITEMACTIVATE* a = (NMITEMACTIVATE*)(header);
				if (a->iItem > 0)
				{
					spec.EventHandler().OnDoubleClickAtSelection(a->iItem);
					return 0L;
				}
			}
			return DefWindowProc(hWnd, WM_NOTIFY, wParam, lParam);
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
				break;
			}

			return 0L;
		}

		void SyncSize()
		{
		}

		enum { DLG_CTRL_LIST_ID = 1001 };

		void Construct(const WindowConfig& childConfig, IWindow& parent)
		{
			WindowConfig c = childConfig;
			c.style = WS_POPUP | WS_BORDER;
			c.exStyle = 0;
			c.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, c, static_cast<IWindowHandler*>(this));

			RECT rect;
			GetClientRect(hWnd, &rect);

			
			WindowConfig listConfig = { 0 };
			
			listConfig.left = 0;
			listConfig.top = 0;
			listConfig.width = rect.right;
			listConfig.height = rect.bottom;
			listConfig.hWndParent = hWnd;
			listConfig.style |= WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL;

			hWndList = CreateWindowIndirect(WC_LISTVIEWA, listConfig, nullptr);
			SetDlgCtrlID(hWndList, DLG_CTRL_LIST_ID);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static SuperListBox* Create(ISuperListSpec& spec, const WindowConfig& childConfig, IWindow& parent)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			SuperListBox* p = new SuperListBox(spec);
			p->Construct(childConfig, parent);
			return p;
		}

		~SuperListBox()
		{
			DestroyWindow(hWnd);
		}

		void AdvanceSelection(int delta)
		{
			int i = GetFirstSelection();

			if (i < 0 || i >= ListView_GetItemCount(hWndList))
			{
				return;
			}

			i += delta;

			if (i < 0)
			{
				i = ListView_GetItemCount(hWndList) - 1;
			}
			else if (i >= ListView_GetItemCount(hWndList))
			{
				i = 0;
			}

			ListView_SetItemState(hWndList, i, LVIS_SELECTED, LVIS_SELECTED);
		}

		void Select(cstr key) override
		{
			LVFINDINFOA f = { 0 };
			f.flags = LVFI_STRING;
			f.psz = key;

			int pos = ListView_FindItem(hWndList, 0, &f);

			if (pos > 0)
			{
				ListView_SetItemState(hWndList, pos, LVIS_SELECTED, LVIS_SELECTED);
			}
		}

		int GetSelection(char* key, size_t sizeofKeyBuffer)
		{
			int pos = GetFirstSelection();

			if (pos < 0)
			{
				return pos;
			}

			ListView_GetItemText(hWndList, pos, 0, key, (int) sizeofKeyBuffer);
			return pos;
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

	enum { SUPER_EDITOR_CLASS_ID = 15007 };

	static LRESULT SuperEditorProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		using namespace Rococo::Editors;

		UNUSED(dwRefData);

		if (uIdSubclass == SUPER_EDITOR_CLASS_ID)
		{
			switch (uMsg)
			{
			case WM_KEYUP:
				if (wParam == VK_SPACE)
				{
					PostMessage(GetParent(hWnd), WM_POPUP_COMBO_LIST, 0, 0);
					return 0L;
				}
				if (wParam == VK_DOWN)
				{
					PostMessage(GetParent(hWnd), WM_ADVANCE_COMBO_LIST, 1, 0);
				}
				if (wParam == VK_UP)
				{
					PostMessage(GetParent(hWnd), WM_ADVANCE_COMBO_LIST, (WPARAM) - 1, 0);
				}
				if (wParam == VK_RETURN)
				{
					PostMessage(GetParent(hWnd), WM_USE_COMBO_LIST_OPTION, 0, 0);
				}
				return 0L;
			}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	class SuperComboBox : public IWin32SuperComboBox, private IWindowHandler, ISuperComboBuilder
	{
	private:
		HWND hWnd = nullptr;
		HWND hWndEditControl = nullptr;
		HWND hWndLeftListToggle = nullptr;
		HWND hWndRightListToggle = nullptr;
		AutoFree<SuperListBox> listBox;
		ISuperListSpec& spec;

		HBRUSH hFocusBrush;
		COLORREF focusColour;

		SuperComboBox(ISuperListSpec& _spec): spec(_spec)
		{
			focusColour = RGB(255, 240, 240);
			hFocusBrush = CreateSolidBrush(focusColour);
		}

		ISuperListBuilder& ListBuilder() override
		{
			return *listBox;
		}

		ISuperComboBuilder& ComboBuilder() override
		{
			return *this;
		}

		void HidePopup() override
		{
			ShowWindow(*listBox, SW_HIDE);
			SetFocus(hWndEditControl);
		}

		enum { MAX_KEY_LEN = 1024 };

		void SetSelection(cstr keyName) override
		{
			if (keyName == nullptr || *keyName == 0)
			{
				Throw(0, "%s: blank key", __FUNCTION__);
			}

			if (strlen(keyName) >= MAX_KEY_LEN)
			{
				Throw(0, "%s: bad key length. Max is %u characters", MAX_KEY_LEN - 1, __FUNCTION__);
			}
			SetWindowText(hWndEditControl, keyName);
			ListBuilder().Select(keyName);
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
					target.left = absOrigin.x + buttonRect.right - popupRect.right;
				}
			}

			// And below the right button
			target.top = absOrigin.y + buttonRect.bottom;

			target.right = target.left + (popupRect.right - popupRect.left);
			target.bottom = target.top + (popupRect.bottom - popupRect.top);

			MoveWindow(hWndListEditorPopup, target.left, target.top, target.right - target.left, target.bottom - target.top, FALSE);

			ShowWindow(hWndListEditorPopup, IsWindowVisible(hWndListEditorPopup) ? SW_HIDE : SW_SHOW);

			if (IsWindowVisible(hWndListEditorPopup))
			{
				char key[MAX_KEY_LEN];
				int len = GetWindowTextA(hWndEditControl, key, sizeof key);
				if (len > 0 && len < MAX_KEY_LEN)
				{
					listBox->Select(key);
				}
			}

			SetFocus(hWndEditControl);
		}

		LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			auto id = LOWORD(wParam);
			auto command = HIWORD(wParam);

			UNUSED(id);

			HWND hSender = (HWND)lParam;

			switch (command)
			{
			case BN_CLICKED:
				OnToggleClick(hSender);
				return 0L;
			case EN_KILLFOCUS:
				if (!IsWindowVisible(*listBox))
				{
					DWORD wParamParent = (DWORD)MAKELONG(GetWindowLongPtrA(hWnd, GWLP_ID), EN_KILLFOCUS);
					PostMessage(GetParent(hWnd), WM_COMMAND, wParamParent, (LPARAM) hWnd);
				}
				return 0L;
			}

			return DefWindowProc(hWnd, WM_COMMAND, wParam, lParam);
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			using namespace Rococo::Editors;

			switch (uMsg)
			{
			case WM_CTLCOLORSTATIC:
			{
				HWND hEditor = (HWND)lParam;

				HDC hdc = (HDC)wParam;

				if (GetFocus() == hEditor)
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, focusColour);
					return (LRESULT)hFocusBrush;
				}
				else
				{
					SetTextColor(hdc, RGB(0, 0, 0));
					SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
					return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
				}
			}
			case WM_RBUTTONUP:
			{
				POINT cursorPos;
				GetCursorPos(&cursorPos);
				break;
			}
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_COMMAND:
				return OnCommand(hWnd, wParam, lParam);
			case WM_POPUP_COMBO_LIST:
				if (hWndLeftListToggle)
				{
					OnToggleClick(hWndLeftListToggle);
				}
				else if (hWndRightListToggle)
				{
					OnToggleClick(hWndRightListToggle);
				}
				return 0L;
			case WM_ADVANCE_COMBO_LIST:
				if (IsWindowVisible(*listBox))
				{
					listBox->AdvanceSelection((int)wParam);
				}
				else
				{
					PostMessage(GetParent(hWnd), WM_ADVANCE_COMBO_LIST, GetWindowLongPtrA(*listBox, GWLP_ID), wParam);
				}
				return 0L;
			case WM_USE_COMBO_LIST_OPTION:
				if (IsWindowVisible(*listBox))
				{
					char key[MAX_KEY_LEN];
					int pos = listBox->GetSelection(key, MAX_KEY_LEN);
					if (pos > 0)
					{
						SetWindowTextA(hWndEditControl, key);
					}

					ShowWindow(*listBox, SW_HIDE);
					SetFocus(hWndEditControl);
				}
				else
				{
					PostMessage(GetParent(hWnd), WM_ADVANCE_COMBO_LIST, GetWindowLongPtrA(*listBox, GWLP_ID), 1);
				}
				return 0L;
			case WM_SETFOCUS:
				SetFocus(hWndEditControl);
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
				break;
			}

			Reconstruct();

			return 0L;
		}

		void Reconstruct()
		{
			if (hWndLeftListToggle)
			{
				DestroyWindow(hWndLeftListToggle);
				hWndLeftListToggle = nullptr;
			}

			if (hWndRightListToggle)
			{
				DestroyWindow(hWndRightListToggle);
				hWndRightListToggle = nullptr;
			}

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

			MoveWindow(hWndEditControl, editorOffset, 0, width - buttonWidth - editorOffset, height, TRUE);
			
			if (buttonWidth > 0)
			{
				DWORD buttonStyle = WS_CHILD | WS_VISIBLE | BS_FLAT;

				DWORD toggleExStyle = 0;

				hWndRightListToggle = CreateWindowExA(toggleExStyle, "BUTTON", "*", buttonStyle, r.right - r.left - buttonWidth, 0, buttonWidth, height, hWnd, NULL, hThisInstance, NULL);

				if (editorOffset > 0)
				{
					hWndLeftListToggle = CreateWindowExA(toggleExStyle, "BUTTON", "*", buttonStyle, 0, 0, buttonWidth, height, hWnd, NULL, hThisInstance, NULL);
				}
			}
		}

		void Construct(const WindowConfig& childConfig, IWindow& parent, ControlId id)
		{
			WindowConfig c = childConfig;
			c.style = WS_CHILD | WS_VISIBLE;
			c.exStyle = 0;
			c.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, c, static_cast<IWindowHandler*>(this));

			DWORD editorExStyle = 0;
			hWndEditControl = CreateWindowExA(editorExStyle, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_READONLY, 0, 0, 0, 0, hWnd, (HMENU) id , hThisInstance, NULL);
			SetWindowTextA(hWndEditControl, childConfig.windowName);
			SetWindowSubclass(hWndEditControl, SuperEditorProc, SUPER_EDITOR_CLASS_ID, 0);

			Reconstruct();

			WindowConfig listConfig = { 0 };
			listConfig.hWndParent = hWnd;
			listConfig.width = 200;
			listConfig.height = 320;
			listBox = SuperListBox::Create(spec, listConfig, *this);
		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static SuperComboBox* Create(ISuperListSpec& spec, const WindowConfig& childConfig, IWindow& parent, ControlId id)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			SuperComboBox* p = new SuperComboBox(spec);
			p->Construct(childConfig, parent, id);
			return p;
		}

		~SuperComboBox()
		{
			DestroyWindow(hWnd);
			DeleteObject(hFocusBrush);
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
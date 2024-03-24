namespace Rococo::Windows
{
	class ComboBoxSupervisor : public IComboBoxSupervisor, private IWindowHandler
	{
	private:
		HWND hWnd;
		HWND hWndComboBox;
		IItemRenderer* itemRenderer;

		ComboBoxSupervisor(IItemRenderer* _itemRenderer) :
			hWnd(nullptr),
			hWndComboBox(nullptr),
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
				MoveWindow(hWndComboBox, 0, 0, width, height, TRUE);
				break;
			}

			return 0L;
		}

		LRESULT OnDrawItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			if (!itemRenderer) return DefWindowProc(hWnd, WM_DRAWITEM, wParam, lParam);

			DRAWITEMSTRUCT& dis = *LPDRAWITEMSTRUCT(lParam);
			itemRenderer->OnDrawItem(dis);
			return TRUE;
		}

		LRESULT OnMeasureItem(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			if (!itemRenderer) return DefWindowProc(hWnd, WM_MEASUREITEM, wParam, lParam);

			MEASUREITEMSTRUCT& mis = *LPMEASUREITEMSTRUCT(lParam);
			itemRenderer->OnMeasureItem(hWndComboBox, mis);
			return TRUE;
		}

		void Construct(const WindowConfig& listConfig, IParentWindowSupervisor& parent, IItemRenderer* itemRenderer, DWORD containerStyle, DWORD containerStyleEx)
		{
			UNUSED(containerStyleEx);
			UNUSED(containerStyle);
			UNUSED(itemRenderer);

			WindowConfig containerConfig = listConfig;
			containerConfig.style = WS_CHILD | WS_VISIBLE;
			containerConfig.exStyle = 0;
			containerConfig.hWndParent = parent;
			hWnd = CreateWindowIndirect(customClassName, containerConfig, static_cast<IWindowHandler*>(this));

			WindowConfig configCorrected = listConfig;
			configCorrected.left = 0;
			configCorrected.top = 0;
			configCorrected.hWndParent = hWnd;
			configCorrected.style |= WS_CHILD | WS_VISIBLE;

			hWndComboBox = CreateWindowIndirect("COMBOBOX", configCorrected, nullptr);
		}

		void OnModal() override
		{

		}

		void OnPretranslateMessage(MSG&) override
		{

		}
	public:
		static ComboBoxSupervisor* Create(const WindowConfig& listConfig, IParentWindowSupervisor& parent, IItemRenderer* itemRenderer, DWORD containerStyle, DWORD containerStyleEx)
		{
			if (customAtom == 0)
			{
				customAtom = CreateCustomAtom();
			}

			ComboBoxSupervisor* p = new ComboBoxSupervisor(itemRenderer);
			p->Construct(listConfig, parent, itemRenderer, containerStyle, containerStyleEx);
			return p;
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

		int AddString(cstr text) override
		{
			return ComboBox_AddString(hWndComboBox, text);
		}

		int FindString(cstr text) override
		{
			return ComboBox_FindString(hWndComboBox, 0, text);
		}

		int GetCurrentSelection() override
		{
			return ComboBox_GetCurSel(hWndComboBox);
		}

		void SetCurrentSelection(int index) override
		{
			ComboBox_SetCurSel(hWndComboBox, index);
		}

		bool GetString(int index, char* buffer, size_t capacity) override
		{
			int length = ComboBox_GetLBTextLen(hWndComboBox, index);
			if (length == CB_ERR) return false;

			char* stackbuffer = (char*)alloca(2 * max(capacity, (size_t)length + 1));
			if (!ComboBox_GetLBText(hWndComboBox, index, stackbuffer))
			{
				return false;
			}

			StackStringBuilder sb(buffer, capacity);
			sb << stackbuffer;

			return true;
		}
	};
}
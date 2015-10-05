namespace
{
	class Menu : public IWin32Menu
	{
	private:
		Menu(Menu& src) = delete;
		Menu& operator = (Menu& src) = delete;

		HMENU hMenu;

		typedef std::vector<Menu*> TChildren;
		TChildren children;
	public:
		Menu();
		virtual ~Menu();
		operator HMENU () { return hMenu; }
		Menu& AddPopup(LPCWSTR name);
		void AddString(LPCWSTR name, UINT_PTR id, LPCWSTR keyCommand);
		void Free() { delete this; }
	};

	Menu::Menu() : hMenu(::CreateMenu())
	{
	}

	Menu::~Menu()
	{
		DestroyMenu(hMenu);

		for (auto child : children)
		{
			delete child;
		}
	}

	Menu& Menu::AddPopup(LPCWSTR name)
	{
		UINT index = 0;
		for (auto m : children)
		{
			MENUITEMINFO mi = { 0 };
			mi.cbSize = sizeof(mi);			
			mi.fMask = MIIM_STRING | MIIM_SUBMENU;
			mi.cch = 256;

			wchar_t text[256];
			*(int*)text = 0;
			mi.dwTypeData = text;

			GetMenuItemInfo(hMenu, index, TRUE, &mi);
			if (wcscmp(text, name) == 0 && mi.hSubMenu != nullptr)
			{
				return *m;
			}

			index++;
		}

		Menu* child = new Menu();
		children.push_back(child);
		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)(HMENU)*child, name);
		return *child;
	}

	void Menu::AddString(LPCWSTR name, UINT_PTR id, LPCWSTR keyCommand)
	{
		wchar_t text[64];

		if (keyCommand)
		{
			SecureFormat(text, L"%s\t%s", name, keyCommand);
		}
		else
		{
			SecureFormat(text, L"%s", name);
		}

		AppendMenu(hMenu, MF_STRING, id, text);
	}
}
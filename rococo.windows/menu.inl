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
		Menu(bool contextMenu);
		virtual ~Menu();
		operator HMENU () { return hMenu; }
		Menu& AddPopup(cstr name);
		void AddString(cstr name, UINT_PTR id, cstr keyCommand);
		void Free() { delete this; }
	};

	Menu::Menu(bool contextMenu) : hMenu(contextMenu ? ::CreatePopupMenu() : ::CreateMenu())
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

	Menu& Menu::AddPopup(cstr name)
	{
		UINT index = 0;
		for (auto m : children)
		{
			MENUITEMINFOA mi = { 0 };
			mi.cbSize = sizeof(mi);			
			mi.fMask = MIIM_STRING | MIIM_SUBMENU;
			mi.cch = 256;

			char text[256];
			*(int*)text = 0;
			mi.dwTypeData = text;

			GetMenuItemInfoA(hMenu, index, TRUE, &mi);
			if (strcmp(text, name) == 0 && mi.hSubMenu != nullptr)
			{
				return *m;
			}

			index++;
		}

		Menu* child = new Menu(false);
		children.push_back(child);
		AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)(HMENU)*child, name);
		return *child;
	}

	void Menu::AddString(cstr name, UINT_PTR id, cstr keyCommand)
	{
		char text[64];

		if (keyCommand)
		{
			SecureFormat(text, sizeof(text), "%s\t%s", name, keyCommand);
		}
		else
		{
			SecureFormat(text, sizeof(text), "%s", name);
		}

		AppendMenuA(hMenu, MF_STRING, id, text);
	}
}
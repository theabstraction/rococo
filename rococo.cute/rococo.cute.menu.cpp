#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "rococo.cute.h"

#include "cute.sxh.h"

#include <unordered_map>
#include <vector>

using namespace Rococo;
using namespace Rococo::Cute;

namespace
{
	struct CuteMenu;
	void Update(CuteMenu* menu);

	uint32 nextId = 1000;
	std::unordered_map<std::string, uint32> keyToId;

	struct MenuNode : public IMenu
	{
		HMENU hMenu;
		CuteMenu* base;
		std::vector<MenuNode*> children;

		MenuNode(HMENU _hMenu) : hMenu(_hMenu) {}

		~MenuNode()
		{
			for (auto i : children)
			{
				delete i;
			}
			DestroyMenu(hMenu);
		}

		void AddItem(const fstring& text, const fstring& key)
		{
			auto i = keyToId.find((cstr)key);

			uint32 id = 0;

			if (i != keyToId.end())
			{
				id = i->second;
			}
			else
			{
				auto id = nextId++;
				keyToId[(cstr)key] = id;
			}

			AppendMenuA(hMenu, MF_STRING, id, text);

			Update(base);
		}

		IMenu* SubMenu(const fstring& name) override
		{
			auto* child = new MenuNode(CreatePopupMenu());
			child->base = base;
			AppendMenuA(hMenu, MF_POPUP | MF_STRING, (UINT_PTR)child->hMenu, name);
			children.push_back(child);
			Update(base);
			return child;
		}
	};

	struct CuteMenu : IMenuSupervisor
	{
		HWND hWnd;
		MenuNode* root;

		void LazyInit()
		{
			if (root == nullptr)
			{
				root = new MenuNode(CreateMenu());
				root->base = this;
			}
		}

		CuteMenu(HWND _hWnd): hWnd(_hWnd), root(nullptr)
		{

		}

		~CuteMenu()
		{
			delete root;
		}

		void Free() override
		{
			delete this;
		}

		IMenu& Menu() override
		{
			LazyInit();
			return *root;
		}

		void AddItem(const fstring& text, const fstring& key)
		{
			return root->AddItem(text, key);
		}
	};

	void Update(CuteMenu* menu)
	{
		if (!SetMenu(menu->hWnd, menu->root->hMenu))
		{
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			Throw(hr, "CuteMenu::SetMenu failed");
		}
	}
}

namespace Rococo
{
	namespace Cute
	{
		IMenuSupervisor* CreateCuteMenu(HWND hWnd)
		{
			return new CuteMenu(hWnd);
		}
	}
}


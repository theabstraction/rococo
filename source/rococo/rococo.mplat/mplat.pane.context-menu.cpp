#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

#include <vector>
#include <memory>
#include <string>

typedef int32 ID_MENU_BRANCH;

struct MenuColours
{
	RGBAb fontLo { 224,224,224,255 };
	RGBAb fontHi { 255,255,255,255 };
	RGBAb back1Lo{ 0,0,0,255 };
	RGBAb back1Hi{ 0,0,64,255 };
	RGBAb back2Lo{ 0,0,0,255 };
	RGBAb back2Hi{ 0,0,128,255 };
};

struct MenuItem
{
	MenuColours colours;
	EventIdRef triggerEvent = ""_event;
	std::string text;
	char shortcutKey[16] = "";
	bool enabled = true;
};

struct MenuBranch
{
	ID_MENU_BRANCH id = 0;
	MenuItem item;
	bool isOpen = false;
	GuiRect lastRenderedRect{ 0,0,0,0 };
	MenuBranch* parent = nullptr;
	std::vector<MenuBranch> children;
};

MenuBranch* FindBranch(MenuBranch& menu, ID_MENU_BRANCH id)
{
	if (id == menu.id)
	{
		return &menu;
	}
	else
	{
		for (auto& child : menu.children)
		{
			MenuBranch* x = FindBranch(child, id);
			if (x) return x;
		}
	}

	return nullptr;
}

class ContextMenu: public IContextMenuSupervisor
{
	MenuBranch root;
	MenuColours colourTemplate;
	IPublisher& publisher;
	GuiRect menuRect = { 0,0,0,0 };
	int32 nextBranchId = 1;
	IContextMenuEvents& eventHandler;

	MenuBranch& GetBranch(ID_MENU_BRANCH id)
	{
		auto* x = FindBranch(root, id);
		if (!x) Throw(0, "ContextMenu::GetBranch with id #%d: No such branch", id);
		return *x;
	}

	void RenderChild(IGuiRenderContext& grc, MenuBranch& child, bool isLit, int32 shortCutPixels)
	{
		auto text = to_fstring(child.item.text.c_str());

		auto& rect = child.lastRenderedRect;

		GuiRectf textRect
		{
			(float)rect.left + 2,
			(float)rect.top + 2,
			(float)rect.right - 2 - (float) shortCutPixels,
			(float)rect.bottom - 2
		};

		if (isLit)
		{
			Graphics::DrawRectangle(grc, rect, child.item.colours.back1Hi, child.item.colours.back2Hi);
		}
		else
		{
			Graphics::DrawRectangle(grc, rect, child.item.colours.back1Lo, child.item.colours.back2Lo);
		}

		RGBAb fontColour = isLit ? child.item.colours.fontHi : child.item.colours.fontLo;
		Graphics::DrawText(grc, textRect, Graphics::Alignment_Left, text, 0, fontColour);

		GuiRectf textRectShortcut
		{
			textRect.right,
			(float)rect.top + 2,
			(float)rect.right - 2,
			(float)rect.bottom - 2
		};

		auto shortcut = to_fstring(child.item.shortcutKey);
		Graphics::DrawText(grc, textRectShortcut, Graphics::Alignment_Left, shortcut, 0, fontColour);

		if (!child.children.empty())
		{
			// Item is a submenu
			int32 border = 4;
			int32 ds = rect.bottom - rect.top;
			GuiRect triRect{ rect.right - ds + border, rect.top + border, rect.right - border, rect.bottom - border };
			Graphics::DrawTriangleFacingRight(grc, triRect, fontColour);
		}
	}

	// Shift rectangle to a location where it will fit nicely on screen, or die trying
	void FitRect(IGuiRenderContext& grc, GuiRect& rect)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (rect.right > metrics.screenSpan.x)
		{
			// Too far right, so shift left
			
			int32 dx = rect.right - rect.left;
			rect.right = rect.left;
			rect.left = rect.right - dx;
		}

		if (rect.bottom > metrics.screenSpan.y)
		{
			// Shift up
			int32 dy = rect.bottom - rect.top;
			if (dy < metrics.screenSpan.y)
			{
				rect.bottom = metrics.screenSpan.y;
				rect.top = rect.bottom - dy;
			}
		}
	}

	void RenderBranch(IGuiRenderContext& grc, MenuBranch& branch, Vec2i screenPos)
	{
		int32 maxWidth = 0;
		for (auto child : branch.children)
		{
			maxWidth = max(maxWidth, (int32)child.item.text.size());
		}

		maxWidth += 8; // 8 chars is enough for 'Ctrl+A'

		enum { CELL_HEIGHT = 32, CELL_WIDTH = 16 };
		Vec2i span{ maxWidth * CELL_WIDTH + 4,  4 + CELL_HEIGHT * (int32)branch.children.size() };

		GuiRect menuRect{ screenPos.x, screenPos.y, screenPos.x + span.x, screenPos.y + span.y };

		if (branch.id == 0)
		{
			FitRect(grc, menuRect);
		}

		branch.lastRenderedRect = menuRect;

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		Vec2i pos{ menuRect.left, menuRect.top };

		for (auto& child : branch.children)
		{
			child.lastRenderedRect = { pos.x, pos.y, pos.x + span.x, pos.y + CELL_HEIGHT };
			pos.y += CELL_HEIGHT;
			int32 shortCutPixels = 6 * CELL_WIDTH;
			RenderChild(grc, child, IsPointInRect(metrics.cursorPosition, child.lastRenderedRect), shortCutPixels);
		}

		Graphics::DrawBorderAround(grc, menuRect, { 1,1 }, RGBAb(255, 255, 255, 128), RGBAb(255, 255, 255, 192));

		bool renderedOneSubMenu = false;

		for (auto& child : branch.children)
		{
			if (!child.children.empty() && IsPointInRect(metrics.cursorPosition, child.lastRenderedRect))
			{
				Vec2i delta3D{ -6, 6 };
				Vec2i childPos{ child.lastRenderedRect.right + delta3D.x, child.lastRenderedRect.top + delta3D.y };
				RenderBranch(grc, child, childPos);
				child.isOpen = true;
				renderedOneSubMenu = true;
				for (auto& sibling : branch.children)
				{
					if (&sibling != &child)
					{
						sibling.isOpen = false;
					}
				}
				break;
			}
		}

		if (!renderedOneSubMenu)
		{
			for (auto& child : branch.children)
			{
				if (child.isOpen)
				{
					Vec2i delta3D{ -6, 6 };
					Vec2i childPos{ child.lastRenderedRect.right + delta3D.x, child.lastRenderedRect.top + delta3D.y };
					RenderBranch(grc, child, childPos);
					break;
				}
			}
		}
	}

public:
	ContextMenu(IPublisher& _publisher, IContextMenuEvents& _eventHandler) :
		publisher(_publisher), eventHandler(_eventHandler) {}

	void Free() override
	{
		delete this;
	}

	void Render(IGuiRenderContext& gc) override
	{
		RenderBranch(gc, root, { menuRect.left, menuRect.top });
	}

	void AppendEvent(const KeyboardEvent&)
	{

	}

	void Trigger(const MenuBranch& b)
	{
		if (b.item.triggerEvent.name != nullptr && b.item.triggerEvent.name[0] != 0)
		{
			eventHandler.OnItemSelected(*this);

			TEventArgs<cstr> args;
			args.value = b.item.text.c_str();
			publisher.Publish(args, b.item.triggerEvent);
		}
	}

	bool TrySelectAt(Vec2i pos, MenuBranch& branch)
	{
		for (auto& child : branch.children)
		{
			if (child.isOpen)
			{
				if (TrySelectAt(pos, child))
				{
					return true;
				}
			}
		}

		for (auto& child : branch.children)
		{
			if (IsPointInRect(pos, child.lastRenderedRect))
			{
				Trigger(child);
				return true;
			}
		}

		return false;
	}

	void AppendEvent(const MouseEvent& me)
	{
		if (me.HasFlag(MouseEvent::LUp))
		{
			if (!TrySelectAt(me.cursorPos, root))
			{
				eventHandler.OnClickOutsideControls(*this);
			}
		}

		if (me.HasFlag(MouseEvent::RUp))
		{
			eventHandler.OnClickOutsideControls(*this);
		}
	}

	void AddString(int32 branchId, const fstring& displayName, const fstring& eventName, const fstring& shortcutKey) override
	{
		auto& m = GetBranch(branchId);
		MenuBranch newBranch;
		newBranch.id = nextBranchId++;
		newBranch.item.colours = colourTemplate;
		newBranch.item.text = displayName;
		CopyString(newBranch.item.shortcutKey, sizeof(MenuItem::shortcutKey), shortcutKey);
		newBranch.item.triggerEvent = publisher.CreateEventIdFromVolatileString(eventName);
		newBranch.parent = &m;
		m.children.push_back(newBranch);
	}

	int32 AddSubMenu(const fstring& displayName, int32 parentBranchId) override
	{
		auto& m = GetBranch(parentBranchId);
		MenuBranch newBranch;
		newBranch.item.colours = colourTemplate;
		newBranch.id = nextBranchId++;
		newBranch.item.text = displayName;
		newBranch.parent = &m;
		m.children.push_back(newBranch);
		return newBranch.id;
	}

	void Clear(int32 branchId) override
	{
		auto& m = GetBranch(branchId);
		m.children.clear();
		m.item.text.clear();
		if (m.parent != nullptr)
		{
			for (auto i = m.parent->children.begin(); i != m.parent->children.end(); ++i)
			{
				auto& child = *i;
				if (&child == &m)
				{
					m.parent->children.erase(i);
					break;
				}
			}
		}

		if (branchId == 0)
		{
			// the root, so reset the branch id generator
			nextBranchId = 1;
		}
	}

	void SetNextBackColour(int32 normal, int32 hilight) override
	{
		colourTemplate.back1Hi = colourTemplate.back2Hi = RGBAb(hilight);
		colourTemplate.back1Lo = colourTemplate.back2Lo = RGBAb(normal);
	}

	void SetNextStringColour(int32 normal, int32 hilight) override
	{
		colourTemplate.fontHi = hilight;
		colourTemplate.fontLo = normal;
	}

	void SetPopupPoint(const Vec2i& pos) override
	{
		menuRect = { pos.x, pos.y, pos.x, pos.y };
	}
};

class ContextMenuPane : public BasePane, public GUI::IContextMenuPane, public IObserver
{
	IPublisher& publisher;
	IKeyboardSupervisor& keyboard;
	IContextMenuSupervisor& menu;
	GuiRect screenRect = { 0,0,0,0 };
	HString popupKey; 
	EventIdRef evCreatedPopup;

	void OnEvent(Event&) override
	{
	}
public:
	ContextMenuPane(IPublisher& _publisher, IKeyboardSupervisor& _keyboard, const fstring& _popupKey, IContextMenuSupervisor& _menu) :
		publisher(_publisher), keyboard(_keyboard), popupKey(_popupKey), menu(_menu),
		evCreatedPopup(publisher.CreateEventIdFromVolatileString(_popupKey))
	{
	}

	~ContextMenuPane()
	{
	}

	void NoOperation() override
	{
	}

	void Free() override
	{
		delete this;
	}

	bool AppendEvent(const KeyboardEvent& k, const Vec2i&, const Vec2i&) override
	{
		menu.AppendEvent(k);
		return true;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i&) override
	{
		menu.AppendEvent(me);
	}

	void Render(IGuiRenderContext& grc, const Vec2i&, const Modality&) override
	{
		menu.Render(grc);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::GUI::IContextMenuPane* AddContextMenuPane(IPublisher& publisher, IKeyboardSupervisor& keyboard, BasePane& parent, const fstring& key, const GuiRect& rect, IContextMenuSupervisor& cm)
		{
			auto* menu = new ContextMenuPane(publisher, keyboard, key, cm);
			parent.AddChild(menu);
			menu->SetRect(rect);
			return menu;
		}

		IContextMenuSupervisor* CreateContextMenu(IPublisher& publisher, IContextMenuEvents& eventHandler)
		{
			return new ContextMenu(publisher, eventHandler);
		}
	}
}
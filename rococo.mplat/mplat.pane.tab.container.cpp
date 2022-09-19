#include <rococo.mplat.h>
#include "mplat.panel.base.h"
#include <rococo.strings.h>
#include <rococo.ui.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

struct Tab
{
	std::string caption;
	std::string panelText;
	int32 width;
	GuiRect lastRect{ 0,0,0,0 };
};

auto evPopulateTabs = "tabs.populate"_event;

class TabContainer : public BasePane, public ITabContainer, public IObserver
{
	int32 fontIndex;
	int32 tabHeight;
	IPublisher& publisher;
	IKeyboardSupervisor& keyboard;
	std::vector<Tab> tabs;
	size_t tabSelect = 0;
	std::string populatorName;

	void OnEvent(Event& ev) override
	{
		bool replaceTabs = false;

		if (evPopulateTabs == ev)
		{
			auto& p = As<PopulateTabsEvent>(ev);

			if (populatorName.empty() || p.populatorName == nullptr || !Eq(p.populatorName, populatorName.c_str()))
			{
				return;
			}

			if (tabs.size() == p.numberOfTabs)
			{
				for (size_t i = 0; i < p.numberOfTabs; ++i)
				{
					auto& t = p.tabArray[i];
					if (!Eq(t.name, tabs[i].caption.c_str()))
					{
						replaceTabs = true;
						break;
					}

					if (!Eq(t.populator, tabs[i].panelText.c_str()))
					{
						replaceTabs = true;
						break;
					}

					if (t.width != tabs[i].width)
					{
						replaceTabs = true;
						break;
					}
				}
			}
			else
			{
				replaceTabs = true;
			}

			if (replaceTabs)
			{
				tabSelect = 0;
				tabs.clear();
				for (size_t i = 0; i < p.numberOfTabs; ++i)
				{
					auto& t = p.tabArray[i];

					Tab tab;
					tab.caption = t.name;
					tab.panelText = t.populator;
					tab.width = t.width;
					tab.lastRect = { 0,0,0,0 };

					tabs.push_back(tab);
				}
			}
		}
	}
public:
	TabContainer(IPublisher& _publisher, IKeyboardSupervisor& _keyboard, int _tabHeight, int _fontIndex) :
		publisher(_publisher), keyboard(_keyboard), tabHeight(_tabHeight), fontIndex(_fontIndex)
	{
	}

	~TabContainer()
	{
		publisher.Unsubscribe(this);
	}

	void Free() override
	{
		delete this;
	}

	void AddTab(int32 width, const fstring& caption, const fstring& panelText) override
	{
		Tab tab{ (cstr) caption, (cstr) panelText, width };
		tabs.push_back(tab);
	}

	bool AppendEvent(const KeyboardEvent& k, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		Key key = keyboard.GetKeyFromEvent(k);
		if (key.isPressed)
		{
			bool consume = true;

			if (Eq(key.KeyName, "HOME"))
			{
				tabSelect = 0;
			}
			else if (Eq(key.KeyName, "END"))
			{
				tabSelect = tabs.size() - 1;
			}
			else if (Eq(key.KeyName, "LEFT"))
			{
				tabSelect--;
			}
			else if (Eq(key.KeyName, "RIGHT"))
			{
				tabSelect++;
				if (tabSelect >= tabs.size()) tabSelect = tabs.size() - 1;
			}
			else if (Eq(key.KeyName, "TAB"))
			{
				tabSelect++;
			}
			else
			{
				consume = false;
			}

			if (tabSelect >= tabs.size()) tabSelect = 0;

			if (consume) return true;
		}

		if (tabSelect < tabs.size())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnKeyboardEvent(k);
			}
		}

		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (tabs.empty()) return;

		if (me.HasFlag(MouseEvent::LUp))
		{
			for (size_t i = 0; i < tabs.size(); ++i)
			{
				if (IsPointInRect(me.cursorPos, tabs[i].lastRect))
				{
					tabSelect = i;
					return;
				}
			}

			if (IsPointInRect(me.cursorPos, leftButtonRect))
			{
				startIndex--;
				if (startIndex < 0) startIndex = 0;
				tabSelect--;
				if (tabSelect < 0) tabSelect = 0;
				return;
			}
			else if (IsPointInRect(me.cursorPos, rightButtonRect))
			{
				startIndex++;
				tabSelect++;

				if (tabSelect >= tabs.size())
				{
					tabSelect = tabs.size() - 1;
					if (tabSelect == -1) tabSelect = 0;
				}

				if (startIndex >= (int32)tabs.size())
				{
					startIndex = 0;
				}

				if (startIndex > tabSelect)
				{
					tabSelect = startIndex;
				}
				return;
			}
		}

		if (me.HasFlag(MouseEvent::LDown) || me.HasFlag(MouseEvent::LUp))
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnMouseLClick(me.cursorPos, me.HasFlag(MouseEvent::LDown));
			}
		}

		if (me.HasFlag(MouseEvent::RDown) || me.HasFlag(MouseEvent::RUp))
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnMouseRClick(me.cursorPos, me.HasFlag(MouseEvent::RDown));
			}
		}

		if (tabSelect < tabs.size())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate, evUIPopulate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnRawMouseEvent(me);
			}
		}
	}

	void RenderTabButton(IGuiRenderContext& grc, const Tab& tab, bool isHilighted, size_t index)
	{
		RGBAb fontColour;
		if (index == tabSelect)
		{
			fontColour = RGBAb(0, 0, 0, 255);

			GuiRect selectedTabRect = tab.lastRect;
			selectedTabRect.bottom += 4;
			Graphics::DrawRectangle(grc, selectedTabRect, RGBAb(192, 192, 255, 255), RGBAb(224, 224, 255, 255));
		}
		else
		{
			fontColour = RGBAb(255, 255, 255, 255);
			Graphics::DrawLine(grc, 1, TopRight(tab.lastRect), BottomRight(tab.lastRect), RGBAb(128, 128, 128, 255));
		}

		if (isHilighted)
		{
			fontColour.alpha = 255;
		}
		else
		{
			fontColour.alpha = 224;
		}

		GuiRectf rect = { (float)tab.lastRect.left + 4, (float)tab.lastRect.top + 2, (float)tab.lastRect.right - 2, (float)tab.lastRect.bottom - 2 };

		if (!tab.caption.empty())
		{
			Graphics::DrawText(grc, rect, Graphics::Alignment_Left, to_fstring(tab.caption.c_str()), 0, fontColour);
		}
	}

	int32 startIndex = 0; // The first tab index rendered on the tab control row
	int32 endIndex = 0; // The end iterator for iterating through tabs
	GuiRect leftButtonRect = { -1,-1,-1,-1 };
	GuiRect rightButtonRect = { -1,-1,-1,-1 };

	int32 DetermineFinalTabIndex(int32 startingFrom, int32 left, int32 right, int32 buttonWidth) const
	{
		int32 finalIndex = (int32)tabs.size();

		int32 x = left;

		for (int32 i = startingFrom; i < finalIndex; ++i)
		{
			auto& t = tabs[i];

			int32 rhs = x + t.width;
			if (rhs >= right)
			{
				// We don't have enough width to display all the tabs
				// Strip tabs until we have enough spaace to display tab buttons
				while (x + buttonWidth > right)
				{
					i--;

					if (i < startIndex)
					{
						return -1;
					}

					x -= tabs[i].width;
				}

				return i;
			}

			x = rhs;
		}

		return finalIndex;
	}

	void RenderControls(IGuiRenderContext& grc, const Vec2i& topLeft)
	{
		GuiRect rect;
		GetRect(rect);

		GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } + topLeft;

		int dy = tabHeight;

		GuiRect controlRect{ absRect.left + 1, topLeft.y + 1, absRect.right - 2, topLeft.y + tabHeight };
		Graphics::DrawRectangle(grc, controlRect, RGBAb(0, 0, 96, 255), RGBAb(0, 0, 128, 255));

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		int x = absRect.left;

		int32 availableWidth = Width(absRect);

		int32 tabButtonWidth = 20;

		if (tabSelect != -1 && (startIndex > (int32)tabSelect))
		{
			startIndex = (int32)tabSelect;
		}

		if (startIndex > 0 && startIndex < (int32)tabs.size())
		{
			leftButtonRect = GuiRect{ controlRect.left + 6, controlRect.top + 3, controlRect.left - 6 + tabButtonWidth, controlRect.bottom - 3 };
			x = controlRect.left + tabButtonWidth;
		}
		else
		{
			leftButtonRect = GuiRect{ -1,-1,-1,-1 };
			x = absRect.left;
		}

		endIndex = DetermineFinalTabIndex(startIndex, x, absRect.right, tabButtonWidth);

		if (endIndex < 0) return;

		if (tabSelect != -1 && tabSelect >= endIndex)
		{
			for (int32 i = 0; i < (int32)tabs.size(); ++i)
			{
				int jEnd = DetermineFinalTabIndex(i, x, absRect.right, tabButtonWidth);
				if (jEnd > tabSelect)
				{
					startIndex = i;
					endIndex = jEnd;
					break;
				}
			}
		}

		for (auto& t : tabs)
		{
			t.lastRect = { -1,-1,-1,-1 };
		}

		for (size_t i = startIndex; i < endIndex; ++i)
		{
			auto& t = tabs[i];

			t.lastRect = GuiRect{ x, absRect.top + 1, x + t.width,absRect.top + tabHeight - 2 };

			RenderTabButton(grc, t, IsPointInRect(metrics.cursorPosition, t.lastRect), i);

			x += Width(t.lastRect);
		}

		Graphics::DrawTriangleFacingLeft(grc, leftButtonRect, RGBAb(255, 255, 255));

		if (endIndex < tabs.size())
		{
			rightButtonRect = GuiRect{ absRect.right + 6 - tabButtonWidth, controlRect.top + 3, absRect.right - 6, controlRect.bottom - 3 };
			Graphics::DrawTriangleFacingRight(grc, rightButtonRect, RGBAb(255, 255, 255));
		}
		else
		{
			rightButtonRect = { -1,-1,-1,-1 };
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderControls(grc, topLeft);

		if (tabSelect < tabs.size())
		{
			RenderTab(grc, topLeft, tabs[tabSelect]);
		}
	}

	void RenderTabContent(IGuiRenderContext& grc, const GuiRect& contentRect, cstr panelKey)
	{
		UIPopulate populate;
		populate.renderElement = nullptr;
		populate.name = panelKey;
		publisher.Publish(populate, evUIPopulate);

		if (populate.renderElement)
		{
			populate.renderElement->Render(grc, contentRect);
		}
	}

	void RenderTab(IGuiRenderContext& grc, const Vec2i& topLeft, const Tab& tab)
	{
		GuiRect rect;
		GetRect(rect);

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } + topLeft;

		Vec2i nw{ absRect.left, tab.lastRect.bottom + 2 };
		Vec2i lmid{ tab.lastRect.left, nw.y };
		Vec2i ne{ absRect.right - 2, tab.lastRect.bottom + 2 };
		Vec2i rmid{ tab.lastRect.right - 2, nw.y };
		Vec2i sw{ nw.x, absRect.bottom - 3 };
		Vec2i se{ ne.x, absRect.bottom - 3 };

		GuiRect tabRect{ nw.x, nw.y, se.x, se.y };
		if (IsPointInRect(metrics.cursorPosition, tabRect) || IsPointInRect(metrics.cursorPosition, tab.lastRect))
		{
			Graphics::DrawLine(grc, 2, nw, lmid, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, rmid, ne, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, nw, sw, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, sw, se + Vec2i{ 2, 0 }, Scheme().hi_bottomRightEdge);
			Graphics::DrawLine(grc, 2, se + Vec2i{ 0, 2 }, ne - Vec2i{ 0, 0 }, Scheme().hi_bottomRightEdge);

			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), TopRight(tab.lastRect), Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), BottomLeft(tab.lastRect), Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, TopRight(tab.lastRect), BottomRight(tab.lastRect) + Vec2i{ 0,2 }, Scheme().hi_bottomRightEdge);
		}
		else
		{
			Graphics::DrawLine(grc, 2, nw, sw, Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, sw, se + Vec2i{ 2, 0 }, Scheme().bottomRightEdge);
			Graphics::DrawLine(grc, 2, se + Vec2i{ 0, 2 }, ne - Vec2i{ 0, 0 }, Scheme().bottomRightEdge);

			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), TopRight(tab.lastRect), Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), BottomLeft(tab.lastRect), Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, TopRight(tab.lastRect), BottomRight(tab.lastRect) + Vec2i{ 0,2 }, Scheme().bottomRightEdge);

			Graphics::DrawLine(grc, 2, nw, lmid, Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, rmid, ne, Scheme().topLeftEdge);
		}

		RenderTabContent(grc, GuiRect{ nw.x + 1, nw.y + 1, se.x, se.y }, tab.panelText.c_str());
	}

	void SetTabPopulator(const fstring& populatorName) override
	{
		if (!this->populatorName.empty())
		{
			Throw(0, "A tabbed panel can only have its populator set once");
		}

		this->populatorName = populatorName;
		publisher.Subscribe(this, evPopulateTabs);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		Rococo::ITabContainer* AddTabContainer(IPublisher& publisher, IKeyboardSupervisor& keyboard, BasePane& pane, int32 tabHeight, int32 fontIndex, const GuiRect& rect)
		{
			auto* tabs = new TabContainer(publisher, keyboard, tabHeight, fontIndex);
			pane.AddChild(tabs);
			tabs->SetRect(rect);
			return tabs;
		}
	}
}
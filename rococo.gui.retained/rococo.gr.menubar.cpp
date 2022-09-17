#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRMenuBar : IGRWidgetMenuBar
	{
		IGRPanel& panel;

		GRMenuBar(IGRPanel& owningPanel) : panel(owningPanel)
		{
			
		}

		void AddItem(const GRMenuItem& item) override
		{
			auto& button = CreateMenuButton(*this);
			button.Panel().Resize({ 1, panel.Span().y });
			button.SetTitle(item.text ? item.text : "");
			button.SetMetaData(item.metaData);
			button.SetEventPolicy(GREventPolicy::NotifyAncestors);
			button.SetClickCriterion(GRClickCriterion::OnDownThenUp);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			struct : IEventCallback<IGRPanel>
			{
				int32 index = 0;
				int32 lastX = 1;
				int32 height = 0;
				void OnEvent(IGRPanel& child) override
				{
					Vec2i minimalSpan = child.Widget().EvaluateMinimalSpan();
					Vec2i newSpan = { minimalSpan.x + 10, height };
					child.Resize(newSpan);
					child.SetParentOffset({ lastX, 0 });
					lastX += newSpan.x + 1;
					index++;
				}
			} cb;
			cb.height = panel.Span().y;
			panel.EnumerateChildren(&cb);
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			panel.Root().Custodian().OnGREvent(widgetEvent);
			return EventRouting::Terminate;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return Vec2i{ 100, 24 };
		}
	};

	struct GRMenuBarFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRMenuBar(panel);
		}
	} s_MenuBarFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& bar = static_cast<ANON::GRMenuBar&>(gr.AddWidget(parent.Panel(), ANON::s_MenuBarFactory));
		return bar;
	}
}
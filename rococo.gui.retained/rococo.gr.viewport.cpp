#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRViewportWidget : IGRWidgetViewport, IGRWidget
	{
		IGRPanel& panel;
		IGRWidgetDivision* clientArea = nullptr;
		IGRWidgetVerticalScroller* vscroller = nullptr;

		GRViewportWidget(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void PostConstruct()
		{
			if (!clientArea)
			{
				clientArea = &CreateDivision(*this);
			}

			if (!vscroller)
			{
				vscroller = &CreateVerticalScroller(*this);
			}
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			enum { scrollbarWidth = 16 };

			if (clientArea)
			{
				clientArea->Panel().Resize({ Width(panelDimensions) - scrollbarWidth, Height(panelDimensions)});
				clientArea->Panel().SetParentOffset({ 0,0 });
			}

			if (vscroller)
			{
				vscroller->Widget().Panel().Resize({ scrollbarWidth, Height(panelDimensions) - 1 });
				vscroller->Widget().Panel().SetParentOffset({ Width(panelDimensions) - scrollbarWidth, 0 });
			}

			vscroller->SetSliderPosition(0);
			vscroller->SetSliderHeight(128);
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		IGRWidgetVerticalScroller& VScroller() override
		{
			return *vscroller;
		}

		void Render(IGRRenderContext& g) override
		{
			DrawPanelBackground(panel, g);
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetViewport>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}
	};

	struct : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			auto* instance = new GRViewportWidget(panel);
			instance->PostConstruct();
			return *instance;
		}
	} s_ViewportWidgetFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetViewport::InterfaceId()
	{
		return "IGRViewportWidget";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetViewport& CreateViewportWidget(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetViewport>(gr.AddWidget(parent.Panel(), ANON::s_ViewportWidgetFactory));
		return *scroller;
	}
}
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRSlider : IGRWidgetSlider, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		
		GRSlider(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRSlider()
		{

		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (!IsPointInRect(ce.position, panel.AbsRect()) && panel.Root().CapturedPanelId() == panel.Id())
			{
				// The cursor has been moved outside the button, so capture should be lost
				panel.Root().ReleaseCursor();
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		double minValue = 0.0;
		double maxValue = 100.0;

		// If minValue > maxValue then the order is reversed. If they match are are not finite, the bar is disabled
		void SetRange(double minValue, double maxValue) override
		{
			this->minValue = minValue;
			this->maxValue = maxValue;
		}

		double quantum = 1.0;

		// Represents the number of units to increment the position when an extremum button is clicked, or the left/right keys are used 
		void SetQuantum(double clickDelta) override
		{
			quantum = clickDelta;
		}

		double position = 0;

		// Note that the true value is clamp of the supplied value using the range values
		void SetPosition(double value) override
		{
			position = value;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			bool isHovered = g.IsHovered(panel);

			GuiRect sliderSlot = panel.AbsRect();
			sliderSlot.left += 4;
			sliderSlot.right -= 4;
			
			int y = Centre(sliderSlot).y;
			sliderSlot.top = y - 4;
			sliderSlot.bottom = y + 4;

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_BACKGROUND, GRRenderState(false, isHovered, false), RGBAb(255,255,0,255));
			g.DrawRect(panel.AbsRect(), backColour);

			RGBAb sliderSlotColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, GRRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
			g.DrawRect(sliderSlot, sliderSlotColour);
		}

		GRAlignmentFlags alignment { 0 };
		Vec2i spacing { 0,0 };

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetSlider, GRSlider>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRSlider";
		}
	};

	struct GRSliderFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRSlider(panel);
		}
	} s_SliderFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetSlider::InterfaceId()
	{
		return "IGRWidgetSlider";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetSlider& CreateSlider(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), GRANON::s_SliderFactory);
		IGRWidgetSlider* slider = Cast<IGRWidgetSlider>(widget);
		return *slider;
	}
}
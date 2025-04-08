#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRSlider : IGRWidgetSlider, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		bool isRaised = true;
		Strings::HString raisedImagePath;
		Strings::HString pressedImagePath;
		IGRImage* raisedImage = nullptr;
		IGRImage* pressedImage = nullptr;
		GRAnchorPadding slotPadding{ 24, 24, 4, 4 };
		int sliderPos = 0;
		
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

		Vec2i EvaluateMinimalSpan() const
		{
			const IGRImage* image = isRaised ? raisedImage : pressedImage;
			if (image)
			{
				return image->Span() + Vec2i{ 2,2 };
			}

			return Vec2i{ 8, 8 };
		}

		void SyncMinimalSpan()
		{
			Vec2i minimalSpan = EvaluateMinimalSpan();
			panel.SetMinimalSpan(minimalSpan);
		}

		IGRWidgetSlider& SetImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			this->pressedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetSlider& SetPressedImagePath(cstr imagePath) override
		{
			this->pressedImagePath = imagePath ? imagePath : "";
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		IGRWidgetSlider& SetRaisedImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			SyncMinimalSpan();
			return *this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				isRaised = false;
				panel.CaptureCursor();
				if (!isRaised) UpdateSliderPos(ce.position);
			}
			else if (ce.click.LeftButtonUp)
			{
				isRaised = true;
				panel.Root().ReleaseCursor();
			}
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

		void UpdateSliderPos(Vec2i cursorPos)
		{
			sliderPos = clamp(cursorPos.x, slotPadding.left, Width(panel.AbsRect()) - slotPadding.right);
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (!isRaised) UpdateSliderPos(ce.position);
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
			sliderSlot.left += slotPadding.left;
			sliderSlot.right -= slotPadding.right;
			
			int y = Centre(sliderSlot).y;
			sliderSlot.top = y - slotPadding.top;
			sliderSlot.bottom = y + slotPadding.bottom;

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_BACKGROUND, GRRenderState(false, isHovered, false), RGBAb(255,255,0,255));
			g.DrawRect(panel.AbsRect(), backColour);

			RGBAb sliderSlotColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, GRRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
			g.DrawRect(sliderSlot, sliderSlotColour);

			IGRImage* image = isRaised ? raisedImage : pressedImage;

			GRRenderState rs(!isRaised, isHovered, false);

			if (image)
			{
				GRAlignmentFlags centred;
				centred.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

				Vec2i imageSpan = image->Span();

				sliderPos = clamp(sliderPos, slotPadding.left, Width(panel.AbsRect()) - slotPadding.right);

				int x = sliderPos - (imageSpan.x / 2);

				GuiRect targetRect;
				targetRect.left = x;
				targetRect.top = y - 200;
				targetRect.bottom = y + 200;
				targetRect.right = x + imageSpan.x;
				g.DrawImageUnstretched(*image, targetRect, panel.AbsRect(), centred);
			}
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
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <math.h>

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

		int lastSliderSpan = 0;
		
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
			int32 pixelRange = Width(panel.AbsRect()) - slotPadding.left - slotPadding.right;
			if (pixelRange <= 0)
			{
				return;
			}

			sliderPos = clamp(cursorPos.x, slotPadding.left, Width(panel.AbsRect()) - slotPadding.right);

			double quotient = (sliderPos - slotPadding.left) / (double)pixelRange;

			position = minValue + quotient * (maxValue - minValue);

			if (quantum > 0)
			{
				double intPosition = floor(position);

				double qPosition = intPosition;

				for(double q = intPosition; q < position; q += quantum)
				{
					qPosition = q;
				}

				position = qPosition;
			}
		}

		void SetSliderPosFromValuePos()
		{
			int32 pixelRange = Width(panel.AbsRect()) - slotPadding.left - slotPadding.right;
			if (pixelRange <= 0)
			{
				return;
			}

			position = clamp(position, minValue, maxValue);

			double range = maxValue - minValue;

			if (range == 0)
			{
				return;
			}

			double posRatio = (position - minValue) / (maxValue - minValue);

			double pixelPos = pixelRange * posRatio + slotPadding.left;
			sliderPos = (int)pixelPos;
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

		double quantum = 0.25;

		GRFontId guageFont = GRFontId::MENU_FONT;
		int guageDecimalPlaces = 2;
		int guageVerticalOffset = 4;
		EGRSchemeColourSurface guageTextSurface = EGRSchemeColourSurface::GAME_OPTION_TEXT;

		void SetGuage(GRFontId fontId, int decimalPlaces, int verticalOffset, EGRSchemeColourSurface surface)
		{
			guageFont = fontId;
			guageDecimalPlaces = decimalPlaces;
			guageVerticalOffset = verticalOffset;
			guageTextSurface = surface;
		}

		// Represents the number of units to increment the position when an extremum button is clicked, or the left/right keys are used 
		void SetQuantum(double quantum) override
		{
			this->quantum = quantum;
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

			if (lastSliderSpan != Width(panel.AbsRect()))
			{
				lastSliderSpan = Width(panel.AbsRect());
				SetSliderPosFromValuePos();
			}

			if (image)
			{
				GRAlignmentFlags centred;
				centred.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

				Vec2i imageSpan = image->Span();

				int renderedSliderPos = clamp(sliderPos, slotPadding.left, Width(panel.AbsRect()) - slotPadding.right);

				int x = renderedSliderPos - (imageSpan.x / 2);

				GuiRect targetRect;
				targetRect.left = x;
				targetRect.top = y - 200;
				targetRect.bottom = y + 200;
				targetRect.right = x + imageSpan.x;
				g.DrawImageUnstretched(*image, targetRect, panel.AbsRect(), centred);
			}

			if (guageFont != GRFontId::NONE)
			{
				char guageText[16];
				char format[16];

				Strings::SafeFormat(format, "%%.%df", guageDecimalPlaces);

				Strings::SafeFormat(guageText, format, position);

				GRAlignmentFlags textAlignment;
				textAlignment.Add(EGRAlignment::Top).Add(EGRAlignment::HCentre);

				RGBAb colour = panel.GetColour(guageTextSurface, rs);
				g.DrawText(GRFontId::MENU_FONT, panel.AbsRect(), textAlignment, { 0, guageVerticalOffset }, to_fstring(guageText), colour);
			}

			bool isObscured = panel.Parent()->HasFlag(EGRPanelFlags::HintObscure);
			if (isObscured)
			{
				g.DrawRect(panel.AbsRect(), RGBAb(64, 64, 64, 192));
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
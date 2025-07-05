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

		void* renderArgContext = nullptr;

		FN_RENDER_SLIDER fnRenderSlider = RenderSlider_Default;
		
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

		void SetSlotPadding(GRAnchorPadding padding) override
		{
			slotPadding = padding;
		}

		IGRWidgetSlider& SetImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			this->pressedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			return *this;
		}

		IGRWidgetSlider& SetPressedImagePath(cstr imagePath) override
		{
			this->pressedImagePath = imagePath ? imagePath : "";
			pressedImage = panel.Root().Custodian().CreateImageFromPath("pressed button", this->pressedImagePath.c_str());
			return *this;
		}

		IGRWidgetSlider& SetRaisedImagePath(cstr imagePath) override
		{
			this->raisedImagePath = imagePath ? imagePath : "";
			raisedImage = panel.Root().Custodian().CreateImageFromPath("raised button", this->raisedImagePath.c_str());
			return *this;
		}

		void SetRenderFunction(FN_RENDER_SLIDER fnRender, void* context)
		{
			this->fnRenderSlider = fnRender;
			this->renderArgContext = context;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				isRaised = false;
				panel.CaptureCursor();
				if (!isRaised) UpdateSliderPos(ce.position);

				GRWidgetEvent mouseUp;
				mouseUp.eventType = EGRWidgetEventType::SLIDER_HELD;
				mouseUp.isCppOnly = true;
				mouseUp.iMetaData = 0;
				mouseUp.sMetaData = GetImplementationTypeName();
				panel.NotifyAncestors(mouseUp, *this);

				return EGREventRouting::Terminate;
			}
			else if (ce.click.LeftButtonUp)
			{
				isRaised = true;
				panel.Root().ReleaseCursor();

				GRWidgetEvent mouseUp;
				mouseUp.eventType = EGRWidgetEventType::SCROLLER_RELEASED;
				mouseUp.isCppOnly = true;
				mouseUp.iMetaData = 0;
				mouseUp.sMetaData = GetImplementationTypeName();
				panel.NotifyAncestors(mouseUp, *this);
				return EGREventRouting::Terminate;
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

			int absLeft = panel.AbsRect().left + slotPadding.left;

			sliderPos = clamp(cursorPos.x, absLeft, panel.AbsRect().right - slotPadding.right) - absLeft;

			double quotient = sliderPos / (double)pixelRange;

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

			double pixelPos = pixelRange * posRatio;
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

		double Max() const override
		{
			return maxValue;
		}

		double Min() const override
		{
			return minValue;
		}

		double quantum = 0.25;

		GRFontId guageFont = GRFontId::NONE;
		int guageDecimalPlaces = 2;
		int guageVerticalOffset = 4;
		EGRSchemeColourSurface guageTextSurface = EGRSchemeColourSurface::GAME_OPTION_TEXT;

		void SetGuage(GRFontId fontId, int decimalPlaces, EGRSchemeColourSurface surface) override
		{
			guageFont = fontId;
			guageDecimalPlaces = decimalPlaces;
			guageTextSurface = surface;
		}

		// Represents the number of units to increment the position when an extremum button is clicked, or the left/right keys are used 
		void SetQuantum(double quantum) override
		{
			this->quantum = quantum;
		}

		double position = 0;

		double Position() const override
		{
			return position;
		}

		// Note that the true value is clamp of the supplied value using the range values
		void SetPosition(double value) override
		{
			position = value;
			SetSliderPosFromValuePos();
		}

		GRAlignmentFlags guageAlignment;
		Vec2i guageSpacing{ 0,0 };

		void SetGuageAlignment(GRAlignmentFlags alignment, Vec2i spacing) override
		{
			guageAlignment = alignment;
			guageSpacing = spacing;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			if (lastSliderSpan != Width(panel.AbsRect()))
			{
				lastSliderSpan = Width(panel.AbsRect());
				SetSliderPosFromValuePos();
			}

			SliderDesc slider 
			{
				panel, slotPadding, isRaised, raisedImage, pressedImage, sliderPos, guageFont,
				guageAlignment, guageSpacing, guageDecimalPlaces, guageTextSurface, position, renderArgContext,
				minValue, maxValue
			};

			fnRenderSlider(g, slider);
		}

		GRAlignmentFlags alignment;
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

		void Advance(int quanta) override
		{
			position += quanta * quantum;
			SetSliderPosFromValuePos();
		}
	};

	struct GRSliderFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			auto* slider = new GRSlider(panel);
			return *slider;
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
		auto* slider = static_cast<GRANON::GRSlider*>(Cast<IGRWidgetSlider>(widget));
		return *slider;
	}

	ROCOCO_GUI_RETAINED_API void RenderSlider_Default(IGRRenderContext& g, SliderDesc& slider)
	{
		auto& panel = slider.panel;
		auto& slotPadding = slider.slotPadding;
		bool isRaised = slider.isRaised;

		bool isHovered = g.IsHovered(panel);

		GuiRect sliderSlot = panel.AbsRect();
		sliderSlot.left += slotPadding.left;
		sliderSlot.right -= slotPadding.right;

		sliderSlot.top += slotPadding.top;
		sliderSlot.bottom -= slotPadding.bottom;

		int y = Centre(sliderSlot).y;

		RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_BACKGROUND, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 255, 0, 255));
		g.DrawRect(panel.AbsRect(), backColour);

		RGBAb sliderSlotColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
		g.DrawRect(sliderSlot, sliderSlotColour);

		RGBAb sliderEdge1Colour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_EDGE_1, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
		RGBAb sliderEdge2Colour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_EDGE_2, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));

		g.DrawRectEdge(sliderSlot, sliderEdge1Colour, sliderEdge2Colour);

		IGRImage* image = isRaised ? slider.raisedImage : slider.pressedImage;

		GRWidgetRenderState rs(!isRaised, isHovered, false);

		if (image)
		{
			GRAlignmentFlags centred;
			centred.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

			Vec2i imageSpan = image->Span();

			int renderedSliderPos = panel.AbsRect().left + slotPadding.left + clamp(slider.sliderPos, 0, Width(panel.AbsRect()) - slotPadding.right - slotPadding.left);

			int x = renderedSliderPos - (imageSpan.x / 2);

			GuiRect targetRect;
			targetRect.left = x;
			targetRect.top = y - 200;
			targetRect.bottom = y + 200;
			targetRect.right = x + imageSpan.x;
			g.DrawImageUnstretched(*image, targetRect, centred);
		}

		if (slider.guageFont != GRFontId::NONE)
		{
			char guageText[16];
			char format[16];

			Strings::SafeFormat(format, "%%.%df", slider.guageDecimalPlaces);

			Strings::SafeFormat(guageText, format, slider.position);

			RGBAb colour = panel.GetColour(slider.guageTextSurface, rs);
			g.DrawText(slider.guageFont, panel.AbsRect(), slider.guageAlignment, slider.guageSpacing, to_fstring(guageText), colour);
		}

		bool isObscured = panel.Parent()->HasFlag(EGRPanelFlags::HintObscure);
		if (isObscured)
		{
			g.DrawRect(panel.AbsRect(), RGBAb(64, 64, 64, 192));
		}
	}

	ROCOCO_GUI_RETAINED_API void RenderSlider_AsLeftToRightBulbs(IGRRenderContext& g, SliderDesc& slider)
	{
		auto& panel = slider.panel;
		auto& slotPadding = slider.slotPadding;
		bool isRaised = slider.isRaised;

		bool isHovered = g.IsHovered(panel);

		GuiRect sliderSlot = panel.AbsRect();
		sliderSlot.left += slotPadding.left;
		sliderSlot.right -= slotPadding.right;

		sliderSlot.top += slotPadding.top;
		sliderSlot.bottom -= slotPadding.bottom;

		const int BULB_COUNT = 20;

		double valueSpan = slider.maxValue - slider.minValue;
		if (valueSpan != 0.0)
		{
			double quotient = clamp((slider.position - slider.minValue) / valueSpan, 0.0, 1.0);
			int nBulbsLit = (int)(quotient * BULB_COUNT);
			int bulbWidth = Width(sliderSlot) / BULB_COUNT;

			RGBAb bulbColour = panel.GetColour(EGRSchemeColourSurface::BUTTON, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 255, 0, 255));

			if (nBulbsLit == 0)
			{
				RGBAb sliderSlotColour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_BACKGROUND, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
				g.DrawRect(sliderSlot, sliderSlotColour);

				RGBAb sliderEdge1Colour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_EDGE_1, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));
				RGBAb sliderEdge2Colour = panel.GetColour(EGRSchemeColourSurface::SLIDER_SLOT_EDGE_2, GRWidgetRenderState(false, isHovered, false), RGBAb(255, 0, 255, 255));

				g.DrawRectEdge(sliderSlot, sliderEdge1Colour, sliderEdge2Colour);
			}

			for (int i = 0; i < nBulbsLit; i++)
			{
				GuiRect bulbRect = sliderSlot;
				bulbRect.top += 1;
				bulbRect.bottom -= 1;

				bulbRect.left += i * bulbWidth;
				bulbRect.right = bulbRect.left + bulbWidth - 1;

				g.DrawRect(bulbRect, bulbColour);
			}
		}

		GRWidgetRenderState rs(!isRaised, isHovered, false);

		if (slider.guageFont != GRFontId::NONE)
		{
			char guageText[16];
			char format[16];

			Strings::SafeFormat(format, "%%.%df", slider.guageDecimalPlaces);

			Strings::SafeFormat(guageText, format, slider.position);

			RGBAb colour = panel.GetColour(slider.guageTextSurface, rs);

			g.DrawText(slider.guageFont, panel.AbsRect(), slider.guageAlignment, slider.guageSpacing, to_fstring(guageText), colour);
		}

		bool isObscured = panel.Parent()->HasFlag(EGRPanelFlags::HintObscure);
		if (isObscured)
		{
			g.DrawRect(panel.AbsRect(), RGBAb(64, 64, 64, 192));
		}
	}

	Rococo::Gui::FN_RENDER_SLIDER sliderCustomRenderer = RenderSlider_Default;

	ROCOCO_GUI_RETAINED_API void SetCustomSliderRenderer(FN_RENDER_SLIDER fnRender)
	{
		sliderCustomRenderer = fnRender;
	}

	ROCOCO_GUI_RETAINED_API void RenderSlider_Custom(IGRRenderContext& g, SliderDesc& slider)
	{
		sliderCustomRenderer(g, slider);
	}
}
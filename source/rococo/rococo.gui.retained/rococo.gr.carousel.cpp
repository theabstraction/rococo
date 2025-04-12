#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <math.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRCarousel : IGRWidgetCarousel, IGRWidgetSupervisor, IGRWidgetLayout
	{
		struct Option
		{
			HString key;
			HString value;
		};

		IGRPanel& panel;

		std::vector<Option> options;

		int optionIndex = 1;

		int optionSpan = 200;

		GRAnchorPadding optionPadding{ 4, 4, 16, 16 };

		IGRWidgetButton* leftButton = nullptr;
		IGRWidgetButton* rightButton = nullptr;
		IGRWidgetScrollableMenu* dropDown = nullptr;
		
		GRCarousel(IGRPanel& owningPanel) : panel(owningPanel)
		{
			panel.SetLayoutDirection(ELayoutDirection::None);
		}

		virtual ~GRCarousel()
		{

		}

		void Free() override
		{
			delete this;
		}

		void PostConstruct()
		{
			leftButton = &CreateButton(*this);
			rightButton = &CreateButton(*this);

			leftButton->SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Previous.tiff");
			rightButton->SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Next.tiff");

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::Top);
			leftButton->SetAlignment(alignment, { 0,0 });
			leftButton->SetAlignment(alignment, { 0, 0 });

			leftButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			rightButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);

			dropDown = &CreateScrollableMenu(*this);
			dropDown->Panel().SetCollapsed(true);
			dropDown->Panel().SetRenderLast(true);
		}

		void AddOption(cstr name, cstr caption) override
		{
			options.push_back({ name, caption });
			dropDown->AddOption(name, caption);
		}

		void Advance(int delta)
		{
			optionIndex += delta;
		}

		void SetActiveChoice(cstr name) override
		{
			for (size_t i = 0; i < options.size(); i++)
			{
				if (Eq(options[i].key, name))
				{
					optionIndex = (int)i;
					break;
				}
			}

			CollapseDropDownAndNotify({ 0,0 });
		}

		void CollapseDropDownAndNotify(Vec2i clickPosition)
		{
			dropDown->Panel().SetCollapsed(true);

			GRWidgetEvent we;
			we.clickPosition = clickPosition;
			we.eventType = EGRWidgetEventType::DROP_DOWN_COLLAPSED;
			we.iMetaData = 0;
			we.isCppOnly = true;
			we.panelId = panel.Id();
			we.sMetaData = "<carousel.dropdown>";
			panel.NotifyAncestors(we, *this);
		}

		void ExpandDropDownAndNotify(Vec2i clickPosition)
		{
			dropDown->Panel().SetCollapsed(false);

			GRWidgetEvent we;
			we.clickPosition = clickPosition;
			we.eventType = EGRWidgetEventType::DROP_DOWN_EXPANDED;
			we.iMetaData = 0;
			we.isCppOnly = true;
			we.panelId = panel.Id();
			we.sMetaData = "<carousel.dropdown>";
			panel.NotifyAncestors(we, *this);
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonUp)
			{
				GuiRect edge = ComputeEdgeRect();
				if (IsPointInRect(ce.position, edge))
				{
					ExpandDropDownAndNotify(ce.position);
					return EGREventRouting::Terminate;
				}
			}
			return EGREventRouting::NextHandler;
		}

		IGRWidgetScrollableMenu& DropDown() override
		{
			return *dropDown;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
		}

		void LayoutBeforeFit() override
		{
			Vec2i buttonSpan = leftButton->ImageSpan();

			Vec2i padding{ 4,4 };

			const auto& rect = panel.AbsRect();

			padding.y = (Height(rect) - buttonSpan.y) / 2;

			GuiRect edge = ComputeEdgeRect();
			Vec2i centre = Span(panel.AbsRect());
			centre.x /= 2;
			centre.y /= 2;

			Vec2i edgeSpan = Span(edge);

			Vec2i leftOffset{  centre.x - (edgeSpan.x / 2) - buttonSpan.x - padding.x, padding.y };
			Vec2i rightOffset{ centre.x + (edgeSpan.x / 2) + padding.x, padding.y };

			leftButton->Panel().SetParentOffset(leftOffset).SetConstantSpan(buttonSpan);
			rightButton->Panel().SetParentOffset(rightOffset).SetConstantSpan(buttonSpan);

			if (!dropDown->Panel().IsCollapsed())
			{
				int dropDownHeight = 120;

				bool isRenderedUnderneathEdge = true;

				Vec2i screen = panel.Root().ScreenDimensions();
				if (Centre(panel.AbsRect()).y + dropDownHeight > screen.y)
				{
					isRenderedUnderneathEdge = false;
				}

				if (isCarouselDisabledWhenDropDownVisible)
				{
					leftButton->Panel().SetCollapsed(true);
					rightButton->Panel().SetCollapsed(true);
				}

				dropDown->Panel().SetConstantWidth(edgeSpan.x);
				dropDown->Panel().SetConstantHeight(dropDownHeight);
				dropDown->Panel().SetParentOffset({ centre.x - (edgeSpan.x / 2), isRenderedUnderneathEdge ? edge.bottom - rect.top : edge.top - rect.top - dropDownHeight });
			}
			else
			{
				leftButton->Panel().SetCollapsed(false);
				rightButton->Panel().SetCollapsed(false);
			}
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		EGREventRouting OnChildEvent(GRWidgetEvent& we, IGRWidget& source) override
		{
			if (source == leftButton->Widget())
			{
				if (we.eventType == EGRWidgetEventType::BUTTON_CLICK)
				{
					optionIndex--;
				}

				return EGREventRouting::Terminate;
			}

			if (source == rightButton->Widget())
			{
				if (we.eventType == EGRWidgetEventType::BUTTON_CLICK)
				{
					optionIndex++;
				}

				return EGREventRouting::Terminate;
			}

			if (source == dropDown->Widget())
			{
				if (we.eventType == EGRWidgetEventType::BUTTON_CLICK_OUTSIDE)
				{
					CollapseDropDownAndNotify(we.clickPosition);
					return EGREventRouting::Terminate;
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		bool isCarouselDisabledWhenDropDownVisible = false;

		void SetDisableCarouselWhenDropDownVisible(bool isDisabledAccordingly) override
		{
			isCarouselDisabledWhenDropDownVisible = isDisabledAccordingly;
		}

		int ModulateOptionIndexToArrayIndex(int index)
		{
			int iOptionsSize = (int)options.size();

			while (index < 0)
			{
				index += iOptionsSize;
			}

			while (index >= iOptionsSize)
			{
				index -= iOptionsSize;
			}

			return index;
		}

		GuiRect ComputeEdgeRect() const
		{
			GuiRect optionRect = panel.AbsRect();
			optionRect.left = Centre(panel.AbsRect()).x - (optionSpan / 2);
			optionRect.right = optionRect.left + optionSpan;
			optionRect.top += optionPadding.top;
			optionRect.bottom -= optionPadding.bottom;
			return optionRect;
		}

		void Render(IGRRenderContext& g) override
		{
			if (options.empty())
			{
				GRAlignmentFlags optionTextAlignment;
				optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, GRRenderState(0, 0, 0));
				g.DrawText(GRFontId::MENU_FONT, panel.AbsRect(), optionTextAlignment, { 0,0 }, "<no options>"_fstring, colour);
				return;
			}

			optionIndex = ModulateOptionIndexToArrayIndex(optionIndex);

			GuiRect edge = ComputeEdgeRect();
			
			auto& option = options[optionIndex];

			GRAlignmentFlags optionTextAlignment;
			optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

			bool isDisabled = isCarouselDisabledWhenDropDownVisible && !dropDown->Panel().IsCollapsed();
			bool isHovered = IsPointInRect(g.CursorHoverPoint(), edge) && !isDisabled;
			GRRenderState rs(false, isHovered, false);
			RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, rs);
			g.DrawText(GRFontId::MENU_FONT, edge, optionTextAlignment, { 0,0 }, to_fstring(option.value), colour);

			RGBAb topLeftColour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, rs);
			RGBAb bottomRightColour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(edge, topLeftColour, bottomRightColour);
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return Gui::QueryForParticularInterface<IGRWidgetCarousel, GRCarousel>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRCarousel";
		}
	};

	struct GRCarouselFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			AutoFree<GRCarousel> newCarousel = new GRCarousel(panel);
			newCarousel->PostConstruct();
			return *newCarousel.Detach();
		}
	} s_CarouselFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetCarousel::InterfaceId()
	{
		return "IGRWidgetCarousel";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetCarousel& CreateCarousel(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), GRANON::s_CarouselFactory);
		auto* carousel = Cast<IGRWidgetCarousel>(widget);
		return *carousel;
	}
}
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <math.h>
#include <vector>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

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

		GRAnchorPadding optionPixelPadding{ 4, 4, 16, 16 };
		GRAnchorPadding optionPadding{ 4, 4, 16, 16 };

		IGRWidgetButton* leftButton = nullptr;
		IGRWidgetButton* rightButton = nullptr;
		IGRWidgetScrollableMenu* dropDown = nullptr;

		GRFontId fontId = GRFontId::NONE;
		
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

		void SetFont(GRFontId fontId) override
		{
			this->fontId = fontId;
		}

		void SetOptionPadding(GRAnchorPadding padding) override
		{
			this->optionPixelPadding = padding;
		}

		void PostConstruct(cstr leftImagePathRaised, cstr rightImagePathRaised, cstr leftImagePathPressed, cstr rightImagePathPressed)
		{
			leftButton = &CreateButton(*this);
			rightButton = &CreateButton(*this);

			leftButton->SetRaisedImagePath(leftImagePathRaised);
			rightButton->SetRaisedImagePath(rightImagePathRaised);
			leftButton->SetPressedImagePath(leftImagePathPressed);
			rightButton->SetPressedImagePath(rightImagePathPressed);

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::Top);
			leftButton->SetAlignment(alignment, { 0,0 });
			leftButton->SetAlignment(alignment, { 0, 0 });

			leftButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			rightButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			leftButton->Panel().Remove(EGRPanelFlags::AcceptsFocus);
			rightButton->Panel().Remove(EGRPanelFlags::AcceptsFocus);

			MakeTransparent(leftButton->Panel(), EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT);
			MakeTransparent(leftButton->Panel(), EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT);
			MakeTransparent(leftButton->Panel(), EGRSchemeColourSurface::BUTTON);
			MakeTransparent(leftButton->Panel(), EGRSchemeColourSurface::BUTTON_IMAGE_FOG);

			MakeTransparent(rightButton->Panel(), EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT);
			MakeTransparent(rightButton->Panel(), EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT);
			MakeTransparent(rightButton->Panel(), EGRSchemeColourSurface::BUTTON);
			MakeTransparent(rightButton->Panel(), EGRSchemeColourSurface::BUTTON_IMAGE_FOG);

			dropDown = &CreateScrollableMenu(*this);
			dropDown->Panel().SetCollapsed(true);
			dropDown->Panel().SetRenderLast(true);
		}

		void AddOption(cstr name, cstr caption, cstr hint) override
		{
			options.push_back({ name, caption });
			dropDown->AddOption(name, caption, hint);
		}

		void Advance(int delta)
		{
			optionIndex += delta;
		}

		void FlipDropDown() override
		{
			if (dropDown->Panel().IsCollapsed())
			{
				ExpandDropDownAndNotify(Centre(panel.AbsRect()));
			}
			else
			{
				CollapseDropDownAndNotify(Centre(panel.AbsRect()));
			}
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
			dropDown->OnVisible();

			// Ensure this carousel is fully visible in the container viewport
			panel.FocusAndNotifyAncestors();

			// Then focus the dropdown
			auto* focusWidget = TrySetDeepFocus(dropDown->Panel());
			UNUSED(focusWidget);
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

			optionPadding = GetCustodian(panel).Scale(optionPixelPadding);

			Vec2i padding{ 4,4 };

			const auto& rect = panel.AbsRect();

			padding.y = (Height(rect) - buttonSpan.y) / 2;

			GuiRect edge = ComputeEdgeRect();
			Vec2i centre = Span(rect);
			centre.x /= 2;
			centre.y /= 2;

			Vec2i edgeSpan = Span(edge);

			Vec2i leftOffset{  optionPadding.left - buttonSpan.x - 2, padding.y + 1 };
			Vec2i rightOffset{ Width(rect) - optionPadding.right + 2, padding.y + 1 };

			leftButton->Panel().SetParentOffset(leftOffset).SetConstantSpan(buttonSpan);
			rightButton->Panel().SetParentOffset(rightOffset).SetConstantSpan(buttonSpan);

			if (!dropDown->Panel().IsCollapsed())
			{
				int domainHeight = max(dropDown->ComputeDomainHeight(), 20);
				int dropDownHeight = domainHeight;

				bool isRenderedUnderneathEdge = true;

				Vec2i screenSpan = panel.Root().ScreenDimensions();
				Vec2i panelCentre = Centre(rect);
				if (panelCentre.y + domainHeight > screenSpan.y)
				{
					if (panelCentre.y > screenSpan.y / 2)
					{
						isRenderedUnderneathEdge = false;

						if (rect.top - domainHeight < 0)
						{
							dropDownHeight = rect.top;
						}
					}
					else
					{
						dropDownHeight = screenSpan.y - rect.bottom;
					}
				}

				int cellHeight = dropDown->LastComputedButtonSpan().y;


				// Hack in cosmetic height range (So that cells can be rendered fully and not partially more frequently)
				if (dropDownHeight > cellHeight)
				{
					// Some arbitrary sanitization range
					if (cellHeight >= 10 && cellHeight < 200)
					{
						int nCells = dropDownHeight / cellHeight;
						int newDropDownHeight = (dropDownHeight / cellHeight) * cellHeight;
						if (newDropDownHeight < cellHeight)
						{
							newDropDownHeight = cellHeight;
						}

						if (newDropDownHeight > 5 * cellHeight)
						{
							// Odd number of cells. We need to make even so that we can have a page size that does not break cell bounds
							if ((nCells & 1) != 0)
							{
								newDropDownHeight -= cellHeight;
							}
						}

						dropDownHeight = newDropDownHeight;
					}
				}

				if (isCarouselDisabledWhenDropDownVisible)
				{
					leftButton->Panel().SetCollapsed(true);
					rightButton->Panel().SetCollapsed(true);
				}

				dropDown->Panel().SetConstantWidth(edgeSpan.x);
				dropDown->Panel().SetConstantHeight(dropDownHeight);
				dropDown->Viewport().SetDomainHeight(domainHeight);
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

		void NotifyAncestorChange()
		{
			optionIndex = ModulateOptionIndexToArrayIndex(optionIndex);
			dropDown->SignalButtonClick(optionIndex);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& we, IGRWidget& source) override
		{
			if (source == leftButton->Widget())
			{
				if (we.eventType == EGRWidgetEventType::BUTTON_CLICK)
				{
					optionIndex--;
					NotifyAncestorChange();
					NotifySelectionChanged(panel, EGRSelectionChangeOrigin::CarouselLRArrows);
				}

				return EGREventRouting::Terminate;
			}

			if (source == rightButton->Widget())
			{
				if (we.eventType == EGRWidgetEventType::BUTTON_CLICK)
				{
					optionIndex++;
					NotifyAncestorChange();
					NotifySelectionChanged(panel, EGRSelectionChangeOrigin::CarouselLRArrows);
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

			if (we.eventType == EGRWidgetEventType::BUTTON_KEYPRESS_UP)
			{
				if (source.Panel().HasFocus())
				{
					switch (static_cast<IO::VirtualKeys::VKCode>(we.iMetaData))
					{
					case IO::VirtualKeys::VKCode_ESCAPE:
						CollapseDropDownAndNotify(Centre(panel.AbsRect()));
						panel.Parent()->FocusAndNotifyAncestors();
						return EGREventRouting::Terminate;
					}
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			switch (ke.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_ESCAPE:
				if (ke.osKeyEvent.IsUp())
				{
					CollapseDropDownAndNotify({ 0,0 });
				}
				return EGREventRouting::Terminate;
			}

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
			optionRect.left += optionPadding.left;
			optionRect.right -= optionPadding.right;
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

				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::CAROUSEL_TEXT, GRWidgetRenderState(0, 0, 0));
				g.DrawText(fontId, panel.AbsRect(), optionTextAlignment, { 0,0 }, "<no options>"_fstring, colour);
				return;
			}

			optionIndex = ModulateOptionIndexToArrayIndex(optionIndex);

			GuiRect edge = ComputeEdgeRect();
			
			auto& option = options[optionIndex];

			GRAlignmentFlags optionTextAlignment;
			optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre).Add(EGRAlignment::AutoFonts);

			bool isDisabled = isCarouselDisabledWhenDropDownVisible && !dropDown->Panel().IsCollapsed();
			bool isHovered = IsPointInRect(g.CursorHoverPoint(), edge) && !isDisabled;
			GRWidgetRenderState rs(false, isHovered, false);

			bool obscured = DoesAncestorObscure(panel);

			RGBAb backColour = panel.GetColour(obscured ? EGRSchemeColourSurface::GAME_OPTION_DISABLED_BACKGROUND : EGRSchemeColourSurface::CAROUSEL_BACKGROUND, rs);
			g.DrawRect(edge, backColour, panel.RectStyle(), panel.CornerRadius());

			RGBAb colour = panel.GetColour(obscured ? EGRSchemeColourSurface::GAME_OPTION_DISABLED_TEXT : EGRSchemeColourSurface::CAROUSEL_TEXT, rs);
			g.DrawText(fontId, edge, optionTextAlignment, { 0,0 }, to_fstring(option.value), colour);

			if (!obscured)
			{
				RGBAb topLeftColour = panel.GetColour(EGRSchemeColourSurface::CAROUSEL_TOP_LEFT, rs);
				RGBAb bottomRightColour = panel.GetColour(EGRSchemeColourSurface::CAROUSEL_BOTTOM_RIGHT, rs);
				g.DrawRectEdge(edge, topLeftColour, bottomRightColour, panel.RectStyle(), panel.CornerRadius());
			}
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
		cstr leftImageRaised = nullptr;
		cstr rightImageRaised = nullptr;
		cstr leftImagePressed = nullptr;
		cstr rightImagePressed = nullptr;
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			AutoFree<GRCarousel> newCarousel = new GRCarousel(panel);
			newCarousel->PostConstruct(leftImageRaised, rightImageRaised, leftImagePressed, rightImagePressed);
			return *newCarousel.Detach();
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetCarousel::InterfaceId()
	{
		return "IGRWidgetCarousel";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetCarousel& CreateCarousel(IGRWidget& parent, cstr leftImageRaised, cstr rightImageRaised, cstr leftImagePressed, cstr rightImagePressed)
	{
		auto& gr = parent.Panel().Root().GR();
		GRANON::GRCarouselFactory factory;
		factory.leftImageRaised = leftImageRaised;
		factory.rightImageRaised = rightImageRaised;
		factory.leftImagePressed = leftImagePressed;
		factory.rightImagePressed = rightImagePressed;
		auto& widget = gr.AddWidget(parent.Panel(), factory);
		auto* carousel = Cast<IGRWidgetCarousel>(widget);
		return *carousel;
	}

	ROCOCO_GUI_RETAINED_API bool DoesAncestorObscure(IGRPanel& descendant)
	{
		GRWidgetEvent ev;
		ev.clickPosition = { 0,0 };
		ev.eventType = EGRWidgetEventType::ARE_DESCENDANTS_OBSCURED;
		ev.iMetaData = 0; // ancestors will increment this if they need descendants to be obscured
		ev.isCppOnly = true;
		ev.panelId = descendant.Id();
		ev.sMetaData = nullptr;
		descendant.NotifyAncestors(ev, descendant.Widget());
		return ev.iMetaData > 0;
	}
}
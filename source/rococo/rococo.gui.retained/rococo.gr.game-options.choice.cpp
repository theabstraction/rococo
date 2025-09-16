#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>
#include <rococo.strings.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Game::Options;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRGameOptionChoiceWidget : IGRWidgetGameOptionsChoice, IGRWidgetSupervisor, IChoiceInquiry, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetCarousel* carousel = nullptr;
		GameOptionConfig config;

		GRGameOptionChoiceWidget(IGRPanel& _panel) : panel(_panel)
		{
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			_panel.Add(EGRPanelFlags::AcceptsFocus);
			if (_panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(_panel, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Panel parent was null");
				return;
			}

			panel.SetExpandToParentHorizontally();
			panel.Set(GRAnchorPadding{ 1, 1, 1, 1 });
		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
		}

		void PostConstruct(const GameOptionConfig& config)
		{
			this->config = config;

			if (config.TitlesOnLeft)
			{
				panel.SetLayoutDirection(ELayoutDirection::LeftToRight);
			}

			title = &AddGameOptionTitleWidget(*this, config);

			carousel = &Gui::CreateCarousel(*this, config.LeftImageRaised, config.RightImageRaised, config.LeftImagePressed, config.RightImagePressed);
			carousel->Panel().SetExpandToParentHorizontally();
			carousel->Panel().SetExpandToParentVertically();
			carousel->SetDisableCarouselWhenDropDownVisible(true);
			carousel->SetOptionPadding(config.CarouselPadding);
			carousel->SetFont(config.CarouselFontId);
			carousel->DropDown().SetOptionFont(config.CarouselButtonFontId);
			carousel->DropDown().SetOptionPadding(config.CarouselButtonPadding);
		}

		void LayoutBeforeFit() override
		{

		}

		void LayoutBeforeExpand() override
		{
			int height = (int)(config.FontHeightToOptionHeightMultiplier * GetCustodian(panel).Fonts().GetFontHeight(config.TitleFontId));
			panel.SetConstantHeight(height);
		}

		void LayoutAfterExpand() override
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

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			IGRWidgetButton* button = carousel->DropDown().GetButtonUnderPoint(ce.position);
			if (button)
			{
				cstr hint = button->Panel().Hint();
				if (hint)
				{
					GRWidgetEvent evRouteHoverHint;
					evRouteHoverHint.clickPosition = ce.position;
					evRouteHoverHint.eventType = EGRWidgetEventType::ON_HINT_HOVER;
					evRouteHoverHint.iMetaData = button->Panel().Id();
					evRouteHoverHint.isCppOnly = true;
					evRouteHoverHint.panelId = panel.Id();
					evRouteHoverHint.sMetaData = hint;
					static_cast<IGRPanelSupervisor&>(panel).RouteToParent(evRouteHoverHint);
				}
			}
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRWidgetCarousel& Carousel() override
		{
			return *carousel;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& rc) override
		{
			auto& parentPanel = *panel.Parent();

			carousel->Panel().SetCornerRadius(parentPanel.CornerRadius());
			carousel->Panel().SetRectStyle(parentPanel.RectStyle());

			DrawGameOptionBackground(*title, panel, rc);
		}

		EGREventRouting OnButtonClick(IGRWidget& sourceWidget, Vec2i clickPosition)
		{
			auto* button = Cast<IGRWidgetButton>(sourceWidget);
			if (button)
			{
				cstr choice = button->MetaData().stringData;

				GRWidgetEvent choiceMade;
				choiceMade.clickPosition = clickPosition;
				choiceMade.eventType = EGRWidgetEventType::CHOICE_MADE;
				choiceMade.iMetaData = 0;
				choiceMade.isCppOnly = true;
				choiceMade.panelId = panel.Id();
				choiceMade.sMetaData = choice;

				carousel->SetActiveChoice(choice);

				return panel.NotifyAncestors(choiceMade, *this);
			}
			else
			{
				return EGREventRouting::NextHandler;
			}
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			switch (widgetEvent.eventType)
			{
			case EGRWidgetEventType::BUTTON_CLICK:				
				return OnButtonClick(sourceWidget, widgetEvent.clickPosition);
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			if (ke.osKeyEvent.IsUp())
			{
				return EGREventRouting::NextHandler;
			}

			switch (ke.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_LEFT:
				carousel->Advance(-1);
				break;
			case IO::VirtualKeys::VKCode_RIGHT:
				carousel->Advance(1);
				break;
			case IO::VirtualKeys::VKCode_UP:
				if (panel.HasFocus())
				{
					RotateFocusToNextSibling(Widget(), false);
				}
				break;
			case IO::VirtualKeys::VKCode_DOWN:
				if (panel.HasFocus())
				{
					RotateFocusToNextSibling(Widget(), true);
				}
				break;
			case IO::VirtualKeys::VKCode_ENTER:
				carousel->FlipDropDown();
				panel.FocusAndNotifyAncestors();
				MoveFocusIntoChildren(panel);
				break;
			default:
				return EGREventRouting::NextHandler;
			}
			return EGREventRouting::Terminate;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}
			return Gui::QueryForParticularInterface<IGRWidgetGameOptionsChoice>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionChoiceWidget";
		}

		void SetTitle(cstr text) override
		{
			title->SetText(text);
		}

		IChoiceInquiry& Inquiry() override
		{
			return *this;
		}

		void AddChoice(cstr choiceName, cstr choiceText, cstr hint) override
		{
			carousel->AddOption(choiceName, choiceText, hint);
		}

		void SetActiveChoice(cstr choiceName) override
		{
			carousel->SetActiveChoice(choiceName);
		}

		void SetHint(cstr text) override
		{
			panel.SetHint(text);
		}
	};

	struct GRGameOptionsChoiceFactory : IGRWidgetFactory
	{
		GRGameOptionsChoiceFactory()
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionChoiceWidget(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptionsChoice::InterfaceId()
	{
		return "IGRWidgetGameOptionsChoice";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsChoice& CreateGameOptionsChoice(IGRWidget& parent, const GameOptionConfig& config)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsChoiceFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionChoiceWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(config);
		return l;
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetText& AddGameOptionTitleWidget(IGRWidget& parentWidget, const GameOptionConfig& config)
	{
		auto* title = &Gui::CreateText(parentWidget);
		title->Widget().Panel().SetExpandToParentHorizontally();
		title->Widget().Panel().SetExpandToParentVertically();
		title->SetFont(config.TitleFontId);

		int fontHeight = GetCustodian(parentWidget).Fonts().GetFontHeight(config.TitleFontId);

		Vec2i spacing{ 0,0 };
		spacing.x = (int) (fontHeight * config.TitleXSpacingMultiplier);

		title->SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_TEXT);
		title->SetBackColourSurface(EGRSchemeColourSurface::LABEL_BACKGROUND);
		title->SetAlignment(config.TitleAlignment, spacing);

		MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
		MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

		return *title;
	}
}
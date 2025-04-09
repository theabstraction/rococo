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
	struct GRGameOptionChoiceWidget : IGRWidgetGameOptionsChoice, IGRWidgetSupervisor, IChoiceInquiry
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetCarousel* carousel = nullptr;

		GRGameOptionChoiceWidget(IGRPanel& _panel) : panel(_panel)
		{
			_panel.SetMinimalSpan({ 100, 24 });
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			if (_panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(_panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}

			panel.SetExpandToParentHorizontally();
			panel.Set(GRAnchorPadding{ 1, 1, 1, 1 });
		}

		void PostConstruct(GRFontId titleFont)
		{
			title = &AddGameOptionTitleWidget(*this, titleFont);

			carousel = &Gui::CreateCarousel(*this);
			carousel->Widget().Panel().SetExpandToParentHorizontally();
			carousel->Widget().Panel().SetExpandToParentVertically();
			carousel->SetDisableCarouselWhenDropDownVisible(true);

			int height = (int) (1.25 * GetCustodian(panel).Fonts().GetFontHeight(titleFont));
			panel.SetConstantHeight(2 * height);
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
			UNUSED(ce);
			return EGREventRouting::NextHandler;
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

		void Render(IGRRenderContext& rc) override
		{
			DrawEdge(EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, panel, rc);

			if (panel.HasFlag(EGRPanelFlags::HintObscure))
			{
				title->SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_DISABLED_TEXT).SetBackColourSurface(EGRSchemeColourSurface::GAME_OPTION_DISABLED_BACKGROUND);
			}
			else
			{
				title->SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_TEXT).SetBackColourSurface(EGRSchemeColourSurface::LABEL_BACKGROUND);
			}
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
			if (!ke.osKeyEvent.IsUp())
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
			default:
				return EGREventRouting::NextHandler;
			}
			return EGREventRouting::Terminate;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
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

		void AddChoice(cstr choiceName, cstr choiceText) override
		{
			carousel->AddOption(choiceName, choiceText);
		}

		void SetActiveChoice(cstr choiceName) override
		{
			carousel->SetActiveChoice(choiceName);
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

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsChoice& CreateGameOptionsChoice(IGRWidget& parent, GRFontId titleFont)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsChoiceFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionChoiceWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(titleFont);
		return l;
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetText& AddGameOptionTitleWidget(IGRWidget& parentWidget, GRFontId titleFont)
	{
		auto* title = &Gui::CreateText(parentWidget);
		title->Widget().Panel().SetExpandToParentHorizontally();
		title->Widget().Panel().SetExpandToParentVertically();
		title->SetFont(titleFont);
		title->SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_TEXT);

		MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
		MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

		return *title;
	}
}
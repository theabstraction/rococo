#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>
#include <rococo.strings.h>

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
		IGRWidgetButton* button = nullptr;

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
			panel.SetConstantHeight(64);
		}

		void PostConstruct()
		{
			title = &Gui::CreateText(*this);
			title->Widget().Panel().SetExpandToParentHorizontally();
			title->Widget().Panel().SetExpandToParentVertically();

			button = &Gui::CreateButton(*this);
			button->Widget().Panel().SetExpandToParentHorizontally();
			button->Widget().Panel().SetExpandToParentVertically();

			CopyAllColours(panel, title->Widget().Panel(), EGRSchemeColourSurface::GAME_OPTION_TEXT, EGRSchemeColourSurface::TEXT);
			CopyAllColours(panel, title->Widget().Panel(), EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, EGRSchemeColourSurface::LABEL_BACKGROUND);
			CopyAllColours(panel, title->Widget().Panel(), EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, EGRSchemeColourSurface::BACKGROUND);
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
			DrawPanelBackground(panel, rc);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			UNUSED(widgetEvent);
			UNUSED(sourceWidget);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
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
			
		}

		void SetActiveChoice(cstr choiceName) override
		{

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

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsChoice& CreateGameOptionsChoice(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsChoiceFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionChoiceWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct();
		return l;
	}
}
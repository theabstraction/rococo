#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Game::Options;

namespace GRANON
{
	struct GRGameOptionsList : IGRWidgetGameOptions, IGRWidgetSupervisor, IGameOptionsBuilder
	{
		IGRPanel& panel;
		IGameOptions& options;

		GRGameOptionsList(IGRPanel& _panel, IGameOptions& _options) : panel(_panel), options(_options)
		{
			_panel.SetMinimalSpan({ 100, 24 });
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			if (_panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(_panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}
		}

		void PostConstruct()
		{
			options.AddOptions(*this);
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
			return Gui::QueryForParticularInterface<IGRWidgetGameOptions>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionsList";
		}

		IChoiceInquiry& AddChoice(cstr name) override
		{

		}

		IBoolInquiry& AddBool(cstr name) override
		{

		}

		IScalarInquiry& AddScalar(cstr name) override
		{

		}

	};

	struct GRGameOptionsFactory : IGRWidgetFactory
	{
		IGameOptions& options;

		GRGameOptionsFactory(IGameOptions& _options): options(_options)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionsList(panel, options);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptions::InterfaceId()
	{
		return "IGRWidgetGameOptions";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptions& CreateGameOptionsList(IGRWidget& parent, IGameOptions& options)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsFactory factory(options);
		auto& l = static_cast<GRANON::GRGameOptionsList&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct();
		return l;
	}
}
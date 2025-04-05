#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.game.options.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Game::Options;

namespace GRANON
{
	struct GRGameOptionsList : IGRWidgetGameOptions, IGRWidgetSupervisor, IGameOptionsBuilder
	{
		IGRPanel& panel;
		IGameOptions& options;
		stringmap<IGRWidgetGameOptionsChoice*> mapNameToChoiceControl;
		stringmap<IGRWidgetGameOptionsBool*> mapNameToBoolControl;
		stringmap<IGRWidgetGameOptionsScalar*> mapNameToScalarControl;

		GRGameOptionsList(IGRPanel& _panel, IGameOptions& _options) : panel(_panel), options(_options)
		{
			_panel.SetMinimalSpan({ 100, 24 });
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			_panel.SetExpandToParentHorizontally();
			_panel.SetExpandToParentVertically();
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
			CopyAllColours(panel, panel, EGRSchemeColourSurface::GAME_OPTION_TEXT, EGRSchemeColourSurface::TEXT);
			CopyAllColours(panel, panel, EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, EGRSchemeColourSurface::LABEL_BACKGROUND);
			CopyAllColours(panel, panel, EGRSchemeColourSurface::GAME_OPTION_BACKGROUND, EGRSchemeColourSurface::BACKGROUND);

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

		template<class T>
		void GuaranteeUnique(T& t, cstr name)
		{
			auto i = t.find(name);
			if (i != t.end())
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "GameOption<%s> already defined. GRGameOptionsList does not support duplicate names", name);
			}
		}

		IChoiceInquiry& AddChoice(cstr name) override
		{
			GuaranteeUnique(mapNameToChoiceControl, name);
			IGRWidgetGameOptionsChoice& choiceWidget = CreateGameOptionsChoice(*this);
			mapNameToChoiceControl.insert(name, &choiceWidget);
			return choiceWidget.Inquiry();
		}

		IBoolInquiry& AddBool(cstr name) override
		{
			GuaranteeUnique(mapNameToBoolControl, name);
			IGRWidgetGameOptionsBool& boolWidget = CreateGameOptionsBool(*this);
			mapNameToBoolControl.insert(name, &boolWidget);
			return boolWidget.Inquiry();
		}

		IScalarInquiry& AddScalar(cstr name) override
		{
			GuaranteeUnique(mapNameToScalarControl, name);
			IGRWidgetGameOptionsScalar& scalarWidget = CreateGameOptionsScalar(*this);
			mapNameToScalarControl.insert(name, &scalarWidget);
			return scalarWidget.Inquiry();
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
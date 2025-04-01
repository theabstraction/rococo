#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRGameOptionsList : IGRWidgetGameOptions, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		GRGameOptionsList(IGRPanel& owningPanel) : panel(owningPanel)
		{
			owningPanel.SetMinimalSpan({ 100, 24 });
			owningPanel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			if (owningPanel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(owningPanel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}
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

		void Render(IGRRenderContext&) override
		{
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
	};

	struct GRGameOptionsFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionsList(panel);
		}
	} s_GameOptionsFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptions::InterfaceId()
	{
		return "IGRWidgetGameOptions";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptions& CreateGameOptionsList(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& l = static_cast<GRANON::GRGameOptionsList&>(gr.AddWidget(parent.Panel(), GRANON::s_GameOptionsFactory));
		return l;
	}
}
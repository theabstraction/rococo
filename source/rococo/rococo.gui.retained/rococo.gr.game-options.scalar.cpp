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
	struct GRGameOptionScalarWidget : IGRWidgetGameOptionsScalar, IGRWidgetSupervisor, IScalarInquiry
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetSlider* slider = nullptr;

		GRGameOptionScalarWidget(IGRPanel& _panel) : panel(_panel)
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
			slider = &Gui::CreateSlider(*this);
			slider->Widget().Panel().SetExpandToParentHorizontally();
			slider->Widget().Panel().SetExpandToParentVertically();
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
			return Gui::QueryForParticularInterface<IGRWidgetGameOptionsScalar>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionScalarWidget";
		}

		void SetTitle(cstr text) override
		{
			title->SetText(text);
		}

		IScalarInquiry& Inquiry() override
		{
			return *this;
		}

		void SetRange(double minValue, double maxValue) override
		{
			
		}

		void SetActiveValue(double scalarValue) override
		{

		}
	};

	struct GRGameOptionsScalarFactory : IGRWidgetFactory
	{
		GRGameOptionsScalarFactory()
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionScalarWidget(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptionsScalar::InterfaceId()
	{
		return "IGRWidgetGameOptionsScalar";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsScalar& CreateGameOptionsScalar(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsScalarFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionScalarWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct();
		return l;
	}
}
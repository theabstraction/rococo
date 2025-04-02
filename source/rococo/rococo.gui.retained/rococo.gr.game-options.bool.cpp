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
	struct GRGameOptionsBoolWidget : IGRWidgetGameOptionsBool, IGRWidgetSupervisor, IBoolInquiry
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetButton* button = nullptr;

		GRGameOptionsBoolWidget(IGRPanel& _panel) : panel(_panel)
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
			button->MakeToggleButton();
			button->SetPressedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/yes.tiff");
			button->SetRaisedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/no.tiff");
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
			rc.DrawRect(panel.AbsRect(), RGBAb(255,0,0));
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
			return Gui::QueryForParticularInterface<IGRWidgetGameOptionsBool>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGameOptionsBoolWidget";
		}

		void SetTitle(cstr text) override
		{
			title->SetText(text);
		}

		IBoolInquiry& Inquiry() override
		{
			return *this;
		}

		void SetActiveValue(bool boolValue) override
		{
			
		}
	};

	struct GRGameOptionsBoolFactory : IGRWidgetFactory
	{
		GRGameOptionsBoolFactory()
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGameOptionsBoolWidget(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGameOptionsBool::InterfaceId()
	{
		return "IGRWidgetGameOptionsBool";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsBool& CreateGameOptionsBool(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsBoolFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionsBoolWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct();
		return l;
	}
}
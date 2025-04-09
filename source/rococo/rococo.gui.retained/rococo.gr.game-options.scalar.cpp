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
			panel.Set(GRAnchorPadding{ 1, 1, 1, 1 });
		}

		void PostConstruct(GRFontId titleFont)
		{
			title = &AddGameOptionTitleWidget(*this, titleFont);

			MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

			slider = &Gui::CreateSlider(*this);
			slider->Widget().Panel().SetExpandToParentHorizontally();
			slider->Widget().Panel().SetExpandToParentVertically();

			slider->SetRaisedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/up.tiff");
			slider->SetPressedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/update.tiff");

			MakeTransparent(slider->Widget().Panel(), EGRSchemeColourSurface::SLIDER_BACKGROUND);

			int height = (int)(1.25 * GetCustodian(panel).Fonts().GetFontHeight(titleFont));
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
			slider->SetRange(minValue, maxValue);
		}

		void SetActiveValue(double scalarValue) override
		{
			slider->SetPosition(scalarValue);
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

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsScalar& CreateGameOptionsScalar(IGRWidget& parent, GRFontId titleFont)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsScalarFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionScalarWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(titleFont);
		return l;
	}
}
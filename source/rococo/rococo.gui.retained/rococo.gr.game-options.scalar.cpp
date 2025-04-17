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
	struct GRGameOptionScalarWidget : IGRWidgetGameOptionsScalar, IGRWidgetSupervisor, IScalarInquiry
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetSlider* slider = nullptr;

		GRGameOptionScalarWidget(IGRPanel& _panel) : panel(_panel)
		{
			_panel.SetMinimalSpan({ 100, 24 });
			_panel.SetLayoutDirection(ELayoutDirection::TopToBottom);
			_panel.Add(EGRPanelFlags::AcceptsFocus);
			if (_panel.Parent() == nullptr)
			{
				// We require a parent so that we can anchor to its dimensions
				RaiseError(_panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Panel parent was null");
				return;
			}

			panel.SetExpandToParentHorizontally();
			panel.Set(GRAnchorPadding{ 1, 1, 1, 1 });
		}

		void PostConstruct(const GameOptionConfig& config)
		{
			if (config.TitlesOnLeft)
			{
				panel.SetLayoutDirection(ELayoutDirection::LeftToRight);
			}

			title = &AddGameOptionTitleWidget(*this, config);

			MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			MakeTransparent(title->Widget().Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

			slider = &Gui::CreateSlider(*this);
			slider->Widget().Panel().SetExpandToParentHorizontally();
			slider->Widget().Panel().SetExpandToParentVertically();

			slider->SetRaisedImagePath(config.ScalarKnobRaised);
			slider->SetPressedImagePath(config.ScalarKnobPressed);

			slider->SetGuageAlignment(config.ScalarGuageAlignment, config.ScalarGuageSpacing);
			slider->SetSlotPadding(config.ScalarSlotPadding);

			MakeTransparent(slider->Widget().Panel(), EGRSchemeColourSurface::SLIDER_BACKGROUND);

			int height = (int)(config.FontHeightToOptionHeightMultiplier * GetCustodian(panel).Fonts().GetFontHeight(config.TitleFontId));
			panel.SetConstantHeight(height);
		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& we) override
		{
			if (panel.HasFocus() && we.wheelDelta != 0)
			{
				int acceleration = GetCustodian(panel).Keys().IsCtrlPressed() ? 10 : 1;

				if (we.wheelDelta % 120 == 0)
				{
					// Assume Win32 behaviour with 120 units per click
					slider->Advance(acceleration * (we.wheelDelta / 120));
				}
				else
				{
					slider->Advance(acceleration * we.wheelDelta);
				}
				return EGREventRouting::Terminate;
			}
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
			DrawGameOptionBackground(*title, panel, rc);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			if (slider->Widget() == sourceWidget)
			{
				switch (widgetEvent.eventType)
				{
				case EGRWidgetEventType::SCROLLER_RELEASED:
					return EGREventRouting::Terminate;
				}
			}

			UNUSED(sourceWidget);
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
			case IO::VirtualKeys::VKCode_PGUP:
				slider->Advance(-10);
				break;
			case IO::VirtualKeys::VKCode_PGDOWN:
				slider->Advance(10);
				break;
			case IO::VirtualKeys::VKCode_LEFT:
				slider->Advance(-1);
				break;
			case IO::VirtualKeys::VKCode_RIGHT:
				slider->Advance(1);
				break;
			case IO::VirtualKeys::VKCode_HOME:
				slider->SetPosition(slider->Min());
				break;
			case IO::VirtualKeys::VKCode_END:
				slider->SetPosition(slider->Max());
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
			case IO::VirtualKeys::VKCode_SPACEBAR:
			{
				double midPt = (slider->Max() + slider->Min()) / 2.0;
				if (slider->Position() <= midPt)
				{
					slider->SetPosition(slider->Max());
				}
				else
				{
					slider->SetPosition(slider->Min());
				}
			}
				break;
			default:
				return EGREventRouting::NextHandler;
			}
			return EGREventRouting::Terminate;
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

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsScalar& CreateGameOptionsScalar(IGRWidget& parent, const GameOptionConfig& config)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsScalarFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionScalarWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(config);
		return l;
	}

	ROCOCO_GUI_RETAINED_API void DrawGameOptionBackground(IGRWidgetText& title, IGRPanel& panel, IGRRenderContext& rc)
	{
		DrawEdge(EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, panel, rc);

		if (panel.HasFlag(EGRPanelFlags::HintObscure))
		{
			title.SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_DISABLED_TEXT).SetBackColourSurface(EGRSchemeColourSurface::GAME_OPTION_DISABLED_BACKGROUND);
		}
		else
		{
			title.SetTextColourSurface(EGRSchemeColourSurface::GAME_OPTION_TEXT).SetBackColourSurface(EGRSchemeColourSurface::LABEL_BACKGROUND);
		}

		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_CHILD_SPACER, GRRenderState{ false, false, false });
		if (colour.alpha > 0)
		{
			Vec2i bottomLeft = BottomLeft(panel.AbsRect());
			bottomLeft.y -= 1;

			Vec2i bottomRight = BottomRight(panel.AbsRect());
			bottomRight.y -= 1;
			rc.DrawLine(bottomLeft, bottomRight, colour);
		}
	}
}
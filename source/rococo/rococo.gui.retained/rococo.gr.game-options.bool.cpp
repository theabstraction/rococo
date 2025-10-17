// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
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
	struct GRGameOptionsBoolWidget : IGRWidgetGameOptionsBool, IGRWidgetSupervisor, IBoolInquiry, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRWidgetText* title = nullptr;
		IGRWidgetButton* button = nullptr;

		GameOptionConfig config;

		GRGameOptionsBoolWidget(IGRPanel& _panel) : panel(_panel)
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

		virtual ~GRGameOptionsBoolWidget()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
		}

		void PostConstruct(const GameOptionConfig& config)
		{
			this->config = config;

			title = &AddGameOptionTitleWidget(*this, config);
			
			if (config.TitlesOnLeft)
			{
				panel.SetLayoutDirection(ELayoutDirection::LeftToRight);
			}

			button = &Gui::CreateButton(*this);
			button->Widget().Panel().SetExpandToParentHorizontally();
			button->Widget().Panel().SetExpandToParentVertically();
			button->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			button->MakeToggleButton();
			button->SetPressedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Yes.tiff");
			button->SetRaisedImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/No.tiff");
			button->Panel().Remove(EGRPanelFlags::AcceptsFocus);
			
			MakeTransparent(button->Widget().Panel(), EGRSchemeColourSurface::BUTTON);
			MakeTransparent(button->Widget().Panel(), EGRSchemeColourSurface::BUTTON_IMAGE_FOG);
			MakeTransparent(button->Widget().Panel(), EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT);
			MakeTransparent(button->Widget().Panel(), EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT);
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
			if (widgetEvent.eventType == EGRWidgetEventType::BUTTON_CLICK && sourceWidget == button->Widget())
			{
				GRWidgetEvent optionBool{ EGRWidgetEventType::BOOL_CHANGED, panel.Id(), !button->ButtonFlags().isRaised, "", widgetEvent.clickPosition, true };
				Gui::NotifySelectionChanged(panel, EGRSelectionChangeOrigin::ButtonClick);
				return panel.NotifyAncestors(optionBool, *this);
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
			case IO::VirtualKeys::VKCode_RIGHT:
			case IO::VirtualKeys::VKCode_PGUP:
			case IO::VirtualKeys::VKCode_PGDOWN:
			case IO::VirtualKeys::VKCode_HOME:
			case IO::VirtualKeys::VKCode_END:
			case IO::VirtualKeys::VKCode_SPACEBAR:
			case IO::VirtualKeys::VKCode_ENTER:
				button->Toggle();
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
			button->SetPressedNoCallback(boolValue);
		}

		void SetHint(cstr text) override
		{
			panel.SetHint(text);
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

	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsBool& CreateGameOptionsBool(IGRWidget& parent, const GameOptionConfig& config)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRGameOptionsBoolFactory factory;
		auto& l = static_cast<GRANON::GRGameOptionsBoolWidget&>(gr.AddWidget(parent.Panel(), factory));
		l.PostConstruct(config);
		return l;
	}
}
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <math.h>
#include <vector>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct ButtonWatcher : IGRPanelWatcher
	{
		void OnSetConstantHeight(IGRPanel& panel, int height) override
		{
			UNUSED(panel);
			UNUSED(height);
		}

		void OnSetConstantWidth(IGRPanel& panel, int width) override
		{
			UNUSED(panel);
			UNUSED(width);
		}

		Vec2i lastButtonSpan{ 0,0 };

		void OnSetAbsRect(IGRPanel& panel, const GuiRect& absRect) override
		{
			UNUSED(panel);
			lastButtonSpan = Span(absRect);
		}
	};

	struct GRScrollableMenu : IGRWidgetScrollableMenu, IGRWidgetSupervisor
	{
		struct Option
		{
			HString key;
			HString value;
			IGRWidgetButton* button;
		};

		IGRPanel& panel;

		std::vector<Option> options;

		int optionIndex = 1;

		int optionSpan = 200;

		GRAnchorPadding buttonPadding{ 0,0,0,0 };

		IGRWidgetViewport* viewport = nullptr;

		ButtonWatcher watcher;
		
		GRScrollableMenu(IGRPanel& owningPanel) : panel(owningPanel)
		{
			panel.SetLayoutDirection(ELayoutDirection::LeftToRight);
		}

		virtual ~GRScrollableMenu()
		{

		}

		void Free() override
		{
			delete this;
		}

		void PostConstruct()
		{
			viewport = &CreateViewportWidget(*this);
			viewport->Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();
			viewport->ClientArea().Panel().SetLayoutDirection(ELayoutDirection::TopToBottom).SetClipChildren(true);
			viewport->SetMovePageScale(0.5);
		}

		void AddOption(cstr name, cstr caption, cstr hint) override
		{
			auto* button = &CreateButton(viewport->ClientArea().Widget());
			button->Panel().SetExpandToParentHorizontally();
			button->Panel().Set(buttonPadding);
			button->FitTextVertically();
			button->SetTitle(caption);
			button->SetClickCriterion(EGRClickCriterion::OnDownThenUp);
			button->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			button->SetBackSurface(EGRSchemeColourSurface::CAROUSEL_DROP_DOWN_BACKGROUND);
			button->SetTextSurface(EGRSchemeColourSurface::CAROUSEL_DROP_DOWN_TEXT);
			button->SetFontId(buttonFontId);
			button->Panel().SetPanelWatcher(&watcher);
			button->TriggerOnKeyDown();
			button->Panel().SetHint(hint);

			GRControlMetaData metaData;
			metaData.stringData = name;
			button->SetMetaData(metaData, true);

			options.push_back({ name, caption, button });

			size_t domainHeight = ComputeDomainHeight();

			viewport->SetDomainHeight((int) domainHeight);
		}

		int ComputeDomainHeight() const override
		{
			if (options.size() == 0)
			{
				return 0;
			}

			int buttonHeight = watcher.lastButtonSpan.y;
			return (int) (buttonHeight * options.size());
		}

		GRFontId buttonFontId = GRFontId::NONE;

		Vec2i LastComputedButtonSpan() const override
		{
			return watcher.lastButtonSpan;
		}

		cstr GetAncestorsHint()
		{
			for (auto* parent = panel.Parent(); parent != nullptr; parent = parent->Parent())
			{
				cstr hint = parent->Hint();
				if (hint && *hint != 0)
				{
					return hint;
				}
			}

			return nullptr;
		}

		void OnVisible() override
		{
			cstr rootHint = GetAncestorsHint();

			for (auto& opt : options)
			{
				auto* hint = opt.button->Panel().Hint();
				if (hint && StartsWith(hint, "$*$"))
				{
					char fullHint[1024];
					SafeFormat(fullHint, "%s%s%s", rootHint, hint + 3, opt.value.c_str());
					opt.button->Panel().SetHint(fullHint);
				}
			}
		}

		void SetOptionFont(GRFontId fontId) override
		{
			buttonFontId = fontId;
		}

		void SetOptionPadding(const GRAnchorPadding& padding) override
		{
			buttonPadding = padding;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonUp && !IsPointInRect(ce.position, panel.AbsRect()))
			{
				GRWidgetEvent ev;
				ev.clickPosition = ce.position;
				ev.eventType = EGRWidgetEventType::BUTTON_CLICK_OUTSIDE;
				ev.iMetaData = 0;
				ev.isCppOnly = true;
				ev.panelId = panel.Id();
				ev.sMetaData = nullptr;
				panel.NotifyAncestors(ev, *this);
				return EGREventRouting::Terminate;
			}
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
		}

		void SetFocusWithoutCallback(IGRWidgetButton& button)
		{
			panel.Root().GR().SetFocus(button.Panel().Id());
		}

		void SetFocusToTopmostVisibleButton(int deltaOffset)
		{
			if (options.size() == 0)
			{
				return;
			}

			int y = panel.AbsRect().top + deltaOffset;

			if (y < options[0].button->Panel().AbsRect().top)
			{
				SetFocusWithoutCallback(*options[0].button);
				return;
			}

			for (auto& opt : options)
			{
				GuiRect rect = opt.button->Panel().AbsRect();
				if (y <= rect.top && y < rect.bottom)
				{
					SetFocusWithoutCallback(*opt.button);
					return;
				}
			}

			SetFocusWithoutCallback(*options.back().button);
		}

		void OnFocusPageChange(int delta)
		{
			int beforeMove = viewport->GetOffset();
			viewport->VScroller().Scroller().MovePage(delta);
			int afterMove = viewport->GetOffset();

			int deltaOffset = afterMove - beforeMove;

			if (deltaOffset == 0)
			{
				// We scrolled to the last bit of the scrollable menu, so pick the extreme end button
				if (delta > 0) SetFocusWithoutCallback(*options.back().button);
				else		   SetFocusWithoutCallback(*options.front().button);
				return;
			}

			SetFocusToTopmostVisibleButton(deltaOffset);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			if (ke.osKeyEvent.IsUp())
			{
				return EGREventRouting::NextHandler;
			}

			auto* focusWidget = panel.Root().GR().FindFocusWidget();

			if (!focusWidget)
			{
				return EGREventRouting::NextHandler;
			}

			auto* pButton = Cast<IGRWidgetButton>(*focusWidget);
			if (!pButton)
			{
				return EGREventRouting::NextHandler;
			}

			auto& button = pButton->Widget();

			switch (ke.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_ANTITAB:
			case IO::VirtualKeys::VKCode_UP:
				RotateFocusToNextSibling(button, false);
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_TAB:
			case IO::VirtualKeys::VKCode_DOWN:
				RotateFocusToNextSibling(button, !ke.context.isCtrlHeld);
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_PGUP:
				OnFocusPageChange(-1);
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_PGDOWN:
				OnFocusPageChange(1);
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_HOME:
				viewport->VScroller().Scroller().SetSliderPosition(0);
				viewport->SetOffset(0, true);
				SetFocusWithoutCallback(*options.front().button);
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_END:
				viewport->VScroller().Scroller().SetSliderPosition(-1);
				viewport->SetOffset(-1, true);
				SetFocusWithoutCallback(*options.back().button);
				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext&) override
		{
			// Viewport expands to the widget area and covers up everything we would render, so our method is empty
			viewport->SetLineDeltaPixels(LastComputedButtonSpan().y);
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetScrollableMenu, GRScrollableMenu>(this, ppOutputArg, interfaceId);
		}

		IGRWidgetViewport& Viewport() override
		{
			return *viewport;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRScrollableMenu";
		}
	};

	struct GRScrollableMenuFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			AutoFree<GRScrollableMenu> newMenu = new GRScrollableMenu(panel);
			newMenu->PostConstruct();
			return *newMenu.Detach();
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetScrollableMenu::InterfaceId()
	{
		return "IGRWidgetScrollableMenu";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetScrollableMenu& CreateScrollableMenu(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRScrollableMenuFactory menuFactory;
		auto& widget = gr.AddWidget(parent.Panel(), menuFactory);
		auto* menu = Cast<IGRWidgetScrollableMenu>(widget);
		return *menu;
	}
}
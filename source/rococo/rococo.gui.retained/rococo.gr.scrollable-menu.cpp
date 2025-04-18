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

		}

		void OnSetConstantWidth(IGRPanel& panel, int width) override
		{

		}

		Vec2i lastButtonSpan{ 0,0 };

		void OnSetAbsRect(IGRPanel& panel, const GuiRect& absRect) override
		{
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
		}

		void AddOption(cstr name, cstr caption) override
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

		GRFontId buttonFontId = GRFontId::MENU_FONT;

		Vec2i LastComputedButtonSpan() const override
		{
			return watcher.lastButtonSpan;
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

		EGREventRouting OnChildEvent(GRWidgetEvent& we, IGRWidget& button) override
		{
			switch (we.eventType)
			{
			case EGRWidgetEventType::BUTTON_KEYPRESS_UP:
				if (button.Panel().HasFocus())
				{
					switch (static_cast<IO::VirtualKeys::VKCode>(we.iMetaData))
					{
					case IO::VirtualKeys::VKCode_UP:
						RotateFocusToNextSibling(button, false);
						return EGREventRouting::Terminate;
					case IO::VirtualKeys::VKCode_DOWN:
						RotateFocusToNextSibling(button, true);
						return EGREventRouting::Terminate;
					}
				}
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext&) override
		{
			// Viewport expands to the widget area and covers up everything we would render, so our method is empty
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
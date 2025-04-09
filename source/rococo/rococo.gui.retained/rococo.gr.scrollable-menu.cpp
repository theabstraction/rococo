#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>
#include <math.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
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

		GRAnchorPadding optionPadding{ 4, 4, 16, 16 };

		IGRWidgetViewport* viewport = nullptr;
		
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
			button->Panel().SetConstantHeight(48);
			button->SetTitle(caption);
			button->SetEventPolicy(EGREventPolicy::NotifyAncestors);

			GRControlMetaData metaData;
			metaData.stringData = name;
			button->SetMetaData(metaData, true);

			options.push_back({ name, caption, button });

			viewport->SetDomainHeight(options.size() * 48);
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

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
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

		IGRWidget& Widget()
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
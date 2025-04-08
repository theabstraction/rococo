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
			viewport->Widget().Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();
			viewport->SetDomainHeight(32 * options.size());
		}

		void AddOption(cstr name, cstr caption) override
		{
			options.push_back({ name, caption });
		}

		EGREventRouting OnCursorClick(GRCursorEvent&) override
		{
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

		void Render(IGRRenderContext& g) override
		{
			if (options.empty())
			{
				GRAlignmentFlags optionTextAlignment;
				optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, GRRenderState(0, 0, 0));
				g.DrawText(GRFontId::MENU_FONT, panel.AbsRect(), panel.AbsRect(), optionTextAlignment, { 0,0 }, "<no options>"_fstring, colour);
				return;
			}

			DrawPanelBackground(panel, g);
			g.DrawRect(panel.AbsRect(), RGBAb(255, 255, 0, 255));
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
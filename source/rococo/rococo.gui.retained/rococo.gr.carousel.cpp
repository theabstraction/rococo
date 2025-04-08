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
	struct GRCarousel : IGRWidgetCarousel, IGRWidgetSupervisor
	{
		struct Option
		{
			HString key;
			HString value;
		};

		IGRPanel& panel;

		std::vector<Option> options;
		
		GRCarousel(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRCarousel()
		{

		}

		void Free() override
		{
			delete this;
		}

		void AddOption(cstr name, cstr caption) override
		{
			options.push_back({ name, caption });
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
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

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
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
			bool isHovered = g.IsHovered(panel);
			
			for (size_t i = 0; i < options.size(); i++)
			{
				GRAlignmentFlags optionTextAlignment;
				optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

				GRRenderState rs(false, false, false);
				RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, rs);

				int x = (int32)i * 200;

				GuiRect optionRect(x, panel.AbsRect().top, x + 200, panel.AbsRect().bottom);
				g.DrawText(GRFontId::MENU_FONT, optionRect, optionRect, optionTextAlignment, { 0,0 }, to_fstring(options[i].value), colour);
			}
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetCarousel, GRCarousel>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRCarousel";
		}
	};

	struct GRCarouselFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRCarousel(panel);
		}
	} s_CarouselFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetCarousel::InterfaceId()
	{
		return "IGRWidgetCarousel";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetCarousel& CreateCarousel(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), GRANON::s_CarouselFactory);
		IGRWidgetCarousel* carousel = Cast<IGRWidgetCarousel>(widget);
		return *carousel;
	}
}
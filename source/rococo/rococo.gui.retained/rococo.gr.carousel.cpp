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

		int optionIndex = 1;

		int optionSpan = 200;

		GRAnchorPadding optionPadding{ 4, 4, 16, 16 };

		IGRWidgetButton* leftButton = nullptr;
		IGRWidgetButton* rightButton = nullptr;
		
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
			if (ce.click.LeftButtonUp)
			{
				ForEachRenderedOption([this, &ce](int optionIndex, const GuiRect& edge, const Option& option)
					{
						UNUSED(option);

						if (IsPointInRect(ce.position, edge))
						{
							this->optionIndex = optionIndex;
						}
					}
				);
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

		template<class T>
		void ForEachRenderedOption(T t)
		{ 
			int iOptionsSize = (int)options.size();

			while (optionIndex < 0)
			{
				optionIndex += iOptionsSize;
			}

			while (optionIndex > iOptionsSize)
			{
				optionIndex -= iOptionsSize;
			}

			int pos = -1;

			for (int i = optionIndex - 1; i <= optionIndex + 1; i++, pos++)
			{
				if (i >= 0 && i < (int)options.size())
				{
					int x = (int32)pos * optionSpan;
					GuiRect optionRect = panel.AbsRect();
					optionRect.left = Centre(panel.AbsRect()).x + x - (optionSpan / 2);
					optionRect.right = optionRect.left + optionSpan;

					GuiRect edge = optionRect;
					edge.left += optionPadding.left;
					edge.right -= optionPadding.right;
					edge.top += optionPadding.top;
					edge.bottom -= optionPadding.bottom;

					t(i, edge, options[i]);
				}
			}
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

			ForEachRenderedOption([this, &g](int optionIndex, const GuiRect& edge, const Option& option)
				{
					UNUSED(optionIndex);

					GRAlignmentFlags optionTextAlignment;
					optionTextAlignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

					bool isHovered = IsPointInRect(g.CursorHoverPoint(), edge);
					GRRenderState rs(false, isHovered, false);
					RGBAb colour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TEXT, rs);
					g.DrawText(GRFontId::MENU_FONT, edge, edge, optionTextAlignment, { 0,0 }, to_fstring(option.value), colour);

					RGBAb topLeftColour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_TOP_LEFT, rs);
					RGBAb bottomRightColour = panel.GetColour(EGRSchemeColourSurface::GAME_OPTION_BOTTOM_RIGHT, rs);
					g.DrawRectEdge(edge, topLeftColour, bottomRightColour);
				}
			);
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
#include <rococo.gui.retained.h>
#include <rococo.maths.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRMainFrameSupervisor
	{
		IGRPanel& panel;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetMenuBar* menuBar = nullptr;
		IGRWidgetToolbar* rhsTools = nullptr;

		GRMainFrame(IGRPanel& _panel) : panel(_panel)
		{
			
		}

		void Render(IGRRenderContext& g)
		{
			DrawPanelBackground(panel, g);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& screenDimensions) override
		{
			// The frame is one of the few widgets that resizes itself
			panel.SetParentOffset(TopLeft(screenDimensions));
			panel.Resize(Span(screenDimensions));

			if (titleBar)
			{
				titleBar->Panel().Resize({ panel.Span().x, 26 });
			}

			GRAnchorPadding onePixel{ 1, 1, 1, 1 };

			if (menuBar)
			{
				GRAnchors menuAnchors;
				menuAnchors.left = true;
				menuAnchors.top = true;
				menuAnchors.bottom = true;
				menuAnchors.expandsVertically = true;
				menuBar->Panel().SetAnchors(menuAnchors);
				menuBar->Panel().SetPadding(onePixel);
			}

			if (rhsTools)
			{
				GRAnchors rhsToolAnchors;
				rhsToolAnchors.right = true;
				rhsToolAnchors.top = true;
				rhsToolAnchors.bottom = true;
				rhsToolAnchors.expandsVertically = true;
				rhsTools->Panel().SetAnchors(rhsToolAnchors);
				rhsTools->Panel().SetPadding(onePixel);
			}
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetMenuBar& GetMenuBar() override
		{
			if (!titleBar)
			{
				titleBar = &CreateDivision(*this);
			}

			if (!menuBar)
			{
				menuBar = &CreateMenuBar(*titleBar);
			}

			return *menuBar;
		}

		IGRWidgetToolbar& GetTopRightHandSideTools() override
		{
			if (!titleBar)
			{
				titleBar = &CreateDivision(*this);
			}

			if (!rhsTools)
			{
				rhsTools = &CreateToolbar(*titleBar);
			}

			return *rhsTools;
		}

		Vec2i EvaluateMinimalSpan() const override
		{			
			return Vec2i{ 320, 200 };
		}
	};
}

namespace Rococo::Gui
{
	IGRMainFrameSupervisor* CreateGRMainFrame(IGRPanel& panel)
	{
		return new ANON::GRMainFrame(panel);
	}

	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g)
	{
		RGBAb colour = panel.Root().Scheme().GetColour(ESchemeColourSurface::BACKGROUND);
		g.DrawRect(panel.AbsRect(), colour);
	}
}
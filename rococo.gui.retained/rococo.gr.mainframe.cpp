#include <rococo.gui.retained.h>
#include <rococo.maths.h>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRMainFrameSupervisor
	{
		cstr name;
		IGRPanel& panel;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetMenuBar* menuBar = nullptr;
		IGRWidgetToolbar* rhsTools = nullptr;


		GRMainFrame(cstr _name, IGRPanel& _panel) : name(_name), panel(_panel)
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
				titleBar->Panel().Resize({ panel.Span().x, 30 });
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
				// The menu bar is a child of the title bar, so should be resized by the title bar
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
				// The rhsTools is a child of the title bar, so should be resized by the title bar
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
	IGRMainFrameSupervisor* CreateGRMainFrame(cstr name, IGRPanel& panel)
	{
		return new GRANON::GRMainFrame(name, panel);
	}

	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g)
	{
		RGBAb colour = panel.Root().Scheme().GetColour(ESchemeColourSurface::BACKGROUND);
		g.DrawRect(panel.AbsRect(), colour);
	}
}
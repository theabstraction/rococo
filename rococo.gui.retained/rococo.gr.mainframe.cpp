#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRWidgetMainFrame, IGRWidget
	{
		cstr name;
		IGRPanel& panel;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetMenuBar* menuBar = nullptr;
		IGRWidgetToolbar* rhsTools = nullptr;
		IGRWidgetDivision* clientArea = nullptr;

		GRMainFrame(cstr _name, IGRPanel& _panel) : name(_name), panel(_panel)
		{
			_panel.SetMinimalSpan({ 320, 200 });
		}

		void PostConstruct()
		{
			if (!clientArea)
			{
				clientArea = &CreateDivision(*this);
			}
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

			int clientAreaTop = 0;

			if (titleBar)
			{
				clientAreaTop = 30;
				titleBar->Panel().Resize({ panel.Span().x, clientAreaTop });
			}

			GRAnchorPadding paddingOnePixel{ 1, 1, 1, 1 };

			if (menuBar)
			{
				GRAnchors menuAnchors;
				menuAnchors.left = true;
				menuAnchors.top = true;
				menuAnchors.bottom = true;
				menuAnchors.expandsVertically = true;
				menuBar->Widget().Panel().Set(menuAnchors).Set(paddingOnePixel).Resize({ 10, clientAreaTop });
				// The menu bar is a child of the title bar, so should be resized by the title bar
			}

			if (rhsTools)
			{
				GRAnchors rhsToolAnchors;
				rhsToolAnchors.right = true;
				rhsToolAnchors.top = true;
				rhsToolAnchors.bottom = true;
				rhsToolAnchors.expandsVertically = true;
				rhsTools->Widget().Panel().Set(rhsToolAnchors).Set(paddingOnePixel);
				// The rhsTools is a child of the title bar, so should be resized by the title bar
			}

			Vec2i frameSpan = Span(screenDimensions);
			clientArea->Panel().Resize({ frameSpan.x, frameSpan.y - clientAreaTop }).SetParentOffset({ 0, clientAreaTop });
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		IGRWidgetMenuBar& MenuBar() override
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

		IGRWidgetToolbar& TopRightHandSideTools() override
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

		IGRWidget& Widget() override
		{
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetMainFrame>(this, ppOutputArg, interfaceId);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetMainFrame::InterfaceId()
	{
		return "IGRWidgetMainFrame";
	}

	IGRWidgetMainFrame* CreateGRMainFrame(cstr name, IGRPanel& panel)
	{
		auto* frame = new GRANON::GRMainFrame(name, panel);
		frame->PostConstruct();
		return frame;
	}

	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g)
	{
		GRRenderState rs(false, false, false);
		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::BACKGROUND, rs);
		g.DrawRect(panel.AbsRect(), colour);
	}
}
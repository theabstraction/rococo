#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRWidgetMainFrameSupervisor, IGRWidgetSupervisor, IGRWidgetLayout
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
			_panel.SetLayoutDirection(ELayoutDirection::None);
		}

		void LayoutBeforeFit() override
		{
			MakeTitleBar();
			int titleHeight = titleBar->InnerWidget().Panel().Span().y;
			titleBar->Panel().SetParentOffset({ 0,0 });
			clientArea->Panel().SetParentOffset({ 0, titleHeight });
			clientArea->Panel().SetConstantHeight(panel.Span().y - titleHeight);
			clientArea->Panel().SetConstantWidth(panel.Span().x);
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		void PostConstruct()
		{
			// We construct the client area first, then the title bar to ensure the title bar and menus are rendered after the client area, causing the menus to be on top

			if (!clientArea)
			{
				clientArea = &CreateDivision(*this);
				clientArea->Panel().SetConstantWidth(0);
				clientArea->Panel().SetConstantHeight(0);
				clientArea->Panel().SetDesc("Frame.Client");
			}

			MakeTitleBar();
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
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
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

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
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

		enum { TOOLBAR_PIXEL_HEIGHT_DEFAULT = 30};

		void MakeTitleBar()
		{
			if (!titleBar)
			{
				titleBar = &CreateDivision(*this);
				titleBar->Panel().SetExpandToParentHorizontally();
				titleBar->Panel().SetConstantHeight(TOOLBAR_PIXEL_HEIGHT_DEFAULT);
				titleBar->Panel().SetLayoutDirection(ELayoutDirection::LeftToRight);
				titleBar->Panel().SetDesc("Frame.TitleBar");
			}
		}

		IGRWidgetMenuBar& MenuBar() override
		{
			MakeTitleBar();

			if (!menuBar)
			{
				menuBar = &CreateMenuBar(titleBar->InnerWidget());
				menuBar->Widget().Panel().SetExpandToParentHorizontally();
				menuBar->Widget().Panel().SetExpandToParentVertically();
				menuBar->Widget().Panel().SetDesc("Frame.TitleBar.MenuBar");
			}

			return *menuBar;
		}

		IGRWidgetToolbar& TopRightHandSideTools() override
		{
			MakeTitleBar();

			if (!rhsTools)
			{
				rhsTools = &CreateToolbar(titleBar->InnerWidget());
				rhsTools->Widget().Panel().SetDesc("Frame.TitleBar.RHS");
			}

			return *rhsTools;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		IGRWidgetSupervisor& WidgetSupervisor() override
		{
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return Gui::QueryForParticularInterface<IGRWidgetMainFrame>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRMainFrame";
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetMainFrame::InterfaceId()
	{
		return "IGRWidgetMainFrame";
	}

	IGRWidgetMainFrameSupervisor* CreateGRMainFrame(cstr name, IGRPanel& panel)
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
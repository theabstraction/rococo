#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	static const char* const defaultExpandPath = "$(COLLAPSER_EXPAND)";
	static const char* const defaultInlinePath = "$(COLLAPSER_COLLAPSE)";

	struct GRCollapser : IGRWidgetCollapser, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		IGRWidgetCollapserEvents& eventHandler;
		IGRWidgetButton* collapseButton = nullptr;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetDivision* clientArea = nullptr;
		HString collapserExpandPath = defaultExpandPath;
		HString collapserInlinePath = defaultInlinePath;

		GRCollapser(IGRPanel& owningPanel, IGRWidgetCollapserEvents& _eventHandler) : panel(owningPanel), eventHandler(_eventHandler)
		{
			
		}

		bool IsCollapsed() const override
		{
			return collapseButton ? !collapseButton->GetButtonFlags().isRaised : false;
		}

		void SetExpandClientAreaImagePath(cstr path) override
		{
			if (path == nullptr || *path == 0)
			{
				collapserExpandPath = defaultExpandPath;
			}
			else
			{
				collapserExpandPath = path;
			}

			collapseButton->SetRaisedImagePath(collapserExpandPath);
		}

		void SetCollapsedToInlineImagePath(cstr path) override
		{
			if (path == nullptr || *path == 0)
			{
				collapserInlinePath = defaultInlinePath;
			}
			else
			{
				collapserInlinePath = path;
			}

			collapseButton->SetPressedImagePath(collapserInlinePath);
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		IGRWidgetDivision& TitleBar() override
		{
			return *titleBar;
		}

		void PostConstruct()
		{
			clientArea = &CreateDivision(*this);
			titleBar = &CreateDivision(*this);
			collapseButton = &CreateButton(titleBar->InnerWidget());
			collapseButton->Widget().Panel().Resize({ 26,26 }).SetParentOffset({0,2});
			collapseButton->SetRaisedImagePath(collapserExpandPath);
			collapseButton->SetPressedImagePath(collapserInlinePath);
			collapseButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			collapseButton->MakeToggleButton();
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			enum { TITLE_BAR_HEIGHT = 30 };
			titleBar->Panel().Resize({Width(panelDimensions), TITLE_BAR_HEIGHT });

			Vec2i newClientSpan;

			if (IsCollapsed())
			{
				newClientSpan = { Width(panelDimensions), 0 };
				clientArea->Panel().SetCollapsed(true);
			}
			else
			{
				newClientSpan = { Width(panelDimensions), Height(panelDimensions) - TITLE_BAR_HEIGHT };
				if (newClientSpan.y < 0)
				{
					newClientSpan.y = TITLE_BAR_HEIGHT;
				}
				clientArea->Panel().SetCollapsed(false);
			}

			clientArea->Panel().Resize(newClientSpan);
			clientArea->Panel().SetParentOffset({ 0, TITLE_BAR_HEIGHT });
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
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

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext&) override
		{
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget& sourceWidget)
		{
			if (sourceWidget.Panel().Id() == collapseButton->Widget().Panel().Id())
			{
				panel.InvalidateLayout(true);
				
				if (IsCollapsed())
				{
					eventHandler.OnCollapserInlined(*this);
				}
				else
				{
					eventHandler.OnCollapserExpanded(*this);
				}
				return EGREventRouting::Terminate;
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			auto& widgetManager = static_cast<IGRWidgetManager&>(collapseButton->Widget());
			return widgetManager.OnKeyEvent(keyEvent);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			GRCollapser* instance = (GRCollapser*)this;
			return Gui::QueryForParticularInterface<IGRWidgetCollapser, GRCollapser>(instance, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRCollapser";
		}
	};

	struct GRCollapserFactory : IGRWidgetFactory
	{
		IGRWidgetCollapserEvents& eventHandler;

		GRCollapserFactory(IGRWidgetCollapserEvents& _eventHandler): eventHandler(_eventHandler)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRCollapser(panel, eventHandler);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetCollapser::InterfaceId()
	{
		return "IGRWidgetCollapser";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetCollapser& CreateCollapser(IGRWidget& parent, IGRWidgetCollapserEvents& eventHandler)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRCollapserFactory factory(eventHandler);
		auto& collapser = static_cast<GRANON::GRCollapser&>(gr.AddWidget(parent.Panel(), factory));
		collapser.PostConstruct();
		return collapser;
	}
}
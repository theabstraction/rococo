#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	static const char* defaultExpandPath = "$(COLLAPSER_EXPAND)";
	static const char* defaultInlinePath = "$(COLLAPSER_COLLAPSE)";

	struct GRCollapser : IGRWidgetCollapser, IGRWidget
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
			collapseButton = &CreateButton(*titleBar);
			collapseButton->Widget().Panel().Resize({ 26,26 }).SetParentOffset({0,2});
			collapseButton->SetRaisedImagePath(collapserExpandPath);
			collapseButton->SetPressedImagePath(collapserInlinePath);
			collapseButton->SetEventPolicy(GREventPolicy::NotifyAncestors);
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

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
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

		void Render(IGRRenderContext& g) override
		{
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
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
				return EventRouting::Terminate;
			}
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return collapseButton->Widget().OnKeyEvent(keyEvent);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			GRCollapser* instance = (GRCollapser*)this;
			return Gui::QueryForParticularInterface<IGRWidgetCollapser, GRCollapser>(instance, ppOutputArg, interfaceId);
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
#pragma once

// The SexyStudio Widget API. This file should be kept free of OS dependent data structures and functions
// Widgets ineract with OS windows via IWindow interface on their Window method

#include <rococo.types.h>

namespace Rococo
{
	namespace Events
	{
		struct EventIdRef;
		class IPublisher;
	}

	namespace Windows
	{
		struct IWindow;
	}
}

namespace Rococo::SexyStudio
{
	struct IGuiWidget;
	struct IWidgetSet;

	using namespace Rococo::Events;
	using namespace Rococo::Windows;

	ROCOCOAPI ILayout
	{
		virtual void Layout(IGuiWidget& widget, GuiRect & rect) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ILayoutSet
	{
		virtual void Add(ILayout * d) = 0;
		virtual void Free() = 0;
		virtual void Layout(IGuiWidget& widget) = 0;
	};

	ROCOCOAPI IGuiWidget
	{
		/* Reshape this control and its children according to the layout controls.
		   parents should call this when they are resized 
		 */
		virtual void Layout() = 0;

		// add a modifier - to modify the way this widget is layed out
		virtual void AddLayoutModifier(ILayout* preprocessor) = 0;

		// For windows: the background window associated with this widget
		// other OSes must figure it out for themselves
		virtual IWindow& Window() = 0; 

		// Release the memory associated with this widget, invalidating it.
		virtual void Free() = 0;

		// Modify visibility of the widget
		virtual void SetVisible(bool isVisible) = 0;

		// returns the set of children if it can possess children, otherwise returns nullptr
		virtual IWidgetSet* Children() = 0;
	};

	namespace Widgets
	{
		void AnchorToParentLeft(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentRight(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentTop(IGuiWidget& widget, int pixelBorder);
		void AnchorToParent(IGuiWidget& widget, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
		void ExpandBottomFromTop(IGuiWidget& widget, int pixels);
		void ExpandLeftFromRight(IGuiWidget& widget, int pixels);

		GuiRect GetRect(IGuiWidget& widget);
		Vec2i GetSpan(IGuiWidget& widget);
		Vec2i GetParentSpan(IGuiWidget& widget);
		void SetWidgetPosition(IGuiWidget& widget, const GuiRect& rect);
		void Maximize(IWindow& window);
		void Minimize(IWindow& window);

		void SetText(IWindow& window, const char* text);
	}

	ROCOCOAPI IWidgetSet
	{
		// Add a widget to the widget set, when the set owner is done it will call Free() on the widget
		virtual void Add(IGuiWidget * widget) = 0;
		// the parent window to which this widget set belongs
		virtual IWindow& Parent() = 0;
		// IGuiWidget* iterator begin()
		virtual IGuiWidget** begin() = 0;
		// IGuiWidget* iterator end()
		virtual IGuiWidget** end() = 0;
	};

	ROCOCOAPI IToolbar : public IGuiWidget
	{
		// Tells the toolbar that the specified widget will manage its own layout
		// Otherwise the toolbar lays out the widget to the right of its predecessor
		virtual void SetManualLayout(IGuiWidget * widget) = 0;

		// The spacing between each widget in the toolbar
		virtual void SetSpacing(int32 firstBorder, int32 widgetSpacing) = 0;
	};

	ROCOCOAPI IIDEFrame
	{
		virtual IWindow & Window() = 0;
		virtual void SetVisible(bool isVisible) = 0;
		virtual IWidgetSet& Children() = 0;
	};

	ROCOCOAPI IIDEFrameSupervisor : IIDEFrame
	{
		virtual void Free() = 0;
		virtual void SetCloseEvent(const EventIdRef& evClose) = 0;
		virtual void SetResizeEvent(const EventIdRef& evResize) = 0;
	};

	IIDEFrameSupervisor* CreateMainIDEFrame(IPublisher& publisher);

	ROCOCOAPI IButtonWidget : IGuiWidget
	{
	};

	ILayoutSet* CreateLayoutSet();

	IButtonWidget* CreateButtonByResource(Rococo::Events::IPublisher& publisher, IWidgetSet& widgets, int16 resourceId, Rococo::Events::EventIdRef evOnClick);

	struct ButtonClickContext
	{
		IButtonWidget* sourceWidget;
	};

	ROCOCOAPI IWidgetSetSupervisor : IWidgetSet
	{
		virtual void Free() = 0;
	};

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent);

	ROCOCOAPI ISplitScreen : public IGuiWidget
	{
		virtual ISplitScreen* GetFirstHalf() = 0;
		virtual ISplitScreen* GetSecondHalf() = 0;
		virtual GuiRect GetRect() = 0;
		virtual void SplitIntoColumns(int32 firstSpan) = 0;
		virtual void SplitIntoRows(int32 firstSpan) = 0;
		virtual void Merge() = 0;
		virtual void SetBackgroundColour(RGBAb colour) = 0;
	};

	ISplitScreen* CreateSplitScreen(Rococo::Events::IPublisher& publisher, IWidgetSet& widgets);

	ROCOCOAPI ITab
	{
		virtual int64 AddRef() = 0;
		virtual int64 Release() = 0;
		virtual cstr Name() const = 0;
		virtual cstr Tooltip() const = 0;
		virtual void SetName(cstr name) = 0;
		virtual void SetTooltip(cstr tooltip) = 0;
	};

	ITab* CreateTab();

	struct TabDefiniton
	{
		ITab* tab;
	};

	ROCOCOAPI ITabSplitter : public IGuiWidget
	{
		virtual void AddTab(TabDefiniton& tab) = 0;
	};

	ITabSplitter* CreateTabSplitter(Rococo::Events::IPublisher& publisher, IWidgetSet& widgets);
}

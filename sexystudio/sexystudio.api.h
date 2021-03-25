#pragma once

// The SexyStudio Widget API. This file should be kept free of OS dependent data structures and functions
// Widgets ineract with OS windows via IWindow interface on their Window method

#include <rococo.types.h>
#include <rococo.events.h>

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

	struct IOSFont;

	struct WidgetContext
	{
		IPublisher& publisher;
		IOSFont& fontSmallLabel;
	};

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

		// Release the memory associated with this widget, invalidating it.
		virtual void Free() = 0;

		// Modify visibility of the widget
		virtual void SetVisible(bool isVisible) = 0;

		// returns the set of children if it can possess children, otherwise returns nullptr
		virtual IWidgetSet* Children() = 0;

		// Get the OS or other implementation of this widget
		virtual IWindow& Window() = 0;

		operator IWindow& () { return Window(); }
	};

	namespace Widgets
	{
		void AnchorToParentLeft(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentRight(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentTop(IGuiWidget& widget, int pixelBorder);
		void AnchorToParent(IGuiWidget& widget, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
		void ExpandBottomFromTop(IGuiWidget& widget, int pixels);
		void ExpandLeftFromRight(IGuiWidget& widget, int pixels);

		Vec2i GetParentSpan(IWindow& window);
		GuiRect GetScreenRect(IWindow& window);
		Vec2i GetSpan(IWindow& widget);
		GuiRect MapScreenToWindowRect(const GuiRect& rect, IWindow& window);
		void SetWidgetPosition(IWindow& widget, const GuiRect& rect);
		void Maximize(IWindow& window);
		void Minimize(IWindow& window);

		void SetSpan(IWindow& window, int32 dx, int32 dy);
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
		// Get the publisher associated with this widget set
		virtual WidgetContext& Context() = 0;
	};

	typedef int64 ID_TREE_ITEM;

	ROCOCOAPI IGuiTree : IGuiWidget
	{
		virtual ID_TREE_ITEM AppendItem(ID_TREE_ITEM branch) = 0;
		virtual void EnableExpansionIcons(bool enable) = 0;
		virtual void SetItemText(cstr text, ID_TREE_ITEM hItem) = 0;
		virtual void Clear() = 0;
	};

	struct TreeStyle
	{
		bool hasCheckBoxes = false;
		bool hasButtons = false;
		bool hasLines = false;
	};

	IGuiTree* CreateTree(IWidgetSet& widgets, const TreeStyle& style);

	ROCOCOAPI IGuiWidgetEditor : IGuiWidget
	{
		virtual cstr Name() const = 0;
		virtual void SetName(cstr name) = 0;
	};

	ROCOCOAPI IAsciiStringEditor : IGuiWidgetEditor
	{
		virtual void Bind(char* buffer, size_t capacityBytes) = 0;
		virtual void SetText(cstr text) = 0;
		virtual cstr Text() const = 0;
	};

	ROCOCOAPI IVariableList : IGuiWidget
	{
		virtual IAsciiStringEditor* AddAsciiString() = 0;

		// Gives number of pixels from LHS of the list to the editor column
		virtual int NameSpan() const = 0;
	};

	IVariableList* CreateVariableList(IWidgetSet& widgets);

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
		operator IWindow& () { return Window(); };
		// Update child geometry. This is issued when the control is resized and also by calling SetVisible
		virtual void LayoutChildren() = 0;
	};

	ROCOCOAPI IIDEFrameSupervisor : IIDEFrame
	{
		virtual void Free() = 0;
		virtual void SetCloseEvent(const EventIdRef& evClose) = 0;
		virtual void SetResizeEvent(const EventIdRef& evResize) = 0;
	};

	IIDEFrameSupervisor* CreateMainIDEFrame(WidgetContext& context);

	ROCOCOAPI IButtonWidget : IGuiWidget
	{
	};

	ILayoutSet* CreateLayoutSet();

	IButtonWidget* CreateButtonByResource(WidgetContext& context, IWidgetSet& widgets, int16 resourceId, Rococo::Events::EventIdRef evOnClick);

	struct ButtonClickContext
	{
		IButtonWidget* sourceWidget;
	};

	ROCOCOAPI IWidgetSetSupervisor : IWidgetSet
	{
		virtual void Free() = 0;
	};

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent, WidgetContext& context);

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

	struct TooltipArgs: Rococo::Events::EventArgs
	{
		cstr text;
		OS::ticks hoverTime;
		bool useSingleLine;
		RGBAb textColour;
		RGBAb backColour;
		RGBAb borderColour;
	};

	ISplitScreen* CreateSplitScreen(IWidgetSet& widgets);

	ROCOCOAPI ITab
	{
		virtual int64 AddRef() = 0;
		virtual int64 Release() = 0;
		virtual cstr Name() const = 0;
		virtual cstr Tooltip() const = 0;
		virtual void SetName(cstr name) = 0;
		virtual void SetTooltip(cstr tooltip) = 0;
		virtual void Activate() = 0;
		virtual void Deactivate() = 0;
		virtual void Layout() = 0;
		virtual IWidgetSet& Children() = 0;
	};

	ROCOCOAPI ITabSplitter : IGuiWidget
	{
		virtual ITab& AddTab() = 0;
	};

	ITabSplitter* CreateTabSplitter(IWidgetSet& widgets);

	struct ColourSet
	{
		RGBAb bkColor;
		RGBAb edgeColor;
		RGBAb txColor;
	};

	struct Theme
	{
		ColourSet normal;
		ColourSet lit;
	};

	ROCOCOAPI ITheme
	{
		// Get a mutable ref to the theme, allowing modification of a theme
		virtual Theme & GetTheme() = 0;
		virtual void Free() = 0;
	};

	// Use a theme, if the name is unknown a default theme is used
	// call free to cancel application of the theme
	ITheme* UseNamedTheme(cstr name, IPublisher& publisher);

	Theme GetTheme(IPublisher& publisher);

	struct ThemeInfo
	{
		cstr name;
		const Theme& theme;
	};

	void EnumerateThemes(IEventCallback<const ThemeInfo>& cb);

	// uses TEventArg<Theme> 
	extern EventIdRef evGetTheme;
}

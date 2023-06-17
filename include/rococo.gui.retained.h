#include <rococo.types.h>

#ifdef ROCOCO_GUI_RETAINED_EXPORTS
#define ROCOCO_GUI_RETAINED_API __declspec(dllexport) 
#else
#define  ROCOCO_GUI_RETAINED_API __declspec(dllimport) 
#endif

namespace Rococo
{
	struct KeyboardEvent;
}

namespace Rococo::Reflection
{
	struct IReflectionTarget;
}

namespace Rococo::Gui
{
	struct IGRPanel;
	struct IGRWidget;
	struct IGuiRetained;

#pragma pack(push, 1)
	struct CursorClick
	{
		uint16 LeftButtonDown : 1;
		uint16 LeftButtonUp : 1;
		uint16 RightButtonDown : 1;
		uint16 RightButtonUp : 1;
		uint16 MidButtonDown : 1;
		uint16 MidButtonUp : 1;
		uint16 MouseWheel : 1;
		uint16 Unknown : 9;
	};

	struct GRPanelEvent
	{
		int64 panelId;
		IGRPanel* panel;
		GuiRect absRect;
	};

	ROCOCO_INTERFACE IGRPanelEventBuilder
	{
		virtual IGRPanelEventBuilder& operator += (const GRPanelEvent& ev) = 0;
	};

	struct IGREventHistory
	{
		virtual void RecordWidget(IGRWidget& widget) = 0;
	};

	enum class ECursorIcon
	{
		Unspecified,
		Invisible,
		Arrow,
		LeftAndRightDragger
	};

	struct CursorEvent
	{
		IGREventHistory& history;
		const Vec2i position;
		const int64 eventId;
		const CursorClick click;
		ECursorIcon nextIcon;
	};

	struct KeyEvent
	{
		IGREventHistory& history;
		const int64 eventId;
		const KeyboardEvent& osKeyEvent;
	};
#pragma pack(pop)

	struct IdWidget
	{
		cstr Name;
	};

	enum class EventRouting
	{
		NextHandler,
		Terminate
	};

	ROCOCO_INTERFACE IGRLayout
	{
		virtual void Layout(IGRPanel& panel, const GuiRect& absRect) = 0;
	};

	ROCOCO_INTERFACE IGRLayoutSupervisor: IGRLayout
	{
		virtual void Free() = 0;
	};

	enum class GRAlignment : int32
	{
		None = 0,
		Left = 1,
		Right = 2,
		Top = 4,
		Bottom = 8,
		HCentre = 3,
		VCentre = 12
	};

	enum class GRFontId
	{
		MENU_FONT = 0, // THE DEFAULT FONT, used for menus and tabs
	};

	struct GRAlignmentFlags
	{
		int32 alignmentFlags = 0;

		GRAlignmentFlags& Add(GRAlignment alignment)
		{
			alignmentFlags |= (int32)alignment;
			return *this;
		}

		GRAlignmentFlags& Remove(GRAlignment alignment)
		{
			alignmentFlags &= ~(int32)alignment;
			return *this;
		}

		bool HasAllFlags(GRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) == (int32)alignment;
		}

		bool HasSomeFlags(GRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) != 0;
		}
	};

	// The interface to the platform dependent rendering of the retained GUI
	ROCOCO_INTERFACE IGRRenderContext
	{
		// Get some kind of hover point for the cursor
		virtual Vec2i CursorHoverPoint() const = 0;

		// It is up to the renderer to decide if a panel is hovered.
		virtual bool IsHovered(IGRPanel& panel) const = 0;	

		// Get the screen dimensions
		virtual GuiRect ScreenDimensions() const = 0;

		/* heading: 0 = N, E = 90 etc */
		virtual void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) = 0;

		virtual void DrawRect(const GuiRect& absRect, RGBAb colour) = 0;
		virtual void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		virtual void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;

		virtual void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, int32 caretPos, RGBAb colour) = 0;
		virtual void DrawText(GRFontId fontId, const GuiRect& targetRect, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) = 0;

		// Causes all render operations to complete
		virtual void Flush() = 0;

		virtual void EnableScissors(const GuiRect& scissorRect) = 0;
		virtual void DisableScissors() = 0;
		virtual bool TryGetScissorRect(GuiRect& scissorRect) const = 0;
	};

	enum class ESchemeColourSurface
	{
		BACKGROUND,
		BUTTON,
		BUTTON_EDGE_TOP_LEFT,
		BUTTON_EDGE_BOTTOM_RIGHT,
		BUTTON_TEXT,
		MENU_BUTTON,
		MENU_BUTTON_EDGE_TOP_LEFT,
		MENU_BUTTON_EDGE_BOTTOM_RIGHT,
		MENU_BUTTON_TEXT,
		CONTAINER_BACKGROUND,
		CONTAINER_TOP_LEFT,
		CONTAINER_BOTTOM_RIGHT,
		SCROLLER_BUTTON_BACKGROUND,
		SCROLLER_BUTTON_TOP_LEFT,
		SCROLLER_BUTTON_BOTTOM_RIGHT,
		SCROLLER_BAR_BACKGROUND,
		SCROLLER_BAR_TOP_LEFT,
		SCROLLER_BAR_BOTTOM_RIGHT,
		SCROLLER_SLIDER_BACKGROUND,
		SCROLLER_SLIDER_TOP_LEFT,
		SCROLLER_SLIDER_BOTTOM_RIGHT,
		SCROLLER_TRIANGLE_NORMAL,
		EDITOR,
		TEXT,
		FOCUS_RECTANGLE, // The rectangle surrounding a control to indicate focus, separate from the controls own focus settings
		IMAGE_FOG, // Colour, typically with mid alpha values that fogs out an image when it is not activated
		EDIT_TEXT, // Text colour when editor box is focused
		SPLITTER_BACKGROUND,
		SPLITTER_EDGE,
		USER_DEFINED_START_INDEX = 7000 // Make this the last index, then users can cast a surface to this enum + delta of their choice
	};

	// GRRenderState -> int32 combination of state bits. Pass by value rather than constant reference
	struct GRRenderState
	{
		struct Bits
		{
			int32 pressed : 1;
			int32 hovered : 1;
			int32 focused : 1;
		};

		union Value
		{
			Bits bitValues;
			int32 intValue;
		} value;

		GRRenderState(bool pressed, bool hovered, bool focused)
		{
			value.intValue = 0;
			value.bitValues.focused = focused;
			value.bitValues.hovered = hovered;
			value.bitValues.pressed = pressed;
		}
	};

	inline GRRenderState GRRenderState_HoveredOnly() { return GRRenderState(false, true, false); }

	// Passed to SetColour(...) methods to indicate that the scheme should automatically generate intensities for the various state combinations
	inline GRRenderState GRGenerateIntensities() { return GRRenderState(false, false, false); }

	ROCOCO_INTERFACE IGRScheme
	{
		virtual RGBAb GetColour(ESchemeColourSurface surface, GRRenderState state) const = 0;
		virtual void SetColour(ESchemeColourSurface surface, RGBAb colour, GRRenderState state) = 0;
		virtual bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour, GRRenderState state) const = 0;
	};

	ROCOCO_INTERFACE IGRSchemeSupervisor : IGRScheme
	{
		virtual void Free() = 0;
	};

	IGRSchemeSupervisor* CreateScheme();

	enum WidgetEventType
	{
		BUTTON_CLICK,
		EDITOR_UPDATED, // Cast WidgetEvent to WidgetEvent_EditorUpdated
		USER_DEFINED = 1024
	};

	struct WidgetEvent
	{
		WidgetEventType eventType; // What kind of event this is
		int64 panelId;			// Valid as long as the underlying widget that sent the event is not destroyed
		int64 iMetaData;		// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		cstr sMetaData;			// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		Vec2i clickPosition;	// Cursor location when the event was triggered. If irrelevant is undefined
	};

	struct IGRCustodian;

	ROCOCO_INTERFACE IGRPanelRoot
	{
		// Redirects all mouse events to the target panel, until it is either destroyed, another panel is captured, or ReleaseCursor is called on this interface
		virtual void CaptureCursor(IGRPanel & panel) = 0;

		// Returns the Id of the captured panel, or -1, if no capture exists
		virtual int64 CapturedPanelId() const = 0;

		// Increments the bad span count for the UI. Resets to zero at the start of every gui render
		virtual void IncBadSpanCountThisFrame(IGRPanel& origin) = 0;

		virtual int BadSpanCount() const = 0;

		// Queue a garbage collect for the next render cycle
		virtual void QueueGarbageCollect() = 0;

		// Sets the id of the capture panel to -1. No panel with id -1 can exist.
		virtual void ReleaseCursor() = 0;

		// The platform dependent custodian that handles platform dependent issues for the GUI
		virtual IGRCustodian& Custodian() = 0;

		// The visual scheme of the panel system
		virtual IGRScheme& Scheme() = 0;

		// the Gui Retained object that houses the root
		virtual IGuiRetained& GR() = 0;
	};

	struct [[nodiscard]] GRAnchors
	{
		uint32 left: 1 = 0;	// The widget sticks to the left side of the container
		uint32 top : 1 = 0; // The widget sticks to the top side of the container
		uint32 right : 1 = 0; // The widget sticks to the right side of the container
		uint32 bottom : 1 = 0; // The widget sticks to the bottom side of the container
		uint32 expandsHorizontally : 1 = 0; // If true then sticking to left or right of a container may expand or contract the span. Irrelevant if both left and right anchors are set.
		uint32 expandsVertically : 1 = 0; // If true then sticking to the top of the bottom of a container may expand or contract the span. Irrelevant if both top and bottom anchors are set.

		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors Left();
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors LeftAndRight();
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors Right();
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors Top();
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors TopAndBottom();
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors Bottom();

		// The widget expands to fill its parent's vertical span
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors ExpandVertically();

		// The widget expands to fill its parent's horizontal span
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors ExpandHorizontally();

		// The widget expands to fill its parent's space
		ROCOCO_GUI_RETAINED_API static [[nodiscard]] GRAnchors ExpandAll();
	};

	// Gives the number of pixels between an anchored side and the parent control. Implicit construction order is Left, Right, Top, Bottom
	struct GRAnchorPadding
	{
		int32 left = 0;
		int32 right = 0;
		int32 top = 0;
		int32 bottom = 0;
	};

	ROCOCO_INTERFACE IGRPanelRenderer
	{
		virtual void PreRender(IGRPanel& panel, const GuiRect & absRect, IGRRenderContext & g) = 0;
		virtual void PostRender(IGRPanel& panel, const GuiRect& absRect, IGRRenderContext& g) = 0;
		virtual bool IsReplacementForWidgetRendering(IGRPanel& panel) const = 0;
	};

	// Represents the underlying widget slot. This is a better mechanism than having a base widget, which imposes class derivation issues
	ROCOCO_INTERFACE IGRPanel
	{
		// Typically called after a widget resize when the parent need not ask the child to resize itself
		// It marks the layout as having been computed
		virtual void ConfirmLayout() = 0;

		// Enumerate the panel and its ancestors for a colour, if none found returns the second argument (which defaults to bright red).
		virtual RGBAb GetColour(ESchemeColourSurface surface, GRRenderState state, RGBAb defaultColour = RGBAb(255,0,0,255)) const = 0;

		// Enumerate the panel and its ancestors for a colour
		virtual bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour, GRRenderState state) const = 0;

		// Creates a local visual scheme if one does not exist, then maps a colour to the local scheme.
		virtual IGRPanel& Set(ESchemeColourSurface surface, RGBAb colour, GRRenderState state) = 0;

		virtual void MarkForDelete() = 0;
		virtual bool IsMarkedForDeletion() const = 0;
		virtual IGRPanel* Parent() = 0;
		virtual EventRouting NotifyAncestors(WidgetEvent& widgetEvent, IGRWidget& widget) = 0;
		virtual void PreventInvalidationFromChildren() = 0;
		virtual IGRWidget& Widget() = 0;
		virtual IGRPanel& Resize(Vec2i span) = 0;
		virtual IGRPanel& SetParentOffset(Vec2i offset) = 0;
		virtual IGRPanel* GetChild(int32 index) = 0;
		virtual Vec2i Span() const = 0;
		virtual Vec2i ParentOffset() const = 0;
		virtual IGRPanelRoot& Root() const = 0;
		virtual IGRPanel& AddChild() = 0;
		// If callback is not null then is invoked for each child. The number of children is returned
		virtual int32 EnumerateChildren(IEventCallback<IGRPanel>* callback) = 0;
		virtual int64 Id() const = 0;
		virtual GuiRect AbsRect() const = 0;
		virtual void CaptureCursor() = 0;
		virtual GRAnchors Anchors() = 0;
		virtual GRAnchorPadding Padding() = 0;

		// Overwrites the anchor settings for a panel
		virtual IGRPanel& Set(GRAnchors anchors) = 0;

		// Add an anchor to the panel
		virtual IGRPanel& Add(GRAnchors anchors) = 0;

		// Overwrites the padding for an anchor
		virtual IGRPanel& Set(GRAnchorPadding padding) = 0;

		// Indicates that the layout needs to be recomputed. If the argument is true then the layout of the ancestors are also marked to be recomputed
		virtual void InvalidateLayout(bool invalidateAncestors) = 0;

		// Returns the boolean collapsed state
		virtual bool IsCollapsed() const = 0;

		// Retrieve minimal span
		virtual Vec2i MinimalSpan() const = 0;

		// Indicates the layout has yet to be finalized
		virtual bool RequiresLayout() const = 0;

		virtual void Focus() = 0;
		virtual bool HasFocus() const = 0;

		virtual void SetClipChildren(bool enabled) = 0;
		virtual bool DoesClipChildren() const = 0;

		// Get extra rendering before and after widget rendering for the panel
		virtual IGRPanelRenderer* GetPanelRenderer() = 0;

		// Sets the boolean collapsed state. If collapsed a panel and its descendants will not be rendered or laid out
		virtual void SetCollapsed(bool isCollapsed) = 0;

		// Assign minimal span
		virtual void SetMinimalSpan(Vec2i span) = 0;

		// Add extra rendering before and after widget rendering for the panel
		virtual void SetPanelRenderer(IGRPanelRenderer* renderer) = 0;
	};

	// Interface used internally by the GUI retained implementation. Clients of the API only see IGRPanel(s)
	ROCOCO_INTERFACE IGRPanelSupervisor : IGRPanel
	{
		// A dangerous function, particularly if called within a recursive query. Ensure it is not called on children that are referenced in the callstack
		virtual void ClearChildren() = 0;
		virtual void GarbageCollectRecursive() = 0;
		virtual void LayoutRecursive(Vec2i absoluteOrigin) = 0;
		virtual void RenderRecursive(IGRRenderContext & g, const GuiRect& clipRect) = 0;
		virtual EventRouting RouteCursorClickEvent(CursorEvent& ce, bool filterChildrenByParentRect) = 0;
		virtual void BuildCursorMovementHistoryRecursive(CursorEvent& ce, IGRPanelEventBuilder& wb) = 0;
		virtual void SetWidget(IGRWidget& widget) = 0;
		virtual void Free() = 0;
	};

	// The base class from which queriable interfaces are derived. Used by QueryInterface methods herein
	ROCOCO_INTERFACE IGRBase
	{

	};

	enum class EQueryInterfaceResult
	{
		SUCCESS,
		NOT_IMPLEMENTED,
		INVALID_ID
	};

	ROCOCO_INTERFACE IGRWidget: IGRBase
	{
		virtual [[nodiscard]] EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) = 0;
		virtual void Layout(const GuiRect& parentDimensions) = 0;
		virtual EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) = 0;
		virtual EventRouting OnCursorClick(CursorEvent& ce) = 0;
		virtual void OnCursorEnter() = 0;
		virtual void OnCursorLeave() = 0;
		virtual EventRouting OnCursorMove(CursorEvent& ce) = 0;
		virtual EventRouting OnKeyEvent(KeyEvent& keyEvent) = 0;
		virtual IGRPanel& Panel() = 0;

		// Invoked by the IGRRetained render call
		virtual void Render(IGRRenderContext& g) = 0;

		// Invoked by the IGRRetained instance management logic
		virtual void Free() = 0;
	};

	template<class CAST_TO_THIS_CLASS> inline CAST_TO_THIS_CLASS* Cast(IGRWidget& widget, cstr interfaceId)
	{
		IGRBase* castBase = nullptr;
		auto result = widget.QueryInterface(&castBase, interfaceId);
		return result == EQueryInterfaceResult::SUCCESS ? static_cast<CAST_TO_THIS_CLASS*>(castBase) : nullptr;
	};

	template<class CAST_TO_THIS_CLASS> inline CAST_TO_THIS_CLASS* Cast(IGRWidget& widget)
	{
		return Cast<CAST_TO_THIS_CLASS>(widget, CAST_TO_THIS_CLASS::InterfaceId());
	};

	enum GRClickCriterion
	{
		OnDown, // Click event fires when the left button down event is routed to the button widget (THIS IS THE DEFAULT)
		OnUp, // Click event when the left button up event is routed to the button widget
		OnDownThenUp // Click event when the left button is down, the cursor remains in the button rectangle, and then the left button is released.
	};

	enum GREventPolicy
	{
		PublicEvent, // The widget notifies the GR custodian that an event has fired (THIS IS THE DEFAULT)
		NotifyAncestors, // The widget notifies the chain of parents that an event has fired. If nothing terminates it, the root panel makes it a public event		
	};

	struct ControlMetaData
	{
		int64 intData = 0;
		cstr stringData = nullptr; // If nullptr is provided, the meta data is stored as an empty string and retrieved as an empty string

		static ControlMetaData None()
		{
			return { 0,nullptr };
		}
	};

	struct ButtonFlags
	{
		uint32 isMenu : 1;
		uint32 forSubMenu : 1;
		uint32 isEnabled : 1;
		uint32 isRaised : 1;
	};

	// The platform independent view of the platform dependent image associated with some widget
	ROCOCO_INTERFACE IImageMemento
	{
		virtual bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, IGRRenderContext& rc) = 0;
		virtual Vec2i Span() const = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetText: IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;
		virtual IGRWidgetText& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;
		virtual IGRWidgetText& SetFont(GRFontId fontId) = 0;
		virtual IGRWidgetText& SetText(cstr text) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetButton : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual IGRWidget& Widget() = 0;

		// Sets the rule by which events are fired
		virtual IGRWidgetButton& SetClickCriterion(GRClickCriterion criterion) = 0;

		// Set what happens when the button is fired
		virtual IGRWidgetButton& SetEventPolicy(GREventPolicy policy) = 0;

		// Sets the image icon path for the button in the raised and pressed state. It is up to the custodian to decide how to interpret the path as an image and how to render it
		// The custodian decides if/when to prioritize the image over the title
		virtual IGRWidgetButton& SetImagePath(cstr imagePath) = 0;

		// Sets the image icon path for the button in a raised state. It is up to the custodian to decide how to interpret the path as an image and how to render it
		// The custodian decides if/when to prioritize the image over the title.
		virtual IGRWidgetButton& SetRaisedImagePath(cstr imagePath) = 0;

		// Sets the image icon path for the button in a pressed state. It is up to the custodian to decide how to interpret the path as an image and how to render it
		// The custodian decides if/when to prioritize the image over the title.
		virtual IGRWidgetButton& SetPressedImagePath(cstr imagePath) = 0;

		// Sets user meta data for the button
		virtual IGRWidgetButton& SetMetaData(const ControlMetaData& metaData) = 0;

		// Sets the display text for the button
		virtual IGRWidgetButton& SetTitle(cstr text) = 0;

		// Gets the display text and returns its length. If the buffer is insufficient, the result is truncated
		virtual size_t GetTitle(char* titleBuffer, size_t nBytes) const = 0;

		virtual IGRWidgetButton& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;

		// Returns meta data set with SetMetaData. The pointers are valid until meta data is changed or the control is destroyed
		virtual ControlMetaData GetMetaData() = 0;

		virtual ButtonFlags GetButtonFlags() const = 0;

		virtual void MakeToggleButton() = 0;
	};

	struct GRMenuButtonItem
	{
		cstr text;
		ControlMetaData metaData;
		int isEnabled: 1;
	};

	struct GRMenuSubMenu
	{
		GRMenuSubMenu(cstr _text, bool enable = true) : text(_text), isEnabled(enable) {};
		cstr text;
		int isEnabled : 1;
	};

	struct GRMenuItemId
	{
		int64 id;
		static GRMenuItemId Root() { return GRMenuItemId{ 0 }; }
		static GRMenuItemId Error() { return GRMenuItemId{ -1 }; }
	};

	ROCOCO_INTERFACE IGRWidgetMenuBar : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget & Widget() = 0;
		virtual bool AddButton(GRMenuItemId parentMenu, const GRMenuButtonItem& item) = 0;
		virtual GRMenuItemId AddSubMenu(GRMenuItemId parentMenu, const GRMenuSubMenu& subMenu) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetFactory
	{
		virtual IGRWidget& CreateWidget(IGRPanel& panel) = 0;
	};

	ROCOCO_GUI_RETAINED_API IGRWidgetFactory& GetWidgetButtonFactory();

	ROCOCO_INTERFACE IGRWidgetToolbar : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		// If not left, then alignment is right
		virtual void SetChildAlignment(GRAlignment alignment, int32 interChildPadding = 4, int32 borderPadding = 1) = 0;

		// Shrinks the children to their minimal size and resizes the control to fit in all children with specified padding, and returns the span
		// This may cause the control and its children to invoke InvalidateLayout on the containing panels
		virtual Vec2i ResizeToFitChildren() = 0;
	};

	// A widget with a rectangular background for holding child widgets
	ROCOCO_INTERFACE IGRWidgetDivision : IGRWidget
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	struct GRColumnSpec
	{
		cstr name = nullptr;
		int32 minWidth = 24;
		int32 defaultWidth = 120;
		int32 maxWidth = 1024;
	};

	struct GRRowSpec
	{
		int32 rowHeight = 24;
	};

	ROCOCO_INTERFACE IGRWidgetTable : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		// Adds a new column and returns the column index of the new column
		virtual int32 AddColumn(const GRColumnSpec & spec) = 0;

		// Adds a new row and returns the row index of the new row
		virtual int32 AddRow(const GRRowSpec& spec) = 0;

		// Returns a reference to the cell at the given location. If the location indices are out of bounds, the method returns nullptr
		virtual IGRWidgetDivision* GetCell(int32 column, int32 row) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetPropertyEditorTree : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetSplitter : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		virtual IGRWidgetDivision& First() = 0;
		virtual IGRWidgetDivision& Second() = 0;
		virtual IGRWidgetSplitter& SetDraggerMinMax(int32 minValue, int32 maxValue) = 0;
	};

	// A collapsable region with a client area and title area. The title area contains the collapse button. The client area will vanish when the collapse button is engaged
	ROCOCO_INTERFACE IGRWidgetCollapser: IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual IGRWidget& Widget() = 0;

		// The area under the collapser's title bar. Will be of zero area if the collapse button is engaged
		virtual IGRWidgetDivision& ClientArea() = 0;

		// The collapser button is on the left side, so it is recommended to right align any additions and give enough room for the collapser to work
		virtual IGRWidgetDivision& TitleBar() = 0;
	};

	// The main frame with menu, toolbar and client area beneath the title bar
	ROCOCO_INTERFACE IGRWidgetMainFrame : IGRBase
	{
		// The unique id associated with this interface
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		// The widget for the main frame
		virtual IGRWidget& Widget() = 0;

		// Retrieves a reference to the frame's top menu bar. If one does not exist, it is created		
		virtual IGRWidgetMenuBar& MenuBar() = 0;

		// Retrieves a reference to the frame's top right tool bar. Typically used for minimize, maximize/restore and close window buttons.
		virtual IGRWidgetToolbar& TopRightHandSideTools() = 0;

		// The part of the main frame that is below the title bar. If there is no title bar the client area covers the entire area
		virtual IGRWidgetDivision& ClientArea() = 0;
	};

	enum class GRDebugFlags
	{
		None = 0,
		ThrowWhenPanelIsZeroArea = 1
	};

	// Highest level of the retained GUI manages frames, frame render order, event routing, visibility, building and rendering
	ROCOCO_INTERFACE IGuiRetained
	{
		// Associates a frame with an id and returns it. If it already exists, gets the existant one.
		virtual IGRWidgetMainFrame& BindFrame(IdWidget id) = 0;

		// Deletes the frame with the given id, invalidating all references to the frame and its panel and its layout
		virtual void DeleteFrame(IdWidget id) = 0;

		// Returns true if at least one GRDebugFlag is present
		virtual bool HasDebugFlag(GRDebugFlags flag) const = 0;

		// Combination of GRDebugFlags to overwrite the current flag state
		virtual void SetDebugFlags(int grDebugFlags) = 0;

		// Get a frame associated with an id. If none exist, null is returned
		virtual IGRWidgetMainFrame* FindFrame(IdWidget id) = 0;

		// Lower the frame so that it is the first to render.
		virtual void MakeFirstToRender(IdWidget id) = 0;

		// Raise the frame so that it is the final to render
		virtual void MakeLastToRender(IdWidget id) = 0;	

		// Free all panels marked for delete
		virtual void GarbageCollect() = 0;
		
		// Renders the list of frames
		virtual void RenderGui(IGRRenderContext& g) = 0;

		virtual IGRPanelRoot& Root() = 0;

		// Invoked by widget factories to add widgets to the retained gui
		virtual IGRWidget& AddWidget(IGRPanel& parent, IGRWidgetFactory& factory) = 0;

		// Constant time lookup of a widget with a given panel Id.
		virtual IGRWidget* FindWidget(int64 panelId) = 0;

		// Set the visibility status. If invisible, it will ignore all input and not be rendered
		virtual void SetVisible(bool isVisible) = 0;

		// Returns true if the retained GUI is visible and there are frames to show, otherwise false
		virtual bool IsVisible() const = 0;

		virtual int64 GetFocusId() const = 0;

		// Sets the keyboard focus to the id of a panel.
		virtual void SetFocus(int64 id = -1);

		virtual EventRouting RouteCursorClickEvent(CursorEvent& mouseEvent) = 0;
		virtual EventRouting RouteCursorMoveEvent(CursorEvent& mouseEvent) = 0;
		virtual EventRouting RouteKeyEvent(KeyEvent& keyEvent) = 0;

		virtual void UpdateNextFrame(IGRPanel& panel) = 0;
	};

	enum GRErrorCode
	{
		None,
		BadSpanHeight,
		BadSpanWidth,
		Generic,
		InvalidArg,
		RecursionLocked
	};

	struct ScrollerMetrics
	{
		// PixelPosition = 0 -> scroller is in the start position, with >= 0 -> scroller has moved that many pixels towards the end position
		int32 PixelPosition; 

		// the maximum pixel position, i.e the slider zone span - the slider span. Example: if the slider zone is 1024 pixels and the slider span is 24 pixels, the pixel range would be 1000.
		int32 PixelRange; 

		// The number of pixels between the extremes of the slider zone
		int32 SliderZoneSpan;
	};

	struct IGRWidgetScroller;

	struct GRSliderSpec
	{
		int32 sliderSpanInPixels;
	};

	ROCOCO_INTERFACE IGRScrollerEvents
	{
		virtual void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) = 0;
		virtual GRSliderSpec OnCalculateSliderRect(int32 scrollerSpan) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetScroller : IGRBase
	{
		virtual IGRWidget& Widget() = 0;
		virtual ScrollerMetrics GetMetrics() const = 0;
		virtual void SetSliderPosition(int32 topPixelDelta) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetVerticalScroller : IGRWidgetScroller
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	ROCOCO_INTERFACE IGRWidgetVerticalScrollerWithButtons : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		virtual IGRWidgetButton& BottomButton() = 0;
		virtual IGRWidgetVerticalScroller& Scroller() = 0;
		virtual IGRWidgetButton& TopButton() = 0;
	};

	// A viewport is a rectangle adjacent to scrollbars that let the user navigate a larger visual domain.
	ROCOCO_INTERFACE IGRWidgetViewport : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual void SetDomainHeight(int32 heightInPixels) = 0;
		virtual IGRWidget& Widget() = 0;
		virtual IGRWidgetDivision& ClientArea() = 0;
		virtual IGRWidgetVerticalScrollerWithButtons& VScroller() = 0;
	};

	// A vertical list that aligns its children vertically
	ROCOCO_INTERFACE IGRWidgetVerticalList : IGRWidget
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	ROCOCO_INTERFACE IGRWidgetEditBox : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		// Returns length of the internal storage, which includes space for the trailing nul character. Never returns < 2, i.e there is always space for one character and a trailing nul
		virtual size_t GetCapacity() const = 0;
		virtual IGRWidgetEditBox& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;
		virtual IGRWidgetEditBox& SetFont(GRFontId fontId) = 0;
		virtual IGRWidgetEditBox& SetReadOnly(bool isReadOnly) = 0;
		virtual IGRWidgetEditBox& SetMetaData(const ControlMetaData& metaData) = 0;

		// Assigns a string to internal storage, truncating if needs be. If null is passed it is treated as an empty string
		virtual void SetText(cstr argText) = 0;

		// Copies the text to the buffer, truncating if needs be. Returns the length of the internal representation, which includes the trailing nul character. Never returns < 1.
		virtual int32 GetTextAndLength(char* buffer, int32 receiveCapacity) const = 0;
	};

	struct IGREditorMicromanager;

	struct WidgetEvent_EditorUpdated : WidgetEvent
	{
		IGRWidgetEditBox* editor;
		IGREditorMicromanager* manager;
		int32 caretPos;
	};

	struct IGRCustodian;

	struct GRConfig
	{
		int32 unused = 0;
	};

	enum class GRPaths: int32
	{
		// This matches Windows and Unix MAX_PATH.
		MAX_FULL_PATH_LENGTH = 260
	};

	// Factory functions for creating widgets. All call IGuiRetained::AddWidget(...) to add themselves to the GUI
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetDivision& CreateDivision(IGRWidget& parent);

	// Create a property tree editor. The instance of IGRWidgetPropertyEditorTreeEvents& has to be valid of the lifespan of the widget, or mark the widget panel for deletion when events can no longer be handled
	ROCOCO_GUI_RETAINED_API IGRWidgetPropertyEditorTree& CreatePropertyEditorTree(IGRWidget& parent, Rococo::Reflection::IReflectionTarget& target);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent, IGRScrollerEvents& events);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScrollerWithButtons& CreateVerticalScrollerWithButtons(IGRWidget& parent, IGRScrollerEvents& events);

	// Creates a viewport into a larger UI domain, providing horizontal and vertical scrollbars to navigate the domain
	ROCOCO_GUI_RETAINED_API IGRWidgetViewport& CreateViewportWidget(IGRWidget& parent);

	// Creates a vertical list that aligns its children vertically
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalList& CreateVerticalList(IGRWidget& parent, bool enforcePositiveChildHeights = true);
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateMenuButton(IGRWidget& parent, bool forSubmenu = false);
	ROCOCO_GUI_RETAINED_API IGRWidgetToolbar& CreateToolbar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetText& CreateText(IGRWidget& parent);

	// Create a collapsable region with a client area and title area. The title area contains the collapse button. The client area will vanish when the collapse button is engaged
	ROCOCO_GUI_RETAINED_API IGRWidgetCollapser& CreateCollapser(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetSplitter& CreateLeftToRightSplitter(IGRWidget& parent, int32 splitStartPosition, bool updateWithMouseMove);
	ROCOCO_GUI_RETAINED_API IGRWidgetTable& CreateTable(IGRWidget& parent);

	// Implemented by various editor filters
	ROCOCO_INTERFACE IGREditFilter
	{
		virtual void Free() = 0;

		// When an edit even occurs OnUpdate(...) is called here to correct the editor according to the filter rules and update the edit state
		virtual void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager) = 0;
	};

	// Creates an editor widget along with a filter. The filter has to be valid for the lifespan of the editor box
	ROCOCO_GUI_RETAINED_API IGRWidgetEditBox& CreateEditBox(IGRWidget& parent, IGREditFilter* filter, int32 capacity = (int32) GRPaths::MAX_FULL_PATH_LENGTH);

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IGRScheme& scheme);

	// Get an editor filter for 32-bit floating point numbers. Valid for the lifespan of the executable
	ROCOCO_GUI_RETAINED_API IGREditFilter& GetF32Filter();

	// Get an editor filter for 64-bit floating point numbers. Valid for the lifespan of the executable
	ROCOCO_GUI_RETAINED_API IGREditFilter& GetF64Filter();

	// Get an editor filter for signed32-bit integers. Valid for the lifespan of the executable
	ROCOCO_GUI_RETAINED_API IGREditFilter& GetI32Filter();

	// Get an editor filter for signed 64-bit integers. Valid for the lifespan of the executable
	ROCOCO_GUI_RETAINED_API IGREditFilter& GetI64Filter();

	// Get an editor filter for unsigned 64-bit integers. Valid for the lifespan of the executable
	ROCOCO_GUI_RETAINED_API IGREditFilter& GetUnsignedFilter();

	ROCOCO_GUI_RETAINED_API void InvalidateLayoutForAllChildren(IGRPanel& panel);

	ROCOCO_GUI_RETAINED_API void InvalidateLayoutForAllDescendants(IGRPanel& panel);

	ROCOCO_GUI_RETAINED_API EventRouting RouteEventToHandler(IGRPanel& panel, WidgetEvent& ev);
}
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

		virtual void DrawRect(const GuiRect& absRect, RGBAb colour) = 0;
		virtual void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		virtual void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;

		virtual void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, int32 caretPos, RGBAb colour) = 0;
		virtual void DrawText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) = 0;

		virtual void EnableScissors(const GuiRect& scissorRect) = 0;
		virtual void DisableScissors() = 0;
	};

	enum class ESchemeColourSurface
	{
		BACKGROUND,
		BUTTON_RAISED,
		BUTTON_PRESSED,
		BUTTON_RAISED_AND_HOVERED,
		BUTTON_PRESSED_AND_HOVERED,
		BUTTON_EDGE_TOP_LEFT,
		BUTTON_EDGE_BOTTOM_RIGHT,
		BUTTON_EDGE_TOP_LEFT_PRESSED,
		BUTTON_EDGE_BOTTOM_RIGHT_PRESSED,
		BUTTON_TEXT,
		MENU_BUTTON_RAISED,
		MENU_BUTTON_PRESSED,
		MENU_BUTTON_RAISED_AND_HOVERED,
		MENU_BUTTON_PRESSED_AND_HOVERED,
		MENU_BUTTON_EDGE_TOP_LEFT,
		MENU_BUTTON_EDGE_BOTTOM_RIGHT,
		MENU_BUTTON_EDGE_TOP_LEFT_PRESSED,
		MENU_BUTTON_EDGE_BOTTOM_RIGHT_PRESSED,
		MENU_BUTTON_TEXT,
		CONTAINER_BACKGROUND,
		CONTAINER_TOP_LEFT,
		CONTAINER_BOTTOM_RIGHT,
		CONTAINER_BACKGROUND_HOVERED,
		CONTAINER_TOP_LEFT_HOVERED,
		CONTAINER_BOTTOM_RIGHT_HOVERED,
		SCROLLER_BUTTON_BACKGROUND,
		SCROLLER_BUTTON_TOP_LEFT,
		SCROLLER_BUTTON_BOTTOM_RIGHT,
		SCROLLER_BUTTON_BACKGROUND_HOVERED,
		SCROLLER_BUTTON_TOP_LEFT_HOVERED,
		SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED,
		TEXT,
		TEXT_HOVERED,
		IMAGE_FOG, // Colour, typically with mid alpha values that fogs out an image when it is not activated
		IMAGE_FOG_HOVERED, // Colour, typically with mid alpha values that fogs out an image when it is hovered but not activated
		FOCUSED_EDITOR_HOVERED, // Background colour for an edit box when it focused and hovered
		FOCUSED_EDITOR, // Background colour for an edit box when it focused
		FOCUS_RECTANGLE, // Edge colour for the focused widget
		EDIT_TEXT_HOVERED, // Text colour when editor box is focused and hovered
		EDIT_TEXT, // Text colour when editor box is focused
		SPLITTER_BACKGROUND,
		SPLITTER_BACKGROUND_HOVERED,
		SPLITTER_EDGE,
		SPLITTER_EDGE_HILIGHTED
	};

	ROCOCO_INTERFACE IScheme
	{
		virtual RGBAb GetColour(ESchemeColourSurface surface) const = 0;
		virtual void SetColour(ESchemeColourSurface surface, RGBAb colour) = 0;
		virtual bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour) const = 0;
	};

	ROCOCO_INTERFACE ISchemeSupervisor : IScheme
	{
		virtual void Free() = 0;
	};

	ISchemeSupervisor* CreateScheme();

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

		// Queue a garbage collect for the next render cycle
		virtual void QueueGarbageCollect() = 0;

		// Sets the id of the capture panel to -1. No panel with id -1 can exist.
		virtual void ReleaseCursor() = 0;

		// The platform dependent custodian that handles platform dependent issues for the GUI
		virtual IGRCustodian& Custodian() = 0;

		// The visual scheme of the panel system
		virtual IScheme& Scheme() = 0;

		// the Gui Retained object that houses the root
		virtual IGuiRetained& GR() = 0;
	};

	struct GRAnchors
	{
		uint32 left: 1 = 0;	// The widget sticks to the left side of the container
		uint32 top : 1 = 0; // The widget sticks to the top side of the container
		uint32 right : 1 = 0; // The widget sticks to the right side of the container
		uint32 bottom : 1 = 0; // The widget sticks to the bottom side of the container
		uint32 expandsHorizontally : 1 = 0; // If true then sticking to left or right of a container may expand or contract the span. Irrelevant if both left and right anchors are set.
		uint32 expandsVertically : 1 = 0; // If true then sticking to the top of the bottom of a container may expand or contract the span. Irrelevant if both top and bottom anchors are set.

		ROCOCO_GUI_RETAINED_API static GRAnchors Left();
		ROCOCO_GUI_RETAINED_API static GRAnchors LeftAndRight();
		ROCOCO_GUI_RETAINED_API static GRAnchors Right();
		ROCOCO_GUI_RETAINED_API static GRAnchors Top();
		ROCOCO_GUI_RETAINED_API static GRAnchors TopAndBottom();
		ROCOCO_GUI_RETAINED_API static GRAnchors Bottom();
		ROCOCO_GUI_RETAINED_API static GRAnchors ExpandVertically();
		ROCOCO_GUI_RETAINED_API static GRAnchors ExpandHorizontally();
		ROCOCO_GUI_RETAINED_API static GRAnchors ExpandAll();
	};

	// Gives the number of pixels between an anchored side and the parent control. Implicit construction order is Left, Right, Top, Bottom
	struct GRAnchorPadding
	{
		int32 left = 0;
		int32 right = 0;
		int32 top = 0;
		int32 bottom = 0;
	};

	// Represents the underlying widget slot. This is a better mechanism than having a base widget, which imposes class derivation issues
	ROCOCO_INTERFACE IGRPanel
	{
		// Typically called after a widget resize when the parent need not ask the child to resize itself
		// It marks the layout as having been computed
		virtual void ConfirmLayout() = 0;

		// Enumerate the panel and its ancestors for a colour, if none found returns the second argument (which defaults to bright red).
		virtual RGBAb GetColour(ESchemeColourSurface surface, RGBAb defaultColour = RGBAb(255,0,0,255)) const = 0;

		// Enumerate the panel and its ancestors for a colour
		virtual bool TryGetColour(ESchemeColourSurface surface, RGBAb& colour) const = 0;

		// Creates a local visual scheme if one does not exist, then maps a colour to the local scheme.
		virtual IGRPanel& Set(ESchemeColourSurface surface, RGBAb colour) = 0;

		virtual void MarkForDelete() = 0;
		virtual bool IsMarkedForDeletion() const = 0;
		virtual IGRPanel* Parent() = 0;
		virtual EventRouting NotifyAncestors(WidgetEvent& widgetEvent, IGRWidget& widget) = 0;
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
		virtual IGRPanel& Set(GRAnchors anchors) = 0;
		virtual IGRPanel& Add(GRAnchors anchors) = 0;
		virtual IGRPanel& Set(GRAnchorPadding padding) = 0;

		// Indicates that the layout needs to be recomputed. If the argument is true then the layout of the ancestors are also marked to be recomputed
		virtual void InvalidateLayout(bool invalidateAnscestors) = 0;

		// Indicates the layout has yet to be finalized
		virtual bool RequiresLayout() const = 0;

		virtual void Focus() = 0;
		virtual bool HasFocus() const = 0;
	};

	// Interface used internally by the GUI retained implementation. Clients of the API only see IGRPanel(s)
	ROCOCO_INTERFACE IGRPanelSupervisor : IGRPanel
	{
		// A dangerous function, particularly if called within a recursive query. Ensure it is not called on children that are referenced in the callstack
		virtual void ClearChildren() = 0;
		virtual void GarbageCollectRecursive() = 0;
		virtual void LayoutRecursive(Vec2i absoluteOrigin) = 0;
		virtual void RenderRecursive(IGRRenderContext & g) = 0;
		virtual EventRouting RouteCursorClickEvent(CursorEvent& ce, bool filterChildrenByParentRect) = 0;
		virtual EventRouting RouteCursorMoveEvent(CursorEvent& ce) = 0;
		virtual void SetWidget(IGRWidget& widget) = 0;
		virtual void Free() = 0;
	};


	ROCOCO_GUI_RETAINED_API void LayoutChildByAnchors(IGRPanel& child, const GuiRect& parentDimensions);
	ROCOCO_GUI_RETAINED_API void LayoutChildrenByAnchors(IGRPanel& parent, const GuiRect& parentDimensions);

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
		virtual Vec2i EvaluateMinimalSpan() const = 0;
		virtual void Layout(const GuiRect& parentDimensions) = 0;
		virtual EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) = 0;
		virtual EventRouting OnCursorClick(CursorEvent& ce) = 0;
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

	ROCOCO_INTERFACE IGRWidgetDivision : IGRWidget
	{

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

	ROCOCO_INTERFACE IGRWidgetSplitter : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRWidget& Widget() = 0;

		virtual IGRWidgetDivision& First() = 0;
		virtual IGRWidgetDivision& Second() = 0;
		virtual IGRWidgetSplitter& SetDraggerMinMax(int32 minValue, int32 maxValue) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetCollapser: IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual IGRWidget& Widget() = 0;
		virtual IGRWidgetDivision& ClientArea() = 0;

		// The collapser button is on the left side, so it is recommended to right align any additions and give enough room for the collapser to work
		virtual IGRWidgetDivision& TitleBar() = 0;
	};

	// The public api side of the main frame
	ROCOCO_INTERFACE IGRMainFrame
	{
		// Retrieves a reference to the frame's top menu bar. If one does not exist, it is created		
		virtual IGRWidgetMenuBar& MenuBar() = 0;

		// Retrieves a reference to the frame's top right tool bar. Typically used for minimize, maximume/restore and close window icons.
		virtual IGRWidgetToolbar& TopRightHandSideTools() = 0;

		// The part of the main frame that is below the title bar. If there is no title bar the client area covers the entire area
		virtual IGRWidgetDivision& ClientArea() = 0;
	};

	// The widget side of the main frame
	ROCOCO_INTERFACE IGRWidgetMainFrame : IGRWidget
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual IGRMainFrame& Frame() = 0;
	};

	// Highest level of the retained GUI manages frames, frame render order, event routing, visibility, building and rendering
	ROCOCO_INTERFACE IGuiRetained
	{
		// Associates a frame with an id and returns it. If it already exists, gets the existant one.
		virtual IGRMainFrame& BindFrame(IdWidget id) = 0;

		// Deletes the frame with the given id, invalidating all references to the frame and its panel and its layout
		virtual void DeleteFrame(IdWidget id) = 0;

		// Get a frame associated with an id. If none exist, null is returned
		virtual IGRMainFrame* FindFrame(IdWidget id) = 0;

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
	};

	ROCOCO_INTERFACE IGuiRetainedSupervisor : IGuiRetained
	{
		virtual void NotifyPanelDeleted(int64 uniqueId) = 0;
		virtual void Free() = 0;
	};

	enum GRErrorCode
	{
		None,
		Generic,
		InvalidArg,
		RecursionLocked
	};

	ROCOCO_INTERFACE IGRWidgetVerticalScroller : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual IGRWidget& Widget() = 0;

		virtual void SetPosition(int32 position) = 0;
		virtual void SetRange(int32 range) = 0;
		virtual void SetWindowSize(int32 domain) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetVerticalList : IGRWidget
	{

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


	// Implemented by an editor widget, IGRCustodian will map key events to the method calls here to adjust the content of an editor 
	ROCOCO_INTERFACE IGREditorMicromanager
	{
		virtual void AddToCaretPos(int32 delta) = 0;
		virtual void AppendCharAtCaret(char c) = 0;
		virtual void BackspaceAtCaret() = 0;
		virtual int32 CaretPos() const = 0;
		virtual void DeleteAtCaret() = 0;
		virtual void Return() = 0;
		virtual int32 GetTextAndLength(char* buffer, int32 receiveCapacity) const = 0;
	};

	struct WidgetEvent_EditorUpdated : WidgetEvent
	{
		IGRWidgetEditBox* editor;
		IGREditorMicromanager* manager;
		int32 caretPos;
	};

	// The platform dependent implementation of the custodian handles events and routes to the UI appropriately
	ROCOCO_INTERFACE IGRCustodian
	{
		// The caller will grab the reference to the memento and is responsible for calling IImageMemento->Free() when the memento is no longer used.
		virtual IImageMemento* CreateImageMemento(cstr imagePath) = 0;

		// Takes a platform interpreted key event and translates to an editor delta event
		virtual void TranslateToEditor(const KeyEvent& keyEvent, IGREditorMicromanager& manager) = 0;

		// Given a font id and text string, uses the platform font definition to determine the minimam span containing it.
		virtual Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring & text) const = 0;
		virtual EventRouting OnGREvent(WidgetEvent& ev) = 0;
		virtual void RaiseError(GRErrorCode code, cstr function, cstr message) = 0;
	};

	ROCOCO_INTERFACE IGRCustodianSupervisor : IGRCustodian
	{
		virtual void Free() = 0;
	};

	struct GRConfig
	{
		int32 unused = 0;
	};

	enum class GRPaths: int32
	{
		// This matches Windows and Unix MAX_PATH.
		MAX_FULL_PATH_LENGTH = 260
	};

	// This is the key factory function that creates the Gui system. The custodian handles the platform dependent side of the GUI.
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GRConfig& config, IGRCustodian& custodian);

	// Factory functions for creating widgets. All call IGuiRetained::AddWidget(...) to add themselves to the GUI
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetDivision& CreateDivision(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalList& CreateVerticalList(IGRWidget& parent, bool enforcePositiveChildHeights = true);
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateMenuButton(IGRWidget& parent, bool forSubmenu = false);
	ROCOCO_GUI_RETAINED_API IGRWidgetToolbar& CreateToolbar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetText& CreateText(IGRWidget& parent);
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


	// Rendering functions used by the widget implementations to create a standardized appearance across the UI
	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IScheme& scheme);

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

	ROCOCO_GUI_RETAINED_API bool DoInterfaceNamesMatch(cstr a, cstr b);

	// Query to see if the particular interface is part of the supplied instance. Will only compile if there is an elementary derivation of GR_TARGET_INTERFACE from GRBASED_CLASS.
	template<typename GR_TARGET_INTERFACE, class GRBASED_CLASS> inline EQueryInterfaceResult QueryForParticularInterface(GRBASED_CLASS* instance, IGRBase** ppOutputArg, cstr interfaceId)
	{
		if (ppOutputArg) *ppOutputArg = nullptr;

		if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;

		if (DoInterfaceNamesMatch(interfaceId, GR_TARGET_INTERFACE::InterfaceId()))
		{
			auto* target = static_cast<GR_TARGET_INTERFACE*>(instance);
			if (target)
			{
				if (ppOutputArg) *ppOutputArg = target;
				return EQueryInterfaceResult::SUCCESS;
			}
		}

		return EQueryInterfaceResult::NOT_IMPLEMENTED;
	}
}
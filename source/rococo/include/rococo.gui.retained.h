#pragma once
#include <rococo.types.h>

#ifndef ROCOCO_GUI_RETAINED_API
#define ROCOCO_GUI_RETAINED_API __declspec(dllimport) 
#endif

namespace Rococo
{
	struct KeyboardEvent;
	struct MouseEvent;
}

namespace Rococo::Game::Options
{
	DECLARE_ROCOCO_INTERFACE IBoolInquiry;
	DECLARE_ROCOCO_INTERFACE IChoiceInquiry;
	DECLARE_ROCOCO_INTERFACE IScalarInquiry;
	DECLARE_ROCOCO_INTERFACE IGameOptions;
}

namespace Rococo::Sex
{
	DECLARE_ROCOCO_INTERFACE ISExpression;
	typedef const ISExpression& cr_sex;
}

namespace Rococo::Gui
{
	DECLARE_ROCOCO_INTERFACE IGRImage;
	DECLARE_ROCOCO_INTERFACE IGRImageSupervisor;
	DECLARE_ROCOCO_INTERFACE IGRPanel;
	DECLARE_ROCOCO_INTERFACE IGRWidget;
	DECLARE_ROCOCO_INTERFACE IGRWidgetSupervisor;
	DECLARE_ROCOCO_INTERFACE IGRSystem;
	DECLARE_ROCOCO_INTERFACE IGRWidgetEditBox;
	DECLARE_ROCOCO_INTERFACE IGRCustodian;
	DECLARE_ROCOCO_INTERFACE IGRWidgetViewport;
	DECLARE_ROCOCO_INTERFACE IGRPanelWatcher;
	DECLARE_ROCOCO_INTERFACE IGRWidgetScrollableMenu;

#pragma pack(push, 1)
	struct GRCursorClick
	{
		uint16 LeftButtonDown : 1;
		uint16 LeftButtonUp : 1;
		uint16 RightButtonDown : 1;
		uint16 RightButtonUp : 1;
		uint16 MidButtonDown : 1;
		uint16 MidButtonUp : 1;
		uint16 xButton1Down : 1;
		uint16 xButton1Up : 1;
		uint16 xButton2Down : 1;
		uint16 xButton2Up : 1;
		uint16 MouseVWheel : 1; // Vertical wheel
		uint16 MouseHWheel : 1; // Horizontal wheel
		uint16 Unknown : 4;
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

	enum class EGRCursorIcon
	{
		Unspecified,
		Invisible,
		Arrow,
		LeftAndRightDragger
	};

	struct GRCursorEvent
	{
		IGREventHistory& history;
		const Vec2i position;
		const int64 eventId;
		const GRCursorClick click;
		EGRCursorIcon nextIcon;
		int32 wheelDelta;
	};

	struct GRKeyEvent
	{
		IGREventHistory& history;
		const int64 eventId;
		const KeyboardEvent& osKeyEvent;
	};
#pragma pack(pop)

	struct GRIdWidget
	{
		cstr Name;
	};

	enum class EGREventRouting
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

	enum class EGRAlignment : uint64
	{
		None = 0,
		Left = 1,
		Right = 2,
		Top = 4,
		Bottom = 8,
		HCentre = 3,
		VCentre = 12
	};

	enum class GRFontId: size_t
	{
		NONE = 0,
		MENU_FONT = 1, // THE DEFAULT FONT, used for menus and tabs
	};

	struct GRAlignmentFlags
	{
		int32 alignmentFlags = 0;

		GRAlignmentFlags() {}

		ROCOCO_GUI_RETAINED_API GRAlignmentFlags(cstr textRepresentation);

		GRAlignmentFlags& Add(EGRAlignment alignment)
		{
			alignmentFlags |= (int32)alignment;
			return *this;
		}

		GRAlignmentFlags& Remove(EGRAlignment alignment)
		{
			alignmentFlags &= ~(int32)alignment;
			return *this;
		}

		bool HasAllFlags(EGRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) == (int32)alignment;
		}

		bool HasSomeFlags(EGRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) != 0;
		}

		bool IsHCentred() const
		{
			return HasAllFlags(EGRAlignment::HCentre) || !HasSomeFlags(EGRAlignment::HCentre);
		}

		bool IsVCentred() const
		{
			return HasAllFlags(EGRAlignment::VCentre) || !HasSomeFlags(EGRAlignment::VCentre);
		}
	};

	enum class ECharSet
	{
		ANSI = 0
	};

	struct FontSpec
	{
		ECharSet CharSet = ECharSet::ANSI;
		cstr FontName = nullptr;
		bool Italic = false;
		bool Bold = false;
		bool Underlined = false;
		int CharHeight = 12;
	};

	ROCOCO_INTERFACE IGRFonts
	{
		virtual GRFontId BindFontId(const FontSpec & desc) = 0;
		virtual int GetFontHeight(GRFontId id) const = 0;
	};

	ROCOCO_INTERFACE IGRImages
	{
		// The caller will grab the reference to the image and is responsible for calling IGRImage->Free() when the image is no longer used.
		// The debug hint may be used in error message to help narrow down the source of the error. The error message will typically display the imagePath
		 virtual IGRImageSupervisor* CreateImageFromPath(cstr debugHint, cstr imagePath) = 0;
	};

	struct CaretSpec
	{
		// Either inserting (true) or overwriting (false)
		bool IsInserting = false;
		int32 CaretPos = 0; // Number of characters from left of string at which caret should be rendered
		RGBAb CaretColour1;
		RGBAb CaretColour2;

		// Blinks per second: <= 0 use caret colour1, > 10 use caret colour 2, and between 1 and 10, alternative between them at the specified blink rate
		int BlinksPerSecond = 0;

		// Height in pixels of the caret - should generally be determined by the size of the font. 1 is good for small fonts, large fonts should be 2+.
		int LineThickness = 1;
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

		virtual void DrawImageStretched(IGRImage& image, const GuiRect& absRect) = 0;
		virtual void DrawImageUnstretched(IGRImage& image, const GuiRect& absRect, GRAlignmentFlags alignment) = 0;

		virtual void DrawRect(const GuiRect& absRect, RGBAb colour) = 0;
		virtual void DrawRectEdge(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour) = 0;

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		virtual void DrawRectEdgeLast(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour) = 0;

		virtual void DrawEditableText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, const CaretSpec& caret) = 0;
		virtual void DrawText(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) = 0;

		// Causes all render operations to complete
		virtual void Flush() = 0;

		virtual void EnableScissors(const GuiRect& scissorRect) = 0;
		virtual void DisableScissors() = 0;
		virtual bool TryGetScissorRect(GuiRect& scissorRect) const = 0;

		virtual IGRFonts& Fonts() = 0;
		virtual IGRImages& Images() = 0;
	};

	enum class EGRSchemeColourSurface
	{
		BACKGROUND,
		BUTTON,
		BUTTON_EDGE_TOP_LEFT,
		BUTTON_EDGE_BOTTOM_RIGHT,
		BUTTON_IMAGE_FOG, // Colour, typically with mid alpha values that fogs out an image when it is not activated
		BUTTON_TEXT,
		MENU_BUTTON,
		MENU_BUTTON_EDGE_TOP_LEFT,
		MENU_BUTTON_EDGE_BOTTOM_RIGHT,
		MENU_BUTTON_TEXT,
		CONTAINER_BACKGROUND,
		CONTAINER_TOP_LEFT,
		LABEL_BACKGROUND,
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
		SLIDER_BACKGROUND,
		SLIDER_SLOT_BACKGROUND,
		EDITOR,
		READ_ONLY_TEXT,
		TEXT,
		GAME_OPTION_BACKGROUND, // Gives the background colour in a game option title
		CAROUSEL_BACKGROUND, // Gives the background colour of a carousel inner rect
		CAROUSEL_TEXT, // Gives the text colour of a carousel
		GAME_OPTION_TEXT, // Gives the text colour in a game option title
		GAME_OPTION_DISABLED_BACKGROUND, // // Gives the background colour in a disabled game option
		GAME_OPTION_DISABLED_TEXT, // Gives the text colour in a game option title when the option is not currently selectable
		NAME_TEXT, // Gives the label colour in a name-value pair
		VALUE_TEXT, // For name-value pairs, specifies the font colour for values
		FOCUS_RECTANGLE, // The rectangle surrounding a control to indicate focus, separate from the controls own focus settings
		EDIT_TEXT, // Text colour when editor box is focused
		SPLITTER_BACKGROUND,
		SPLITTER_EDGE,
		ROW_COLOUR_ODD, // Some tables use odd and even colour rows, this sets the colour for the odd rows.
		ROW_COLOUR_EVEN, // See ROW_COLOUR_ODD,
		COLLAPSER_TITLE_DEPTH_EVEN, // If supported by the collapser tree, colours collapser title backgrounds with even depth
		COLLAPSER_TITLE_DEPTH_ODD, // If supported by the collapser tree, colours collapser title backgrounds with odd depth
		COLLAPSER_TITLE_TEXT, // If supported by the collapser tree, colours collapser text
		GAME_OPTION_TOP_LEFT,
		GAME_OPTION_BOTTOM_RIGHT,
		USER_DEFINED_START_INDEX = 7000 // Make this the last index, then users can cast a surface to this enum + delta of their choice
	};

	// GRRenderState -> int32 combination of state bits. Pass by value rather than constant reference
	struct GRRenderState
	{
		struct Bits
		{
			uint32 pressed : 1;
			uint32 hovered : 1;
			uint32 focused : 1;
		};

		union Value
		{
			Bits bitValues;
			uint32 intValue;
		} value;

		GRRenderState(bool pressed, bool hovered, bool focused)
		{
			value.intValue = 0;
			value.bitValues.focused = focused;
			value.bitValues.hovered = hovered;
			value.bitValues.pressed = pressed;
		}

		template<class T> static void ForEachPermutation(T t)
		{
			t(GRRenderState(false, false, false));
			t(GRRenderState(true, false, false));
			t(GRRenderState(false, true, false));
			t(GRRenderState(true, true, false));
			t(GRRenderState(false, false, true));
			t(GRRenderState(true, false, true));
			t(GRRenderState(false, true, true));
			t(GRRenderState(true, true, true));
		}
	};

	ROCOCO_GUI_RETAINED_API void CopyColour(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface, GRRenderState rs);
	ROCOCO_GUI_RETAINED_API void CopyAllColours(IGRPanel& src, IGRPanel& target, EGRSchemeColourSurface srcSurface, EGRSchemeColourSurface trgSurface);


	inline GRRenderState GRRenderState_HoveredOnly() { return GRRenderState(false, true, false); }

	// Passed to SetColour(...) methods to indicate that the scheme should automatically generate colour intensities for the various state combinations by varying some colour parameter supplied to the API call
	inline GRRenderState GRGenerateIntensities() { return GRRenderState(false, false, false); }

	ROCOCO_INTERFACE IGRScheme
	{
		virtual RGBAb GetColour(EGRSchemeColourSurface surface, GRRenderState state) const = 0;
		virtual void SetColour(EGRSchemeColourSurface surface, RGBAb colour, GRRenderState state) = 0;
		virtual bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRRenderState state) const = 0;
	};

	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRScheme& scheme, EGRSchemeColourSurface surface, RGBAb colour);
	ROCOCO_GUI_RETAINED_API void SetUniformColourForAllRenderStates(IGRPanel& panel, EGRSchemeColourSurface surface, RGBAb colour);

	ROCOCO_INTERFACE IGRSchemeSupervisor : IGRScheme
	{
		virtual void Free() = 0;
	};

	IGRSchemeSupervisor* CreateGRScheme();

	enum class EGRWidgetEventType
	{
		BUTTON_CLICK,
		CHOICE_MADE, // A choice was selected, the meta data contains the key
		EDITOR_UPDATED, // Cast WidgetEvent to WidgetEvent_EditorUpdated
		DROP_DOWN_COLLAPSED, // The drop down control collapsed
		DROP_DOWN_EXPANDED, // The drop down control expanded
		BUTTON_CLICK_OUTSIDE, // A control captured a mouse click outside of its panel's AbsRect
		SCROLLER_RELEASED, // A scroll button was released by letting go of the mouse button 
		UPDATED_CLIENTAREA_HEIGHT, // A viewport client-area control calculated its new height (passed to iMetaData). The viewport caches this and applies it during the next layout
		USER_DEFINED = 1025
	};

	enum class EGREditorEventType: int
	{
		ClickFocused,
		CharAppended,
		CharBackspaced,
		CharDeleted,
		SetText,

		// The user clicks return or selects another control, usually treated as committing the edit
		LostFocus
	};

	struct GRWidgetEvent
	{
		EGRWidgetEventType eventType; // What kind of event this is
		int64 panelId;			// Valid as long as the underlying widget that sent the event is not destroyed
		int64 iMetaData;		// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		cstr sMetaData;			// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		Vec2i clickPosition;	// Cursor location when the event was triggered. If irrelevant is undefined
		bool isCppOnly;			// True if C++ is expected to handle the event, false if script languages hosted by C++ are expected to handle it
	};

	struct IGRCustodian;

	struct GRRealtimeConfig
	{
		// Typically -1 is correct under Windows, 1 indicates to scroll in the opposite direction. The OS may independently allow the user to flip the scroll direction
		int32 VerticalScrollerWheelScaling = -1;
	};

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
		virtual IGRScheme& Scheme() = 0;

		// the Gui Retained object that houses the root
		virtual IGRSystem& GR() = 0;

		virtual Vec2i ScreenDimensions() const = 0;
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

	// Contains useful bit states for each panel in the retained gui
	enum class EGRPanelFlags: int64
	{
		None = 0,
		AcceptsFocus = 1,
		// using tab to navigate a panel's children cycles through to the first if the final one is already focused
		CycleTabsEndlessly = 2,

		// The contents of the panel could be grayed or fogged for a better UI experience
		HintObscure = 4
	};

	// The base class from which queriable interfaces are derived. Used by QueryInterface methods herein
	ROCOCO_INTERFACE IGRBase
	{

	};

	ROCOCO_INTERFACE IGRFocusNotifier: IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual void OnDeepChildFocusSet(int64 panelId) = 0;
	};

	enum class ELayoutDirection
	{
		// With none, the children are expected to have their parent offset assigned manually
		None,

		// The parent modifies the children's parent offsets so that widgets are laid out from left to right
		LeftToRight,

		// The parent modifies the children's parent offsets so that widgets are laid out from right to left
		RightToLeft,

		// The parent modifies the children's parent offsets so that widgets are laid out from top to bottom
		TopToBottom,

		// The parent modifies the children's parent offsets so that widgets are laid out from bottom to top
		BottomToTop
	};

	// Represents the underlying widget slot. This is a better mechanism than having a base widget, which imposes class derivation issues
	ROCOCO_INTERFACE IGRPanel
	{
		// Append a useful debugging string to the builder
		virtual void AppendDesc(Strings::StringBuilder& sb) = 0;

		// Assign a useful debugging string to the panel.
		virtual void SetDesc(cstr text) = 0;

		// Enumerate the panel and its ancestors for a colour, if none found returns the second argument (which defaults to bright red).
		virtual RGBAb GetColour(EGRSchemeColourSurface surface, GRRenderState state, RGBAb defaultColour = RGBAb(255,0,0,255)) const = 0;

		// Enumerate the panel and its ancestors for a colour
		virtual bool TryGetColour(EGRSchemeColourSurface surface, RGBAb& colour, GRRenderState state) const = 0;

		// Creates a local visual scheme if one does not exist, then maps a colour to the local scheme.
		virtual IGRPanel& Set(EGRSchemeColourSurface surface, RGBAb colour, GRRenderState state) = 0;

		virtual int32 ChildPadding() const = 0;
		virtual IGRPanel& SetChildPadding(int32 delta) = 0;

		virtual void MarkForDelete() = 0;
		virtual bool IsMarkedForDeletion() const = 0;
		virtual IGRPanel* Parent() = 0;
		virtual EGREventRouting NotifyAncestors(GRWidgetEvent& widgetEvent, IGRWidget& widget) = 0;
		virtual IGRWidget& Widget() = 0;
		virtual IGRPanel& Resize(Vec2i span) = 0;

		// The (dx, dy) offset delta from the top left of the parent to the top left of the child
		virtual IGRPanel& SetParentOffset(Vec2i offset) = 0;
		// Returns the child with the given index. If the index does not map to a child it returns nullptr
		virtual IGRPanel* GetChild(int32 index) = 0;
		virtual Vec2i Span() const = 0;
		virtual Vec2i ParentOffset() const = 0;
		virtual IGRPanelRoot& Root() const = 0;
		virtual IGRPanel& AddChild() = 0;
		// If callback is not null then is invoked for each child. The number of children is returned
		virtual int32 EnumerateChildren(IEventCallback<IGRPanel>* callback) = 0;
		virtual int64 Id() const = 0;
		virtual GuiRect AbsRect() const = 0;
		virtual IGRPanel& CaptureCursor() = 0;
		virtual GRAnchorPadding Padding() = 0;

		// Overwrites the padding for an anchor
		virtual IGRPanel& Set(GRAnchorPadding padding) = 0;

		virtual IGRPanel& Add(EGRPanelFlags flag) = 0;

		virtual bool HasFlag(EGRPanelFlags flag) const = 0;

		virtual IGRPanel& Remove(EGRPanelFlags flag) = 0;

		// Returns the boolean collapsed state
		virtual bool IsCollapsed() const = 0;

		// Retrieve minimal span
		virtual Vec2i MinimalSpan() const = 0;

		virtual IGRPanel& Focus() = 0;
		virtual bool HasFocus() const = 0;

		virtual void SetClipChildren(bool enabled) = 0;
		virtual bool DoesClipChildren() const = 0;

		// Get extra rendering before and after widget rendering for the panel
		virtual IGRPanelRenderer* GetPanelRenderer() = 0;

		// Sets the boolean collapsed state. If collapsed a panel and its descendants will not be rendered or laid out
		virtual void SetCollapsed(bool isCollapsed) = 0;

		virtual void SetRenderLast(bool isRenderingLast) = 0;

		// Assign minimal span
		virtual void SetMinimalSpan(Vec2i span) = 0;

		virtual const Sex::ISExpression* GetAssociatedSExpression() const = 0;

		virtual void SetAssociatedSExpression(Sex::cr_sex s) = 0;

		// For some control clipping is done by a panel other than itself. This method allows the clipping panel to be selected
		// Clipping panels must have SetClipChildren(true) to be effective. If null, no clipping takes place
		virtual void SetClippingPanel(IGRPanel* panel) = 0;

		// Add extra rendering before and after widget rendering for the panel
		virtual IGRPanel& SetPanelRenderer(IGRPanelRenderer* renderer) = 0;

		virtual IGRPanel& SetLayoutDirection(ELayoutDirection direction) = 0;

		virtual IGRPanel& SetFitChildrenHorizontally() = 0;

		virtual IGRPanel& SetFitChildrenVertically() = 0;

		virtual IGRPanel& SetConstantWidth(int width) = 0;

		virtual IGRPanel& SetConstantHeight(int height) = 0;

		virtual IGRPanel& SetConstantSpan(Vec2i span) = 0;

		virtual IGRPanel& SetExpandToParentHorizontally() = 0;

		virtual IGRPanel& SetExpandToParentVertically() = 0;

		// Assigns a new panel watcher, and returns the address of the last one. Can return null if none was assigned (the default)
		// Mainly used to debug sizing operations
		virtual IGRPanelWatcher* SetPanelWatcher(IGRPanelWatcher* newWatcher) = 0;

		virtual EGREventRouting RouteToParent(GRWidgetEvent& ev) = 0;
	};

	// Interface used internally by the GUI retained implementation. Clients of the API only see IGRPanel(s)
	ROCOCO_INTERFACE IGRPanelSupervisor : IGRPanel
	{
		// A dangerous function, particularly if called within a recursive query. Ensure it is not called on children that are referenced in the callstack
		virtual void ClearChildren() = 0;
		virtual void GarbageCollectRecursive() = 0;
		virtual void Layout() = 0;
		virtual void RenderRecursive(IGRRenderContext& g, const GuiRect& clipRect, bool isRenderingFirstLayer) = 0;
		virtual EGREventRouting RouteCursorClickEvent(GRCursorEvent& ce, bool filterChildrenByParentRect) = 0;
		virtual void BuildWidgetCallstackRecursiveUnderPoint(Vec2i point, IGRPanelEventBuilder& wb) = 0;
		virtual void BuildCursorMovementHistoryRecursive(GRCursorEvent& ce, IGRPanelEventBuilder& wb) = 0;
		virtual void SetWidget(IGRWidgetSupervisor& widget) = 0;
		virtual void ReleasePanel() = 0;
	};

	enum class EGRQueryInterfaceResult
	{
		SUCCESS,
		NOT_IMPLEMENTED,
		INVALID_ID
	};

	ROCOCO_INTERFACE IGRWidget : IGRBase
	{
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) = 0;

		virtual cstr GetImplementationTypeName() const = 0;
	};

	inline bool operator == (IGRWidget& src, IGRWidget& target)
	{
		return &src == &target;
	}

	inline bool operator != (IGRWidget& src, IGRWidget& target)
	{
		return &src != &target;
	}

	ROCOCO_INTERFACE IGRPanelWatcher : IGRBase
	{
		virtual void OnSetConstantHeight(IGRPanel& panel, int height) = 0;
		virtual void OnSetConstantWidth(IGRPanel& panel, int width) = 0;
		virtual void OnSetAbsRect(IGRPanel& panel, const GuiRect& absRect) = 0;
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	ROCOCO_INTERFACE IGRWidgetManager : IGRWidget
	{
		virtual EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget) = 0;
		virtual EGREventRouting OnCursorClick(GRCursorEvent& ce) = 0;
		virtual void OnCursorEnter() = 0;
		virtual void OnCursorLeave() = 0;
		virtual EGREventRouting OnCursorMove(GRCursorEvent& ce) = 0;
		virtual EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) = 0;

		// Invoked by the IGRRetained render call
		virtual void Render(IGRRenderContext& g) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetLayout : IGRBase
	{
		virtual void LayoutBeforeFit() = 0;
		virtual void LayoutBeforeExpand() = 0;
		virtual void LayoutAfterExpand() = 0;

		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	ROCOCO_INTERFACE IGRWidgetSupervisor: IGRWidgetManager
	{
		// Invoked by the IGRRetained instance management logic
		virtual void Free() = 0;
	};

	template<class CAST_TO_THIS_INTERFACE> inline CAST_TO_THIS_INTERFACE* Cast(IGRWidget& widget, cstr interfaceId)
	{
		IGRBase* castBase = nullptr;
		auto result = widget.QueryInterface(&castBase, interfaceId);
		return result == EGRQueryInterfaceResult::SUCCESS ? static_cast<CAST_TO_THIS_INTERFACE*>(castBase) : nullptr;
	};

	template<class CAST_TO_THIS_CLASS> inline CAST_TO_THIS_CLASS* Cast(IGRWidget& widget)
	{
		return Cast<CAST_TO_THIS_CLASS>(widget, CAST_TO_THIS_CLASS::InterfaceId());
	};

	enum class EGRClickCriterion
	{
		OnDown, // Click event fires when the left button down event is routed to the button widget (THIS IS THE DEFAULT)
		OnUp, // Click event when the left button up event is routed to the button widget
		OnDownThenUp // Click event when the left button is down, the cursor remains in the button rectangle, and then the left button is released.
	};

	enum class EGREventPolicy
	{
		PublicEvent, // The widget notifies the GR custodian that an event has fired (THIS IS THE DEFAULT)
		NotifyAncestors, // The widget notifies the chain of parents that an event has fired. If nothing terminates it, the root panel makes it a public event		
	};

	struct GRControlMetaData
	{
		int64 intData = 0;
		cstr stringData = nullptr; // If nullptr is provided, the meta data is stored as an empty string and retrieved as an empty string

		static GRControlMetaData None()
		{
			return { 0,nullptr };
		}
	};

	struct GRButtonFlags
	{
		uint32 isMenu : 1;
		uint32 forSubMenu : 1;
		uint32 isEnabled : 1;
		uint32 isRaised : 1;
	};

	// The platform independent view of the platform dependent image associated with some widget
	ROCOCO_INTERFACE IGRImage
	{
		virtual bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& rc) = 0;
		virtual Vec2i Span() const = 0;
	};

	ROCOCO_INTERFACE IGRImageSupervisor : IGRImage
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetText : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual void FitTextH() = 0;
		virtual void FitTextV() = 0;
		virtual [[nodiscard]] int TextWidth() const = 0;
		virtual IGRWidgetText& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;
		virtual IGRWidgetText& SetBackColourSurface(EGRSchemeColourSurface surface) = 0;
		virtual IGRWidgetText& SetFont(GRFontId fontId) = 0;
		virtual IGRWidgetText& SetText(cstr text) = 0;
		virtual IGRWidgetText& SetTextColourSurface(EGRSchemeColourSurface surface) = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetButton : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// Sets the rule by which events are fired
		virtual IGRWidgetButton& SetClickCriterion(EGRClickCriterion criterion) = 0;

		// Set what happens when the button is fired
		virtual IGRWidgetButton& SetEventPolicy(EGREventPolicy policy) = 0;

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
		virtual IGRWidgetButton& SetMetaData(const GRControlMetaData& metaData, bool isEventHandlerCPPOnly) = 0;

		// Sets the display text for the button
		virtual IGRWidgetButton& SetTitle(cstr text) = 0;

		virtual IGRWidgetButton& SetFontId(GRFontId id) = 0;

		virtual void FitTextHorizontally() = 0;
		virtual void FitTextVertically() = 0;

		// Gets the display text and returns its length. If the buffer is insufficient, the result is truncated
		virtual size_t GetTitle(char* titleBuffer, size_t nBytes) const = 0;

		virtual IGRWidgetButton& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;

		// Returns meta data set with SetMetaData. The pointers are valid until meta data is changed or the control is destroyed
		virtual [[nodiscard]] GRControlMetaData MetaData() = 0;

		virtual [[nodiscard]] GRButtonFlags ButtonFlags() const = 0;

		virtual void MakeToggleButton() = 0;

		virtual void SetStretchImage(bool isStretched) = 0;

		virtual [[nodiscard]] IGRPanel& Panel() = 0;

		virtual [[nodiscard]] Vec2i ImageSpan() const = 0;

		virtual void Toggle() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetCarousel : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual void AddOption(cstr name, cstr caption) = 0;
		virtual void Advance(int delta) = 0;
		virtual void SetActiveChoice(cstr name) = 0;
		virtual [[nodiscard]] IGRWidgetScrollableMenu& DropDown() = 0;
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual void SetDisableCarouselWhenDropDownVisible(bool isDisabledAccordingly) = 0;
		virtual void SetFont(GRFontId fontId) = 0;
		virtual void SetOptionPadding(GRAnchorPadding padding) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetScrollableMenu : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual void AddOption(cstr name, cstr caption) = 0;
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetSlider : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		virtual [[nodiscard]] double Max() const = 0;
		virtual [[nodiscard]] double Min() const = 0;

		virtual double Position() const = 0;

		// If minValue > maxValue then the order is reversed. If they match are are not finite, the bar is disabled
		virtual void SetRange(double minValue, double maxValue) = 0;

		// Represents the number of units to increment the position when an extremum button is clicked, or the left/right keys are used 
		virtual void SetQuantum(double quantum) = 0;

		// Sets the text output. Note that with GRFontId::NONE no guage is rendered
		virtual void SetGuage(GRFontId fontId, int decimalPlaces, EGRSchemeColourSurface surface) = 0;

		// Note that the true value is clamp of the supplied value using the range values
		virtual void SetPosition(double value) = 0;

		virtual void Advance(int quanta) = 0;

		virtual IGRWidgetSlider& SetImagePath(cstr imagePath) = 0;
		virtual IGRWidgetSlider& SetPressedImagePath(cstr imagePath) = 0;
		virtual IGRWidgetSlider& SetRaisedImagePath(cstr imagePath) = 0;
		virtual void SetGuageAlignment(GRAlignmentFlags alignment, Vec2i scalarGuageSpacing) = 0;
		virtual void SetSlotPadding(GRAnchorPadding padding) = 0;
	};

	struct GRMenuButtonItem
	{
		cstr text;
		GRControlMetaData metaData;
		uint32 isEnabled: 1;
		uint32 isImplementedInCPP : 1; // True if C++ is expected to handle the button event
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
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual bool AddButton(GRMenuItemId parentMenu, const GRMenuButtonItem& item) = 0;
		virtual void ClearMenus() = 0;
		virtual GRMenuItemId AddSubMenu(GRMenuItemId parentMenu, const GRMenuSubMenu& subMenu) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetFactory
	{
		virtual IGRWidget& CreateWidget(IGRPanel& panel) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetToolbar : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// If not left, then alignment is right
		virtual void SetChildAlignment(EGRAlignment alignment, int32 interChildPadding = 4, int32 borderPadding = 1) = 0;

		// Shrinks the children to their minimal size and resizes the control to fit in all children with specified padding, and returns the span
		// This may cause the control and its children to invoke InvalidateLayout on the containing panels
		virtual Vec2i ResizeToFitChildren() = 0;
	};

	// A widget with a rectangular background for holding child widgets
	ROCOCO_INTERFACE IGRWidgetDivision : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual void SetTransparency(float f = 1.0f) = 0;
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
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// Adds a new column and returns the column index of the new column
		virtual int32 AddColumn(const GRColumnSpec & spec) = 0;

		// Adds a new row and returns the row index of the new row
		virtual int32 AddRow(const GRRowSpec& spec) = 0;

		// Returns a reference to the cell at the given location. If the location indices are out of bounds, the method returns nullptr
		virtual IGRWidgetDivision* GetCell(int32 column, int32 row) = 0;

		// Compute table height in pixels
		virtual int EstimateHeight() const = 0;

		// Sets the pixel width of the specified column
		virtual void SetColumnWidth(int column, int width) = 0;
	};

	ROCOCO_INTERFACE IGRPropertyEditorPopulationEvents
	{
		virtual void OnAddNameValue(IGRWidgetText& nameWidget, IGRWidgetEditBox& editorWidget) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetPropertyEditorTree : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual IGRWidgetViewport& Viewport() = 0;
		virtual void SetRowHeight(int height) = 0;
		virtual void View(Reflection::IReflectionVisitation* visitation) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetGameOptions : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetGameOptionsBool : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual Game::Options::IBoolInquiry& Inquiry() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetGameOptionsChoice : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual Game::Options::IChoiceInquiry& Inquiry() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetGameOptionsScalar : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual Game::Options::IScalarInquiry& Inquiry() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetSplitter : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		virtual IGRWidgetDivision& First() = 0;
		virtual IGRWidgetDivision& Second() = 0;
		virtual IGRWidgetSplitter& SetDraggerMinMax(int32 minValue, int32 maxValue) = 0;
		virtual IEventImpressario<int>& EvOnSplitSizeChanged() = 0;
	};

	struct IGRWidgetCollapser;

	ROCOCO_INTERFACE IGRWidgetCollapserEvents
	{
		virtual void OnCollapserExpanded(IGRWidgetCollapser &collapser) = 0;
		virtual void OnCollapserInlined(IGRWidgetCollapser& collapser) = 0;
	};

	// A collapsable region with a client area and title area. The title area contains the collapse button. The client area will vanish when the collapse button is engaged
	ROCOCO_INTERFACE IGRWidgetCollapser: IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// The area under the collapser's title bar. Will be of zero area if the collapse button is engaged
		virtual [[nodiscard]] IGRWidgetDivision& ClientArea() = 0;

		virtual [[nodiscard]] bool IsCollapsed() const = 0;

		// If the argument is blank defaults to a default expansion icon macro
		virtual void SetExpandClientAreaImagePath(cstr path) = 0;


		// If the argument is blank defaults to a default inline icon macro
		virtual void SetCollapsedToInlineImagePath(cstr path) = 0;

		// The collapser button is on the left side, so it is recommended to right align any additions and give enough room for the collapser to work
		virtual [[nodiscard]] IGRWidgetDivision& TitleBar() = 0;

		// The spacer between the left edge of the title bar and the collapser button
		virtual [[nodiscard]] IGRWidgetDivision& LeftSpacer() = 0;

		virtual [[nodiscard]] IGRWidgetButton& CollapseButton() = 0;
	};

	// The main frame with menu, toolbar and client area beneath the title bar
	ROCOCO_INTERFACE IGRWidgetMainFrame : IGRBase
	{
		// The unique id associated with this interface
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();

		// The widget for the main frame
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// Retrieves a reference to the frame's top menu bar. If one does not exist, it is created		
		virtual [[nodiscard]] IGRWidgetMenuBar& MenuBar() = 0;

		// Retrieves a reference to the frame's top right tool bar. Typically used for minimize, maximize/restore and close window buttons.
		virtual [[nodiscard]] IGRWidgetToolbar& TopRightHandSideTools() = 0;

		// The part of the main frame that is below the title bar. If there is no title bar the client area covers the entire area
		virtual [[nodiscard]] IGRWidgetDivision& ClientArea() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetMainFrameSupervisor: IGRWidgetMainFrame
	{
		// The widget for the main frame
		virtual [[nodiscard]] IGRWidgetSupervisor& WidgetSupervisor() = 0;
	};

	enum class EGRDebugFlags
	{
		None = 0,
		ThrowWhenPanelIsZeroArea = 1
	};

	enum class EGRNavigationDirective
	{
		None = 0,
		Tab
	};

	ROCOCO_INTERFACE IGRNavigator : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual EGREventRouting OnNavigate(EGRNavigationDirective directive) = 0;
	};

	// Highest level of the retained GUI manages frames, frame render order, event routing, visibility, building and rendering
	ROCOCO_INTERFACE IGRSystem
	{
		// Apply a key to the system as a whole, this will occur if nothing consumed the key
		virtual void ApplyKeyGlobally(GRKeyEvent& keyEvent) = 0;

		// Associates a frame with an id and returns it. If it already exists, gets the existant one.
		virtual [[nodiscard]] IGRWidgetMainFrame& BindFrame(GRIdWidget id) = 0;

		// Deletes the frame with the given id, invalidating all references to the frame and its panel and its layout
		virtual void DeleteFrame(GRIdWidget id) = 0;

		// Returns true if at least one GRDebugFlag is present
		virtual [[nodiscard]] bool HasDebugFlag(EGRDebugFlags flag) const = 0;

		// Combination of GRDebugFlags to overwrite the current flag state
		virtual void SetDebugFlags(int grDebugFlags) = 0;

		// Get a frame associated with an id. If none exist, null is returned
		virtual [[nodiscard]] IGRWidgetMainFrame* FindFrame(GRIdWidget id) = 0;

		// Lower the frame so that it is the first to render.
		virtual void MakeFirstToRender(GRIdWidget id) = 0;

		// Raise the frame so that it is the final to render
		virtual void MakeLastToRender(GRIdWidget id) = 0;	

		// Free all panels marked for delete
		virtual void GarbageCollect() = 0;

		// Route posted messages to the event handler. This should be called periodically outside of any GR locked sections, such as GR event handlers or rendering routines
		virtual void DispatchMessages() = 0;
		
		// Renders the list of frames
		virtual void RenderAllFrames(IGRRenderContext& g) = 0;

		virtual [[nodiscard]] IGRPanelRoot& Root() = 0;

		// Invoked by widget factories to add widgets to the retained gui
		virtual [[nodiscard]] IGRWidget& AddWidget(IGRPanel& parent, IGRWidgetFactory& factory) = 0;

		// Constant time lookup of a widget with a given panel Id.
		virtual [[nodiscard]] IGRWidget* FindWidget(int64 panelId) = 0;

		// Set the visibility status. If invisible, it will ignore all input and not be rendered
		virtual void SetVisible(bool isVisible) = 0;

		// Returns true if the retained GUI is visible and there are frames to show, otherwise false
		virtual bool IsVisible() const = 0;

		virtual [[nodiscard]] int64 GetFocusId() const = 0;

		// Sets the keyboard focus to the id of a panel.
		virtual void SetFocus(int64 id = -1);

		virtual EGREventRouting RouteCursorClickEvent(GRCursorEvent& mouseEvent) = 0;
		virtual EGREventRouting RouteCursorMoveEvent(GRCursorEvent& mouseEvent) = 0;
		virtual EGREventRouting RouteKeyEvent(GRKeyEvent& keyEvent) = 0;

		// Get config
		virtual [[nodiscard]] const GRRealtimeConfig& Config() const = 0;

		// Get a mutable version of the config, used to configure the config
		virtual [[nodiscard]] GRRealtimeConfig& MutableConfig() = 0;

		virtual [[nodiscard]] IGRFonts& Fonts() = 0;
	};

	enum EGRErrorCode
	{
		None,
		BadAnchors,
		BadSpanHeight,
		BadSpanWidth,
		Generic,
		InvalidArg,
		RecursionLocked
	};

	struct GRScrollerMetrics
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
		virtual GRSliderSpec OnCalculateSliderRect(int32 scrollerSpan, IGRWidgetScroller& scroller) = 0;
		virtual void OnScrollLines(int delta, IGRWidgetScroller& scroller) = 0;
		virtual void OnScrollPages(int delta, IGRWidgetScroller& scroller) = 0;
		virtual void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetScroller : IGRBase
	{
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual GRScrollerMetrics GetMetrics() const = 0;
		virtual void SetSliderPosition(int32 topPixelDelta) = 0;
	};

	ROCOCO_INTERFACE IGRWidgetVerticalScroller : IGRWidgetScroller
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
	};

	ROCOCO_INTERFACE IGRWidgetVerticalScrollerWithButtons : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		virtual IGRWidgetButton& BottomButton() = 0;
		virtual IGRWidgetVerticalScroller& Scroller() = 0;
		virtual IGRWidgetButton& TopButton() = 0;
	};

	// A viewport is a rectangle adjacent to scrollbars that let the user navigate a larger visual domain.
	ROCOCO_INTERFACE IGRWidgetViewport : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual void SetDomainHeight(int32 heightInPixels) = 0;

		// scaleFactor good range is 0.5 to 1.0. 
		// With 0, page by page scrolling is disabled. 
		// With 1.0, page scrolling moves the scroll zone by the client height in pixels each invocation.
		// With 0.5, page scrolling moves the scroll zone half the client height in pixels, and so forth.
		// The scaling is clamped from 0 to 2.0. The default is 0.75
		virtual void SetMovePageScale(double scaleFactor) = 0;

		// Sets the number of pixels to scroll up or down when the vertical scroller buttons are clicked
		// Defaults to 10. 
		// At 0 line by line scrolling is disabled.
		// 1,000,000 is the maximum delta.
		// Values are clamped from 0 to the maximum
		virtual void SetLineDeltaPixels(int lineDeltaPixels) = 0;

		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
		virtual [[nodiscard]] IGRWidgetDivision& ClientArea() = 0;
		virtual [[nodiscard]] IGRWidgetVerticalScrollerWithButtons& VScroller() = 0;
	};

	// A vertical list that aligns its children vertically
	ROCOCO_INTERFACE IGRWidgetVerticalList : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;
	};

	ROCOCO_INTERFACE IGRWidgetEditBox : IGRBase
	{
		ROCOCO_GUI_RETAINED_API static cstr InterfaceId();
		virtual [[nodiscard]] IGRPanel& Panel() = 0;
		virtual [[nodiscard]] IGRWidget& Widget() = 0;

		// Returns length of the internal storage, which includes space for the trailing nul character. Never returns < 2, i.e there is always space for one character and a trailing nul
		virtual [[nodiscard]] size_t Capacity() const = 0;
		virtual IGRWidgetEditBox& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;
		virtual IGRWidgetEditBox& SetFont(GRFontId fontId) = 0;
		virtual IGRWidgetEditBox& SetReadOnly(bool isReadOnly) = 0;
		virtual IGRWidgetEditBox& SetMetaData(const GRControlMetaData& metaData) = 0;

		// Assigns a string to internal storage, truncating if needs be. If null is passed it is treated as an empty string
		virtual void SetText(cstr argText) = 0;

		// Copies the text to the buffer, truncating if needs be. Returns the length of the internal representation, which includes the trailing nul character. Never returns < 1.
		virtual int32 GetTextAndLength(char* buffer, int32 receiveCapacity) const = 0;

		// Returns true if and only if the box is in manual edit mode
		virtual [[nodiscard]] bool IsEditing() const = 0;
	};

	struct IGREditorMicromanager;

	struct GRWidgetEvent_EditorUpdated : GRWidgetEvent
	{
		IGRWidgetEditBox* editor;
		IGREditorMicromanager* manager;
		EGREditorEventType editorEventType;
		int32 caretPos;
	};

	struct IGRCustodian;

	struct GRConfig
	{
		int32 unused = 0;
	};

	enum class EGRPaths: int32
	{
		// This matches Windows and Unix MAX_PATH.
		MAX_FULL_PATH_LENGTH = 260
	};

	// Factory functions for creating widgets. All call IGuiRetained::AddWidget(...) to add themselves to the GUI
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetDivision& CreateDivision(IGRWidget& parent);

	struct PropertyEditorSpec
	{
		// The font to use for the name in a name-value pair
		GRFontId NameplateFontId = GRFontId::NONE;

		// The font to use for a table heading atop a sequence of name-value pairs
		GRFontId HeadingFontId = GRFontId::NONE;

		// The font to use for a value in a name-value pair
		GRFontId ValueFontId = GRFontId::NONE;
		
		// True displays nameplates with left alignment, otherwise right aligns them
		bool LeftAlignNameplates = false;

		// Number of pixels to shift nameplates right beneath their immediate heading
		int LeftHandMargin = 46;

		// Additional number of pixels to shift namesplaces right per point of collapser depth
		int LeftHandShiftPerDepth = 24; 

		// Number of pixels to expand the nameplate. 0 may be so tight that some cut off occurs and names get truncated
		int NamePlateSafeZone = 4;

		// Spacing between name cell rect and inner text
		Vec2i NameTextSpacing = { 4,2 };

		GRAnchorPadding NameCellPadding = { 4, 0, 0, 0 };

		Vec2i EditorCellPadding = { 8, 2 };

		GRAnchorPadding ValueCellPadding = { 0, 0, 0, 0 };

		int NameColumnDefaultWidth = 160;

		int ValueColumnDefaultWidth = 120;

		GRAnchorPadding CollapserPadding = { 0, 0, 0 , 0 };

		Vec2i CollapserButtonSpacing = { 6, 6 };

		GRAnchorPadding TitleDescPadding = { 0, 0, 0, 0 };

		// Number of pixels to scroll vertically. 0 disables, 1,000,000 is maximum.
		int LineDeltaPixels = 10;
	};

	struct GameOptionConfig
	{
		bool TitlesOnLeft = false; // If false, titles appear above controls, otherwise they appear to the left.
		double FontHeightToOptionHeightMultiplier = 1.0; // Multiplies the font height of a title to create the height of the option control. 1.0 < multiplier < 2.0 is good
		double TitleXSpacingMultiplier = 0.5;
		GRAlignmentFlags TitleAlignment;
		GRAlignmentFlags ScalarGuageAlignment;
		Vec2i ScalarGuageSpacing{ 0,0 };
		GRFontId TitleFontId = GRFontId::MENU_FONT;
		GRAnchorPadding CarouselPadding { 0,0,0,0 };
		GRAnchorPadding ScalarSlotPadding { 0,0,0,0 };
		cstr LeftImageRaised = "!textures/toolbars/MAT/previous.tif";
		cstr RightImageRaised = "!textures/toolbars/MAT/next.tif";
		cstr LeftImagePressed = "!textures/toolbars/MAT/previousHi.tif";
		cstr RightImagePressed = "!textures/toolbars/MAT/nextHi.tif";
		cstr ScalarKnobRaised = "!textures/toolbars/MAT/slider-knob.tif";
		cstr ScalarKnobPressed = "!textures/toolbars/MAT/slider-knobHi.tif";
		GRFontId CarouselFontId = GRFontId::MENU_FONT;
	};

	// Create a property tree editor. The instance of IGRWidgetPropertyEditorTreeEvents& has to be valid for the lifespan of the widget, or mark the widget panel for deletion when events can no longer be handled
	ROCOCO_GUI_RETAINED_API IGRWidgetPropertyEditorTree& CreatePropertyEditorTree(IGRWidget& parent, IGRPropertyEditorPopulationEvents& events, const PropertyEditorSpec& spec);
	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptions& CreateGameOptionsList(IGRWidget& parent, Game::Options::IGameOptions& options, const GameOptionConfig& config);
	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsBool& CreateGameOptionsBool(IGRWidget& parent, const GameOptionConfig& config);
	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsChoice& CreateGameOptionsChoice(IGRWidget& parent, const GameOptionConfig& config);
	ROCOCO_GUI_RETAINED_API IGRWidgetGameOptionsScalar& CreateGameOptionsScalar(IGRWidget& parent, const GameOptionConfig& config);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent, IGRScrollerEvents& events);
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScrollerWithButtons& CreateVerticalScrollerWithButtons(IGRWidget& parent, IGRScrollerEvents& events);

	// Creates a viewport into a larger UI domain, providing horizontal and vertical scrollbars to navigate the domain
	ROCOCO_GUI_RETAINED_API IGRWidgetViewport& CreateViewportWidget(IGRWidget& parent);

	// Creates a vertical list that aligns its children vertically
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalList& CreateVerticalList(IGRWidget& parent, bool enforcePositiveChildHeights = true);
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateMenuButton(IGRWidget& parent, bool forSubmenu = false);
	ROCOCO_GUI_RETAINED_API IGRWidgetCarousel& CreateCarousel(IGRWidget& parent, cstr leftImageRaised, cstr rightImageRaised, cstr leftImagePressed, cstr rightImagePressed);
	ROCOCO_GUI_RETAINED_API IGRWidgetScrollableMenu& CreateScrollableMenu(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetSlider& CreateSlider(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetToolbar& CreateToolbar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetText& CreateText(IGRWidget& parent);

	// Create a collapsable region with a client area and title area. The title area contains the collapse button. The client area will vanish when the collapse button is engaged
	// The events structure must be valid for the lifetime of the collapser
	ROCOCO_GUI_RETAINED_API IGRWidgetCollapser& CreateCollapser(IGRWidget& parent, IGRWidgetCollapserEvents& eventHandler);
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
	ROCOCO_GUI_RETAINED_API IGRWidgetEditBox& CreateEditBox(IGRWidget& parent, IGREditFilter* filter, int32 capacity = (int32) EGRPaths::MAX_FULL_PATH_LENGTH, GRFontId fontId = GRFontId::NONE);

	ROCOCO_GUI_RETAINED_API void SetSchemeColours_ThemeGrey(IGRScheme& scheme);

	ROCOCO_GUI_RETAINED_API void MakeTransparent(IGRPanel& panel, EGRSchemeColourSurface surface);

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

	ROCOCO_GUI_RETAINED_API EGREventRouting RouteEventToHandler(IGRPanel& panel, GRWidgetEvent& ev);

	ROCOCO_GUI_RETAINED_API bool IsCandidateDescendantOfParent(IGRPanel& parent, IGRPanel& candidate);

	// Recursively iterates through the panel and its children, will focus the first panel that has acceptsFocus flag set. Returns the new focus if successful, else nullptr
	ROCOCO_GUI_RETAINED_API IGRPanel* TrySetDeepFocus(IGRPanel& panel);

	ROCOCO_GUI_RETAINED_API IGRCustodian& GetCustodian(IGRPanel& panel);
	ROCOCO_GUI_RETAINED_API IGRCustodian& GetCustodian(IGRWidget& widget);

	ROCOCO_GUI_RETAINED_API void RaiseError(IGRPanel& panel, EGRErrorCode errCode, cstr function, const char* format, ...);

	ROCOCO_GUI_RETAINED_API void DrawEdge(EGRSchemeColourSurface topLeft, EGRSchemeColourSurface bottomRight, IGRPanel& panel, IGRRenderContext& rc);

	ROCOCO_GUI_RETAINED_API IGRWidgetText& AddGameOptionTitleWidget(IGRWidget& parentWidget, const GameOptionConfig& config);
}
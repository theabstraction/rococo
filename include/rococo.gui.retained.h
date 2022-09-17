#include <rococo.types.h>

#ifdef ROCOCO_GUI_RETAINED_EXPORTS
#define ROCOCO_GUI_RETAINED_API __declspec(dllexport) 
#else
#define  ROCOCO_GUI_RETAINED_API __declspec(dllimport) 
#endif

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
		uint16 Unknown : 1;
	};

	struct IGREventHistory
	{
		virtual void RecordWidget(IGRWidget& widget) = 0;
	};

	struct CursorEvent
	{
		IGREventHistory& history;
		const Vec2i position;
		const int64 eventId;
		const CursorClick click;
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

	ROCOCOAPI IGRLayout
	{
		virtual void Layout(IGRPanel& panel, const GuiRect& absRect) = 0;
	};

	ROCOCOAPI IGRLayoutSupervisor: IGRLayout
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

		bool HasAllFlags(GRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) == (int32)alignment;
		}

		bool HasSomeFlags(GRAlignment alignment) const
		{
			return (alignmentFlags & (int32)alignment) != 0;
		}
	};

	ROCOCOAPI IGRRenderContext
	{
		virtual Vec2i CursorHoverPoint() const = 0;
		// It is up to the renderer to decide if a panel is hovered.
		virtual bool IsHovered(IGRPanel& panel) const = 0;	
		virtual GuiRect ScreenDimensions() const = 0;

		virtual void DrawRect(const GuiRect& absRect, RGBAb colour) = 0;
		virtual void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;
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
		MENU_BUTTON_TEXT
	};

	ROCOCOAPI IScheme
	{
		virtual RGBAb GetColour(ESchemeColourSurface surface) = 0;
		virtual void SetColour(ESchemeColourSurface surface, RGBAb colour) = 0;
	};

	ROCOCOAPI ISchemeSupervisor : IScheme
	{
		virtual void Free() = 0;
	};

	ISchemeSupervisor* CreateScheme();

	enum WidgetEventType
	{
		BUTTON_CLICK,
		USER_DEFINED = 1024
	};

	struct WidgetEvent
	{
		WidgetEventType eventType; // What kind of event this is
		int64 panelId;			// Valid as long as the underlying widget that sent the event is not destroyed
		int64 iMetaData;		// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		cstr sMetaData;			// Valid as long as the underlying widget that sent the event is not destroyed, or its meta data changed
		Vec2i clickPosition;	// Cursor location when the event was triggered. If irrelevant is undefined
		size_t responseSize;	// The event responder can set the number of bytes in the response
		void* response;			// the event responder can point this to a buffer that contains the response
	};

	struct IGRCustodian;

	ROCOCOAPI IGRPanelRoot
	{
		// Redirects all mouse events to the target panel, until it is either destroyed, another panel is captured, or ReleaseCursor is called on this interface
		virtual void CaptureCursor(IGRPanel & panel) = 0;

		// Returns the Id of the captured panel, or -1, if no capture exists
		virtual int64 CapturedPanelId() const = 0;

		// Sets the id of the capture panel to -1. No panel with id -1 can exist.
		virtual void ReleaseCursor() = 0;

		virtual IGRCustodian& Custodian() = 0;

		// The visual scheme of the panel system
		virtual IScheme& Scheme() = 0;

		// the Gui Retained object that houses the root
		virtual IGuiRetained& GR() = 0;
	};

	ROCOCOAPI IGRPanel
	{
		virtual EventRouting NotifyAncestors(WidgetEvent& widgetEvent, IGRWidget& widget) = 0;
		virtual IGRWidget& Widget() = 0;
		virtual IGRPanel& Resize(Vec2i span) = 0;
		virtual IGRPanel& SetParentOffset(Vec2i offset) = 0;
		virtual Vec2i Span() const = 0;
		virtual Vec2i ParentOffset() const = 0;
		virtual IGRPanelRoot& Root() = 0;
		virtual IGRPanel& AddChild() = 0;
		// If callback is not null then is invoked for each child. The number of children is returned
		virtual int32 EnumerateChildren(IEventCallback<IGRPanel>* callback) = 0;
		virtual int64 Id() const = 0;
		virtual GuiRect AbsRect() const = 0;
		virtual void CaptureCursor() = 0;
	};

	ROCOCOAPI IGRPanelSupervisor : IGRPanel
	{
		virtual void LayoutRecursive(Vec2i absoluteOrigin) = 0;
		virtual void RenderRecursive(IGRRenderContext & g) = 0;
		virtual EventRouting RouteCursorClickEvent(CursorEvent& ce) = 0;
		virtual EventRouting RouteCursorMoveEvent(CursorEvent& ce) = 0;
		virtual void SetWidget(IGRWidget& widget) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IGRWidget
	{
		virtual Vec2i EvaluateMinimalSpan() const = 0;
		virtual void Layout(const GuiRect& parentDimensions) = 0;
		virtual EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) = 0;
		virtual EventRouting OnCursorClick(CursorEvent& ce) = 0;
		virtual EventRouting OnCursorMove(CursorEvent& ce) = 0;
		virtual IGRPanel& Panel() = 0;
		virtual void Render(IGRRenderContext& g) = 0;
		virtual void Free() = 0;
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

	ROCOCOAPI IGRWidgetButton : IGRWidget
	{
		// Sets the rule by which events are fired
		virtual void SetClickCriterion(GRClickCriterion criterion) = 0;

		// Set what happens when the button is fired
		virtual void SetEventPolicy(GREventPolicy policy) = 0;

		// Sets user meta data for the button
		virtual void SetMetaData(const ControlMetaData& metaData) = 0;

		// Sets the display text for the button
		virtual void SetTitle(cstr text) = 0;

		// Gets the display text and returns its length. If the buffer is insufficient, the result is truncated
		virtual size_t GetTitle(char* titleBuffer, size_t nBytes) const = 0;

		virtual void SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) = 0;

		// Returns meta data set with SetMetaData. The pointers are valid until meta data is changed or the control is destroyed
		virtual ControlMetaData GetMetaData() = 0;
	};

	struct GRMenuItem
	{
		cstr text;
		ControlMetaData metaData;
	};

	ROCOCOAPI IGRWidgetMenuBar : IGRWidget
	{
		virtual void AddItem(const GRMenuItem& item) = 0;
	};

	ROCOCOAPI IGRWidgetFactory
	{
		virtual IGRWidget& CreateWidget(IGRPanel& panel) = 0;
	};

	ROCOCO_GUI_RETAINED_API IGRWidgetFactory& GetWidgetButtonFactory();

	ROCOCOAPI IGRMainFrame : IGRWidget
	{
		// Retrieves a reference to the frame's top menu bar. If one does not exist, it is created		
		virtual IGRWidgetMenuBar& GetMenuBar() = 0;
	};

	ROCOCOAPI IGRMainFrameSupervisor: IGRMainFrame
	{
	};

	ROCOCOAPI IGuiRetained
	{
		// Associates a frame with an id and returns it. If it already exists, gets the existant one.
		virtual IGRMainFrame& BindFrame(IdWidget id) = 0;

		// Deletes the frame with the given id, invalidating all references to the frame and its panel and its layout
		virtual void DeleteFrame(IdWidget id) = 0;

		// Get a frame associated with an id. If none exist, null is returned
		virtual IGRMainFrame* TryGetFrame(IdWidget id) = 0;

		// Lower the frame so that it is the first to render.
		virtual void MakeFirstToRender(IdWidget id) = 0;

		// Raise the frame so that it is the final to render
		virtual void MakeLastToRender(IdWidget id) = 0;	
		
		// Renders the list of frames
		virtual void RenderGui(IGRRenderContext& g) = 0;

		virtual IGRPanelRoot& Root() = 0;

		virtual IGRWidget& AddWidget(IGRPanel& parent, IGRWidgetFactory& factory) = 0;

		// Constant time lookup of a widget with a given panel Id.
		virtual IGRWidget* FindWidget(int64 panelId) = 0;

		// Set the visibility status. If invisible, it will ignore all input and not be rendered
		virtual void SetVisible(bool isVisible) = 0;

		// Returns true if the retained GUI is visible and there are frames to show, otherwise false
		virtual bool IsVisible() const = 0;

		virtual EventRouting RouteCursorClickEvent(CursorEvent& ev) = 0;
		virtual EventRouting RouteCursorMoveEvent(CursorEvent& ev) = 0;
	};

	ROCOCOAPI IGuiRetainedSupervisor: IGuiRetained
	{
		virtual void Free() = 0;

	};

	enum GRErrorCode
	{
		None,
		Generic,
		RecursionLocked
	};

	ROCOCOAPI IGRCustodian
	{
		virtual Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring & text) const = 0;
		virtual EventRouting OnGREvent(WidgetEvent& ev) = 0;
		virtual void RaiseError(GRErrorCode code, cstr function, cstr message) = 0;
	};

	ROCOCOAPI IGRCustodianSupervisor : IGRCustodian
	{
		virtual void Free() = 0;
	};

	struct GRConfig
	{
		int32 unused = 0;
	};

	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetMenuBar& CreateMenuBar(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateMenuButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GRConfig& config, IGRCustodian& custodian);
	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawMenuButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawButtonText(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g);
}
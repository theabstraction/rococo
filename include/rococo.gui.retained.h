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

	struct IdWidget
	{
		cstr Name;
	};

	ROCOCOAPI IGRLayout
	{
		virtual void Layout(IGRPanel& panel, const GuiRect& absRect) = 0;
	};

	ROCOCOAPI IGRLayoutSupervisor: IGRLayout
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IGRRenderContext
	{
		virtual Vec2i CursorHoverPoint() const = 0;
		// It is up to the renderer to decide if a panel is hovered.
		virtual bool IsHovered(IGRPanel& panel) const = 0;	
		virtual GuiRect ScreenDimensions() const = 0;

		virtual void DrawRect(const GuiRect& absRect, RGBAb colour) = 0;
		virtual void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) = 0;
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

	ROCOCOAPI IGRPanelRoot
	{
		virtual IScheme& Scheme() = 0;
		virtual IGuiRetained& GR() = 0;
	};

	ROCOCOAPI IGRPanel
	{
		virtual IGRLayout& LayoutSystem() = 0;
		virtual IGRWidget& Widget() = 0;
		virtual IGRPanel& Resize(Vec2i span) = 0;
		virtual IGRPanel& SetParentOffset(Vec2i offset) = 0;
		virtual Vec2i Span() const = 0;
		virtual Vec2i ParentOffset() const = 0;
		virtual IGRPanelRoot& Root() = 0;
		virtual IGRPanel& AddChild() = 0;
		virtual int64 Id() const = 0;
		virtual GuiRect AbsRect() const = 0;
	};

	ROCOCOAPI IGRPanelSupervisor : IGRPanel
	{
		virtual void LayoutRecursive(Vec2i absoluteOrigin) = 0;
		virtual void RenderRecursive(IGRRenderContext & g) = 0;
		virtual void SetWidget(IGRWidget& widget) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IGRWidget
	{
		virtual void Layout(const GuiRect& parentDimensions) = 0;
		virtual IGRPanel& Panel() = 0;
		virtual void Render(IGRRenderContext& g) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IGRWidgetButton: IGRWidget
	{		

	};

	ROCOCOAPI IGRWidgetFactory
	{
		virtual IGRWidget& CreateWidget(IGRPanel& panel) = 0;
	};

	ROCOCO_GUI_RETAINED_API IGRWidgetFactory& GetWidgetButtonFactory();

	ROCOCOAPI IGRMainFrame: IGRWidget
	{
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
	};

	ROCOCOAPI IGuiRetainedSupervisor: IGuiRetained
	{
		virtual void Free() = 0;

	};

	ROCOCOAPI IGuiRetainedCustodian
	{

	};

	ROCOCOAPI IGuiRetainedCustodianSupervisor : IGuiRetainedCustodian
	{
		virtual void Free() = 0;
	};

	struct GuiRetainedConfig
	{
		int32 unused = 0;
	};

	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent);
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GuiRetainedConfig& config, IGuiRetainedCustodian& custodian);
	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g);
	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g);
}
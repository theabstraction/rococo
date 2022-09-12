#include <rococo.types.h>

#ifdef ROCOCO_GUI_RETAINED_EXPORTS
#define ROCOCO_GUI_RETAINED_API __declspec(dllexport) 
#else
#define  ROCOCO_GUI_RETAINED_API __declspec(dllimport) 
#endif

namespace Rococo::Gui
{
	struct IdWidget
	{
		cstr Name;
	};

	struct IGRPanel;

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
		virtual GuiRect ScreenDimensions() const = 0;
	};

	struct IGRWidget;

	ROCOCOAPI IGRPanel
	{
		virtual IGRLayout& LayoutSystem() = 0;
		virtual IGRWidget& Widget() = 0;
		virtual void Resize(Vec2i span) = 0;
	};

	ROCOCOAPI IGRPanelSupervisor : IGRPanel
	{
		virtual void SetWidget(IGRWidget& widget) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IGRPanelRoot
	{

	};

	ROCOCOAPI IGRWidget
	{
		virtual IGRPanel& Panel() = 0;	
		virtual void Render(IGRRenderContext& g) = 0;
	};

	ROCOCOAPI IGRMainFrame: IGRWidget
	{
		
	};

	ROCOCOAPI IGRMainFrameSupervisor: IGRMainFrame
	{
		virtual void Free() = 0;
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
	};

	ROCOCOAPI IGuiRetainedSupervisor: IGuiRetained
	{
		virtual void Free() = 0;
		virtual void RenderGui(IGRRenderContext& g) = 0;
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

	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GuiRetainedConfig& config, IGuiRetainedCustodian& custodian);
}
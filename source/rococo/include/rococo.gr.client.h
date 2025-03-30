#pragma once
#include <rococo.types.h>

namespace Rococo::Gui
{
	DECLARE_ROCOCO_INTERFACE IGRRenderContext;
}

namespace Rococo
{
	ROCOCO_INTERFACE IGR2DScene
	{
		virtual void Render(Gui::IGRRenderContext & rc) = 0;
	};

	namespace Gui
	{
		DECLARE_ROCOCO_INTERFACE IGREventHandler;
		DECLARE_ROCOCO_INTERFACE IGRSystem;

		ROCOCO_INTERFACE IGRClientWindow
		{
			// Attach an event handler to the GRSystem, which is triggered by IGRSystem::DispatchMessages
			// It must remain valid for the life time of the client window
			virtual IGREventHandler * SetEventHandler(Gui::IGREventHandler * eventHandler) = 0;

			// Presents the given scene as background in the window. (The GUI is foreground).
			// The scene object must be valid for the lifetime of the client window, or until LinkScene is invoked again
			virtual void LinkScene(IGR2DScene* scene) = 0;

			// Triggers WM_PAINT as soon as the message queue is free
			virtual void QueuePaint() = 0;

			virtual IGRSystem& GRSystem() = 0;
		};
	}
}
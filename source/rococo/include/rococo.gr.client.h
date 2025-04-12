#pragma once
#include <rococo.types.h>

namespace Rococo::GreatSex
{
	DECLARE_ROCOCO_INTERFACE IGreatSexGenerator;
}

namespace Rococo::Gui
{
	DECLARE_ROCOCO_INTERFACE IGRRenderContext;
	DECLARE_ROCOCO_INTERFACE IGRWidget;
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

			virtual void ShowError(Vec2i start, Vec2i end, cstr nameRef, cstr sourceBuffer, cstr message) = 0;

			virtual IGRSystem& GRSystem() = 0;

			// Tries to load a frame specified by the great sex sexml File into the parent widget.
			// If it fails the client window displays the error and returns false, else it returns true
			virtual bool LoadFrame(cstr sexmlFile, IGRWidget& parentWidget, IEventCallback<GreatSex::IGreatSexGenerator>& onGenerate) = 0;
		};
	}
}
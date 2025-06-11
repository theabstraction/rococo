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
	DECLARE_ROCOCO_INTERFACE IGRWidgetMainFrame;
	enum class EGREventRouting;
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

		ROCOCO_INTERFACE IGRAppControl
		{
			virtual Gui::EGREventRouting OnRawVKey(uint16 vKeyCode) = 0;
		};

		ROCOCO_INTERFACE IGRClientWindow
		{
			// Attach an event handler to the GRSystem, which is triggered by IGRSystem::DispatchMessages
			// It must remain valid for the life time of the client window
			virtual IGREventHandler* SetEventHandler(Gui::IGREventHandler * eventHandler) = 0;

			// Simulate a keypress by sending a virtual keycode to the GR API
			// User internally together with MapJoystickVirtualKeyToVirtualKeyboardKey for joystick support
			// But it can be used externally to support 3rd party control systems
			virtual void InsertKeyboardEvent(uint16 vCode, bool isUp, uint16 unicode) = 0;

			// Allows the supplied sink to intercept virtual key presses, potentially stopping them from being handled by the GR system
			virtual void InterceptVKeys(IGRAppControl& sink) = 0;

			// Tells the window that a joystick generated virtual key code should be presented to the API as an equivalent keyboard key
			virtual void MapJoystickVirtualKeyToVirtualKeyboardKey(uint16 joystickVirtualKeyCode, uint16 keyboardVirtualKeyCode) = 0;

			// Presents the given scene as background in the window. (The GUI is foreground).
			// The scene object must be valid for the lifetime of the client window, or until LinkScene is invoked again
			virtual void LinkScene(IGR2DScene* scene) = 0;

			// Triggers WM_PAINT as soon as the message queue is free
			virtual void QueuePaint() = 0;

			[[nodiscard]] virtual IGRSystem& GRSystem() = 0;

			// Tries to load a frame specified by the great sex sexml File into the parent widget.
			// If it fails the client window displays the error and returns false, else it returns true
			virtual bool LoadFrame(cstr sexmlFile, IGRWidgetMainFrame& frame, IEventCallback<GreatSex::IGreatSexGenerator>& onGenerate) = 0;
		};
	}
}
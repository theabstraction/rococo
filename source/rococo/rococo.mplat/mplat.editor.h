#include <rococo.types.h>

namespace Rococo
{
	struct Platform;
}

namespace Rococo::Gui
{
	struct IGRSystem;
	struct GRWidgetEvent;
}

namespace Rococo::MPEditor
{
	enum class ToolbarMetaId : int64 { MINIMIZE = 400'000'001, RESTORE, EXIT };

	ROCOCO_INTERFACE IMPEditorEventHandler
	{
		// Called when a button click hits the frame.
		virtual void OnMPEditor_ButtonClick(Gui::GRWidgetEvent& buttonEvent) = 0;
	};

	ROCOCO_INTERFACE IMPEditor
	{
		virtual bool IsVisible() const = 0;

		// Illustrate the reflection target in the gui system
		virtual void Preview(Rococo::Gui::IGRSystem& gr, Rococo::Reflection::IReflectionVisitation* visitation) = 0;

		virtual void SetVisibility(bool isVisible) = 0;

		virtual void AddHook(IMPEditorEventHandler* eventHandler) = 0;
		virtual void RemoveHook(IMPEditorEventHandler* eventHandler) = 0;

		// The current reflection target illustrated in the gui
		virtual Reflection::IReflectionTarget& ReflectionTarget() = 0;
	};

	ROCOCO_INTERFACE IMPEditorSupervisor : IMPEditor
	{
		virtual void SetPlatform(Platform* platform) = 0;
		virtual void Free() = 0;
	};

	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGRSystem& gr);
}
#include <rococo.types.h>

namespace Rococo::Gui
{
	struct IGuiRetained;
}

namespace Rococo::Reflection
{
	struct IReflectionTarget;
}

namespace Rococo::MPEditor
{
	ROCOCO_INTERFACE IMPEditor
	{
		virtual bool IsVisible() const = 0;
		virtual void Preview(Rococo::Reflection::IReflectionTarget& target) = 0;
		virtual void SetVisibility(bool isVisible) = 0;	
		virtual void SyncUIToPreviewer(Rococo::Gui::IGuiRetained& gr) = 0;
	};

	ROCOCO_INTERFACE IMPEditorSupervisor : IMPEditor
	{
		virtual void Free() = 0;
	};

	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGuiRetained& gr);
}
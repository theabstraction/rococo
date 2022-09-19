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
		virtual void Reflect(Rococo::Reflection::IReflectionTarget& target) = 0;
		virtual void SetVisibility(bool isVisible) = 0;	
	};

	ROCOCO_INTERFACE IMPEditorSupervisor : IMPEditor
	{
		virtual void Free() = 0;
	};

	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGuiRetained& gr);
}
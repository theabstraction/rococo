#include <rococo.types.h>

namespace Rococo
{
	struct Platform;
}

namespace Rococo::Gui
{
	struct IGRSystem;
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
		virtual void Preview(Rococo::Gui::IGRSystem& gr, Rococo::Reflection::IReflectionTarget& target) = 0;
		virtual void SetVisibility(bool isVisible) = 0;	
	};

	ROCOCO_INTERFACE IMPEditorSupervisor : IMPEditor
	{
		virtual void SetPlatform(Platform* platform) = 0;
		virtual void Free() = 0;
	};

	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGRSystem& gr);
}
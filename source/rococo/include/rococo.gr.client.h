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
}
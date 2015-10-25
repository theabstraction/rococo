#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"

#include <string>

#include <vector>

using namespace Rococo;
using namespace Dystopia;

namespace
{
	class InventoryPane : public IUIPaneSupervisor
	{
		Environment& e;
		ILevel& level;

	public:
		InventoryPane(Environment& _e, ILevel& _level) :
			e(_e), level(_level)
		{
		}

		virtual void Free()
		{
			delete this;
		}

		virtual PaneModality OnTimestep(const TimestepEvent& clock)
		{
			return PaneModality_Modeless;
		}

		virtual PaneModality OnKeyboardEvent(const KeyboardEvent& ke)
		{
			return PaneModality_Modeless;
		}

		virtual PaneModality OnMouseEvent(const MouseEvent& me)
		{
			if (me.HasFlag(MouseEvent::LUp))
			{
			}

			return PaneModality_Modal;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& gc)
		{
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreateInventoryPane(Environment& e, ILevel& level)
	{
		return new InventoryPane(e, level);
	}
}
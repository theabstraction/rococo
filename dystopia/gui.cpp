#include "dystopia.h"
#include <string>

#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"
#include "rococo.strings.h"
#include "dystopia.post.h"

#include <vector>

#include "dystopia.ui.h"

#include <wchar.h>

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class Gui : public IGuiSupervisor
	{
	private:
		IEventCallback<GuiEventArgs>* guiEventHandler;
		Environment& e;
	public:
		Gui(Environment& _e, IUIStack& _stack) : e(_e), guiEventHandler(nullptr) {}

		virtual void SetEventHandler(IEventCallback<GuiEventArgs>* guiEventHandler)
		{
			this->guiEventHandler = guiEventHandler;
		}

		virtual void Free() { delete this; }

		virtual void ShowDialogBox(const Vec2i& span, int32 retzone, int32 hypzone, const fstring& title, const fstring& message, const fstring& buttons)
		{
			e.uiStack.PushTop(CreateDialogBox(e, *guiEventHandler, title.buffer, message.buffer, buttons.buffer, span, retzone, hypzone), ID_PANE_GENERIC_DIALOG_BOX);
		}

		virtual void Add3DHint(const Vec3& worldPos, const fstring& message, float duration)
		{
			HintMessage3D msg;
			SafeCopy(msg.message, message.buffer, _TRUNCATE);
			msg.position = worldPos;
			msg.duration = duration;

			e.postbox.PostForLater(msg, true);
		}
	};
}

namespace Dystopia
{
	IGuiSupervisor* CreateGui(Environment& e, IUIStack& stack)
	{
		return new Gui(e, stack);
	}
}
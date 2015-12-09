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
#include <stdarg.h>

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class Gui : public IGuiSupervisor
	{
	private:
		IEventCallback<GuiEventArgs>* guiEventHandler;
		Environment& e;

      struct DebugMessage
      {
         enum { CAPACITY = 64 };
         wchar_t text[CAPACITY];
      };

      std::vector<DebugMessage> debugList;

	public:
		Gui(Environment& _e, IUIStack& _stack) : e(_e), guiEventHandler(nullptr) {}

		virtual void SetEventHandler(IEventCallback<GuiEventArgs>* guiEventHandler)
		{
			this->guiEventHandler = guiEventHandler;
		}

		virtual void Free() { delete this; }

      virtual void AppendDebugElement(const wchar_t* format, ...)
      {
         if (debugList.size() > 64) return;

         DebugMessage msg;

         va_list arglist;
         va_start(arglist, format);
         SafeVFormat(msg.text, DebugMessage::CAPACITY, _TRUNCATE, format, arglist);

         debugList.push_back(msg);
      }

      void RenderDebugElementsAndClear(IGuiRenderContext& grc, int32 fontIndex, int32 pxLeftAlign)
      {
         int y = 20;

         for (auto& msg : debugList)
         {
            Graphics::StackSpaceGraphics ssg;
            auto& job = Graphics::CreateHorizontalCentredText(ssg, fontIndex, msg.text, RGBAb(255, 255, 255, 255));
            auto& span = grc.EvalSpan({ 0,0 }, job);

            GuiRect messageRect(pxLeftAlign, y, pxLeftAlign + span.x + 4, y + 20);

            Graphics::DrawRectangle(grc, messageRect, RGBAb(0, 0, 0, 64), RGBAb(0, 0, 0, 64));
            grc.RenderText({ messageRect.left + 2, messageRect.top }, job);

            y += 20;
         }

         debugList.clear();
      }

      virtual void EnumerateAndClear(IEnumerator<const wchar_t*>& cb)
      {
         for (auto& msg : debugList)
         {
            cb(msg.text);
         }

         debugList.clear();
      }

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
#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.widgets.h>
#include <rococo.strings.h>
#include <rococo.events.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Widgets;

namespace
{
   EventIdRef evStatusUpate = "ui.status.upate"_event;

   struct StatusEvent : public EventArgs
   {
      cstr status;
   };

   class StatusBar : public IStatusBar, public IObserver
   {
      IPublisher& publisher;
      char text[256];

      int32 updateHighlight;
   public:
      StatusBar(IPublisher& _publisher) : publisher(_publisher), text{0}
      {
         publisher.Subscribe(this, evStatusUpate);
      }

      ~StatusBar()
      {
         publisher.Unsubscribe(this);
      }

	  void OnEvent(Event& ev) override
	  {
		  if (evStatusUpate == ev)
		  {
				auto& evs = As<StatusEvent>(ev);
				StackStringBuilder sb(text, sizeof(text));
				sb << evs.status;
				updateHighlight = 255;
		  }
	  }

      void Render(IGuiRenderContext& gc, const GuiRect& rect) override
      {
         Graphics::DrawRectangle(gc, rect, RGBAb(updateHighlight, updateHighlight, updateHighlight), RGBAb(updateHighlight, updateHighlight, updateHighlight));
         if (updateHighlight > 191) updateHighlight--;
         Graphics::DrawBorderAround(gc, rect, { 1,1 }, RGBAb(128, 128, 128), RGBAb(128, 128, 128));
         Graphics::RenderVerticalCentredText(gc, text, RGBAb(0, 0, 0), 9, { rect.left + 4, (rect.top + rect.bottom) >> 1 });
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Widgets
   {   
      IStatusBar* CreateStatusBar(IPublisher& publisher)
      {
         return new StatusBar(publisher);
      }

      void SetStatus(cstr statustext, Events::IPublisher& publisher)
      {
         StatusEvent ev;
         ev.status = statustext;
         publisher.Publish(ev, evStatusUpate);
      }
   }
}
#include "dystopia.h"
#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"
#include "rococo.strings.h"
#include "dystopia.post.h"

#include <string>

#include <vector>

#include "human.types.h"

#include "dystopia.ui.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
   class CVPane : public IUIPaneSupervisor, public IEventCallback<ActionMap>, public Post::IRecipient
   {
      Environment& e;

   public:
      CVPane(Environment& _e) :
         e(_e)
      {
         e.postbox.Subscribe<VerbOpenCV>(this);
         e.postbox.Subscribe<UIStackPaneChange>(this);
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnPost(const Mail& mail)
      {
         auto* openCV = Post::InterpretAs<VerbOpenCV>(mail);
         if (openCV)
         {
            if (e.uiStack.Top().id != ID_PANE_INVENTORY_SELF)
            {
               e.uiStack.PushTop(ID_PANE_INVENTORY_SELF);
            }
         }

         auto* uiChange = Post::InterpretAs<UIStackPaneChange>(mail);
         if (uiChange)
         {
            if (uiChange->newTopId == ID_PANE_INVENTORY_SELF)
            {
               OnTop();
            }
            else if (uiChange->poppedId == ID_PANE_INVENTORY_SELF)
            {
               OnPop();
            }
         }
      }

      virtual Relay OnTimestep(const TimestepEvent& clock)
      {
        return Relay_None;
      }

      virtual void OnEvent(ActionMap& arg)
      {
         switch (arg.type)
         {
         case ActionMapTypeCV:
            if (arg.isActive) e.uiStack.PopTop();
            break;
         }
      }

      virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
      {
         e.controls.MapKeyboardEvent(ke, *this);
         return Relay_None;
      }

      virtual Relay OnMouseEvent(const MouseEvent& me)
      {
         e.controls.MapMouseEvent(me, *this);
         return Relay_None;
      }

      virtual void OnPop()
      {
      }

      void OnTop()
      {
      }

      virtual void RenderObjects(IRenderContext& rc)
      {
      }

      virtual void RenderGui(IGuiRenderContext& gc)
      {
         e.paneBuilder.RenderHierarchy(gc, L"cv");
      }

      virtual void OnLostTop()
      {

      }
   };
}

namespace Dystopia
{
   IUIPaneSupervisor* CreateCVPane(Environment& e)
   {
      return new CVPane(e);
   }
}
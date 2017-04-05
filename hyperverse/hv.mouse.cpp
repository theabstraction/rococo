#include "hv.events.h"
#include <windows.h>
namespace
{
   using namespace HV;
   using namespace HV::Events;
   using namespace Rococo::Events;

   class Mouse : public IMouse
   {
   private:
      IPublisher& publisher;

   public:
      Mouse(IPublisher& _publisher): publisher(_publisher)
      {

      }

      virtual void TranslateMouseEvent(const MouseEvent& ev)
      {
         auto& rm = reinterpret_cast<const RAWMOUSE&>(ev);
         if (rm.usFlags == MOUSE_MOVE_RELATIVE)
         {
            Input::OnMouseMoveRelativeEvent mmre;
            mmre.dx = rm.lLastX;
            mmre.dy = rm.lLastY;

            if ((rm.usButtonFlags & RI_MOUSE_WHEEL))
            {
               mmre.dz = ((int32)(short)rm.usButtonData) / 120;
            }
            else
            {
               mmre.dz = 0;
            }

            Publish(publisher, mmre);
       
            Input::OnMouseChangedEvent mce;
            mce.flags = rm.usButtonFlags & Input::MouseFlags_LRMW;
            if (mce.flags != 0)
            {
               Publish(publisher, mce);
            }
         }
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace HV
{
   IMouse* CreateMouse(IPublisher& publisher)
   {
      return new Mouse(publisher);
   }
}
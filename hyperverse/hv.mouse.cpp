#include "hv.h"

namespace
{
   using namespace HV;
   
   ROCOCOAPI IMouse
   {
      virtual void Free() = 0;
   };

   class Mouse : public IMouse
   {
   private:
   public:
      Mouse()
      {

      }

      virtual void TranslateMouseEvent(const MouseEvent& ev)
      {

      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace HV
{
   IMouse* CreateMouse()
   {
      return new Mouse();
   }
}
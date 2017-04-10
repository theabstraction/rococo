#include "hv.events.h"

namespace HV
{
   namespace Events
   {
      namespace OS
      {
         EventId OnTick                = L"os.tick"_event;
         EventId OnFileChanged         = L"os.file.changed"_event;
      }

      namespace Entities
      {
          EventId OnTryMoveMobile      = L"mobile.try.move"_event;
      }

      namespace Player
      {
         EventId OnPlayerAction        = L"player.action"_event;
         EventId OnPlayerDelta         = L"player.delta"_event;
        
         EventId OnPlayerViewChange    = L"player.view.change"_event;
      }
   }
}

#include "hv.events.h"

namespace HV
{
   namespace Events
   {
      namespace OS
      {
         EventId OnTick                = "os.tick"_event;
         EventId OnFileChanged         = "os.file.changed"_event;
      }

      namespace Entities
      {
          EventId OnTryMoveMobile      = "mobile.try.move"_event;
      }

      namespace Player
      {
         EventId OnPlayerAction        = "player.action"_event;
         EventId OnPlayerDelta         = "player.delta"_event;
        
         EventId OnPlayerViewChange    = "player.view.change"_event;
      }
   }
}

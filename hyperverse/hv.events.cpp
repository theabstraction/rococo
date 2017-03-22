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

      namespace Player
      {
         EventId OnPlayerAction        = L"player.action"_event;
         EventId OnPlayerDelta         = L"player.delta"_event;
         EventId OnPlayerTryMove       = L"player.try.move"_event;
         EventId OnPlayerViewChange    = L"player.view.change"_event;
      }

      namespace Input
      {
         EventId OnMouseMoveRelative   = L"input.mouse.delta"_event;
         EventId OnMouseChanged        = L"input.mouse.buttons"_event;
      }
   }
}

#include "hv.h"

namespace HV
{
   namespace Events
   {
      EventId OnTick = L"hv.onTick"_event;
      EventId OnFileChanged = L"hv.onFileChanged"_event;
      EventId OnPlayerAction = L"hv.onPlayerAction"_event;
      EventId OnPlayerTryMove = L"hv.OnPlayerTryMove"_event;
   }
}

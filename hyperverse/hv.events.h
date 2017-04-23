#include "hv.h"

namespace HV
{
   namespace Events
   {
      namespace OS
      {
         extern EventId OnTick;

         struct OnTickEvent : public Event
         {
            OnTickEvent() : Event(OnTick) {}
            IUltraClock* clock;
            uint32 frameSleep{ 5 };
         };

         extern EventId OnFileChanged;

         struct OnFileChangedEvent : public Event
         {
            OnFileChangedEvent() : Event(OnFileChanged) {}
            FileModifiedArgs* args;
         };
      }

      namespace Player
      {
         extern EventId OnPlayerAction;

         struct OnPlayerActionEvent : public Event
         {
            OnPlayerActionEvent() : Event(OnPlayerAction) {}
            cstr Name;
            bool start;
         };

         extern EventId OnPlayerDelta;

         struct OnPlayerDeltaEvent : public Event
         {
            OnPlayerDeltaEvent() : Event(OnPlayerDelta) {}
            cstr Name;
            float delta;
         };

         extern EventId OnPlayerViewChange;

         struct OnPlayerViewChangeEvent : public Event
         {
            OnPlayerViewChangeEvent() : Event(OnPlayerViewChange) {}
            ID_ENTITY playerEntityId;
            float32 elevationDelta;
         };
      } // Player

      namespace Entities
      {
         extern EventId OnTryMoveMobile;

         struct OnTryMoveMobileEvent : public Event
         {
            OnTryMoveMobileEvent() : Event(OnTryMoveMobile) {}
            ID_ENTITY entityId;
            float fowardDelta;
            float straffeDelta;
            FPSAngles delta; 
            FPSAngles angles;
         };
      }
   } // 
} // HV
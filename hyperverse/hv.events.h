#include "hv.h"

namespace HV
{
   namespace Events
   {
      extern EventId changeDefaultTextureId;

      struct ChangeDefaultTextureEvent : public Event
      {
         ChangeDefaultTextureEvent() : Event(changeDefaultTextureId)
         {

         }

         cstr wallName;
      };

	  extern EventId setNextLevelEventId;

	  struct SetNextLevelEvent : public Event
	  {
		  SetNextLevelEvent() : Event(setNextLevelEventId) {}
		  cstr name;
	  };

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
      } // Player
   } // Events
} // HV
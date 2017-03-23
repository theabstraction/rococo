#include "hv.h"

namespace HV
{
   namespace Events
   {
      namespace Input
      {
         enum MouseFlags : int32
         {
            MouseFlags_LDown = 0x0001,
            MouseFlags_LUp   = 0x0002,
            MouseFlags_MDown = 0x0010,
            MouseFlags_MUp   = 0x0020,
            MouseFlags_RDown = 0x0040,
            MouseFlags_RUp   = 0x0080,
            MouseFlags_LRM   = 0x00F3
         };

         extern EventId OnMouseMoveRelative;

         struct OnMouseMoveRelativeEvent : public Event
         {
            OnMouseMoveRelativeEvent() : Event(OnMouseMoveRelative) {}
            int32 dx;
            int32 dy;
         };

         extern EventId OnMouseChanged;

         struct OnMouseChangedEvent : public Event
         {
            OnMouseChangedEvent() : Event(OnMouseChanged) {}
            Vec2i cursor;
            int32 flags;

         };
      }

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
            const wchar_t* Name;
            bool start;
         };

         extern EventId OnPlayerDelta;

         struct OnPlayerDeltaEvent : public Event
         {
            OnPlayerDeltaEvent() : Event(OnPlayerDelta) {}
            const wchar_t* Name;
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
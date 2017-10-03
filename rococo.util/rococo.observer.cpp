#include <rococo.api.h>
#include <rococo.strings.h>

#include <unordered_map>
#include <algorithm>

#include <stdlib.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Events;

   std::unordered_map<size_t, cstr> knownEvents;

   struct EventIdMapHasher
   {
      size_t operator()(EventId id) const
      {
         return (size_t)id;
      }
   };

   struct NullEvent : Event
   {
      NullEvent() : Event("null.event"_event) {}
      char data[232]; 
   };

   struct EventBinding
   {
      NullEvent nullEvent;
      bool isLossy;

      EventBinding& operator = (const EventBinding& src)
      {
         memcpy(&nullEvent, &src.nullEvent, src.nullEvent.sizeInBytes);
         isLossy = src.isLossy;
         return *this;
      }
   };

   struct Publisher : public IPublisherSupervisor
   {
      std::unordered_map<EventId, std::vector<IObserver*>, EventIdMapHasher> eventHandlers;
      std::unordered_map<IObserver*, std::vector<EventId>> knownObservers;
      std::vector<EventBinding> postedEvents;
      std::vector<EventBinding> outgoingEvents;

      enum { LOSS_AT = 32, MAX_Q = 64 };

      void FlushLossyPackets()
      {
         struct
         {
            bool operator()(const EventBinding& b)
            {
               return b.isLossy;
            }
         } pred;
         auto delPoint = std::remove_if(postedEvents.begin(), postedEvents.end(), pred);
         postedEvents.erase(delPoint, postedEvents.end());
      }

      void RawPost(const Event& ev, bool isLossy) override
      {
         static_assert(sizeof(NullEvent) == 256, "Unhandled null event size");

         if (ev.sizeInBytes <= 0 || ev.sizeInBytes > 256)
         {
            Throw(0, "Publisher: cannot post event. The event.sizeInBytes was bad. Check that it was posted correctly");
         }

         if (isLossy && postedEvents.size() > LOSS_AT)
         {
            FlushLossyPackets();

            if (postedEvents.size() > LOSS_AT)
            {
               return;
            }
         }

         if (postedEvents.size() >= MAX_Q)
         {
            Throw(0, "Publisher: Queue capacity reached");
         }

         EventBinding evb;
         auto* src = reinterpret_cast<const NullEvent*>(&ev);
         auto* dst = reinterpret_cast<NullEvent*>(&evb.nullEvent);
         memcpy(dst, src, sizeof(NullEvent));
         evb.isLossy = isLossy;
         postedEvents.push_back(evb);
      }

      void Deliver() override
      {
         for (auto i = postedEvents.rbegin(); i != postedEvents.rend(); i++)
         {
            outgoingEvents.push_back(*i);
         }
         postedEvents.clear();

         for (auto& i : outgoingEvents)
         {
            Publish(i.nullEvent);
         }
         outgoingEvents.clear();
      }

      void Detach(IObserver* observer) override
      {
         auto i = knownObservers.find(observer);
         if (i != knownObservers.end())
         {
            for (auto& ev : i->second)
            {
               auto j = eventHandlers.find(ev);
               if (j != eventHandlers.end())
               {
                  auto& observers = j->second;
                  observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
               }
            }

            knownObservers.erase(i);
         }
      }

      void Attach(IObserver* observer, const EventId& eventId) override
      {
         auto i = knownObservers.find(observer);
         if (i == knownObservers.end())
         {
            knownObservers[observer] = std::vector<EventId>{ eventId };
         }
         else
         {
            i->second.push_back(eventId);
         }

         auto j = eventHandlers.find(eventId);
         if (j == eventHandlers.end())
         {
            eventHandlers[eventId] = std::vector<IObserver*>{ observer };
         }
         else
         {
            j->second.push_back(observer);
         }
      }

      void RawPublish(Event& ev) override
      {
         auto i = eventHandlers.find(ev.id);
         if (i != eventHandlers.end())
         {
            // Create a temporary iteration table
            size_t count = i->second.size();
            IObserver** observers = (IObserver**) alloca(sizeof(void*) * count);
            for (size_t j = 0; j < count; ++j)
            {
               observers[j] = i->second[j];
            }
           
            // We are now free to modify the eventHandlers container without invalidating the enumeration
            for (size_t j = 0; j < count; ++j)
            {
               auto k = knownObservers.find(observers[j]);
               if (k != knownObservers.end())
               {
                  observers[j]->OnEvent(ev);
               }
            }
         }
      }

      void Free() override
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Events
   {
      EventId::operator EventHash() const
      {
         if (!id)
         {
            const auto& i = knownEvents.find(hash);
            if (i != knownEvents.end())
            {
               if (strcmp(i->second, name) != 0)
               {
                  Throw(0, "Hash clash between %s and %s", i->second, name);
               }
            }
            else
            {
               knownEvents[hash] = name;
            }

            id = hash;
         }

         return id;
      }

      EventId operator "" _event(cstr name, size_t len)
      {
         return EventId{ name, (EventHash)FastHash(name) };
      }

      void ThrowBadEvent(const Event& ev)
      {
         Throw(0, "Event %s: body was incorrect size", ev.id.Name());
      }

      IPublisherSupervisor* CreatePublisher()
      {
         return new Publisher();
      }

      namespace Input
      {
         EventId OnMouseMoveRelative = "input.mouse.delta"_event;
         EventId OnMouseChanged = "input.mouse.buttons"_event;
      }
   } // Events
} // Rococo
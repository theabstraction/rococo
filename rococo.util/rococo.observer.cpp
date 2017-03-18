#include <rococo.api.h>
#include <rococo.strings.h>

#include <unordered_map>
#include <unordered_set>
#include <stdlib.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Events;

   std::unordered_map<size_t, const wchar_t*> knownEvents;

   struct Publisher : public IPublisherSupervisor
   {
      std::unordered_set<IObserver*> observers;
      int64 lockDepth{ 0 };

      virtual void Detach(IObserver* observer)
      {
         if (lockDepth) Throw(0, L"The publisher is locked.");
         observers.erase(observer);
      }

      virtual void Attach(IObserver* observer)
      {
         if (lockDepth) Throw(0, L"The publisher is locked.");
         observers.insert(observer);
      }

      virtual void Publish(Event& ev)
      {
         lockDepth++;
         for (auto i : observers)
         {
            i->OnEvent(ev);
         }
         lockDepth--;
      }

      virtual void Free()
      {
         if (lockDepth) Throw(0, L"The publisher is locked.");
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
            auto& i = knownEvents.find(hash);
            if (i != knownEvents.end())
            {
               if (wcscmp(i->second, name) != 0)
               {
                  Throw(0, L"Hash clash between %s and %s", i->second, name);
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

      EventId operator "" _event(const wchar_t* name, size_t len)
      {
         return EventId{ name, (EventHash)FastHash(name) };
      }

      void ThrowBadEvent(const Event& ev)
      {
         Throw(0, L"Event %s: body was incorrect size", ev.id.Name());
      }

      IPublisherSupervisor* CreatePublisher()
      {
         return new Publisher();
      }
   } // Events
} // Rococo
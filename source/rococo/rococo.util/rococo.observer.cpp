#define ROCOCO_API __declspec(dllexport)
#include <rococo.events.h>
#include <rococo.hashtable.h>
#include <vector>
#include <algorithm>
#include <stdlib.h>

namespace Rococo::Strings
{
	ROCOCO_API [[nodiscard]] uint32 FastHash(cstr text);
}

namespace
{
   using namespace Rococo;
   using namespace Rococo::Events;

   std::unordered_map<size_t, cstr> knownEvents;

   struct EventIdMapHasher
   {
      size_t operator()(EventHash id) const
      {
         return (size_t)id;
      }
   };

   struct NullEvent : EventArgs
   {
      char data[224]; 
   };

   struct EventBinding
   {
	  EventIdRef ref;
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
      std::unordered_map<EventHash, std::vector<IObserver*>, EventIdMapHasher> eventHandlers;
      std::unordered_map<IObserver*, std::vector<EventHash>> knownObservers;
      std::vector<EventBinding> postedEvents;
      std::vector<EventBinding> outgoingEvents;
	  std::unordered_map<EventHash, HString> hashToNames;
	  stringmap<uint32> persistentStrings;

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

      void RawPost(const EventArgs& ev, const EventIdRef& ref, bool isLossy) override
      {
         static_assert(sizeof(NullEvent) == 232, "Unhandled null event size");

		 if (ref.name == nullptr)
		 {
			 Throw(0, "IPublisher::RawPost(...): event name was null");
		 }

		 LazyInit(ref);

         if (ev.sizeInBytes < sizeof(EventArgs) || ev.sizeInBytes > 256)
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
            Publish(i.nullEvent, i.ref);
         }
         outgoingEvents.clear();
      }

      void Unsubscribe(IObserver* observer) override
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

	  void Subscribe(IObserver* observer, const EventIdRef& id) override
	  {
		  if (id.name == nullptr)
		  {
			  Throw(0, "IPublisher::Subscribe(...): event name was null");
		  }

		  LazyInit(id);

		  auto i = knownObservers.find(observer);
		  if (i == knownObservers.end())
		  {
			  knownObservers[observer] = std::vector<EventHash>{ id.hashCode };
		  }
		  else
		  {
			  i->second.push_back(id.hashCode);
		  }

		  auto j = eventHandlers.find(id.hashCode);
		  if (j == eventHandlers.end())
		  {
			  eventHandlers[id.hashCode] = std::vector<IObserver*>{ observer };
		  }
		  else
		  {
			  j->second.push_back(observer);
		  }
	  }

	  void LazyInit(const EventIdRef& mutableRef)
	  {
		  if (mutableRef.hashCode != 0) return;
		  auto hashValue = FastHash(mutableRef.name);
		  auto i = hashToNames.find(hashValue);
		  if (i == hashToNames.end())
		  {
			  hashToNames.insert(std::make_pair(hashValue, HString(mutableRef.name)));
		  }
		  else
		  {
			  auto currentBindingName = i->second.c_str();
			  if (!Eq(currentBindingName, mutableRef.name))
			  {
				  Throw(0, "EventHash clash between %s and %s", currentBindingName, mutableRef.name);
			  }
		  }

		  mutableRef.hashCode = hashValue;
	  }

	  void RawPublish(EventArgs& args, const EventIdRef& id) override
	  {
		  if (id.name == nullptr)
		  {
			  Throw(0, "IPublisher::RawPublish(...): event name was null");
		  }

		  LazyInit(id);

		  auto i = eventHandlers.find(id.hashCode);
		  if (i != eventHandlers.end())
		  {
			  // Create a temporary iteration table
			  size_t count = i->second.size();
			  IObserver** observers = (IObserver**)alloca(sizeof(void*) * count);
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
					  Event ev{ *this, args, id };
					  observers[j]->OnEvent(ev);
				  }
			  }
		  }
	  }

	  void ThrowBadEvent(const Event& ev) override
	  {
		  Throw(0, "Event %s: body was incorrect size", ev.id.name);
	  }

      void Free() override
      {
         delete this;
      }

	  bool Match(const Event& args, const EventIdRef& id) override
	  {
		  if (id.hashCode == 0)
		  {
			  if (id.name == nullptr)
			  {
				  Throw(0, "Publisher::Match(args,id) -> id.name was NULL");
			  }
			  LazyInit(id);
		  }

		  return args.id.hashCode == id.hashCode;
	  }

	  const char* CreatePersistentString(const char* volatileString)
	  {
		  auto i = persistentStrings.find(volatileString);
		  if (i == persistentStrings.end())
		  {
			  i = persistentStrings.insert(volatileString,0).first;
		  }

		  return i->first;
	  }

	  EventIdRef CreateEventIdFromVolatileString(const char* volatileString) override
	  {
		  if (volatileString == nullptr)
		  {
			  Throw(0, "IPublisher::CreateEventIdFromVolatileString(...). Argument NULL");
		  }

		  auto* s = CreatePersistentString(volatileString);
		  return EventIdRef{ s, FastHash(volatileString) };
	  }
   };
}

namespace Rococo
{
	namespace Events
	{
		ROCOCO_API IPublisherSupervisor* CreatePublisher()
		{
			return new Publisher();
		}
	} // Events
} // Rococo
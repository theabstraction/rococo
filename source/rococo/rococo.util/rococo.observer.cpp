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

	struct EventIdMapHasher
	{
		size_t operator()(EventHash id) const
		{
			return (size_t)id;
		}
	};

	struct LargestPossibleEvent : EventArgs
	{
		enum { LARGEST_POSSIBLE_EVENT_LENGTH = 256 };
		char data[LARGEST_POSSIBLE_EVENT_LENGTH - sizeof EventArgs];
	};

	struct EventBinding
	{
		EventIdRef ref;
		LargestPossibleEvent largestPossibleEvent;
		cstr senderSignature;
		bool isLossy;

		EventBinding& operator = (const EventBinding& src)
		{
			static_assert(sizeof LargestPossibleEvent == LargestPossibleEvent::LARGEST_POSSIBLE_EVENT_LENGTH);

			memcpy(&largestPossibleEvent, &src.largestPossibleEvent, src.largestPossibleEvent.sizeInBytes);
			isLossy = src.isLossy;
			senderSignature = src.senderSignature;
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
			auto delPoint = std::remove_if(postedEvents.begin(), postedEvents.end(), [](const EventBinding& b)->bool
				{
					return b.isLossy;
				}
			);
			postedEvents.erase(delPoint, postedEvents.end());
		}

		void RawPost(const EventArgs& ev, const EventIdRef& ref, bool isLossy, cstr senderSignature) override
		{
			static_assert(sizeof(LargestPossibleEvent) == 256);

			if (ref.name == nullptr)
			{
				Throw(0, "%s: event name was null", __FUNCTION__);
			}

			LazyInit(ref);

			if (ev.sizeInBytes < sizeof(EventArgs))
			{
				Throw(0, "Publisher: cannot post event. The event.sizeInBytes was < sizeof(EventArgs). Check that it was posted correctly");
			}

			if (ev.sizeInBytes > sizeof(LargestPossibleEvent))
			{
				Throw(0, "Publisher: cannot post event. The event.sizeInBytes was > %llu. Check that it was posted correctly", sizeof(LargestPossibleEvent));
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
			auto* src = reinterpret_cast<const LargestPossibleEvent*>(&ev);
			auto* dst = &evb.largestPossibleEvent;
			memcpy(dst, src, sizeof(LargestPossibleEvent));
			evb.isLossy = isLossy;
			evb.ref = ref;
			evb.senderSignature =  senderSignature;
			postedEvents.push_back(evb);
		}

		void Deliver() override
		{
			std::swap(outgoingEvents, postedEvents);
			postedEvents.clear();

			for (auto& i : outgoingEvents)
			{
				RawPublish(i.largestPossibleEvent, i.ref, i.senderSignature);
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
			auto i = hashToNames.insert(std::make_pair(hashValue, HString(mutableRef.name)));
			if (i.second)
			{
				// insertion took place
			}
			else
			{
				// The event name is already known to the system
				cstr currentBindingName = i.first->second.c_str();
				if (!Eq(currentBindingName, mutableRef.name))
				{
					Throw(0, "EventHash clash between %s and %s", currentBindingName, mutableRef.name);
				}
			}

			mutableRef.hashCode = hashValue;
		}

		void RawPublish(EventArgs& args, const EventIdRef& id, cstr senderSignature) override
		{
			if (id.name == nullptr)
			{
				Throw(0, "%s: event name was null", __FUNCTION__);
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
						Event ev{ *this, args, id, senderSignature };
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
				i = persistentStrings.insert(volatileString, 0).first;
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

namespace Rococo::Events
{
	ROCOCO_API IPublisherSupervisor* CreatePublisher()
	{
		return new Publisher();
	}

	cstr FindMethodArgsFromFuncSig(cstr functionSignature)
	{
		cstr a = functionSignature;
		cstr result = a;

		auto indirector = "::* )("_fstring;
		
		for (;;)
		{
			a = strstr(a, indirector);
			if (a == nullptr)
			{
				break;
			}

			a = a + indirector.length;
			result = a;
		}

		return result;
	}

	ROCOCO_API void ValidateMethodReturnsVoid(const EventIdRef& id, cstr functionSignature)
	{
		cstr voidDef = strstr(functionSignature, "<void(__cdecl ");
		if (voidDef == nullptr)
		{
			Throw(0, "Error subscribing to event '%s': Expected method to be of type void.\n%s", id.name, functionSignature);
		}
	}

	ROCOCO_API void ValidateEventSignature(const Event& ev, cstr functionSignature)
	{
		cstr fargs = FindMethodArgsFromFuncSig(functionSignature);

		Substring evArgType;
		evArgType.start = Strings::FindChar(ev.callerSignature, '<');
		if (!evArgType.start)
		{
			Throw(0, "Expected caller signature to be CallMethod<TYPE>(...) but no <: %s", ev.callerSignature);
		}

		evArgType.start++;
		evArgType.finish = strstr(evArgType.start, ">(");
		if (!evArgType.finish)
		{
			Throw(0, "Expected  caller signature to be CallMethod<TYPE>(...) but no >", ev.callerSignature);
		}

		int64 len = evArgType.Length();

		if (Strings::Compare(fargs, evArgType.start, len) != 0 || !Eq(fargs + len, " &))"))
		{
			char buf[192];
			evArgType.CopyWithTruncate(buf, sizeof buf);
			Throw(0, "Expected method handler for %s to be of type void (%s& args).\nCaller: %s\nHandler: %s", ev.id.name, buf, ev.callerSignature, functionSignature);
		}
	}
}
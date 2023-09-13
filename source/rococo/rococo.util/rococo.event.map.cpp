#define ROCOCO_API __declspec(dllexport)
#include <rococo.events.h>
#include <unordered_map>
#include <new>

using namespace Rococo;
using namespace Rococo::Events;

namespace Rococo::Events
{
	struct EventHasher
	{
		size_t operator()(const EventIdRef& ev) const
		{
			return ev.hashCode;
		}
	};

	bool operator == (const EventIdRef& a, const EventIdRef& b)
	{
		return a.hashCode == b.hashCode;
	}

	using TMapIdToPointer = std::unordered_map<EventIdRef, void*, EventHasher>;

	// We avoid including <unordered_map> in our rococo headers - which is template heavy and thus slow to compile.
	// So we interpret opaque memory. We avoid the PIMPL pattern to speed up searches a fraction.
	TMapIdToPointer& ToMap(EventMapBacking& backing)
	{
		return *reinterpret_cast<TMapIdToPointer*>(&backing);
	}

	EventMapBacking::EventMapBacking()
	{
		static_assert(sizeof TMapIdToPointer <= sizeof EventMapBacking, "Too little memory: if this fails increase the number of backing integers until it works");
		static_assert(2 * sizeof(TMapIdToPointer) >= sizeof EventMapBacking, "Wasted memory: if this fails decrease the number of backing integers until it works");
	}

	EventMap::EventMap()
	{
		TMapIdToPointer& map = ToMap(Backing());
		new (&map) TMapIdToPointer();
	}

	EventMap::~EventMap()
	{
		TMapIdToPointer& map = ToMap(Backing());
		map.~TMapIdToPointer();
	}

	bool EventMap::TryAdd(EventIdRef id, void* ptr)
	{
		if (!ptr)
		{
			Throw(0, "%s: %s: Tried to add a null pointer to an EventMap", __FUNCTION__, id.name);
		}

		auto& map = ToMap(Backing());
		return map.try_emplace(id, ptr).second;
	}

	void* EventMap::Find(EventIdRef id)
	{
		auto& map = ToMap(Backing());
		auto i = map.find(id);
		return i != map.end() ? i->second : nullptr;
	}
}

#pragma once

#include <rococo.events.h>
#include <rococo.clock.h>
#include <unordered_map>

namespace Rococo::Events
{
	template<class HANDLER>
	class MessageMap
	{
		HANDLER& handler;
	public:
		typedef void (HANDLER::*EventHandlerMethod)(EventArgs& ev);
	
	private:		
		std::unordered_map<EventIdRef, EventHandlerMethod> routingTable;

	public:
		MessageMap(HANDLER& thisHandler) : handler(thisHandler)
		{

		}

		void Add(EventIdRef id, HANDLER::EventHandlerMethod method)
		{
			if (!routingTable.try_emplace(id, method))
			{
				Throw(0, "The routing table already had the method: %s", id.name);
			}
		}

		void RouteEvent(Event ev)
		{
			auto i = routingTable.find(ev.id);
			if (i != routingTable.end())
			{
				auto method = i->second;
				(handler.*method)(ev.args);
			}
		}
	};
}
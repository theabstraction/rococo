#pragma once
// Generated at Wed Jun  7 21:20:40 2023

#include "hv.event.declarations.h"

namespace Rococo::Events
{
	extern const EventIdRef EvId_SetNextLevel;

	struct EvArgs_SetNextLevel: Rococo::Events::EventArgs
	{
		cstr levelName;
	};

	void SendSetNextLevel(IPublisher& publisher, cstr levelName);
	void SendSetNextLevelDirect(IPublisher& publisher, EvArgs_SetNextLevel& args);
	EvArgs_SetNextLevel* As_SetNextLevel(Event& ev);

	template<class HANDLER>
	void Add(MessageMap<HANDLER>& map, void (HANDLER::*fn_method)(EvArgs_SetNextLevel& args))
	{
		auto* asMethod = reinterpret_cast<MessageMap<HANDLER>::EventHandlerMethod>(fn_method);
		map.Add(EvId_SetNextLevel, asMethod);
	}
}

namespace Rococo::Events::OS
{
	extern const EventIdRef EvId_Tick;

	struct EvArgs_Tick: Rococo::Events::EventArgs
	{
		IUltraClock* clock;
		uint32 frameSleep;
	};

	void SendTick(IPublisher& publisher, IUltraClock* clock, uint32 frameSleep);
	void SendTickDirect(IPublisher& publisher, EvArgs_Tick& args);
	EvArgs_Tick* As_Tick(Event& ev);

	template<class HANDLER>
	void Add(MessageMap<HANDLER>& map, void (HANDLER::*fn_method)(EvArgs_Tick& args))
	{
		auto* asMethod = reinterpret_cast<MessageMap<HANDLER>::EventHandlerMethod>(fn_method);
		map.Add(EvId_SetNextLevel, asMethod);
	}
}


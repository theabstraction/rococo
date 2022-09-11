#pragma once
// Generated at Sun Sep 11 20:22:00 2022

#include "hv.event.declarations.h"

namespace Rococo::Events
{
	struct EvArgs_SetNextLevel: Rococo::Events::EventArgs
	{
		cstr levelName;
	};

	void SendSetNextLevel(IPublisher& publisher, cstr levelName);
	void SendSetNextLevelDirect(IPublisher& publisher, EvArgs_SetNextLevel& args);
	EvArgs_SetNextLevel* As_SetNextLevel(Event& ev);
}

namespace Rococo::Events::OS
{
	struct EvArgs_Tick: Rococo::Events::EventArgs
	{
		IUltraClock* clock;
		uint32 frameSleep;
	};

	void SendTick(IPublisher& publisher, IUltraClock* clock, uint32 frameSleep);
	void SendTickDirect(IPublisher& publisher, EvArgs_Tick& args);
	EvArgs_Tick* As_Tick(Event& ev);
}


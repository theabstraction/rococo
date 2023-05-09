// Generated: Tue May  9 22:37:34 2023

#include "hv.events.h"

using namespace Rococo;

namespace Rococo::Events
{
	const EventIdRef EvId_SetNextLevel = "EvId_SetNextLevel"_event;

	void SendSetNextLevel(IPublisher& publisher, cstr levelName)
	{
		EvArgs_SetNextLevel args;
		args.levelName = levelName;
		publisher.Publish(args, EvId_SetNextLevel);
	}

	void SendSetNextLevelDirect(IPublisher& publisher, EvArgs_SetNextLevel& args)
	{
		publisher.Publish(args, EvId_SetNextLevel);
	}

	EvArgs_SetNextLevel* As_SetNextLevel(Event& ev)
	{
		if (ev == EvId_SetNextLevel)
		{
			auto& args = As<EvArgs_SetNextLevel>(ev);
			return &args;
		}

		return nullptr;
	}
}

namespace Rococo::Events::OS
{
	const EventIdRef EvId_Tick = "EvId_Tick"_event;

	void SendTick(IPublisher& publisher, IUltraClock* clock, uint32 frameSleep)
	{
		EvArgs_Tick args;
		args.clock = clock;
		args.frameSleep = frameSleep;
		publisher.Publish(args, EvId_Tick);
	}

	void SendTickDirect(IPublisher& publisher, EvArgs_Tick& args)
	{
		publisher.Publish(args, EvId_Tick);
	}

	EvArgs_Tick* As_Tick(Event& ev)
	{
		if (ev == EvId_SetNextLevel)
		{
			auto& args = As<EvArgs_Tick>(ev);
			return &args;
		}

		return nullptr;
	}
}


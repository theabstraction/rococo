#pragma once

namespace Rococo
{
	using namespace Rococo::Events;

	Events::EventId UIInvoke::EvId()
	{
		static EventId invokeEvent = "ui.invoke"_event;
		return invokeEvent;
	}

	namespace Events
	{
		EventId BusyEventId = "mplat.busy"_event;
	}
}
#pragma once

using namespace Rococo::Events;

namespace Rococo
{
	namespace Events
	{
		EventIdRef evBusy = "system.status.busy"_event;
		EventIdRef evUIInvoke = "ui.invoke"_event;
		EventIdRef evUIPopulate = "ui.populate"_event;
		EventIdRef evUIMouseEvent = "ui.mouse"_event;
	}
}

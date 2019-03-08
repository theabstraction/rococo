#pragma once

using namespace Rococo::Events;

namespace Rococo
{
	namespace Input
	{
		EventIdRef evMouseMoveRelative = "input.mouse.delta"_event;
		EventIdRef evMouseChanged = "input.mouse.buttons"_event;
	}

	namespace Events
	{
		EventIdRef evBusy = "system.status.busy"_event;
		EventIdRef evUIInvoke = "ui.invoke"_event;
		EventIdRef evUIPopulate = "ui.populate"_event;
	}
}

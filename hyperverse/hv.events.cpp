#include "hv.events.h"

namespace HV
{
	namespace Events
	{
		EventIdRef evSetNextLevel = "hv.set.next.level"_event;

		namespace OS
		{
			EventIdRef evTick = "os.tick"_event;
			EventIdRef evFileChanged = "os.file.changed"_event;
		}

		namespace Player
		{
			EventIdRef evPlayerAction = "player.action"_event;
			EventIdRef evPlayerDelta = "player.delta"_event;
		}
	}
}

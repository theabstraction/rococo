#include "hv.events.h"

namespace HV
{
	namespace Events
	{
		EventId setNextLevelEventId = "hv.set.next.level"_event;

		namespace OS
		{
			EventId OnTick = "os.tick"_event;
			EventId OnFileChanged = "os.file.changed"_event;
		}

		namespace Player
		{
			EventId OnPlayerAction = "player.action"_event;
			EventId OnPlayerDelta = "player.delta"_event;
		}
	}
}

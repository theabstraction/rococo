#include "hv.h"

namespace HV
{
	using namespace Rococo::Events;

	namespace Events
	{
		extern const EventIdRef evChangeDefaultTextureId;
		extern const EventIdRef evEnableEditor;

		struct ChangeDefaultTextureEvent : public EventArgs
		{
			cstr wallName;
		};

		extern EventIdRef evSetNextLevel;

		struct SetNextLevelEvent : public EventArgs
		{
			cstr name;
		};

		namespace OS
		{
			extern EventIdRef evTick;

			struct TickEvent : public EventArgs
			{
				IUltraClock* clock;
				uint32 frameSleep{ 5 };
			};

			extern EventIdRef evFileChanged;

			struct FileChangedEvent : public EventArgs
			{
				FileModifiedArgs* args;
			};
		}

		namespace Player
		{
			extern EventIdRef evPlayerAction;

			struct PlayerActionEvent : public EventArgs
			{
				cstr Name;
				bool start;
			};

			extern EventIdRef evPlayerDelta;

			struct PlayerDeltaEvent : public EventArgs
			{
				cstr Name;
				float delta;
			};
		} // Player
	} // Events
} // HV
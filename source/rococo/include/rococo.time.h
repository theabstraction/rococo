#pragma once

#include <rococo.types.h>

namespace Rococo::Time
{
	ROCOCO_API void FormatTime(Time::ticks utcTime, char* buffer, size_t nBytes);

	ROCOCO_API [[nodiscard]] ticks TickCount();
	ROCOCO_API [[nodiscard]] ticks TickHz();
	ROCOCO_API [[nodiscard]] ticks UTCTime();
}
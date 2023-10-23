#pragma once

#include <rococo.types.h>

namespace Rococo::Time
{
	ROCOCO_API void FormatTime(Time::ticks utcTime, char* buffer, size_t nBytes);

	ROCOCO_API [[nodiscard]] ticks TickCount();
	ROCOCO_API [[nodiscard]] ticks TickHz();
	ROCOCO_API [[nodiscard]] ticks UTCTime();
	ROCOCO_API [[nodiscard]] double ToMilliseconds(ticks dt);

	class Timer
	{
	private:
		Time::ticks start;
		Time::ticks end;
		const char* name;

	public:
		ROCOCO_API Timer();
		ROCOCO_API void Start();
		ROCOCO_API void End();
		ROCOCO_API Time::ticks ExpiredTime();

		ROCOCO_API Timer(const char* const name);
	};
}

#define TIME_FUNCTION_CALL(argProfileTimer, expressionToBeTimed) argProfileTimer.Start(); expressionToBeTimed; argProfileTimer.End();
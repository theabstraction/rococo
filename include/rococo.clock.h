#pragma once

#include <rococo.types.h>

namespace Rococo
{
	ROCOCO_INTERFACE IUltraClock
	{
		[[nodiscard]] virtual Time::ticks FrameStart() const = 0;	// The time of the current render frame
		[[nodiscard]] virtual Time::ticks Start() const = 0;		// The time at which the mainloop started
		[[nodiscard]] virtual Time::ticks FrameDelta() const = 0;	// The time between the previous frame and the current frame.
		[[nodiscard]] virtual Seconds DT() const = 0; // Get a sanitized timestep in seconds
	};
}
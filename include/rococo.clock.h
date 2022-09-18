#pragma once

#include <rococo.types.h>

namespace Rococo
{
	ROCOCO_INTERFACE IUltraClock
	{
		[[nodiscard]] virtual OS::ticks FrameStart() const = 0;	// The time of the current render frame
		[[nodiscard]] virtual OS::ticks Start() const = 0;		// The time at which the mainloop started
		[[nodiscard]] virtual OS::ticks FrameDelta() const = 0;	// The time between the previous frame and the current frame.
		[[nodiscard]] virtual Seconds DT() const = 0; // Get a sanitized timestep in seconds
	};
}
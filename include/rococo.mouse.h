#pragma once

#include <rococo.types.h>
#include <rococo.events.h>

namespace Rococo::Input
{
	enum MouseFlags : int32
	{
		MouseFlags_LDown = 0x0001,
		MouseFlags_LUp = 0x0002,
		MouseFlags_RDown = 0x0004,
		MouseFlags_RUp = 0x0008,
		MouseFlags_MDown = 0x0010,
		MouseFlags_MUp = 0x0020,
		MouseFlags_Wheel = 0x0400,
		MouseFlags_LRMW = 0x043F
	};

	struct OnMouseMoveRelativeEvent : public Events::EventArgs
	{
		int32 dx;
		int32 dy;
		int32 dz;
	};

	struct OnMouseChangedEvent : public Events::EventArgs
	{
		int32 flags;
	};
}
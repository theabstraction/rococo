#pragma once

#include <rococo.events.h>

namespace Rococo::Events
{
	struct ScrollEvent;

	struct BusyEvent : public EventArgs
	{
		boolean32 isNowBusy;
		cstr message;
		U8FilePath pingPath;
	};

	struct DirectMouseEvent : public EventArgs
	{
		const MouseEvent& me;
		bool consumed = false;
		DirectMouseEvent(const MouseEvent& _me) : me(_me) {}
	};

	extern EventIdRef evUIMouseEvent;
	extern EventIdRef evUIInvoke;
	extern EventIdRef evUIPopulate;
	extern EventIdRef evBusy;
	extern EventIdRef evScreenResize;

	struct AsciiEventArgs : public Events::EventArgs
	{
		fstring asciiText;
	};

	struct UIInvoke : public Events::EventArgs
	{
		char command[232];
	};

	struct PingPathArgs : public Events::EventArgs
	{
		cstr pingPath;
	};

}
#pragma once

namespace Rococo
{
	struct IApp;
	struct Platform;
	struct KeyboardEvent;
	struct MouseEvent;

	ROCOCO_INTERFACE IAppFactory
	{
		virtual IApp * CreateApp(Platform & platform) = 0;
	};

	ROCOCO_INTERFACE IDirectAppControl
	{
		virtual bool TryGetNextKeyboardEvent(KeyboardEvent & k) = 0;
		virtual bool TryGetNextMouseEvent(MouseEvent& m) = 0;
		virtual bool TryRouteSysMessages(uint32 sleepMS) = 0;
		virtual void GetNextMouseDelta(Vec2& delta) = 0;
	};

	ROCOCO_INTERFACE IDirectApp
	{
		virtual void Free() = 0;
		virtual void Run() = 0;
	};

	ROCOCO_INTERFACE IDirectAppFactory
	{
	    virtual IDirectApp * CreateApp(Platform & platform, IDirectAppControl& control) = 0;
	};
}
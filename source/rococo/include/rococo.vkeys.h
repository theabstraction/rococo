// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#ifdef _WIN32
# include <rococo.vkeys.win32.h>
#else
namespace Rococo::IO::VirtualKeys
{
	enum VKCode : int
	{
		VKCode_ENTER,
		VKCode_ESCAPE,
		VKCode_SPACEBAR,
		VKCode_TAB,
		VKCode_BACKSPACE,
		VKCode_DELETE,
		VKCode_LEFT,
		VKCode_RIGHT,
		VKCode_UP,
		VKCode_DOWN,
		VKCode_C,
		VKCode_V,
		VKCode_PGUP,
		VKCode_PGDOWN,
		VKCode_HOME,
		VKCode_END,
		VKCode_ANTITAB
	};
}
#endif
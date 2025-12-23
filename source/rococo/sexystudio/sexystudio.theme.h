#pragma once

#include <rococo.types.h>
#include <rococo.eventargs.h>

namespace Rococo::SexyStudio
{
	struct ColourSet
	{
		RGBAb bkColor;
		RGBAb edgeColor;
		RGBAb txColor;
	};

	struct Theme
	{
		ColourSet normal;
		ColourSet lit;
	};

	ROCOCO_INTERFACE ITheme
	{
		// Get a mutable ref to the theme, allowing modification of a theme
		virtual Theme & GetTheme() = 0;
		virtual void Free() = 0;
	};

	// Use a theme, if the name is unknown a default theme is used
	// call free to cancel application of the theme
	ITheme* UseNamedTheme(cstr name, Events::IPublisher& publisher);

	Theme GetTheme(Events::IPublisher& publisher);

	struct ThemeInfo
	{
		cstr name;
		const Theme& theme;
	};

	void EnumerateThemes(IEventCallback<const ThemeInfo>& cb);

	// uses TEventArg<Theme> 
	extern Events::EventIdRef evGetTheme;
}
#include "sexystudio.api.h"
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;

namespace
{
	Theme darkTheme
	{
		{ // normal		
			RGBAb(48,48,48),  // background
			RGBAb(224,224,224), // edge
			RGBAb(224,224,224) // text
		},
		{ // hilight
			RGBAb(64,64,64), // background
			RGBAb(255,255,255), // edge
			RGBAb(255,255,255)// text
		}
	};

	struct ThemeSink: ITheme, IObserver
	{
		Theme theme;
		IPublisher& publisher;

		ThemeSink(IPublisher& _publisher):
			publisher(_publisher)
		{
			publisher.Subscribe(this, evGetTheme);
		}

		~ThemeSink()
		{
			publisher.Unsubscribe(this);
		}
		
		Theme& GetTheme() override
		{
			return theme;
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evGetTheme)
			{
				auto& arg = As<TEventArgs<Theme>>(ev);
				arg.value = theme;
			}
		}
	};
}

namespace Rococo::SexyStudio
{
	EventIdRef evGetTheme = "evGetTheme"_event;

	Theme darkTheme
	{
		{ // normal		
			RGBAb(48,48,48),  // background
			RGBAb(224,224,224), // edge
			RGBAb(224,224,224) // text
		},
		{ // hilight
			RGBAb(64,64,64), // background
			RGBAb(255,255,255), // edge
			RGBAb(255,255,255)// text
		}
	};

	Theme classicTheme
	{
		{ // normal		
			RGBAb(200,200,200),  // background
			RGBAb(64,64,64), // edge
			RGBAb(0,0,0) // text
		},
		{ // hilight
			RGBAb(224,224,255), // background
			RGBAb(96,96,96), // edge
			RGBAb(64,64,64)// text
		}
	};

	const Theme& DefaultTheme() { return darkTheme; }

	ITheme* UseNamedTheme(cstr name, IPublisher& publisher)
	{
		auto* t = new ThemeSink(publisher);

		// Convert to hashtable if we need more themes
		if (name == nullptr)
		{
			t->theme = DefaultTheme();
		}
		if (Eq(name, "Dark"))
		{
			t->theme = darkTheme;
		}
		else if (Eq(name, "Classic"))
		{
			t->theme = classicTheme;
		}
		else
		{
			t->theme = DefaultTheme();
		}

		return t;
	}

	void EnumerateThemes(IEventCallback<const ThemeInfo>& cb)
	{
		cb.OnEvent({ "Dark", darkTheme });
		cb.OnEvent({ "Classic", classicTheme });
	}

	Theme GetTheme(IPublisher& publisher)
	{
		TEventArgs<Theme> arg;
		arg.value = DefaultTheme();
		publisher.Publish(arg, evGetTheme);
		return arg.value;
	}
}
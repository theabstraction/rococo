#pragma once

#include <rococo.types.h>

namespace Rococo::Events
{
	typedef uint32 EventHash;

#pragma pack(push,1)
	struct EventIdRef
	{
		const char* name;
		mutable EventHash hashCode;
	};

	struct EventArgs
	{
		int64 sizeInBytes;
	};
#pragma pack(pop)

	template<class T> struct TEventArgs : public EventArgs
	{
		T value;
		operator T () { return value; }
	};

	template<class T, class U> struct T2EventArgs : public EventArgs
	{
		T value1;
		U value2;
	};

	class IPublisher;

	struct Event
	{
		IPublisher& publisher;
		EventArgs& args;
		EventIdRef id;
	};
}

namespace Rococo
{
	inline Events::EventIdRef operator "" _event(cstr name, size_t len)
	{
		UNUSED(len);
		return Events::EventIdRef{ name, 0 };
	}
}
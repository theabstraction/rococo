#pragma once

#include <rococo.types.h>

namespace Rococo
{
	struct KeyboardEvent;
	struct MouseEvent;
}

namespace Rococo::Events
{
	typedef uint32 EventHash;

	struct EventIdRef
	{
		const char* name;
		mutable EventHash hashCode;
	};

	struct EventArgs
	{
		int64 sizeInBytes;
	};

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

	// Used by GUI panels to request an upate to a label or field just before rendering.
		// A request is sent out by the gui panel, and the formatter object responds with the same id, but setting isRequested to false
	struct TextOutputEvent : EventArgs
	{
		bool isGetting;
		char text[128];
	};

	struct RouteKeyboardEvent : EventArgs
	{
		const KeyboardEvent* ke;
		bool consume;
	};

	struct RouteMouseEvent : EventArgs
	{
		const MouseEvent* me;
		Vec2i absTopleft;
	};

	struct ScreenResizeEvent : EventArgs
	{
		GuiRect bounds;
	};

	struct VisitorItemClickedEvent : EventArgs
	{
		cstr key;
		cstr value;
	};

	ROCOCOAPI IObserver
	{
	   virtual void OnEvent(Event & ev) = 0;
	};

	class ROCOCO_NO_VTABLE IPublisher
	{
	private:
		virtual void RawPost(const EventArgs& ev, const EventIdRef& id, bool isLossy) = 0;
		virtual void RawPublish(EventArgs& ev, const EventIdRef& id) = 0;
	public:
		virtual bool Match(const Event& ev, const EventIdRef& ref) = 0;
		virtual EventIdRef CreateEventIdFromVolatileString(const char* volatileString) = 0;
		virtual void Deliver() = 0;
		virtual void Subscribe(IObserver* observer, const EventIdRef& eventName) = 0;
		virtual void Unsubscribe(IObserver* observer) = 0;
		virtual void ThrowBadEvent(const Event& ev) = 0;

		template<class T> inline void Post(T& ev, const EventIdRef& id, bool isLossy)
		{
			ev.sizeInBytes = sizeof(T);
			RawPost(ev, id, isLossy);
		}
		template<class T> inline void Publish(T& ev, const EventIdRef& id)
		{
			ev.sizeInBytes = sizeof(T);
			RawPublish(ev, id);
		}
	};

	inline bool operator == (const Event& ev, const EventIdRef& ref)
	{
		return ev.publisher.Match(ev, ref);
	}

	inline bool operator == (const EventIdRef& ref, const Event& ev)
	{
		return ev == ref;
	}

	ROCOCOAPI IPublisherSupervisor : public IPublisher
	{
	   virtual void Free() = 0;
	};

	[[nodiscard]] IPublisherSupervisor* CreatePublisher();

	template<class T> [[nodiscard]] inline T& As(Event& ev)
	{
		T& t = static_cast<T&>(ev.args);
		if (t.sizeInBytes != sizeof(T)) ev.publisher.ThrowBadEvent(ev);
		return t;
	}
}

namespace Rococo
{
	inline Events::EventIdRef operator "" _event(cstr name, size_t len)
	{
		return Events::EventIdRef{ name, 0 };
	}
}
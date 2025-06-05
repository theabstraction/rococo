#pragma once

#include <rococo.eventargs.h>

namespace Rococo
{
	struct KeyboardEvent;
	struct MouseEvent;
}

namespace Rococo::Events
{
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

	struct WindowResizeEvent : EventArgs
	{
		bool isFullscreen;
		Vec2i span;
	};

	ROCOCO_INTERFACE IObserver
	{
	   virtual void OnEvent(Event & ev) = 0;
	};

	enum class PostQuality
	{
		Overwrites, // Removes previous entries to make space for new posts
		Guaranteed,
		Lossy
	};

	class ROCOCO_NO_VTABLE IPublisher
	{
	private:
		virtual void RawPost(const EventArgs& ev, const EventIdRef& id, PostQuality quality, cstr senderSignature) = 0;
		virtual void RawPublish(EventArgs& ev, const EventIdRef& id, cstr senderSignature) = 0;
	public:
		virtual bool Match(const Event& ev, const EventIdRef& ref) = 0;
		virtual EventIdRef CreateEventIdFromVolatileString(const char* volatileString) = 0;
		virtual void Deliver() = 0;
		virtual void Subscribe(IObserver* observer, const EventIdRef& eventName) = 0;
		virtual void Unsubscribe(IObserver* observer) = 0;
		virtual void ThrowBadEvent(const Event& ev) = 0;

		template<class T> inline void Post(T& ev, const EventIdRef& id, PostQuality quality = PostQuality::Guaranteed)
		{
			ev.sizeInBytes = sizeof(T);
			RawPost(ev, id, quality, __FUNCSIG__);
		}

		// Posts a message that is wrapped up in type TEventArgs<T>. 
		// The event handler method should have signature void Instance::Handler(TEventArgs<T>& args)
		template<class T> inline void PostOneArg(T arg, const EventIdRef& id, PostQuality quality = PostQuality::Guaranteed)
		{
			TEventArgs<T> args;
			args.value = arg;
			Post(args, id, quality);
		}

		template<class T> inline void Publish(T& ev, const EventIdRef& id)
		{
			ev.sizeInBytes = sizeof(T);
			RawPublish(ev, id, __FUNCSIG__);
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

	ROCOCO_INTERFACE IPublisherSupervisor : public IPublisher
	{
	   virtual void Free() = 0;
	};

	ROCOCO_API [[nodiscard]] IPublisherSupervisor* CreatePublisher();

	template<class T> [[nodiscard]] inline T& As(Event& ev)
	{
		T& t = static_cast<T&>(ev.args);
		if (t.sizeInBytes != sizeof(T)) ev.publisher.ThrowBadEvent(ev);
		return t;
	}
}

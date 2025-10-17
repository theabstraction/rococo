// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.events.h>
#include <vector>

namespace Rococo::Events
{
	ROCOCO_API void ValidateEventSignature(const Event& ev, cstr functionSignature);
	ROCOCO_API void ValidateMethodReturnsVoid(const EventIdRef& id, cstr functionSignature);

	template<class PARENT>
	struct MessageMap : IObserver
	{
		IPublisher& publisher;
		PARENT& parent;

		MessageMap(IPublisher& _publisher, PARENT& _parent) : publisher(_publisher), parent(_parent) {}

		~MessageMap()
		{
			publisher.Unsubscribe(this);
		}

		typedef void (PARENT::* METHOD_HANDLER)(EventArgs& args);

		struct MethodBinding
		{
			EventIdRef id;
			METHOD_HANDLER handler;
			cstr functionSignature;
		};

		std::vector<MethodBinding> methods;

		template<class METHOD_POINTER>
		void AddHandler(EventIdRef id, METHOD_POINTER ptr)
		{
			cstr functionSig = __FUNCSIG__;
			ValidateMethodReturnsVoid(id, functionSig);

			publisher.Subscribe(this, id);
			methods.push_back(MethodBinding{ id, reinterpret_cast<METHOD_HANDLER>(ptr),functionSig });
		}

		void OnEvent(Event& ev) override
		{
			for (auto& method : methods)
			{
				if (method.id == ev)
				{
#ifdef _DEBUG
					ValidateEventSignature(ev, method.functionSignature);
#endif
					auto methodPtr = method.handler;
					(parent.*methodPtr)(ev.args);
					return;
				}
			}
		}
	};

}
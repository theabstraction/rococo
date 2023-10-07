#pragma once

#include <rococo.types.h>
#include <vector>
#include <rococo.functional.h>

namespace Rococo
{
	template<typename... ARGS>
	struct EventImpressario : IEventImpressario<ARGS...>
	{
		std::vector<Rococo::Function<void(ARGS...)>> callbacks;

		void Add(Rococo::Function<void(ARGS...)> handler) override
		{
			callbacks.push_back(handler);
		}

		void Invoke(ARGS... args)
		{
			for (auto& c : callbacks)
			{				
				c.Invoke(args...);
			}
		}
	};
}
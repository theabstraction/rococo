#pragma once

#include <rococo.types.h>

namespace Rococo
{
	class RecursionGuard
	{
	private:
		int32& counter;
	public:
		FORCE_INLINE RecursionGuard(int32& _counter) : counter(_counter) { counter++; }
		FORCE_INLINE ~RecursionGuard() { counter--; }
	};
}
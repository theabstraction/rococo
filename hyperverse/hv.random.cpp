#include "hv.h"
#include <rococo.random.h>
#include <random>

Rococo::Random::RandomMT rng;

namespace HV
{
	namespace Roll
	{
		uint32 d(uint32 maxValue)
		{
			return (rng() % maxValue) + 1;
		}

		uint32 x(uint32 oneAboveMaxValue)
		{
			return rng() % oneAboveMaxValue;
		}

		// 50% chance to return true, else it returns false
		boolean32 FiftyFifty()
		{
			return (rng() % 2) == 0;
		}

		float AnyOf(float minValue, float maxValue)
		{
			return Random::NextFloat(rng, minValue, maxValue);
		}
	}
}
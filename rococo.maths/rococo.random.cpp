#include <rococo.api.h>
#include <random>
#include <rococo.maths.h>

namespace
{
	typedef std::mt19937 Randomizer;
	Randomizer rng;

	constexpr float OneOverUint32Max() { return 1.0f / (float)0xFFFFFFFFU; }
}

namespace Rococo
{
	namespace Random
	{
		uint32 Next()
		{
			return rng();
		}

		uint32 Next(uint32 modulus)
		{
			return rng() % modulus;
		}

		void Seed(uint32 value)
		{
			if (value == 0)
			{
				value = (uint32)CpuClock();
			}
			rng.seed(value);
		}

		float NextFloat(float minValue, float maxValue)
		{
			float range = maxValue - minValue;

			float q = OneOverUint32Max() * (float)Next();
			return q * range + minValue;
		}

		Vec3 NextNormalVector()
		{
			Vec3 result;
			result.x = NextFloat(-1.0f, 1.0f);
			result.y = NextFloat(-1.0f, 1.0f);
			result.z = NextFloat(-1.0f, 1.0f);

			Vec3 modResult;
			return TryNormalize(result, modResult) ? modResult : Vec3{ 1,0,0 };
		}
	}
}
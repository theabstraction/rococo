#include <rococo.api.h>
#include <random>
#include <rococo.maths.h>
#include <new>

namespace
{
	constexpr float OneOverUint32Max() { return 1.0f / (float)0xFFFFFFFFU; }
	typedef std::mt19937 TRandomizer;
}

namespace Rococo
{
	namespace Random
	{
		RandomMT::RandomMT(uint32 seed)
		{
			static_assert(sizeof(RandomMT::OpaqueBlock) > sizeof(TRandomizer), "Insufficient data in opaque buffer");
			TRandomizer* rng = new (block.opaque) TRandomizer(seed == 0 ? (uint32) CpuClock() : seed);
			(*rng)();
		}

		RandomMT::~RandomMT()
		{
			TRandomizer* rng = reinterpret_cast<TRandomizer*> (block.opaque);
			rng->~TRandomizer();
		}

		void RandomMT::Seed(uint32 index)
		{
			TRandomizer& rng = *reinterpret_cast<TRandomizer*> (block.opaque);
			rng.seed(index);
		}

		uint32 RandomMT::operator()()
		{
			TRandomizer& rng = *reinterpret_cast<TRandomizer*> (block.opaque);
			return rng();
		}
		
		uint32 Next(IRandom& rng)
		{
			return rng();
		}

		uint32 Next(IRandom& rng, uint32 modulus)
		{
			return rng() % modulus;
		}

		void Seed(IRandom& rng, uint32 value)
		{
			if (value == 0)
			{
				value = (uint32)CpuClock();
			}
			rng.Seed(value);
		}

		float NextFloat(IRandom& rng, float minValue, float maxValue)
		{
			float range = maxValue - minValue;

			float q = OneOverUint32Max() * (float)Next(rng);
			return q * range + minValue;
		}

		Vec3 NextNormalVector(IRandom& rng)
		{
			Vec3 result;
			result.x = NextFloat(rng, -1.0f, 1.0f);
			result.y = NextFloat(rng, -1.0f, 1.0f);
			result.z = NextFloat(rng, -1.0f, 1.0f);

			Vec3 modResult;
			return TryNormalize(result, modResult) ? modResult : Vec3{ 1,0,0 };
		}
	}
}
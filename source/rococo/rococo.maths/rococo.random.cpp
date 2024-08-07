#include <rococo.api.h>
#include <rococo.os.h>
#include <random>
#include <rococo.maths.h>
#include <new>
#include <rococo.random.h>
#include <rococo.time.h>

namespace
{
	constexpr float OneOverUint32Max() { return 1.0f / (float)0xFFFFFFFFU; }
	typedef std::mt19937 TRandomizer;
}

namespace Rococo::Random
{
	RandomMT::RandomMT(uint32 seed)
	{
		static_assert(sizeof(RandomMT::OpaqueBlock) > sizeof(TRandomizer), "Insufficient data in opaque buffer");
		TRandomizer* rng = new (block.opaque) TRandomizer(seed == 0 ? (uint32) Time::TickCount() : seed);
		auto dummy = (*rng)();
		UNUSED(dummy);
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
			value = (uint32)Time::TickCount();
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

	struct ShufflerImpl
	{
		Rococo::Random::RandomMT rng;
	};

	Shuffler::Shuffler(uint32 seed) :
		impl(new ShufflerImpl())
	{
		if (seed == 0) seed = 0xFFFFFFFFL & Time::TickCount();
	}

	Shuffler::~Shuffler()
	{
		delete impl;
	}

	Shuffler::result_type Shuffler::operator()()
	{
		return impl->rng() & 0x7FFFFFFFLL;
	}
}
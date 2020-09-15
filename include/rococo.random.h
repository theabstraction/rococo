#pragma once

#include <rococo.types.h>

namespace Rococo::Random
{
	ROCOCOAPI IRandom
	{
		[[nodiscard]] virtual uint32 operator()() = 0;
		virtual void Seed(uint32 value) = 0;
	};

	uint32 Next(IRandom& rng);
	uint32 Next(IRandom& rng, uint32 modulus);
	void Seed(IRandom& rng, uint32 value = 0);
	float NextFloat(IRandom& rng, float minValue, float maxValue);
	Vec3 NextNormalVector(IRandom& rng);

	class RandomMT : public IRandom
	{
	public:
		RandomMT(uint32 seed = 0);
		~RandomMT();
		virtual uint32 operator()();
		virtual void Seed(uint32 index);
	private:
		struct alignas(16) OpaqueBlock
		{
			uint8 opaque[6 * 1024];
		} block;
	};
}

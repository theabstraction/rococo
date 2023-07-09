#pragma once

#include <rococo.types.h>

namespace Rococo::Random
{
	ROCOCO_INTERFACE IRandom
	{
		[[nodiscard]] virtual uint32 operator()() = 0;
		virtual void Seed(uint32 value) = 0;
	};

	uint32 Next(IRandom& rng);
	uint32 Next(IRandom& rng, uint32 modulus);
	void Seed(IRandom& rng, uint32 value = 0);
	float NextFloat(IRandom& rng, float minValue, float maxValue);
	Vec3 NextNormalVector(IRandom& rng);

	class alignas(16) RandomMT : public IRandom
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

	struct ShufflerImpl;

	class Shuffler
	{
	public:
		typedef ptrdiff_t result_type;

		static constexpr result_type min() { return 0; }
		static constexpr result_type max() { return 0x7FFFFFFF; }

		result_type operator()();

		Shuffler(uint32 seed);
		~Shuffler();
	private:
		ShufflerImpl* impl;
	};
}

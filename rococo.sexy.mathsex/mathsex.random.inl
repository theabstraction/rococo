namespace
{
	void NativeSysRandomAnyColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 aMax;
		_offset += sizeof(aMax);
		ReadInput(aMax, _sf, -_offset);

		int32 aMin;
		_offset += sizeof(aMin);
		ReadInput(aMin, _sf, -_offset);

		int32 bMax;
		_offset += sizeof(bMax);
		ReadInput(bMax, _sf, -_offset);

		int32 bMin;
		_offset += sizeof(bMin);
		ReadInput(bMin, _sf, -_offset);

		int32 gMax;
		_offset += sizeof(gMax);
		ReadInput(gMax, _sf, -_offset);

		int32 gMin;
		_offset += sizeof(gMin);
		ReadInput(gMin, _sf, -_offset);

		int32 rMax;
		_offset += sizeof(rMax);
		ReadInput(rMax, _sf, -_offset);

		int32 rMin;
		_offset += sizeof(rMin);
		ReadInput(rMin, _sf, -_offset);

		RGBAb colour = Rococo::Random::AnyColour(rMin, rMax, gMin, gMax, bMin, bMax, aMin, aMax);
		_offset += sizeof(colour);
		WriteOutput(colour, _sf, -_offset);
	}

	void NativeSysRandomAnyFloat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float value = Rococo::Random::AnyFloat(minValue, maxValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysRandomAnyInt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		int32 minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		int32 value = Rococo::Random::AnyInt(minValue, maxValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysRandomRand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 modulus;
		_offset += sizeof(modulus);
		ReadInput(modulus, _sf, -_offset);

		int32 value = Rococo::Random::Rand(modulus);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysRandomRollDice(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 sides;
		_offset += sizeof(sides);
		ReadInput(sides, _sf, -_offset);

		int32 count;
		_offset += sizeof(count);
		ReadInput(count, _sf, -_offset);

		int32 sum = Rococo::Random::RollDice(count, sides);
		_offset += sizeof(sum);
		WriteOutput(sum, _sf, -_offset);
	}

	void NativeSysRandomRollDie(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 sides;
		_offset += sizeof(sides);
		ReadInput(sides, _sf, -_offset);

		int32 value = Rococo::Random::RollDie(sides);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysRandomSeed(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		Rococo::Random::Seed(value);
	}

}

namespace Sys { namespace Random { 
	void AddNativeCalls_SysRandom(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Random"));
		ss.AddNativeCall(ns, NativeSysRandomAnyColour, nullptr, ("AnyColour(Int32 rMin)(Int32 rMax)(Int32 gMin)(Int32 gMax)(Int32 bMin)(Int32 bMax)(Int32 aMin)(Int32 aMax) -> (Int32 colour)"));
		ss.AddNativeCall(ns, NativeSysRandomAnyFloat, nullptr, ("AnyFloat(Float32 minValue)(Float32 maxValue) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysRandomAnyInt, nullptr, ("AnyInt(Int32 minValue)(Int32 maxValue) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysRandomRand, nullptr, ("Rand(Int32 modulus) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysRandomRollDice, nullptr, ("RollDice(Int32 count)(Int32 sides) -> (Int32 sum)"));
		ss.AddNativeCall(ns, NativeSysRandomRollDie, nullptr, ("RollDie(Int32 sides) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysRandomSeed, nullptr, ("Seed(Int32 value) -> "));
	}
}}
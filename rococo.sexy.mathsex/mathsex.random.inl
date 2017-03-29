namespace
{
	void NativeSysRandomRollDie(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 sides;
		_offset += sizeof(sides);
		ReadInput(sides, _sf, -_offset);

		int32 value = Rococo::Random::RollDie(sides);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysRandomRollDice(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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

	void NativeSysRandomSeed(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		Rococo::Random::Seed(value);
	}

	void NativeSysRandomAnyOf(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float value = Rococo::Random::AnyOf(minValue, maxValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

}

namespace Sys { namespace Random { 
	void AddNativeCalls_SysRandom(Sexy::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Random"));
		ss.AddNativeCall(ns, NativeSysRandomRollDie, nullptr, SEXTEXT("RollDie(Int32 sides) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysRandomRollDice, nullptr, SEXTEXT("RollDice(Int32 count)(Int32 sides) -> (Int32 sum)"));
		ss.AddNativeCall(ns, NativeSysRandomSeed, nullptr, SEXTEXT("Seed(Int32 value) -> "));
		ss.AddNativeCall(ns, NativeSysRandomAnyOf, nullptr, SEXTEXT("AnyOf(Float32 minValue)(Float32 maxValue) -> (Float32 value)"));
	}
}}
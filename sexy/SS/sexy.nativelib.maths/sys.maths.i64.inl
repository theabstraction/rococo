namespace
{
	void NativeSysMathsI64Clamp(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		int64 upper;
		_offset += sizeof(upper);
		ReadInput(upper, _sf, -_offset);

		int64 lower;
		_offset += sizeof(lower);
		ReadInput(lower, _sf, -_offset);

		int64 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		int64 value = min(max(x, lower), upper);

		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64Abs(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = llabs(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64LeftShift(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 bitCount;
		_offset += sizeof(bitCount);

		ReadInput(bitCount, _sf, -_offset);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = LeftShift(x, bitCount);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64RightShift(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 bitCount;
		_offset += sizeof(bitCount);

		ReadInput(bitCount, _sf, -_offset);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = RightShift(x, bitCount);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64MaxOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 maxValue = MaxOf(x, y);
		_offset += sizeof(maxValue);
		WriteOutput(maxValue, _sf, -_offset);
	}

	void NativeSysMathsI64MinOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 minValue = MinOf(x, y);
		_offset += sizeof(minValue);
		WriteOutput(minValue, _sf, -_offset);
	}

	void NativeSysMathsI64MinValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 value = MinI64Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64MaxValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 value = MaxI64Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64Mod(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 denominator;
		_offset += sizeof(denominator);

		ReadInput(denominator, _sf, -_offset);
		int64 numerator;
		_offset += sizeof(numerator);

		ReadInput(numerator, _sf, -_offset);
		int64 remainder = Mod(numerator, denominator);
		_offset += sizeof(remainder);
		WriteOutput(remainder, _sf, -_offset);
	}

	void NativeSysMathsI64ToInt32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = ToInt32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64ToFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = ToFloat32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI64ToFloat64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = ToFloat64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
}

namespace Sys { namespace Maths { namespace I64 { 
	void AddNativeCalls_SysMathsI64(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Maths.I64"));
		ss.AddNativeCall(ns, NativeSysMathsI32Clamp, nullptr, ("Clamp(Int64 x)(Int64 lower)(Int64 upper) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64Abs, nullptr, ("Abs(Int64 x) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64LeftShift, nullptr, ("LeftShift(Int64 x)(Int64 bitCount) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64RightShift, nullptr, ("RightShift(Int64 x)(Int64 bitCount) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64MaxOf, nullptr, ("MaxOf(Int64 x)(Int64 y) -> (Int64 maxValue)"));
		ss.AddNativeCall(ns, NativeSysMathsI64MinOf, nullptr, ("MinOf(Int64 x)(Int64 y) -> (Int64 minValue)"));
		ss.AddNativeCall(ns, NativeSysMathsI64MinValue, nullptr, ("MinValue -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64MaxValue, nullptr, ("MaxValue -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64Mod, nullptr, ("Mod(Int64 numerator)(Int64 denominator) -> (Int64 remainder)"));
		ss.AddNativeCall(ns, NativeSysMathsI64ToInt32, nullptr, ("ToInt32(Int64 x) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64ToFloat32, nullptr, ("ToFloat32(Int64 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI64ToFloat64, nullptr, ("ToFloat64(Int64 x) -> (Float64 value)"));
	}
}}}
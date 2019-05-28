namespace
{
	void NativeSysMathsI32Abs(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = abs(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32LeftShift(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 bitCount;
		_offset += sizeof(bitCount);

		ReadInput(bitCount, _sf, -_offset);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = LeftShift(x, bitCount);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32RightShift(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 bitCount;
		_offset += sizeof(bitCount);

		ReadInput(bitCount, _sf, -_offset);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = RightShift(x, bitCount);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32MaxOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 maxValue = MaxOf(x, y);
		_offset += sizeof(maxValue);
		WriteOutput(maxValue, _sf, -_offset);
	}

	void NativeSysMathsI32MinOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 minValue = MinOf(x, y);
		_offset += sizeof(minValue);
		WriteOutput(minValue, _sf, -_offset);
	}

	void NativeSysMathsI32MinValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value = MinI32Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32MaxValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value = MaxI32Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32Mod(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 denominator;
		_offset += sizeof(denominator);

		ReadInput(denominator, _sf, -_offset);
		int32 numerator;
		_offset += sizeof(numerator);

		ReadInput(numerator, _sf, -_offset);
		int32 remainder = Mod(numerator, denominator);
		_offset += sizeof(remainder);
		WriteOutput(remainder, _sf, -_offset);
	}

	void NativeSysMathsI32BitwiseAnd(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int32 result = x & y;
		_offset += sizeof(result);

		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsI32BitwiseOr(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		int32 result = x | y;
		_offset += sizeof(result);

		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsI32BitwiseXor(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);

		int32 result = x ^ y;
		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsI32HasFlags(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);

		boolean32 result = (boolean32) ((x & y) != 0);
		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsI32BitwiseNot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		int32 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);
		
		int32 result = ~x;
		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsI32ToInt64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = ToInt64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32ToFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = ToFloat32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsI32ToFloat64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = ToFloat64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

}

namespace Sys { namespace Maths { namespace I32 { 
	void AddNativeCalls_SysMathsI32(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Maths.I32"));
		ss.AddNativeCall(ns, NativeSysMathsI32Abs, nullptr, ("Abs(Int32 x) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32LeftShift, nullptr, ("LeftShift(Int32 x)(Int32 bitCount) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32RightShift, nullptr, ("RightShift(Int32 x)(Int32 bitCount) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32MaxOf, nullptr, ("MaxOf(Int32 x)(Int32 y) -> (Int32 maxValue)"));
		ss.AddNativeCall(ns, NativeSysMathsI32MinOf, nullptr, ("MinOf(Int32 x)(Int32 y) -> (Int32 minValue)"));
		ss.AddNativeCall(ns, NativeSysMathsI32MinValue, nullptr, ("MinValue -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32MaxValue, nullptr, ("MaxValue -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32Mod, nullptr, ("Mod(Int32 numerator)(Int32 denominator) -> (Int32 remainder)"));
		ss.AddNativeCall(ns, NativeSysMathsI32ToInt64, nullptr, ("ToInt64(Int32 x) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32ToFloat32, nullptr, ("ToFloat32(Int32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32ToFloat64, nullptr, ("ToFloat64(Int32 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsI32BitwiseAnd, nullptr, ("BitwiseAnd(Int32 x)(Int32 y) -> (Int32 result)"));
		ss.AddNativeCall(ns, NativeSysMathsI32BitwiseOr, nullptr, ("BitwiseOr(Int32 x)(Int32 y) -> (Int32 result)"));
		ss.AddNativeCall(ns, NativeSysMathsI32BitwiseNot, nullptr, ("BitwiseOr(Int32 x) -> (Int32 notX)"));
		ss.AddNativeCall(ns, NativeSysMathsI32BitwiseXor, nullptr, ("BitwiseOr(Int32 x)(Int32 y) -> (Int32 result)"));
		ss.AddNativeCall(ns, NativeSysMathsI32HasFlags, nullptr, ("HasFlags(Int32 flags)(Int32 flag) -> (Bool result)"));
	}
}}}
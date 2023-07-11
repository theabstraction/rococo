namespace
{
	void NativeSysMathsF64Clamp(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		double upper;
		_offset += sizeof(upper);
		ReadInput(upper, _sf, -_offset);

		double lower;
		_offset += sizeof(lower);
		ReadInput(lower, _sf, -_offset);

		double x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		double value = min(max(x, lower), upper);

		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Sin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float64 value = sin(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Cos(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float64 value = cos(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Tan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float64 value = tan(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ArcSin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = asin(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ArcCos(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = acos(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ArcTan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = atan(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ArcTanYX(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float64 value = atan2(y, x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Abs(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = fabs(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Sinh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = sinh(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Cosh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = cosh(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Tanh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = tanh(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Exp(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = exp(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Floor(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = floor(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Ceiling(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = ceil(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Power(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 exponent;
		_offset += sizeof(exponent);

		ReadInput(exponent, _sf, -_offset);
		float64 base;
		_offset += sizeof(base);

		ReadInput(base, _sf, -_offset);
		float64 value = pow(base, exponent);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64LogN(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = log(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64Log10(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = log10(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64SquareRoot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = sqrt(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ErrorFunction(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 y = erf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsF64MaxOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 maxValue = MaxOf(x, y);
		_offset += sizeof(maxValue);
		WriteOutput(maxValue, _sf, -_offset);
	}

	void NativeSysMathsF64MinOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 minValue = MinOf(x, y);
		_offset += sizeof(minValue);
		WriteOutput(minValue, _sf, -_offset);
	}

	void NativeSysMathsF64RoundToNearest(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 y = round(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsF64IsInfinity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isInfinity = IsInfinity(x);
		_offset += sizeof(isInfinity);
		WriteOutput(isInfinity, _sf, -_offset);
	}

	void NativeSysMathsF64IsNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF64IsSignalNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsSignalNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF64IsQuietNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsQuietNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF64IsNormal(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNormal = IsNormal(x);
		_offset += sizeof(isNormal);
		WriteOutput(isNormal, _sf, -_offset);
	}

	void NativeSysMathsF64IsFinite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isFinite = IsFinite(x);
		_offset += sizeof(isFinite);
		WriteOutput(isFinite, _sf, -_offset);
	}

	void NativeSysMathsF64QuietNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 value = QuietNanF64();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64MinValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 value = MinF64Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64MaxValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 value = MaxF64Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ToInt32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = ToInt32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ToInt64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = ToInt64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF64ToFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float64 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = ToFloat32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

}

namespace Sys { namespace Maths { namespace F64 { 
	void AddNativeCalls_SysMathsF64(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		UNUSED(nullContext);
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Maths.F64"));
		ss.AddNativeCall(ns, NativeSysMathsF64Clamp, nullptr, ("Clamp(Float64 x)(Float64 lower)(Float64 upper) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Sin, nullptr, ("Sin(Float64 radians) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Cos, nullptr, ("Cos(Float64 radians) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Tan, nullptr, ("Tan(Float64 radians) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ArcSin, nullptr, ("ArcSin(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ArcCos, nullptr, ("ArcCos(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ArcTan, nullptr, ("ArcTan(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ArcTanYX, nullptr, ("ArcTanYX(Float64 y)(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Abs, nullptr, ("Abs(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Sinh, nullptr, ("Sinh(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Cosh, nullptr, ("Cosh(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Tanh, nullptr, ("Tanh(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Exp, nullptr, ("Exp(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Floor, nullptr, ("Floor(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Ceiling, nullptr, ("Ceiling(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Power, nullptr, ("Power(Float64 base)(Float64 exponent) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64LogN, nullptr, ("LogN(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64Log10, nullptr, ("Log10(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64SquareRoot, nullptr, ("SquareRoot(Float64 x) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ErrorFunction, nullptr, ("ErrorFunction(Float64 x) -> (Float64 y)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64MaxOf, nullptr, ("MaxOf(Float64 x)(Float64 y) -> (Float64 maxValue)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64MinOf, nullptr, ("MinOf(Float64 x)(Float64 y) -> (Float64 minValue)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64RoundToNearest, nullptr, ("RoundToNearest(Float64 x) -> (Float64 y)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsInfinity, nullptr, ("IsInfinity(Float64 x) -> (Bool isInfinity)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsNan, nullptr, ("IsNan(Float64 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsSignalNan, nullptr, ("IsSignalNan(Float64 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsQuietNan, nullptr, ("IsQuietNan(Float64 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsNormal, nullptr, ("IsNormal(Float64 x) -> (Bool isNormal)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64IsFinite, nullptr, ("IsFinite(Float64 x) -> (Bool isFinite)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64QuietNan, nullptr, ("QuietNan -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64MinValue, nullptr, ("MinValue -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64MaxValue, nullptr, ("MaxValue -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ToInt32, nullptr, ("ToInt32(Float64 x) -> (Int32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ToInt64, nullptr, ("ToInt64(Float64 x) -> (Int64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF64ToFloat32, nullptr, ("ToFloat32(Float64 x) -> (Float32 value)"), __FILE__, __LINE__);
	}
}}}
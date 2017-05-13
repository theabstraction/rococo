namespace
{
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Maths.F64"));
		ss.AddNativeCall(ns, NativeSysMathsF64Sin, nullptr, SEXTEXT("Sin(Float64 radians) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Cos, nullptr, SEXTEXT("Cos(Float64 radians) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Tan, nullptr, SEXTEXT("Tan(Float64 radians) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ArcSin, nullptr, SEXTEXT("ArcSin(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ArcCos, nullptr, SEXTEXT("ArcCos(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ArcTan, nullptr, SEXTEXT("ArcTan(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ArcTanYX, nullptr, SEXTEXT("ArcTanYX(Float64 y)(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Abs, nullptr, SEXTEXT("Abs(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Sinh, nullptr, SEXTEXT("Sinh(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Cosh, nullptr, SEXTEXT("Cosh(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Tanh, nullptr, SEXTEXT("Tanh(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Exp, nullptr, SEXTEXT("Exp(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Floor, nullptr, SEXTEXT("Floor(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Ceiling, nullptr, SEXTEXT("Ceiling(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Power, nullptr, SEXTEXT("Power(Float64 base)(Float64 exponent) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64LogN, nullptr, SEXTEXT("LogN(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64Log10, nullptr, SEXTEXT("Log10(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64SquareRoot, nullptr, SEXTEXT("SquareRoot(Float64 x) -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ErrorFunction, nullptr, SEXTEXT("ErrorFunction(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsF64MaxOf, nullptr, SEXTEXT("MaxOf(Float64 x)(Float64 y) -> (Float64 maxValue)"));
		ss.AddNativeCall(ns, NativeSysMathsF64MinOf, nullptr, SEXTEXT("MinOf(Float64 x)(Float64 y) -> (Float64 minValue)"));
		ss.AddNativeCall(ns, NativeSysMathsF64RoundToNearest, nullptr, SEXTEXT("RoundToNearest(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsInfinity, nullptr, SEXTEXT("IsInfinity(Float64 x) -> (Bool isInfinity)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsNan, nullptr, SEXTEXT("IsNan(Float64 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsSignalNan, nullptr, SEXTEXT("IsSignalNan(Float64 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsQuietNan, nullptr, SEXTEXT("IsQuietNan(Float64 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsNormal, nullptr, SEXTEXT("IsNormal(Float64 x) -> (Bool isNormal)"));
		ss.AddNativeCall(ns, NativeSysMathsF64IsFinite, nullptr, SEXTEXT("IsFinite(Float64 x) -> (Bool isFinite)"));
		ss.AddNativeCall(ns, NativeSysMathsF64QuietNan, nullptr, SEXTEXT("QuietNan -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64MinValue, nullptr, SEXTEXT("MinValue -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64MaxValue, nullptr, SEXTEXT("MaxValue -> (Float64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ToInt32, nullptr, SEXTEXT("ToInt32(Float64 x) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ToInt64, nullptr, SEXTEXT("ToInt64(Float64 x) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF64ToFloat32, nullptr, SEXTEXT("ToFloat32(Float64 x) -> (Float32 value)"));
	}
}}}
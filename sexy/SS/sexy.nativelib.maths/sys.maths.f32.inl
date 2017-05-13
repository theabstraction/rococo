namespace
{
	void NativeSysMathsF32Sin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float32 value = sinf(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Cos(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float32 value = cosf(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Tan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 radians;
		_offset += sizeof(radians);

		ReadInput(radians, _sf, -_offset);
		float32 value = tanf(radians);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ArcSin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = asinf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ArcCos(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = acosf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ArcTan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = atanf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ArcTanYX(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float32 value = atan2f(y, x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Abs(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = fabsf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Sinh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = sinhf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Cosh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = coshf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Tanh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = tanhf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Exp(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = expf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Floor(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = floorf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Ceiling(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = ceilf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Power(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 exponent;
		_offset += sizeof(exponent);

		ReadInput(exponent, _sf, -_offset);
		float32 base;
		_offset += sizeof(base);

		ReadInput(base, _sf, -_offset);
		float32 value = powf(base, exponent);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32LogN(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = logf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Log10(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = log10f(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32SquareRoot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 value = sqrtf(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ErrorFunction(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 y = erff(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsF32MaxOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 maxValue = MaxOf(x, y);
		_offset += sizeof(maxValue);
		WriteOutput(maxValue, _sf, -_offset);
	}

	void NativeSysMathsF32MinOf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 y;
		_offset += sizeof(y);

		ReadInput(y, _sf, -_offset);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 minValue = MinOf(x, y);
		_offset += sizeof(minValue);
		WriteOutput(minValue, _sf, -_offset);
	}

	void NativeSysMathsF32RoundToNearest(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float32 y = roundf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsF32IsInfinity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isInfinity = IsInfinity(x);
		_offset += sizeof(isInfinity);
		WriteOutput(isInfinity, _sf, -_offset);
	}

	void NativeSysMathsF32IsFinite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isFinite = IsFinite(x);
		_offset += sizeof(isFinite);
		WriteOutput(isFinite, _sf, -_offset);
	}

	void NativeSysMathsF32IsNormal(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNormal = IsNormal(x);
		_offset += sizeof(isNormal);
		WriteOutput(isNormal, _sf, -_offset);
	}

	void NativeSysMathsF32IsNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF32IsSignalNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsSignalNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF32IsQuietNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		boolean32 isNan = IsQuietNan(x);
		_offset += sizeof(isNan);
		WriteOutput(isNan, _sf, -_offset);
	}

	void NativeSysMathsF32QuietNan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 value = QuietNanF32();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32MinValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 value = MinF32Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32MaxValue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 value = MaxF32Value();
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ToInt32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int32 value = ToInt32(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ToInt64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		int64 value = ToInt64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32ToFloat64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float32 x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float64 value = ToFloat64(x);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

}

namespace Sys { namespace Maths { namespace F32 { 
	void AddNativeCalls_SysMathsF32(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Maths.F32"));
		ss.AddNativeCall(ns, NativeSysMathsF32Sin, nullptr, SEXTEXT("Sin(Float32 radians) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Cos, nullptr, SEXTEXT("Cos(Float32 radians) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Tan, nullptr, SEXTEXT("Tan(Float32 radians) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ArcSin, nullptr, SEXTEXT("ArcSin(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ArcCos, nullptr, SEXTEXT("ArcCos(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ArcTan, nullptr, SEXTEXT("ArcTan(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ArcTanYX, nullptr, SEXTEXT("ArcTanYX(Float32 y)(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Abs, nullptr, SEXTEXT("Abs(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Sinh, nullptr, SEXTEXT("Sinh(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Cosh, nullptr, SEXTEXT("Cosh(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Tanh, nullptr, SEXTEXT("Tanh(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Exp, nullptr, SEXTEXT("Exp(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Floor, nullptr, SEXTEXT("Floor(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Ceiling, nullptr, SEXTEXT("Ceiling(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Power, nullptr, SEXTEXT("Power(Float32 base)(Float32 exponent) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32LogN, nullptr, SEXTEXT("LogN(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32Log10, nullptr, SEXTEXT("Log10(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32SquareRoot, nullptr, SEXTEXT("SquareRoot(Float32 x) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ErrorFunction, nullptr, SEXTEXT("ErrorFunction(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsF32MaxOf, nullptr, SEXTEXT("MaxOf(Float32 x)(Float32 y) -> (Float32 maxValue)"));
		ss.AddNativeCall(ns, NativeSysMathsF32MinOf, nullptr, SEXTEXT("MinOf(Float32 x)(Float32 y) -> (Float32 minValue)"));
		ss.AddNativeCall(ns, NativeSysMathsF32RoundToNearest, nullptr, SEXTEXT("RoundToNearest(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsInfinity, nullptr, SEXTEXT("IsInfinity(Float32 x) -> (Bool isInfinity)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsFinite, nullptr, SEXTEXT("IsFinite(Float32 x) -> (Bool isFinite)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsNormal, nullptr, SEXTEXT("IsNormal(Float32 x) -> (Bool isNormal)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsNan, nullptr, SEXTEXT("IsNan(Float32 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsSignalNan, nullptr, SEXTEXT("IsSignalNan(Float32 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF32IsQuietNan, nullptr, SEXTEXT("IsQuietNan(Float32 x) -> (Bool isNan)"));
		ss.AddNativeCall(ns, NativeSysMathsF32QuietNan, nullptr, SEXTEXT("QuietNan -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32MinValue, nullptr, SEXTEXT("MinValue -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32MaxValue, nullptr, SEXTEXT("MaxValue -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ToInt32, nullptr, SEXTEXT("ToInt32(Float32 x) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ToInt64, nullptr, SEXTEXT("ToInt64(Float32 x) -> (Int64 value)"));
		ss.AddNativeCall(ns, NativeSysMathsF32ToFloat64, nullptr, SEXTEXT("ToFloat64(Float32 x) -> (Float64 value)"));
	}
}}}
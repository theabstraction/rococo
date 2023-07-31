#pragma once

// Note this file is hand-coded. It was written before BennyHill existed, and the layout was the basis for the BennyHill generated code.

namespace
{
	void NativeSysMathsF32Clamp(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		float upper;
		_offset += sizeof(upper);
		ReadInput(upper, _sf, -_offset);

		float lower;
		_offset += sizeof(lower);
		ReadInput(lower, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		float value = clamp(x, lower, upper);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsF32Mod(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		float y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		float value = fmodf(x, y);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}

	void NativeSysMathsAddVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* sum;
		_offset += sizeof(sum);
		ReadInput(sum, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		sum->x = a->x + b->x;
		sum->y = a->y + b->y;
	}

	void NativeSysMathsRectfToRecti(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		GuiRect* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		const GuiRectf* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		b->left = (int32)a->left;
		b->bottom = (int32)a->bottom;
		b->right = (int32)a->right;
		b->top = (int32)a->top;
	}

	void NativeSysMathsSubtractVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* diff;
		_offset += sizeof(diff);
		ReadInput(diff, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		diff->x = a->x - b->x;
		diff->y = a->y - b->y;
	}

	void NativeSysMathsMultiplyVec2fFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		float scaleFactor;
		_offset += sizeof(scaleFactor);
		ReadInput(scaleFactor, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(c);
		ReadInput(a, _sf, -_offset);

		c->x = a->x * scaleFactor;
		c->y = a->y  * scaleFactor;
	}

	void NativeSysMathsDivideVec2fFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* sa;
		_offset += sizeof(sa);
		ReadInput(sa, _sf, -_offset);

		float scaleFactor;
		_offset += sizeof(scaleFactor);
		ReadInput(scaleFactor, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		sa->x = a->x * scaleFactor;
		sa->y = a->y  * scaleFactor;
	}

	void NativeSysMathsIsNotEqVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		boolean32 result;
		result = a->x != b->x || a->y != b->y;

		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsIsNotEqVec3fVec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		boolean32 result;
		result = a->x != b->x || a->y != b->y || a->z != b->z;

		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsIsEqVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		boolean32 result;
		result = a->x == b->x && a->y == b->y;

		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}

	void NativeSysMathsIsEqVec3fVec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		boolean32 result;
		result = a->x == b->x && a->y == b->y && a->z == b->z;

		_offset += sizeof(result);
		WriteOutput(result, _sf, -_offset);
	}
						
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
		UNUSED(nullContext);
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Maths.F32"));
		ss.AddNativeCall(ns, NativeSysMathsF32Mod, nullptr, ("Mod(Float32 x)(Float32 y) -> (Float32 xMody)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Clamp, nullptr, ("Clamp(Float32 x)(Float32 lower)(Float32 upper) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Sin, nullptr, ("Sin(Float32 radians) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Cos, nullptr, ("Cos(Float32 radians) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Tan, nullptr, ("Tan(Float32 radians) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ArcSin, nullptr, ("ArcSin(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ArcCos, nullptr, ("ArcCos(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ArcTan, nullptr, ("ArcTan(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ArcTanYX, nullptr, ("ArcTanYX(Float32 y)(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Abs, nullptr, ("Abs(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Sinh, nullptr, ("Sinh(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Cosh, nullptr, ("Cosh(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Tanh, nullptr, ("Tanh(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Exp, nullptr, ("Exp(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Floor, nullptr, ("Floor(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Ceiling, nullptr, ("Ceiling(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Power, nullptr, ("Power(Float32 base)(Float32 exponent) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32LogN, nullptr, ("LogN(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32Log10, nullptr, ("Log10(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32SquareRoot, nullptr, ("SquareRoot(Float32 x) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ErrorFunction, nullptr, ("ErrorFunction(Float32 x) -> (Float32 y)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32MaxOf, nullptr, ("MaxOf(Float32 x)(Float32 y) -> (Float32 maxValue)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32MinOf, nullptr, ("MinOf(Float32 x)(Float32 y) -> (Float32 minValue)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32RoundToNearest, nullptr, ("RoundToNearest(Float32 x) -> (Float32 y)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsInfinity, nullptr, ("IsInfinity(Float32 x) -> (Bool isInfinity)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsFinite, nullptr, ("IsFinite(Float32 x) -> (Bool isFinite)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsNormal, nullptr, ("IsNormal(Float32 x) -> (Bool isNormal)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsNan, nullptr, ("IsNan(Float32 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsSignalNan, nullptr, ("IsSignalNan(Float32 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32IsQuietNan, nullptr, ("IsQuietNan(Float32 x) -> (Bool isNan)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32QuietNan, nullptr, ("QuietNan -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32MinValue, nullptr, ("MinValue -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32MaxValue, nullptr, ("MaxValue -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ToInt32, nullptr, ("ToInt32(Float32 x) -> (Int32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ToInt64, nullptr, ("ToInt64(Float32 x) -> (Int64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsF32ToFloat64, nullptr, ("ToFloat64(Float32 x) -> (Float64 value)"), __FILE__, __LINE__);

		ss.AddNativeCall(ns, NativeSysMathsAddVec2fVec2f, nullptr, ("AddVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 sum)->"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsSubtractVec2fVec2f, nullptr, ("SubtractVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 diff)->"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsMultiplyVec2fFloat32, nullptr, ("MultiplyVec2fFloat32(Sys.Maths.Vec2 a)(Float32 scale)(Sys.Maths.Vec2 product)->"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsDivideVec2fFloat32, nullptr, ("DivideVec2fFloat32(Sys.Maths.Vec2 a)(Float32 scale)(Sys.Maths.Vec2 product)->"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsIsNotEqVec2fVec2f, nullptr, ("IsNotEqVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)->(Bool result)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsIsEqVec2fVec2f, nullptr, ("IsEqVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2  b)->(Bool result)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsIsNotEqVec3fVec3f, nullptr, ("IsNotEqVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)->(Bool result)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsIsEqVec3fVec3f, nullptr, ("IsEqVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3  b)->(Bool result)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysMathsRectfToRecti, nullptr, "RectfToRecti (Sys.Maths.Rectf rectf)(Sys.Maths.Recti recti)->", __FILE__, __LINE__);
	}
}}}
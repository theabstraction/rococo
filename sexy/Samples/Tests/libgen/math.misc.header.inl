// sexy.libgen created: Tue Nov 13 20:57:10 2012

namespace
{
	void Nativeabs(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = abs(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativefabs(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = fabs(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeabs(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int32 x;
		offset += sizeof(int32);
		ReadInput(x, sf, -offset);

		int32 output = abs(x);
		offset += sizeof(int32);
		WriteOutput(output, sf, -offset);
	}

	void Nativeabs(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int64 x;
		offset += sizeof(int64);
		ReadInput(x, sf, -offset);

		int64 output = abs(x);
		offset += sizeof(int64);
		WriteOutput(output, sf, -offset);
	}

	void Nativesqrt(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = sqrt(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativesqrtf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = sqrtf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeceilf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = ceilf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeceil(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = ceil(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativefloorf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = floorf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativefloor(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = floor(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativediv(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int32 denominator;
		offset += sizeof(int32);
		ReadInput(denominator, sf, -offset);

		int32 numerator;
		offset += sizeof(int32);
		ReadInput(numerator, sf, -offset);

		int32 output = div(numerator, denominator);
		offset += sizeof(int32);
		WriteOutput(output, sf, -offset);
	}

	void Nativediv(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int64 denominator;
		offset += sizeof(int64);
		ReadInput(denominator, sf, -offset);

		int64 numerator;
		offset += sizeof(int64);
		ReadInput(numerator, sf, -offset);

		int64 output = div(numerator, denominator);
		offset += sizeof(int64);
		WriteOutput(output, sf, -offset);
	}

	void Nativemod(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int32 denominator;
		offset += sizeof(int32);
		ReadInput(denominator, sf, -offset);

		int32 numerator;
		offset += sizeof(int32);
		ReadInput(numerator, sf, -offset);

		int32 output = mod(numerator, denominator);
		offset += sizeof(int32);
		WriteOutput(output, sf, -offset);
	}

	void Nativemod(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int64 denominator;
		offset += sizeof(int64);
		ReadInput(denominator, sf, -offset);

		int64 numerator;
		offset += sizeof(int64);
		ReadInput(numerator, sf, -offset);

		int64 output = mod(numerator, denominator);
		offset += sizeof(int64);
		WriteOutput(output, sf, -offset);
	}

	void Nativeleftshift(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int32 shiftCount;
		offset += sizeof(int32);
		ReadInput(shiftCount, sf, -offset);

		int32 x;
		offset += sizeof(int32);
		ReadInput(x, sf, -offset);

		int32 output = leftshift(x, shiftCount);
		offset += sizeof(int32);
		WriteOutput(output, sf, -offset);
	}

	void Nativerightshift(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int32 shiftCount;
		offset += sizeof(int32);
		ReadInput(shiftCount, sf, -offset);

		int32 x;
		offset += sizeof(int32);
		ReadInput(x, sf, -offset);

		int32 output = rightshift(x, shiftCount);
		offset += sizeof(int32);
		WriteOutput(output, sf, -offset);
	}

	void Nativeleftshift(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int64 shiftCount;
		offset += sizeof(int64);
		ReadInput(shiftCount, sf, -offset);

		int64 x;
		offset += sizeof(int64);
		ReadInput(x, sf, -offset);

		int64 output = leftshift(x, shiftCount);
		offset += sizeof(int64);
		WriteOutput(output, sf, -offset);
	}

	void Nativerightshift(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		int64 shiftCount;
		offset += sizeof(int64);
		ReadInput(shiftCount, sf, -offset);

		int64 x;
		offset += sizeof(int64);
		ReadInput(x, sf, -offset);

		int64 output = rightshift(x, shiftCount);
		offset += sizeof(int64);
		WriteOutput(output, sf, -offset);
	}

}
void InstallMethods(const INamespace& ns, IPublicScriptSystem& ss)
{
	ss.AddNativeCall(ns, Nativeabs, NULL, SEXTEXT("Abs (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativefabs, NULL, SEXTEXT("Fabs (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeabs, NULL, SEXTEXT("Abs (Int32 x) -> (Int32 result)"), true);
	ss.AddNativeCall(ns, Nativeabs, NULL, SEXTEXT("Abs (Int64 x) -> (Int64 result)"), true);
	ss.AddNativeCall(ns, Nativesqrt, NULL, SEXTEXT("Sqrt (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativesqrtf, NULL, SEXTEXT("Sqrtf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeceilf, NULL, SEXTEXT("Ceilf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeceil, NULL, SEXTEXT("Ceil (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativefloorf, NULL, SEXTEXT("Floorf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativefloor, NULL, SEXTEXT("Floor (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativediv, NULL, SEXTEXT("Div (Int32 numerator) (Int32 denominator) -> (Int32 result)"), true);
	ss.AddNativeCall(ns, Nativediv, NULL, SEXTEXT("Div (Int64 numerator) (Int64 denominator) -> (Int64 result)"), true);
	ss.AddNativeCall(ns, Nativemod, NULL, SEXTEXT("Mod (Int32 numerator) (Int32 denominator) -> (Int32 result)"), true);
	ss.AddNativeCall(ns, Nativemod, NULL, SEXTEXT("Mod (Int64 numerator) (Int64 denominator) -> (Int64 result)"), true);
	ss.AddNativeCall(ns, Nativeleftshift, NULL, SEXTEXT("Leftshift (Int32 x) (Int32 shiftCount) -> (Int32 result)"), true);
	ss.AddNativeCall(ns, Nativerightshift, NULL, SEXTEXT("Rightshift (Int32 x) (Int32 shiftCount) -> (Int32 result)"), true);
	ss.AddNativeCall(ns, Nativeleftshift, NULL, SEXTEXT("Leftshift (Int64 x) (Int64 shiftCount) -> (Int64 result)"), true);
	ss.AddNativeCall(ns, Nativerightshift, NULL, SEXTEXT("Rightshift (Int64 x) (Int64 shiftCount) -> (Int64 result)"), true);
}

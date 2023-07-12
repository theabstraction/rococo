// sexy.libgen created: Tue Nov 13 18:58:20 2012

namespace
{
	void Nativesin(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double rads;
		offset += sizeof(double);
		ReadInput(rads, sf, -offset);

		double output = sin(rads);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativecos(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double rads;
		offset += sizeof(double);
		ReadInput(rads, sf, -offset);

		double output = cos(rads);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativetan(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double rads;
		offset += sizeof(double);
		ReadInput(rads, sf, -offset);

		double output = tan(rads);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativeasin(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = asin(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativeacos(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = acos(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativeatan(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = atan(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativeatan2(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double y;
		offset += sizeof(double);
		ReadInput(y, sf, -offset);

		double output = atan2(y, x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativesinh(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = sinh(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativecosh(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = cosh(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativetanh(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = tanh(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativelog(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = log(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativelog10(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = log10(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativepow(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double y;
		offset += sizeof(double);
		ReadInput(y, sf, -offset);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = pow(x, y);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativeexp(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		double x;
		offset += sizeof(double);
		ReadInput(x, sf, -offset);

		double output = exp(x);
		offset += sizeof(double);
		WriteOutput(output, sf, -offset);
	}

	void Nativesinf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float rads;
		offset += sizeof(float);
		ReadInput(rads, sf, -offset);

		float output = sinf(rads);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativecosf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float rads;
		offset += sizeof(float);
		ReadInput(rads, sf, -offset);

		float output = cosf(rads);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativetanf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float rads;
		offset += sizeof(float);
		ReadInput(rads, sf, -offset);

		float output = tanf(rads);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeasinf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = asinf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeacosf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = acosf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeatanf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = atanf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeatan2f(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float y;
		offset += sizeof(float);
		ReadInput(y, sf, -offset);

		float output = atan2f(y, x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativesinhf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = sinhf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativecoshf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = coshf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativetanhf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = tanhf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativelogf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = logf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativeexpf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = expf(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativelog10f(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float x;
		offset += sizeof(float);
		ReadInput(x, sf, -offset);

		float output = log10f(x);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

	void Nativepowf(NativeCallEnvironment& e, void* context)
	{
		uint8* sf = e.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		float power;
		offset += sizeof(float);
		ReadInput(power, sf, -offset);

		float base;
		offset += sizeof(float);
		ReadInput(base, sf, -offset);

		float output = powf(base, power);
		offset += sizeof(float);
		WriteOutput(output, sf, -offset);
	}

}
void InstallMethods(const INamespace& ns, IPublicScriptSystem& ss)
{
	ss.AddNativeCall(ns, Nativesin, NULL, SEXTEXT("Sin (Float64 rads) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativecos, NULL, SEXTEXT("Cos (Float64 rads) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativetan, NULL, SEXTEXT("Tan (Float64 rads) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativeasin, NULL, SEXTEXT("Asin (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativeacos, NULL, SEXTEXT("Acos (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativeatan, NULL, SEXTEXT("Atan (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativeatan2, NULL, SEXTEXT("Atan2 (Float64 y) (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativesinh, NULL, SEXTEXT("Sinh (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativecosh, NULL, SEXTEXT("Cosh (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativetanh, NULL, SEXTEXT("Tanh (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativelog, NULL, SEXTEXT("Log (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativelog10, NULL, SEXTEXT("Log10 (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativepow, NULL, SEXTEXT("Pow (Float64 x) (Float64 y) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativeexp, NULL, SEXTEXT("Exp (Float64 x) -> (Float64 result)"), true);
	ss.AddNativeCall(ns, Nativesinf, NULL, SEXTEXT("Sinf (Float32 rads) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativecosf, NULL, SEXTEXT("Cosf (Float32 rads) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativetanf, NULL, SEXTEXT("Tanf (Float32 rads) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeasinf, NULL, SEXTEXT("Asinf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeacosf, NULL, SEXTEXT("Acosf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeatanf, NULL, SEXTEXT("Atanf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeatan2f, NULL, SEXTEXT("Atan2f (Float32 y) (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativesinhf, NULL, SEXTEXT("Sinhf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativecoshf, NULL, SEXTEXT("Coshf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativetanhf, NULL, SEXTEXT("Tanhf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativelogf, NULL, SEXTEXT("Logf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativeexpf, NULL, SEXTEXT("Expf (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativelog10f, NULL, SEXTEXT("Log10f (Float32 x) -> (Float32 result)"), true);
	ss.AddNativeCall(ns, Nativepowf, NULL, SEXTEXT("Powf (Float32 base) (Float32 power) -> (Float32 result)"), true);
}

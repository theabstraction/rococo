namespace
{
	void NativeSysMathsSind(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = sin(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsCosd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = cos(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsTand(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = tan(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsSinhd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = sinh(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsCoshd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = cosh(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsTanhd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		double x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		double y = tanh(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsSin(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = sinf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsCos(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = cosf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsTan(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = tanf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsSinh(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = sinhf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsCosh(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = coshf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

	void NativeSysMathsTanh(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		float x;
		_offset += sizeof(x);

		ReadInput(x, _sf, -_offset);
		float y = tanhf(x);
		_offset += sizeof(y);
		WriteOutput(y, _sf, -_offset);
	}

}

namespace Sys::Maths
{
	void AddNativeCalls_SysMaths(Sexy::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Maths"));
		ss.AddNativeCall(ns, NativeSysMathsSind, nullptr, SEXTEXT("Sind(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsCosd, nullptr, SEXTEXT("Cosd(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsTand, nullptr, SEXTEXT("Tand(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsSinhd, nullptr, SEXTEXT("Sinhd(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsCoshd, nullptr, SEXTEXT("Coshd(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsTanhd, nullptr, SEXTEXT("Tanhd(Float64 x) -> (Float64 y)"));
		ss.AddNativeCall(ns, NativeSysMathsSin, nullptr, SEXTEXT("Sin(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsCos, nullptr, SEXTEXT("Cos(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsTan, nullptr, SEXTEXT("Tan(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsSinh, nullptr, SEXTEXT("Sinh(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsCosh, nullptr, SEXTEXT("Cosh(Float32 x) -> (Float32 y)"));
		ss.AddNativeCall(ns, NativeSysMathsTanh, nullptr, SEXTEXT("Tanh(Float32 x) -> (Float32 y)"));
	}
} // Sys::Maths

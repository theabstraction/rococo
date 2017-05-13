namespace
{
	void NativeSysGeometryF32AddVec3fVec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* sum;
		_offset += sizeof(sum);
		ReadInput(sum, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::AddVec3toVec3(*a, *b, *sum);
	}

	void NativeSysGeometryF32SubtractVec3fVec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* difference;
		_offset += sizeof(difference);
		ReadInput(difference, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::SubtractVec3fromVec3(*a, *b, *difference);
	}

	void NativeSysGeometryF32Dot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float dotProduct = Rococo::Maths::Dot(*a, *b);
		_offset += sizeof(dotProduct);
		WriteOutput(dotProduct, _sf, -_offset);
	}

	void NativeSysGeometryF32MultiplyVec3fVec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* crossProduct;
		_offset += sizeof(crossProduct);
		ReadInput(crossProduct, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::CrossByRef(*a, *b, *crossProduct);
	}

	void NativeSysGeometryF32MultMatrix4x4fMatrix4x4f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* product;
		_offset += sizeof(product);
		ReadInput(product, _sf, -_offset);

		Matrix4x4* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Matrix4x4* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::MultiplyMatrixByRef(*a, *b, *product);
	}

	void NativeSysGeometryF32Length(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float len = Rococo::Maths::Length(*a);
		_offset += sizeof(len);
		WriteOutput(len, _sf, -_offset);
	}

	void NativeSysGeometryF32Normalize(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* n;
		_offset += sizeof(n);
		ReadInput(n, _sf, -_offset);

		Rococo::Maths::NormalizeInPlace(*n);
	}

}

namespace Sys { namespace Geometry { namespace F32 { 
	void AddNativeCalls_SysGeometryF32(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Geometry.F32"));
		ss.AddNativeCall(ns, NativeSysGeometryF32AddVec3fVec3f, nullptr, SEXTEXT("AddVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 sum) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32SubtractVec3fVec3f, nullptr, SEXTEXT("SubtractVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 difference) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32Dot, nullptr, SEXTEXT("Dot(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b) -> (Float32 dotProduct)"));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyVec3fVec3f, nullptr, SEXTEXT("MultiplyVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 crossProduct) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultMatrix4x4fMatrix4x4f, nullptr, SEXTEXT("MultMatrix4x4fMatrix4x4f(Sys.Maths.Matrix4x4 a)(Sys.Maths.Matrix4x4 b)(Sys.Maths.Matrix4x4 product) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32Length, nullptr, SEXTEXT("Length(Sys.Maths.Vec3 a) -> (Float32 len)"));
		ss.AddNativeCall(ns, NativeSysGeometryF32Normalize, nullptr, SEXTEXT("Normalize(Sys.Maths.Vec3 n) -> "));
	}
}}}
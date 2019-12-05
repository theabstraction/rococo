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

	void NativeSysGeometryF32AddVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* output;
		_offset += sizeof(output);
		ReadInput(output, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::AddVec2toVec2(*a, *b, *output);
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

	void NativeSysGeometryF32SubtractVec2fVec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* difference;
		_offset += sizeof(difference);
		ReadInput(difference, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::SubtractVec2fromVec2(*a, *b, *difference);
	}

	void NativeSysGeometryF32MultiplyFloat32Vec2f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* scaledVector;
		_offset += sizeof(scaledVector);
		ReadInput(scaledVector, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float f;
		_offset += sizeof(f);
		ReadInput(f, _sf, -_offset);

		Rococo::Maths::ScaleVector2(f, *a, *scaledVector);
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

		float dotProduct = Rococo::Dot(*a, *b);
		_offset += sizeof(dotProduct);
		WriteOutput(dotProduct, _sf, -_offset);
	}

	void NativeSysGeometryF32GetTriSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Vec3* d;
		_offset += sizeof(d);
		ReadInput(d, _sf, -_offset);

		Rococo::Maths::GetTriSpan(*d, *a, *b, *span);
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

	void NativeSysGeometryF32MultiplyFloat32Vec3f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scaledVector;
		_offset += sizeof(scaledVector);
		ReadInput(scaledVector, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float f;
		_offset += sizeof(f);
		ReadInput(f, _sf, -_offset);

		Rococo::Maths::ScaleVector3(f, *a, *scaledVector);
	}

	void NativeSysGeometryF32MultiplyVec3fFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scaledVector;
		_offset += sizeof(scaledVector);
		ReadInput(scaledVector, _sf, -_offset);

		float f;
		_offset += sizeof(f);
		ReadInput(f, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::ScaleVector3(*a, f, *scaledVector);
	}

	void NativeSysGeometryF32MultiplyMatrix4x4fMatrix4x4f(NativeCallEnvironment& _nce)
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

	void NativeSysGeometryF32MultiplyMatrix4x4fVec4f(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec4* mv;
		_offset += sizeof(mv);
		ReadInput(mv, _sf, -_offset);

		Vec4* v;
		_offset += sizeof(v);
		ReadInput(v, _sf, -_offset);

		Matrix4x4* m;
		_offset += sizeof(m);
		ReadInput(m, _sf, -_offset);

		Rococo::Maths::TransformVector(*m, *v, *mv);
	}

	void NativeSysGeometryF32Length(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float len = Rococo::Length(*a);
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

	void NativeSysGeometryF32GetNormal(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::Triangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Rococo::Maths::GetNormal(*t, *normal);
	}

	void NativeSysGeometryF32LerpVec3(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* mixed;
		_offset += sizeof(mixed);
		ReadInput(mixed, _sf, -_offset);

		float t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Vec3* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec3* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::LerpVec3(*a, *b, t, *mixed);
	}

	void NativeSysGeometryF32GetRotationQuat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		float k;
		_offset += sizeof(k);
		ReadInput(k, _sf, -_offset);

		float j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		float i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Degrees theta;
		_offset += sizeof(theta);
		ReadInput(theta, _sf, -_offset);

		Rococo::Maths::GetRotationQuat(theta, i, j, k, *q);
	}

	void NativeSysGeometryF32MultiplyQuatfQuatf(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* output;
		_offset += sizeof(output);
		ReadInput(output, _sf, -_offset);

		Quat* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Quat* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Maths::MultiplyQuatByQuat(*a, *b, *output);
	}

}

namespace Sys { namespace Geometry { namespace F32 { 
	void AddNativeCalls_SysGeometryF32(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Geometry.F32"));
		ss.AddNativeCall(ns, NativeSysGeometryF32AddVec3fVec3f, nullptr, ("AddVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 sum) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32AddVec2fVec2f, nullptr, ("AddVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 output) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32SubtractVec3fVec3f, nullptr, ("SubtractVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 difference) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32SubtractVec2fVec2f, nullptr, ("SubtractVec2fVec2f(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 difference) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyFloat32Vec2f, nullptr, ("MultiplyFloat32Vec2f(Float32 f)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 scaledVector) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32Dot, nullptr, ("Dot(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b) -> (Float32 dotProduct)"));
		ss.AddNativeCall(ns, NativeSysGeometryF32GetTriSpan, nullptr, ("GetTriSpan(Sys.Maths.Vec3 d)(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec2 span) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyVec3fVec3f, nullptr, ("MultiplyVec3fVec3f(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Sys.Maths.Vec3 crossProduct) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyFloat32Vec3f, nullptr, ("MultiplyFloat32Vec3f(Float32 f)(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 scaledVector) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyVec3fFloat32, nullptr, ("MultiplyVec3fFloat32(Sys.Maths.Vec3 a)(Float32 f)(Sys.Maths.Vec3 scaledVector) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyMatrix4x4fMatrix4x4f, nullptr, ("MultiplyMatrix4x4fMatrix4x4f(Sys.Maths.Matrix4x4 a)(Sys.Maths.Matrix4x4 b)(Sys.Maths.Matrix4x4 product) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyMatrix4x4fVec4f, nullptr, ("MultiplyMatrix4x4fVec4f(Sys.Maths.Matrix4x4 m)(Sys.Maths.Vec4 v)(Sys.Maths.Vec4 mv) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32Length, nullptr, ("Length(Sys.Maths.Vec3 a) -> (Float32 len)"));
		ss.AddNativeCall(ns, NativeSysGeometryF32Normalize, nullptr, ("Normalize(Sys.Maths.Vec3 n) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32GetNormal, nullptr, ("GetNormal(Sys.Maths.Triangle t)(Sys.Maths.Vec3 normal) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32LerpVec3, nullptr, ("LerpVec3(Sys.Maths.Vec3 a)(Sys.Maths.Vec3 b)(Float32 t)(Sys.Maths.Vec3 mixed) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32GetRotationQuat, nullptr, ("GetRotationQuat(Sys.Maths.Degrees theta)(Float32 i)(Float32 j)(Float32 k)(Sys.Maths.Quat q) -> "));
		ss.AddNativeCall(ns, NativeSysGeometryF32MultiplyQuatfQuatf, nullptr, ("MultiplyQuatfQuatf(Sys.Maths.Quat a)(Sys.Maths.Quat b)(Sys.Maths.Quat output) -> "));
	}
}}}

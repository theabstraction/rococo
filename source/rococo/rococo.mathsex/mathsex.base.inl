namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeSysTypeMakeColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		int32 b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		int32 g;
		_offset += sizeof(g);
		ReadInput(g, _sf, -_offset);

		int32 r;
		_offset += sizeof(r);
		ReadInput(r, _sf, -_offset);

		RGBAb colour = Rococo::MakeColour(r, g, b, a);
		_offset += sizeof(colour);
		WriteOutput(colour, _sf, -_offset);
	}

	void NativeSysTypeToRGBA(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float alpha;
		_offset += sizeof(alpha);
		ReadInput(alpha, _sf, -_offset);

		float b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		float g;
		_offset += sizeof(g);
		ReadInput(g, _sf, -_offset);

		float r;
		_offset += sizeof(r);
		ReadInput(r, _sf, -_offset);

		RGBAb colour = Rococo::ToRGBA(r, g, b, alpha);
		_offset += sizeof(colour);
		WriteOutput(colour, _sf, -_offset);
	}

}

namespace Sys::Type
{

	void AddNativeCalls_SysType(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("Sys.Type");
		ss.AddNativeCall(ns, NativeSysTypeMakeColour, nullptr, ("MakeColour(Int32 r)(Int32 g)(Int32 b)(Int32 a) -> (Int32 colour)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeSysTypeToRGBA, nullptr, ("ToRGBAb (Float32 r)(Float32 g)(Float32 b)(Float32 alpha) -> (Int32 colour)"), __FILE__, __LINE__);
	}
}


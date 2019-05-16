namespace
{
	void NativeMHostOSIsKeyPressed(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vKeyCode;
		_offset += sizeof(vKeyCode);
		ReadInput(vKeyCode, _sf, -_offset);

		MHost::OS::KeyState* keys;
		_offset += sizeof(keys);
		ReadInput(keys, _sf, -_offset);

		boolean32 isPressed = MHost::OS::IsKeyPressed(*keys, vKeyCode);
		_offset += sizeof(isPressed);
		WriteOutput(isPressed, _sf, -_offset);
	}

	void NativeMHostMathsClamp0to1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float f;
		_offset += sizeof(f);
		ReadInput(f, _sf, -_offset);

		float clampedValue = MHost::Maths::Clamp0to1(f);
		_offset += sizeof(clampedValue);
		WriteOutput(clampedValue, _sf, -_offset);
	}

	void NativeMHostGraphicsToRGBAb(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		float b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		float g;
		_offset += sizeof(g);
		ReadInput(g, _sf, -_offset);

		float r;
		_offset += sizeof(r);
		ReadInput(r, _sf, -_offset);

		RGBAb colour = MHost::Graphics::ToRGBAb(r, g, b, a);
		_offset += sizeof(colour);
		WriteOutput(colour, _sf, -_offset);
	}

	void NativeMHostGraphicsFloatToColourComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		int32 ic = MHost::Graphics::FloatToColourComponent(c);
		_offset += sizeof(ic);
		WriteOutput(ic, _sf, -_offset);
	}

}

namespace MHost { namespace OS { 
	void AddNativeCalls_MHostOS(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.OS"));
		ss.AddNativeCall(ns, NativeMHostOSIsKeyPressed, nullptr, ("IsKeyPressed(MHost.OS.KeyState keys)(Int32 vKeyCode) -> (Bool isPressed)"));
	}
}}
namespace MHost { namespace Maths { 
	void AddNativeCalls_MHostMaths(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Maths"));
		ss.AddNativeCall(ns, NativeMHostMathsClamp0to1, nullptr, ("Clamp0to1(Float32 f) -> (Float32 clampedValue)"));
	}
}}
namespace MHost { namespace Graphics { 
	void AddNativeCalls_MHostGraphics(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Graphics"));
		ss.AddNativeCall(ns, NativeMHostGraphicsToRGBAb, nullptr, ("ToRGBAb(Float32 r)(Float32 g)(Float32 b)(Float32 a) -> (Int32 colour)"));
		ss.AddNativeCall(ns, NativeMHostGraphicsFloatToColourComponent, nullptr, ("FloatToColourComponent(Float32 c) -> (Int32 ic)"));
	}
}}

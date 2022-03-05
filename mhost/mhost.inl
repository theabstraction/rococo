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
		RGBA* colourF32x4;
		_offset += sizeof(colourF32x4);
		ReadInput(colourF32x4, _sf, -_offset);

		RGBAb colourB8x4 = MHost::Graphics::ToRGBAb(*colourF32x4);
		_offset += sizeof(colourB8x4);
		WriteOutput(colourB8x4, _sf, -_offset);
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
		ss.AddNativeCall(ns, NativeMHostOSIsKeyPressed, nullptr, ("IsKeyPressed(MHost.OS.KeyState keys)(Int32 vKeyCode) -> (Bool isPressed)"), __FILE__, __LINE__);
	}
}}
namespace MHost { namespace Maths { 
	void AddNativeCalls_MHostMaths(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Maths"));
		ss.AddNativeCall(ns, NativeMHostMathsClamp0to1, nullptr, ("Clamp0to1(Float32 f) -> (Float32 clampedValue)"), __FILE__, __LINE__);
	}
}}
namespace MHost { namespace Graphics { 
	void AddNativeCalls_MHostGraphics(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Graphics"));
		ss.AddNativeCall(ns, NativeMHostGraphicsToRGBAb, nullptr, ("ToRGBAb(RGBA colourF32x4) -> (Int32 colourB8x4)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostGraphicsFloatToColourComponent, nullptr, ("FloatToColourComponent(Float32 c) -> (Int32 ic)"), __FILE__, __LINE__);
	}
}}

namespace
{
	void NativeRococoCuteNativeGetWindowRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Vec2i* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		WindowRef hWnd;
		_offset += sizeof(hWnd);
		ReadInput(hWnd, _sf, -_offset);

		Rococo::Cute::Native::GetWindowRect(hWnd, *pos, *span);
	}

	void NativeRococoCuteNativeGetSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		WindowRef hWnd;
		_offset += sizeof(hWnd);
		ReadInput(hWnd, _sf, -_offset);

		Rococo::Cute::Native::GetSpan(hWnd, *span);
	}

	void NativeRococoCuteNativeSetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		WindowRef hWnd;
		_offset += sizeof(hWnd);
		ReadInput(hWnd, _sf, -_offset);

		Rococo::Cute::Native::SetText(hWnd, text);
	}

	void NativeRococoCuteNativeGetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		WindowRef hWnd;
		_offset += sizeof(hWnd);
		ReadInput(hWnd, _sf, -_offset);

		int32 stringLength = Rococo::Cute::Native::GetText(hWnd, _sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

	void NativeRococoCuteNativeSetColourTarget(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		ColourTarget target;
		_offset += sizeof(target);
		ReadInput(target, _sf, -_offset);

		WindowRef hWnd;
		_offset += sizeof(hWnd);
		ReadInput(hWnd, _sf, -_offset);

		Rococo::Cute::Native::SetColourTarget(hWnd, target, colour);
	}

}

namespace Rococo { namespace Cute { namespace Native { 
	void AddNativeCalls_RococoCuteNative(Rococo::Script::IPublicScriptSystem& ss, void* nullContext = nullptr)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteNativeGetWindowRect, nullptr, ("GetWindowRect(Pointer hWnd)(Sys.Maths.Vec2i pos)(Sys.Maths.Vec2i span) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteNativeGetSpan, nullptr, ("GetSpan(Pointer hWnd)(Sys.Maths.Vec2i span) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteNativeSetText, nullptr, ("SetText(Pointer hWnd)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteNativeGetText, nullptr, ("GetText(Pointer hWnd)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"));
		ss.AddNativeCall(ns, NativeRococoCuteNativeSetColourTarget, nullptr, ("SetColourTarget(Pointer hWnd)(Int32 target)(Int32 colour) -> "));
	}
}}}
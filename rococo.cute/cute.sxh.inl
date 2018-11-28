// BennyHill generated Sexy native functions for Rococo::Cute::IWindowBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoCuteIWindowBaseGetWindowRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Vec2i* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Rococo::Cute::IWindowBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetWindowRect(*pos, *span);
	}
	void NativeRococoCuteIWindowBaseGetSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Rococo::Cute::IWindowBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSpan(*span);
	}
	void NativeRococoCuteIWindowBaseSetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		Rococo::Cute::IWindowBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetText(text);
	}
	void NativeRococoCuteIWindowBaseGetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		Rococo::Cute::IWindowBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 stringLength = _pObject->GetText(_sbPopulator);
		_offset += sizeof(stringLength);
		WriteOutput(stringLength, _sf, -_offset);
	}

}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIWindowBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IWindowBase* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeRococoCuteIWindowBaseGetWindowRect, nullptr, ("IWindowBaseGetWindowRect (Pointer hObject)(Sys.Maths.Vec2i pos)(Sys.Maths.Vec2i span) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIWindowBaseGetSpan, nullptr, ("IWindowBaseGetSpan (Pointer hObject)(Sys.Maths.Vec2i span) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIWindowBaseSetText, nullptr, ("IWindowBaseSetText (Pointer hObject)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoCuteIWindowBaseGetText, nullptr, ("IWindowBaseGetText (Pointer hObject)(Sys.Type.IStringBuilder sb) -> (Int32 stringLength)"));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Cute::IMasterWindow 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;


	void NativeGetHandleForRococoCuteMasterWindow(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		int32 dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		int32 y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		int32 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _title;
		ReadInput(_title, _sf, -_offset);
		fstring title { _title->buffer, _title->length };


		Rococo::Cute::IMasterWindowFactory* nceContext = reinterpret_cast<Rococo::Cute::IMasterWindowFactory*>(_nce.context);
		// Uses: Rococo::Cute::IMasterWindow* FactoryConstructRococoCuteMasterWindow(Rococo::Cute::IMasterWindowFactory* _context, const fstring& _title, int32 _x, int32 _y, int32 _dx, int32 _dy);
		Rococo::Cute::IMasterWindow* pObject = FactoryConstructRococoCuteMasterWindow(nceContext, title, x, y, dx, dy);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo { namespace Cute { 
	void AddNativeCalls_RococoCuteIMasterWindow(Rococo::Script::IPublicScriptSystem& ss, Rococo::Cute::IMasterWindowFactory* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Cute.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoCuteMasterWindow, _nceContext, ("GetHandleForIMasterWindow0 (Sys.Type.IString title)(Int32 x)(Int32 y)(Int32 dx)(Int32 dy) -> (Pointer hObject)"));
	}
}}

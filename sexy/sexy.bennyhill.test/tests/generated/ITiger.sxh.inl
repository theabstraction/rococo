// BennyHill generated Sexy native functions for Sys::Animals::ITiger 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeSysAnimalsITigerWrite(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);

		struct _IString { uintptr_t vTable; Sexy::int32 length; Sexy::csexstr buffer; };

		_offset += sizeof(void*);

		_IString* _text;
		ReadInput(_text, _sf, -_offset);
		Sys::SexString text;
		text.buffer = _text->buffer;
		text.length = _text->length;

		Sys::Animals::ITiger* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Write(text);
	}

	void NativeGetHandleForSysAnimalsGetTigerByName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);

		struct _IString { uintptr_t vTable; Sexy::int32 length; Sexy::csexstr buffer; };

		_offset += sizeof(void*);

		_IString* _tigerName;
		ReadInput(_tigerName, _sf, -_offset);
		Sys::SexString tigerName;
		tigerName.buffer = _tigerName->buffer;
		tigerName.length = _tigerName->length;

		Sys::IZoo* nceContext = reinterpret_cast<Sys::IZoo*>(_nce.context);
		// Uses: Sys::IZoo* FactoryConstructSysAnimalsGetTigerByName(Sys::IZoo* _context, const Sys::SexString& _tigerName);
		Sys::Animals::ITiger* pObject = FactoryConstructSysAnimalsGetTigerByName(nceContext, tigerName);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}
namespace Sys::Animals
{
	void AddNativeCalls_SysAnimalsITiger(Sexy::Script::IPublicScriptSystem& ss, Sys::IZoo* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Animals.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForSysAnimalsGetTigerByName, nceContext, SEXTEXT("GetHandleForITiger0 (Sys.Type.IString tigerName) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeSysAnimalsITigerWrite, nullptr, SEXTEXT("ITigerWrite (Pointer hObject)(Sys.Type.IString text) -> "));
	}

}

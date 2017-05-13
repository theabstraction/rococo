// BennyHill generated Sexy native functions for Sys::Animals::ITigerPup 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeSysAnimalsITigerPupAppendName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable*);
		VirtualTable* builder;
		ReadInput(builder, _sf, -_offset);
		Rococo::Helpers::StringPopulator _builderPopulator(_nce, builder);
		Sys::Animals::ITigerPup* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendName(_builderPopulator);
	}

}

namespace Sys { namespace Animals { 
	void AddNativeCalls_SysAnimalsITigerPup(Rococo::Script::IPublicScriptSystem& ss, Sys::Animals::ITigerPup* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Animals.Native"));
		ss.AddNativeCall(ns, NativeSysAnimalsITigerPupAppendName, nullptr, SEXTEXT("ITigerPupAppendName (Pointer hObject)(Sys.Type.IStringBuilder builder) -> "));
	}
}}
// BennyHill generated Sexy native functions for Sys::Animals::ITiger 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeSysAnimalsITigerMakeBabies(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Sys::Animals::ITiger* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Sys::Animals::ITigerPup* pup = _pObject->MakeBabies();
		_offset += sizeof(CReflectedClass*);
		auto& _pupStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Sys.Animals"),SEXTEXT("ITigerPup"), SEXTEXT("ProxyITigerPup"), _nce.ss);
		CReflectedClass* _sxypup = _nce.ss.Represent(_pupStruct, pup);
		WriteOutput(&_sxypup->header._vTables[0], _sf, -_offset);
	}

	void NativeGetHandleForSysAnimalsGetTigerByName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _tigerName;
		ReadInput(_tigerName, _sf, -_offset);
		fstring tigerName { _tigerName->buffer, _tigerName->length };


		Sys::IZoo* nceContext = reinterpret_cast<Sys::IZoo*>(_nce.context);
		// Uses: Sys::Animals::ITiger* FactoryConstructSysAnimalsGetTigerByName(Sys::IZoo* _context, const fstring& _tigerName);
		Sys::Animals::ITiger* pObject = FactoryConstructSysAnimalsGetTigerByName(nceContext, tigerName);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Sys { namespace Animals { 
	void AddNativeCalls_SysAnimalsITiger(Rococo::Script::IPublicScriptSystem& ss, Sys::IZoo* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Animals.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForSysAnimalsGetTigerByName, _nceContext, SEXTEXT("GetHandleForITiger0 (Sys.Type.IString tigerName) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeSysAnimalsITigerMakeBabies, nullptr, SEXTEXT("ITigerMakeBabies (Pointer hObject) -> (Sys.Animals.ITigerPup pup)"));
	}
}}

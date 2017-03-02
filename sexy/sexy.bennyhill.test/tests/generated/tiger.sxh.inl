namespace // Benny Hill generated helpers
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	class StringPopulator : public IStringPopulator
	{
		CClassSysTypeStringBuilder* builder;
	public:
		StringPopulator(NativeCallEnvironment& _nce, VirtualTable* vTableBuilder)
		{
			char* _instance = ((char*)vTableBuilder) + vTableBuilder->OffsetToInstance;
			CClassDesc* _abstractClass = reinterpret_cast<CClassDesc*>(_instance);
			if (_abstractClass->structDef != _nce.ss.PublicProgramObject().GetSysType(SEXY_CLASS_ID_STRINGBUILDER))
			{
				_nce.ss.ThrowFromNativeCode(0, SEXTEXT("Builder was not a Sys.Type.StringBuilder"));
			}
			builder = reinterpret_cast<CClassSysTypeStringBuilder*>(_abstractClass);
		}

		virtual void Populate(csexstr text)
		{
			SafeCat(builder->buffer, builder->capacity, text, _TRUNCATE);
			builder->length = StringLength(builder->buffer);
		}
	};
}
// BennyHill generated Sexy native functions for Sys::Animals::ITigerPup 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeSysAnimalsITigerPupAppendName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable*);
		VirtualTable* builder;
		ReadInput(builder, _sf, -_offset);
		StringPopulator _builderPopulator(_nce, builder);
		Sys::Animals::ITigerPup* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendName(_builderPopulator);
	}

}

namespace Sys { namespace Animals { 
	void AddNativeCalls_SysAnimalsITigerPup(Sexy::Script::IPublicScriptSystem& ss, Sys::Animals::ITigerPup* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Animals.Native"));
		ss.AddNativeCall(ns, NativeSysAnimalsITigerPupAppendName, nullptr, SEXTEXT("ITigerPupAppendName (Pointer hObject)(Sys.Type.IStringBuilder builder) -> "));
	}
}}
// BennyHill generated Sexy native functions for Sys::Animals::ITiger 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeSysAnimalsITigerMakeBabies(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Sys::Animals::ITiger* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Sys::Animals::ITigerPup* pup = _pObject->MakeBabies();
		_offset += sizeof(CReflectedClass*);
		const IStructure* _pupStruct = Sexy::Script::FindStructure(_nce.ss,  SEXTEXT("Sys.Animals.ITigerPup"));
		CReflectedClass* _sxypup = _nce.ss.Represent(*_pupStruct, pup);
		WriteOutput(_sxypup, _sf, -_offset);
	}

	void NativeGetHandleForSysAnimalsGetTigerByName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
	void AddNativeCalls_SysAnimalsITiger(Sexy::Script::IPublicScriptSystem& ss, Sys::IZoo* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Sys.Animals.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForSysAnimalsGetTigerByName, _nceContext, SEXTEXT("GetHandleForITiger0 (Sys.Type.IString tigerName) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeSysAnimalsITigerMakeBabies, nullptr, SEXTEXT("ITigerMakeBabies (Pointer hObject) -> (Sys.Animals.ITigerPup pup)"));
	}
}}

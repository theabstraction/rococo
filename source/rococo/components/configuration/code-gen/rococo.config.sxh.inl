// BennyHill generated Sexy native functions for Rococo::Components::Config::IConfigBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoComponentsConfigIConfigBaseDeprecate(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::ROID roid;
		_offset += sizeof(roid);
		ReadInput(roid, _sf, -_offset);

		Rococo::Components::Config::IConfigBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 success = _pObject->Deprecate(roid);
		_offset += sizeof(success);
		WriteOutput(success, _sf, -_offset);
	}
	void NativeRococoComponentsConfigIConfigBaseDeprecateAll(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Config::IConfigBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeprecateAll();
	}
	void NativeRococoComponentsConfigIConfigBaseIsActive(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::ROID roid;
		_offset += sizeof(roid);
		ReadInput(roid, _sf, -_offset);

		Rococo::Components::Config::IConfigBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isActive = _pObject->IsActive(roid);
		_offset += sizeof(isActive);
		WriteOutput(isActive, _sf, -_offset);
	}
	void NativeRococoComponentsConfigIConfigBaseMaxTableEntries(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Config::IConfigBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 numberOfEntries = _pObject->MaxTableEntries();
		_offset += sizeof(numberOfEntries);
		WriteOutput(numberOfEntries, _sf, -_offset);
	}
	void NativeRococoComponentsConfigIConfigBaseNewROID(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Config::IConfigBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Components::ROID roid = _pObject->NewROID();
		_offset += sizeof(roid);
		WriteOutput(roid, _sf, -_offset);
	}

	void NativeGetHandleForRococoComponentsConfigGetConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Config::IConfigBase* nceContext = reinterpret_cast<Rococo::Components::Config::IConfigBase*>(_nce.context);
		// Uses: Rococo::Components::Config::IConfigBase* FactoryConstructRococoComponentsConfigGetConfig(Rococo::Components::Config::IConfigBase* _context);
		Rococo::Components::Config::IConfigBase* pObject = FactoryConstructRococoComponentsConfigGetConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Components::Config
{
	void AddNativeCalls_RococoComponentsConfigIConfigBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Config::IConfigBase* _nceContext)
	{
		HIDE_COMPILER_WARNINGS(_nceContext);
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Components.Config.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoComponentsConfigGetConfig, _nceContext, ("GetHandleForIConfigBase0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoComponentsConfigIConfigBaseDeprecate, nullptr, ("IConfigBaseDeprecate (Pointer hObject)(Rococo.ROID roid) -> (Bool success)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoComponentsConfigIConfigBaseDeprecateAll, nullptr, ("IConfigBaseDeprecateAll (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoComponentsConfigIConfigBaseIsActive, nullptr, ("IConfigBaseIsActive (Pointer hObject)(Rococo.ROID roid) -> (Bool isActive)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoComponentsConfigIConfigBaseMaxTableEntries, nullptr, ("IConfigBaseMaxTableEntries (Pointer hObject) -> (Int64 numberOfEntries)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoComponentsConfigIConfigBaseNewROID, nullptr, ("IConfigBaseNewROID (Pointer hObject) -> (Rococo.ROID roid)"), __FILE__, __LINE__);
	}
}

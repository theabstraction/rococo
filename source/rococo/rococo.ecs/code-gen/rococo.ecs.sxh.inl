// BennyHill generated Sexy native functions for Rococo::IECSBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIECSBaseDeprecate(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::ROID roid;
		_offset += sizeof(roid);
		ReadInput(roid, _sf, -_offset);

		Rococo::IECSBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 success = _pObject->Deprecate(roid);
		_offset += sizeof(success);
		WriteOutput(success, _sf, -_offset);
	}
	void NativeRococoIECSBaseDeprecateAll(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IECSBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeprecateAll();
	}
	void NativeRococoIECSBaseIsActive(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::ROID roid;
		_offset += sizeof(roid);
		ReadInput(roid, _sf, -_offset);

		Rococo::IECSBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isActive = _pObject->IsActive(roid);
		_offset += sizeof(isActive);
		WriteOutput(isActive, _sf, -_offset);
	}
	void NativeRococoIECSBaseMaxTableEntries(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IECSBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 numberOfEntries = _pObject->MaxTableEntries();
		_offset += sizeof(numberOfEntries);
		WriteOutput(numberOfEntries, _sf, -_offset);
	}
	void NativeRococoIECSBaseNewROID(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IECSBase* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Components::ROID roid = _pObject->NewROID();
		_offset += sizeof(roid);
		WriteOutput(roid, _sf, -_offset);
	}

	void NativeGetHandleForRococoECSGetECS(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IECS* nceContext = reinterpret_cast<Rococo::IECS*>(_nce.context);
		// Uses: Rococo::IECSBase* FactoryConstructRococoECSGetECS(Rococo::IECS* _context);
		Rococo::IECSBase* pObject = FactoryConstructRococoECSGetECS(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIECSBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::IECS* _nceContext)
	{
		HIDE_COMPILER_WARNINGS(_nceContext);
		const INamespace& ns = ss.AddNativeNamespace("Rococo.ECS.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoECSGetECS, _nceContext, ("GetHandleForIECS0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIECSBaseDeprecate, nullptr, ("IECSDeprecate (Pointer hObject)(Int64 roid) -> (Bool success)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIECSBaseDeprecateAll, nullptr, ("IECSDeprecateAll (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIECSBaseIsActive, nullptr, ("IECSIsActive (Pointer hObject)(Int64 roid) -> (Bool isActive)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIECSBaseMaxTableEntries, nullptr, ("IECSMaxTableEntries (Pointer hObject) -> (Int64 numberOfEntries)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIECSBaseNewROID, nullptr, ("IECSNewROID (Pointer hObject) -> (Int64 roid)"), __FILE__, __LINE__);
	}
}
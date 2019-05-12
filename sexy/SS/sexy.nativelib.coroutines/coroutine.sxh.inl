// BennyHill generated Sexy native functions for Sys::ICoroutineControl 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeSysICoroutineControlAdd(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Sex::CoroutineRef coroutine;
		_offset += sizeof(coroutine);
		ReadInput(coroutine, _sf, -_offset);

		Sys::ICoroutineControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 id = _pObject->Add(coroutine);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeSysICoroutineControlContinue(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Sys::ICoroutineControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 id = _pObject->Continue();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeSysICoroutineControlContinueSpecific(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Sys::ICoroutineControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isComplete = _pObject->ContinueSpecific(id);
		_offset += sizeof(isComplete);
		WriteOutput(isComplete, _sf, -_offset);
	}
	void NativeSysICoroutineControlRelease(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Sys::ICoroutineControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 success = _pObject->Release(id);
		_offset += sizeof(success);
		WriteOutput(success, _sf, -_offset);
	}

	void NativeGetHandleForSysCoroutines(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Sys::ICoroutineControl* nceContext = reinterpret_cast<Sys::ICoroutineControl*>(_nce.context);
		// Uses: Sys::ICoroutineControl* FactoryConstructSysCoroutines(Sys::ICoroutineControl* _context);
		Sys::ICoroutineControl* pObject = FactoryConstructSysCoroutines(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Sys { 
	void AddNativeCalls_SysICoroutineControl(Rococo::Script::IPublicScriptSystem& ss, Sys::ICoroutineControl* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Sys.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForSysCoroutines, _nceContext, ("GetHandleForICoroutineControl0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeSysICoroutineControlAdd, nullptr, ("ICoroutineControlAdd (Pointer hObject)(Sys.ICoroutine coroutine) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeSysICoroutineControlContinue, nullptr, ("ICoroutineControlContinue (Pointer hObject) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeSysICoroutineControlContinueSpecific, nullptr, ("ICoroutineControlContinueSpecific (Pointer hObject)(Int64 id) -> (Bool isComplete)"));
		ss.AddNativeCall(ns, NativeSysICoroutineControlRelease, nullptr, ("ICoroutineControlRelease (Pointer hObject)(Int64 id) -> (Bool success)"));
	}
}

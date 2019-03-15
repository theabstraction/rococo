// BennyHill generated Sexy native functions for MHost::IScreenBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIScreenBuilderDrawBitmap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* topLeft;
		_offset += sizeof(topLeft);
		ReadInput(topLeft, _sf, -_offset);

		MHost::BitmapSpec* spec;
		_offset += sizeof(spec);
		ReadInput(spec, _sf, -_offset);

		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawBitmap(*spec, *topLeft);
	}
	void NativeMHostIScreenBuilderTryGetBitmapSpec(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::BitmapSpec* spec;
		_offset += sizeof(spec);
		ReadInput(spec, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };


		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSuccessful = _pObject->TryGetBitmapSpec(resourceName, *spec);
		_offset += sizeof(isSuccessful);
		WriteOutput(isSuccessful, _sf, -_offset);
	}

}

namespace MHost { 
	void AddNativeCalls_MHostIScreenBuilder(Rococo::Script::IPublicScriptSystem& ss, MHost::IScreenBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Native"));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderDrawBitmap, nullptr, ("IScreenBuilderDrawBitmap (Pointer hObject)(MHost.BitmapSpec spec)(Sys.Maths.Vec2i topLeft) -> "));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderTryGetBitmapSpec, nullptr, ("IScreenBuilderTryGetBitmapSpec (Pointer hObject)(Sys.Type.IString resourceName)(MHost.BitmapSpec spec) -> (Bool isSuccessful)"));
	}
}
// BennyHill generated Sexy native functions for MHost::IEngine 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIEngineDrawAll(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawAll();
	}
	void NativeMHostIEngineGetScreenBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		MHost::IScreenBuilder* s = _pObject->GetScreenBuilder();
		_offset += sizeof(CReflectedClass*);
		auto& _sStruct = Rococo::Helpers::GetDefaultProxy(("MHost"),("IScreenBuilder"), ("ProxyIScreenBuilder"), _nce.ss);
		CReflectedClass* _sxys = _nce.ss.Represent(_sStruct, s);
		WriteOutput(&_sxys->header._vTables[0], _sf, -_offset);
	}
	void NativeMHostIEngineIsRunning(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isRunning = _pObject->IsRunning();
		_offset += sizeof(isRunning);
		WriteOutput(isRunning, _sf, -_offset);
	}
	void NativeMHostIEngineYieldForSystemMessages(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 sleepMS;
		_offset += sizeof(sleepMS);
		ReadInput(sleepMS, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->YieldForSystemMessages(sleepMS);
	}
	void NativeMHostIEngineSetPhase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 phaseIndex;
		_offset += sizeof(phaseIndex);
		ReadInput(phaseIndex, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPhase(phaseIndex);
	}

	void NativeGetHandleForMHostEngine(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* nceContext = reinterpret_cast<MHost::IEngine*>(_nce.context);
		// Uses: MHost::IEngine* FactoryConstructMHostEngine(MHost::IEngine* _context);
		MHost::IEngine* pObject = FactoryConstructMHostEngine(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace MHost { 
	void AddNativeCalls_MHostIEngine(Rococo::Script::IPublicScriptSystem& ss, MHost::IEngine* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForMHostEngine, _nceContext, ("GetHandleForIEngine0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeMHostIEngineDrawAll, nullptr, ("IEngineDrawAll (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineGetScreenBuilder, nullptr, ("IEngineGetScreenBuilder (Pointer hObject) -> (MHost.IScreenBuilder s)"));
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"));
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineSetPhase, nullptr, ("IEngineSetPhase (Pointer hObject)(Int32 phaseIndex) -> "));
	}
}

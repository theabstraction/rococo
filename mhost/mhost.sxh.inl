// BennyHill generated Sexy native functions for MHost::IScreenBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIScreenBuilderPushTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PushTriangle(*t);
	}
	void NativeMHostIScreenBuilderPushQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiQuad* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PushQuad(*q);
	}
	void NativeMHostIScreenBuilderTryGetBitmapSpec(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };


		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSuccessful = _pObject->TryGetBitmapSpec(resourceName, *loc);
		_offset += sizeof(isSuccessful);
		WriteOutput(isSuccessful, _sf, -_offset);
	}
	void NativeMHostIScreenBuilderRender(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Render();
	}

}

namespace MHost { 
	void AddNativeCalls_MHostIScreenBuilder(Rococo::Script::IPublicScriptSystem& ss, MHost::IScreenBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Native"));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderPushTriangle, nullptr, ("IScreenBuilderPushTriangle (Pointer hObject)(Sys.MPlat.GuiTriangle t) -> "));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderPushQuad, nullptr, ("IScreenBuilderPushQuad (Pointer hObject)(Sys.MPlat.GuiQuad q) -> "));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderTryGetBitmapSpec, nullptr, ("IScreenBuilderTryGetBitmapSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> (Bool isSuccessful)"));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderRender, nullptr, ("IScreenBuilderRender (Pointer hObject) -> "));
	}
}
// BennyHill generated Sexy native functions for MHost::IEngine 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIEngineScreenBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		MHost::IScreenBuilder* s = _pObject->ScreenBuilder();
		_offset += sizeof(CReflectedClass*);
		auto& _sStruct = Rococo::Helpers::GetDefaultProxy(("MHost"),("IScreenBuilder"), ("ProxyIScreenBuilder"), _nce.ss);
		CReflectedClass* _sxys = _nce.ss.Represent(_sStruct, s);
		WriteOutput(&_sxys->header.pVTables[0], _sf, -_offset);
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
		ss.AddNativeCall(ns, NativeMHostIEngineScreenBuilder, nullptr, ("IEngineScreenBuilder (Pointer hObject) -> (MHost.IScreenBuilder s)"));
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"));
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> "));
	}
}

namespace Rococo { 
	bool TryParse(const Rococo::fstring& s, AlignmentFlags& value)
	{
		if (s ==  "AlignmentFlags_None"_fstring)
		{
			value = AlignmentFlags_None;
		}
		else if (s ==  "AlignmentFlags_Left"_fstring)
		{
			value = AlignmentFlags_Left;
		}
		else if (s ==  "AlignmentFlags_Right"_fstring)
		{
			value = AlignmentFlags_Right;
		}
		else if (s ==  "AlignmentFlags_Bottom"_fstring)
		{
			value = AlignmentFlags_Bottom;
		}
		else if (s ==  "AlignmentFlags_Top"_fstring)
		{
			value = AlignmentFlags_Top;
		}
		else if (s ==  "AlignmentFlags_Mirror"_fstring)
		{
			value = AlignmentFlags_Mirror;
		}
		else if (s ==  "AlignmentFlags_Flip"_fstring)
		{
			value = AlignmentFlags_Flip;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, AlignmentFlags& value)
	{
		if (s ==  "None"_fstring)
		{
			value = AlignmentFlags_None;
		}
		else if (s ==  "Left"_fstring)
		{
			value = AlignmentFlags_Left;
		}
		else if (s ==  "Right"_fstring)
		{
			value = AlignmentFlags_Right;
		}
		else if (s ==  "Bottom"_fstring)
		{
			value = AlignmentFlags_Bottom;
		}
		else if (s ==  "Top"_fstring)
		{
			value = AlignmentFlags_Top;
		}
		else if (s ==  "Mirror"_fstring)
		{
			value = AlignmentFlags_Mirror;
		}
		else if (s ==  "Flip"_fstring)
		{
			value = AlignmentFlags_Flip;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(AlignmentFlags value)
	{
		switch(value)
		{
			case AlignmentFlags_None:
				return "None"_fstring;
			case AlignmentFlags_Left:
				return "Left"_fstring;
			case AlignmentFlags_Right:
				return "Right"_fstring;
			case AlignmentFlags_Bottom:
				return "Bottom"_fstring;
			case AlignmentFlags_Top:
				return "Top"_fstring;
			case AlignmentFlags_Mirror:
				return "Mirror"_fstring;
			case AlignmentFlags_Flip:
				return "Flip"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.AlignmentFlags

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
	void NativeMHostIScreenBuilderDrawSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Vec2i* pixelPos;
		_offset += sizeof(pixelPos);
		ReadInput(pixelPos, _sf, -_offset);

		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawSprite(*pixelPos, alignmentFlags, *loc);
	}
	void NativeMHostIScreenBuilderStretchSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		Rococo::GuiRect* screenQuad;
		_offset += sizeof(screenQuad);
		ReadInput(screenQuad, _sf, -_offset);

		MHost::IScreenBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->StretchSprite(*screenQuad, *loc);
	}
	void NativeMHostIScreenBuilderTryGetSpriteSpec(NativeCallEnvironment& _nce)
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
		boolean32 isSuccessful = _pObject->TryGetSpriteSpec(resourceName, *loc);
		_offset += sizeof(isSuccessful);
		WriteOutput(isSuccessful, _sf, -_offset);
	}
	void NativeMHostIScreenBuilderGetSpriteSpec(NativeCallEnvironment& _nce)
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
		_pObject->GetSpriteSpec(resourceName, *loc);
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
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderDrawSprite, nullptr, ("IScreenBuilderDrawSprite (Pointer hObject)(Sys.Maths.Vec2i pixelPos)(Int32 alignmentFlags)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderStretchSprite, nullptr, ("IScreenBuilderStretchSprite (Pointer hObject)(Sys.Maths.Recti screenQuad)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderTryGetSpriteSpec, nullptr, ("IScreenBuilderTryGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> (Bool isSuccessful)"));
		ss.AddNativeCall(ns, NativeMHostIScreenBuilderGetSpriteSpec, nullptr, ("IScreenBuilderGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> "));
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

	void NativeMHostIEnginePollKeyState(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::KeyState* keys;
		_offset += sizeof(keys);
		ReadInput(keys, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PollKeyState(*keys);
	}
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
		ss.AddNativeCall(ns, NativeMHostIEnginePollKeyState, nullptr, ("IEnginePollKeyState (Pointer hObject)(MHost.KeyState keys) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineScreenBuilder, nullptr, ("IEngineScreenBuilder (Pointer hObject) -> (MHost.IScreenBuilder s)"));
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"));
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> "));
	}
}

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

// BennyHill generated Sexy native functions for MHost::IGui 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIGuiPushTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PushTriangle(*t);
	}
	void NativeMHostIGuiDrawSprite(NativeCallEnvironment& _nce)
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

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawSprite(*pixelPos, alignmentFlags, *loc);
	}
	void NativeMHostIGuiStretchSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		Rococo::GuiRect* screenQuad;
		_offset += sizeof(screenQuad);
		ReadInput(screenQuad, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->StretchSprite(*screenQuad, *loc);
	}
	void NativeMHostIGuiAdvanceFrame(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AdvanceFrame();
	}

}

namespace MHost { 
	void AddNativeCalls_MHostIGui(Rococo::Script::IPublicScriptSystem& ss, MHost::IGui* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Native"));
		ss.AddNativeCall(ns, NativeMHostIGuiPushTriangle, nullptr, ("IGuiPushTriangle (Pointer hObject)(Sys.MPlat.GuiTriangle t) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawSprite, nullptr, ("IGuiDrawSprite (Pointer hObject)(Sys.Maths.Vec2i pixelPos)(Int32 alignmentFlags)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiStretchSprite, nullptr, ("IGuiStretchSprite (Pointer hObject)(Sys.Maths.Recti screenQuad)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiAdvanceFrame, nullptr, ("IGuiAdvanceFrame (Pointer hObject) -> "));
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
		MHost::OS::KeyState* keys;
		_offset += sizeof(keys);
		ReadInput(keys, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PollKeyState(*keys);
	}
	void NativeMHostIEngineRender(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::GuiPopulator populator;
		_offset += sizeof(populator);
		ReadInput(populator, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Render(populator);
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
	void NativeMHostIEngineTryGetSpriteSpec(NativeCallEnvironment& _nce)
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


		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSuccessful = _pObject->TryGetSpriteSpec(resourceName, *loc);
		_offset += sizeof(isSuccessful);
		WriteOutput(isSuccessful, _sf, -_offset);
	}
	void NativeMHostIEngineGetSpriteSpec(NativeCallEnvironment& _nce)
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


		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSpriteSpec(resourceName, *loc);
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
		ss.AddNativeCall(ns, NativeMHostIEnginePollKeyState, nullptr, ("IEnginePollKeyState (Pointer hObject)(MHost.OS.KeyState keys) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineRender, nullptr, ("IEngineRender (Pointer hObject)(MHost.GuiPopulator populator) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"));
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineTryGetSpriteSpec, nullptr, ("IEngineTryGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> (Bool isSuccessful)"));
		ss.AddNativeCall(ns, NativeMHostIEngineGetSpriteSpec, nullptr, ("IEngineGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> "));
	}
}

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
		else if (s ==  "AlignmentFlags_Clipped"_fstring)
		{
			value = AlignmentFlags_Clipped;
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
		else if (s ==  "Clipped"_fstring)
		{
			value = AlignmentFlags_Clipped;
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
			case AlignmentFlags_Clipped:
				return "Clipped"_fstring;
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

	void NativeMHostIGuiDrawTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawTriangle(*t);
	}
	void NativeMHostIGuiDrawQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiQuad* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawQuad(*q);
	}
	void NativeMHostIGuiDrawBorder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb br;
		_offset += sizeof(br);
		ReadInput(br, _sf, -_offset);

		RGBAb bl;
		_offset += sizeof(bl);
		ReadInput(bl, _sf, -_offset);

		RGBAb tr;
		_offset += sizeof(tr);
		ReadInput(tr, _sf, -_offset);

		RGBAb tl;
		_offset += sizeof(tl);
		ReadInput(tl, _sf, -_offset);

		float pxThickness;
		_offset += sizeof(pxThickness);
		ReadInput(pxThickness, _sf, -_offset);

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawBorder(*rect, pxThickness, tl, tr, bl, br);
	}
	void NativeMHostIGuiFillRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FillRect(*rect, colour);
	}
	void NativeMHostIGuiDrawColouredSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		float blendFactor;
		_offset += sizeof(blendFactor);
		ReadInput(blendFactor, _sf, -_offset);

		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Vec2* pixelPos;
		_offset += sizeof(pixelPos);
		ReadInput(pixelPos, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawColouredSprite(*pixelPos, alignmentFlags, *loc, blendFactor, colour);
	}
	void NativeMHostIGuiDrawScaledColouredSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float scaleFactor;
		_offset += sizeof(scaleFactor);
		ReadInput(scaleFactor, _sf, -_offset);

		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		float blendFactor;
		_offset += sizeof(blendFactor);
		ReadInput(blendFactor, _sf, -_offset);

		Rococo::Textures::BitmapLocation* loc;
		_offset += sizeof(loc);
		ReadInput(loc, _sf, -_offset);

		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Vec2* pixelPos;
		_offset += sizeof(pixelPos);
		ReadInput(pixelPos, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawScaledColouredSprite(*pixelPos, alignmentFlags, *loc, blendFactor, colour, scaleFactor);
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

		Vec2* pixelPos;
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

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->StretchSprite(*rect, *loc);
	}
	void NativeMHostIGuiDrawClippedText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiRectf* clipRect;
		_offset += sizeof(clipRect);
		ReadInput(clipRect, _sf, -_offset);

		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawClippedText(*rect, alignmentFlags, text, fontIndex, colour, *clipRect);
	}
	void NativeMHostIGuiDrawTextWithCaret(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 caretPos;
		_offset += sizeof(caretPos);
		ReadInput(caretPos, _sf, -_offset);

		Rococo::GuiRectf* clipRect;
		_offset += sizeof(clipRect);
		ReadInput(clipRect, _sf, -_offset);

		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawTextWithCaret(*rect, alignmentFlags, text, fontIndex, colour, *clipRect, caretPos);
	}
	void NativeMHostIGuiDrawText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 alignmentFlags;
		_offset += sizeof(alignmentFlags);
		ReadInput(alignmentFlags, _sf, -_offset);

		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawText(*rect, alignmentFlags, text, fontIndex, colour);
	}
	void NativeMHostIGuiEvalTextSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* pixelSpan;
		_offset += sizeof(pixelSpan);
		ReadInput(pixelSpan, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->EvalTextSpan(text, fontIndex, *pixelSpan);
	}
	void NativeMHostIGuiGetFontDescription(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::Graphics::FontDesc* fd;
		_offset += sizeof(fd);
		ReadInput(fd, _sf, -_offset);

		_offset += sizeof(VirtualTable**);
		VirtualTable** familyName;
		ReadInput(familyName, _sf, -_offset);
		Rococo::Helpers::StringPopulator _familyNamePopulator(_nce, familyName);
		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetFontDescription(fontIndex, _familyNamePopulator, *fd);
	}
	void NativeMHostIGuiGetNumberOfFonts(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 numberOfFonts = _pObject->GetNumberOfFonts();
		_offset += sizeof(numberOfFonts);
		WriteOutput(numberOfFonts, _sf, -_offset);
	}
	void NativeMHostIGuiGetScreenSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetScreenSpan(*span);
	}
	void NativeMHostIGuiGetCursorPos(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetCursorPos(*pos);
	}
	void NativeMHostIGuiSetGuiShaders(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pixelShaderFilename;
		ReadInput(_pixelShaderFilename, _sf, -_offset);
		fstring pixelShaderFilename { _pixelShaderFilename->buffer, _pixelShaderFilename->length };


		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetGuiShaders(pixelShaderFilename);
	}

}

namespace MHost { 
	void AddNativeCalls_MHostIGui(Rococo::Script::IPublicScriptSystem& ss, MHost::IGui* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("MHost.Native"));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawTriangle, nullptr, ("IGuiDrawTriangle (Pointer hObject)(Sys.MPlat.GuiTriangle t) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawQuad, nullptr, ("IGuiDrawQuad (Pointer hObject)(Sys.MPlat.GuiQuad q) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawBorder, nullptr, ("IGuiDrawBorder (Pointer hObject)(Sys.Maths.Rectf rect)(Float32 pxThickness)(Int32 tl)(Int32 tr)(Int32 bl)(Int32 br) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiFillRect, nullptr, ("IGuiFillRect (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 colour) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawColouredSprite, nullptr, ("IGuiDrawColouredSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(Sys.MPlat.BitmapLocation loc)(Float32 blendFactor)(Int32 colour) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawScaledColouredSprite, nullptr, ("IGuiDrawScaledColouredSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(Sys.MPlat.BitmapLocation loc)(Float32 blendFactor)(Int32 colour)(Float32 scaleFactor) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawSprite, nullptr, ("IGuiDrawSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiStretchSprite, nullptr, ("IGuiStretchSprite (Pointer hObject)(Sys.Maths.Rectf rect)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawClippedText, nullptr, ("IGuiDrawClippedText (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour)(Sys.Maths.Rectf clipRect) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawTextWithCaret, nullptr, ("IGuiDrawTextWithCaret (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour)(Sys.Maths.Rectf clipRect)(Int32 caretPos) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiDrawText, nullptr, ("IGuiDrawText (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiEvalTextSpan, nullptr, ("IGuiEvalTextSpan (Pointer hObject)(Sys.Type.IString text)(Int32 fontIndex)(Sys.Maths.Vec2 pixelSpan) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiGetFontDescription, nullptr, ("IGuiGetFontDescription (Pointer hObject)(Int32 fontIndex)(Sys.Type.IStringBuilder familyName)(MHost.Graphics.FontDesc fd) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiGetNumberOfFonts, nullptr, ("IGuiGetNumberOfFonts (Pointer hObject) -> (Int32 numberOfFonts)"));
		ss.AddNativeCall(ns, NativeMHostIGuiGetScreenSpan, nullptr, ("IGuiGetScreenSpan (Pointer hObject)(Sys.Maths.Vec2 span) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiGetCursorPos, nullptr, ("IGuiGetCursorPos (Pointer hObject)(Sys.Maths.Vec2 pos) -> "));
		ss.AddNativeCall(ns, NativeMHostIGuiSetGuiShaders, nullptr, ("IGuiSetGuiShaders (Pointer hObject)(Sys.Type.IString pixelShaderFilename) -> "));
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
	void NativeMHostIEngineGetNextMouseEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::MouseEvent* me;
		_offset += sizeof(me);
		ReadInput(me, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasPopped = _pObject->GetNextMouseEvent(*me);
		_offset += sizeof(wasPopped);
		WriteOutput(wasPopped, _sf, -_offset);
	}
	void NativeMHostIEngineGetNextKeyboardEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::MHostKeyboardEvent* ke;
		_offset += sizeof(ke);
		ReadInput(ke, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasPopped = _pObject->GetNextKeyboardEvent(*ke);
		_offset += sizeof(wasPopped);
		WriteOutput(wasPopped, _sf, -_offset);
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
	void NativeMHostIEngineShutdown(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Shutdown();
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
	void NativeMHostIEngineSetNextScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetNextScript(scriptName);
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
		ss.AddNativeCall(ns, NativeMHostIEngineGetNextMouseEvent, nullptr, ("IEngineGetNextMouseEvent (Pointer hObject)(MHost.OS.MouseEvent me) -> (Bool wasPopped)"));
		ss.AddNativeCall(ns, NativeMHostIEngineGetNextKeyboardEvent, nullptr, ("IEngineGetNextKeyboardEvent (Pointer hObject)(MHost.OS.KeyboardEvent ke) -> (Bool wasPopped)"));
		ss.AddNativeCall(ns, NativeMHostIEngineRender, nullptr, ("IEngineRender (Pointer hObject)(MHost.GuiPopulator populator) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"));
		ss.AddNativeCall(ns, NativeMHostIEngineShutdown, nullptr, ("IEngineShutdown (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineTryGetSpriteSpec, nullptr, ("IEngineTryGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> (Bool isSuccessful)"));
		ss.AddNativeCall(ns, NativeMHostIEngineGetSpriteSpec, nullptr, ("IEngineGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(Sys.MPlat.BitmapLocation loc) -> "));
		ss.AddNativeCall(ns, NativeMHostIEngineSetNextScript, nullptr, ("IEngineSetNextScript (Pointer hObject)(Sys.Type.IString scriptName) -> "));
	}
}

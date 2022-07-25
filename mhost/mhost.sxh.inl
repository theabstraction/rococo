namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostOSIsKeyPressed(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vKeyCode;
		_offset += sizeof(vKeyCode);
		ReadInput(vKeyCode, _sf, -_offset);

		MHost::OS::KeyState* keys;
		_offset += sizeof(keys);
		ReadInput(keys, _sf, -_offset);

		boolean32 isPressed = MHost::OS::IsKeyPressed(*keys, vKeyCode);
		_offset += sizeof(isPressed);
		WriteOutput(isPressed, _sf, -_offset);
	}

	void NativeMHostMathsClamp0to1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float f;
		_offset += sizeof(f);
		ReadInput(f, _sf, -_offset);

		float clampedValue = MHost::Maths::Clamp0to1(f);
		_offset += sizeof(clampedValue);
		WriteOutput(clampedValue, _sf, -_offset);
	}

	void NativeMHostGraphicsToRGBAb(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBA* colourF32x4;
		_offset += sizeof(colourF32x4);
		ReadInput(colourF32x4, _sf, -_offset);

		RGBAb colourB8x4 = MHost::Graphics::ToRGBAb(*colourF32x4);
		_offset += sizeof(colourB8x4);
		WriteOutput(colourB8x4, _sf, -_offset);
	}

	void NativeMHostGraphicsFloatToColourComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		int32 ic = MHost::Graphics::FloatToColourComponent(c);
		_offset += sizeof(ic);
		WriteOutput(ic, _sf, -_offset);
	}

}

namespace MHost::OS
{

	void AddNativeCalls_MHostOS(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.OS");
		ss.AddNativeCall(ns, NativeMHostOSIsKeyPressed, nullptr, ("IsKeyPressed(MHost.OS.KeyState keys)(Int32 vKeyCode) -> (Bool isPressed)"), __FILE__, __LINE__);
	}
}

namespace MHost::Maths
{

	void AddNativeCalls_MHostMaths(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.Maths");
		ss.AddNativeCall(ns, NativeMHostMathsClamp0to1, nullptr, ("Clamp0to1(Float32 f) -> (Float32 clampedValue)"), __FILE__, __LINE__);
	}
}

namespace MHost::Graphics
{

	void AddNativeCalls_MHostGraphics(Rococo::Script::IPublicScriptSystem& ss)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.Graphics");
		ss.AddNativeCall(ns, NativeMHostGraphicsToRGBAb, nullptr, ("ToRGBAb(RGBA colourF32x4) -> (Int32 colourB8x4)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostGraphicsFloatToColourComponent, nullptr, ("FloatToColourComponent(Float32 c) -> (Int32 ic)"), __FILE__, __LINE__);
	}
}

namespace Rococo
{
	bool TryParse(const Rococo::fstring& s, AlignmentFlags& value)
	{
		if (s ==  "AlignmentFlags_None"_fstring)
		{
			value = AlignmentFlags::None;
		}
		else if (s ==  "AlignmentFlags_Left"_fstring)
		{
			value = AlignmentFlags::Left;
		}
		else if (s ==  "AlignmentFlags_Right"_fstring)
		{
			value = AlignmentFlags::Right;
		}
		else if (s ==  "AlignmentFlags_Bottom"_fstring)
		{
			value = AlignmentFlags::Bottom;
		}
		else if (s ==  "AlignmentFlags_Top"_fstring)
		{
			value = AlignmentFlags::Top;
		}
		else if (s ==  "AlignmentFlags_Mirror"_fstring)
		{
			value = AlignmentFlags::Mirror;
		}
		else if (s ==  "AlignmentFlags_Flip"_fstring)
		{
			value = AlignmentFlags::Flip;
		}
		else if (s ==  "AlignmentFlags_Clipped"_fstring)
		{
			value = AlignmentFlags::Clipped;
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
			value = AlignmentFlags::None;
		}
		else if (s ==  "Left"_fstring)
		{
			value = AlignmentFlags::Left;
		}
		else if (s ==  "Right"_fstring)
		{
			value = AlignmentFlags::Right;
		}
		else if (s ==  "Bottom"_fstring)
		{
			value = AlignmentFlags::Bottom;
		}
		else if (s ==  "Top"_fstring)
		{
			value = AlignmentFlags::Top;
		}
		else if (s ==  "Mirror"_fstring)
		{
			value = AlignmentFlags::Mirror;
		}
		else if (s ==  "Flip"_fstring)
		{
			value = AlignmentFlags::Flip;
		}
		else if (s ==  "Clipped"_fstring)
		{
			value = AlignmentFlags::Clipped;
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
			case AlignmentFlags::None:
				return "None"_fstring;
			case AlignmentFlags::Left:
				return "Left"_fstring;
			case AlignmentFlags::Right:
				return "Right"_fstring;
			case AlignmentFlags::Bottom:
				return "Bottom"_fstring;
			case AlignmentFlags::Top:
				return "Top"_fstring;
			case AlignmentFlags::Mirror:
				return "Mirror"_fstring;
			case AlignmentFlags::Flip:
				return "Flip"_fstring;
			case AlignmentFlags::Clipped:
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
	void NativeMHostIGuiDrawLeftAligned(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float rightSoftEdge;
		_offset += sizeof(rightSoftEdge);
		ReadInput(rightSoftEdge, _sf, -_offset);

		float rightHardEdge;
		_offset += sizeof(rightHardEdge);
		ReadInput(rightHardEdge, _sf, -_offset);

		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		int32 fontHeight;
		_offset += sizeof(fontHeight);
		ReadInput(fontHeight, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		Rococo::GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DrawLeftAligned(*rect, text, fontIndex, fontHeight, colour, rightHardEdge, rightSoftEdge);
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

		int32 fontHeight;
		_offset += sizeof(fontHeight);
		ReadInput(fontHeight, _sf, -_offset);

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
		_pObject->EvalTextSpan(text, fontIndex, fontHeight, *pixelSpan);
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
	void NativeMHostIGuiSetScissorRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScissorRect(*rect);
	}
	void NativeMHostIGuiClearScissorRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearScissorRect();
	}

}

namespace MHost
{
	void AddNativeCalls_MHostIGui(Rococo::Script::IPublicScriptSystem& ss, MHost::IGui* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.Native");
		ss.AddNativeCall(ns, NativeMHostIGuiDrawTriangle, nullptr, ("IGuiDrawTriangle (Pointer hObject)(MPlat.GuiTriangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawQuad, nullptr, ("IGuiDrawQuad (Pointer hObject)(MPlat.GuiQuad q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawBorder, nullptr, ("IGuiDrawBorder (Pointer hObject)(Sys.Maths.Rectf rect)(Float32 pxThickness)(Int32 tl)(Int32 tr)(Int32 bl)(Int32 br) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiFillRect, nullptr, ("IGuiFillRect (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 colour) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawColouredSprite, nullptr, ("IGuiDrawColouredSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(MPlat.BitmapLocation loc)(Float32 blendFactor)(Int32 colour) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawLeftAligned, nullptr, ("IGuiDrawLeftAligned (Pointer hObject)(Sys.Maths.Rectf rect)(Sys.Type.IString text)(Int32 fontIndex)(Int32 fontHeight)(Int32 colour)(Float32 rightHardEdge)(Float32 rightSoftEdge) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawScaledColouredSprite, nullptr, ("IGuiDrawScaledColouredSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(MPlat.BitmapLocation loc)(Float32 blendFactor)(Int32 colour)(Float32 scaleFactor) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawSprite, nullptr, ("IGuiDrawSprite (Pointer hObject)(Sys.Maths.Vec2 pixelPos)(Int32 alignmentFlags)(MPlat.BitmapLocation loc) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiStretchSprite, nullptr, ("IGuiStretchSprite (Pointer hObject)(Sys.Maths.Rectf rect)(MPlat.BitmapLocation loc) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawClippedText, nullptr, ("IGuiDrawClippedText (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour)(Sys.Maths.Rectf clipRect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawTextWithCaret, nullptr, ("IGuiDrawTextWithCaret (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour)(Sys.Maths.Rectf clipRect)(Int32 caretPos) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiDrawText, nullptr, ("IGuiDrawText (Pointer hObject)(Sys.Maths.Rectf rect)(Int32 alignmentFlags)(Sys.Type.IString text)(Int32 fontIndex)(Int32 colour) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiEvalTextSpan, nullptr, ("IGuiEvalTextSpan (Pointer hObject)(Sys.Type.IString text)(Int32 fontIndex)(Int32 fontHeight)(Sys.Maths.Vec2 pixelSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiGetFontDescription, nullptr, ("IGuiGetFontDescription (Pointer hObject)(Int32 fontIndex)(Sys.Type.IStringBuilder familyName)(MHost.Graphics.FontDesc fd) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiGetNumberOfFonts, nullptr, ("IGuiGetNumberOfFonts (Pointer hObject) -> (Int32 numberOfFonts)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiGetScreenSpan, nullptr, ("IGuiGetScreenSpan (Pointer hObject)(Sys.Maths.Vec2 span) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiGetCursorPos, nullptr, ("IGuiGetCursorPos (Pointer hObject)(Sys.Maths.Vec2 pos) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiSetGuiShaders, nullptr, ("IGuiSetGuiShaders (Pointer hObject)(Sys.Type.IString pixelShaderFilename) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiSetScissorRect, nullptr, ("IGuiSetScissorRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIGuiClearScissorRect, nullptr, ("IGuiClearScissorRect (Pointer hObject) -> "), __FILE__, __LINE__);
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
	void NativeMHostIEngineGetNextMouseDelta(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* delta;
		_offset += sizeof(delta);
		ReadInput(delta, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetNextMouseDelta(*delta);
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
		Seconds frameDT = _pObject->YieldForSystemMessages(sleepMS);
		_offset += sizeof(frameDT);
		WriteOutput(frameDT, _sf, -_offset);
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
	void NativeMHostIEngineRunMPlatScript(NativeCallEnvironment& _nce)
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
		_pObject->RunMPlatScript(scriptName);
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
	void NativeMHostIEngineCaptureMouse(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CaptureMouse();
	}
	void NativeMHostIEngineReleaseMouse(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ReleaseMouse();
	}
	void NativeMHostIEngineCursorPosition(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* cursorPosition;
		_offset += sizeof(cursorPosition);
		ReadInput(cursorPosition, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CursorPosition(*cursorPosition);
	}
	void NativeMHostIEngineSetOverlayToggleKey(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vkeyCode;
		_offset += sizeof(vkeyCode);
		ReadInput(vkeyCode, _sf, -_offset);

		MHost::IEngine* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOverlayToggleKey(vkeyCode);
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

namespace MHost
{
	void AddNativeCalls_MHostIEngine(Rococo::Script::IPublicScriptSystem& ss, MHost::IEngine* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.Native");
		ss.AddNativeCall(ns, NativeGetHandleForMHostEngine, _nceContext, ("GetHandleForIEngine0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEnginePollKeyState, nullptr, ("IEnginePollKeyState (Pointer hObject)(MHost.OS.KeyState keys) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineGetNextMouseDelta, nullptr, ("IEngineGetNextMouseDelta (Pointer hObject)(Sys.Maths.Vec2 delta) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineGetNextMouseEvent, nullptr, ("IEngineGetNextMouseEvent (Pointer hObject)(MHost.OS.MouseEvent me) -> (Bool wasPopped)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineGetNextKeyboardEvent, nullptr, ("IEngineGetNextKeyboardEvent (Pointer hObject)(MHost.OS.KeyboardEvent ke) -> (Bool wasPopped)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineRender, nullptr, ("IEngineRender (Pointer hObject)(MHost.GuiPopulator populator) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineIsRunning, nullptr, ("IEngineIsRunning (Pointer hObject) -> (Bool isRunning)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineShutdown, nullptr, ("IEngineShutdown (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineYieldForSystemMessages, nullptr, ("IEngineYieldForSystemMessages (Pointer hObject)(Int32 sleepMS) -> (Sys.SI.Seconds frameDT)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineTryGetSpriteSpec, nullptr, ("IEngineTryGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(MPlat.BitmapLocation loc) -> (Bool isSuccessful)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineGetSpriteSpec, nullptr, ("IEngineGetSpriteSpec (Pointer hObject)(Sys.Type.IString resourceName)(MPlat.BitmapLocation loc) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineRunMPlatScript, nullptr, ("IEngineRunMPlatScript (Pointer hObject)(Sys.Type.IString scriptName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineSetNextScript, nullptr, ("IEngineSetNextScript (Pointer hObject)(Sys.Type.IString scriptName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineCaptureMouse, nullptr, ("IEngineCaptureMouse (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineReleaseMouse, nullptr, ("IEngineReleaseMouse (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineCursorPosition, nullptr, ("IEngineCursorPosition (Pointer hObject)(Sys.Maths.Vec2 cursorPosition) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIEngineSetOverlayToggleKey, nullptr, ("IEngineSetOverlayToggleKey (Pointer hObject)(Int32 vkeyCode) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for MHost::IDictionaryStream 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeMHostIDictionaryStreamAddBool(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBool(name, value);
	}
	void NativeMHostIDictionaryStreamAddI32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddI32(name, value);
	}
	void NativeMHostIDictionaryStreamAddI64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddI64(name, value);
	}
	void NativeMHostIDictionaryStreamAddF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddF32(name, value);
	}
	void NativeMHostIDictionaryStreamAddF64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		double value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddF64(name, value);
	}
	void NativeMHostIDictionaryStreamAddString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _value;
		ReadInput(_value, _sf, -_offset);
		fstring value { _value->buffer, _value->length };


		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddString(name, value);
	}
	void NativeMHostIDictionaryStreamGetBool(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 value = _pObject->GetBool(name, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeMHostIDictionaryStreamGetInt32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 value = _pObject->GetInt32(name, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeMHostIDictionaryStreamGetInt64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 value = _pObject->GetInt64(name, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeMHostIDictionaryStreamGetFloat32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->GetFloat32(name, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeMHostIDictionaryStreamGetFloat64(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		double defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		double value = _pObject->GetFloat64(name, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeMHostIDictionaryStreamAppendString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		_offset += sizeof(IString*);
		IString* _defaultString;
		ReadInput(_defaultString, _sf, -_offset);
		fstring defaultString { _defaultString->buffer, _defaultString->length };


		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendString(name, defaultString, _sbPopulator);
	}
	void NativeMHostIDictionaryStreamClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeMHostIDictionaryStreamProhibitOverwrite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ProhibitOverwrite();
	}
	void NativeMHostIDictionaryStreamLoadFrom(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadFrom(pingPath);
	}
	void NativeMHostIDictionaryStreamSaveTo(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		MHost::IDictionaryStream* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SaveTo(pingPath);
	}

	void NativeGetHandleForMHostDictionaryStream(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IInstallation* nceContext = reinterpret_cast<Rococo::IInstallation*>(_nce.context);
		// Uses: MHost::IDictionaryStream* FactoryConstructMHostDictionaryStream(Rococo::IInstallation* _context);
		MHost::IDictionaryStream* pObject = FactoryConstructMHostDictionaryStream(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace MHost
{
	void AddNativeCalls_MHostIDictionaryStream(Rococo::Script::IPublicScriptSystem& ss, Rococo::IInstallation* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("MHost.Native");
		ss.AddNativeCall(ns, NativeGetHandleForMHostDictionaryStream, _nceContext, ("GetHandleForIDictionaryStream0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddBool, nullptr, ("IDictionaryStreamAddBool (Pointer hObject)(Sys.Type.IString name)(Bool value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddI32, nullptr, ("IDictionaryStreamAddI32 (Pointer hObject)(Sys.Type.IString name)(Int32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddI64, nullptr, ("IDictionaryStreamAddI64 (Pointer hObject)(Sys.Type.IString name)(Int64 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddF32, nullptr, ("IDictionaryStreamAddF32 (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddF64, nullptr, ("IDictionaryStreamAddF64 (Pointer hObject)(Sys.Type.IString name)(Float64 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAddString, nullptr, ("IDictionaryStreamAddString (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamGetBool, nullptr, ("IDictionaryStreamGetBool (Pointer hObject)(Sys.Type.IString name)(Bool defaultValue) -> (Bool value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamGetInt32, nullptr, ("IDictionaryStreamGetInt32 (Pointer hObject)(Sys.Type.IString name)(Int32 defaultValue) -> (Int32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamGetInt64, nullptr, ("IDictionaryStreamGetInt64 (Pointer hObject)(Sys.Type.IString name)(Int64 defaultValue) -> (Int64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamGetFloat32, nullptr, ("IDictionaryStreamGetFloat32 (Pointer hObject)(Sys.Type.IString name)(Float32 defaultValue) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamGetFloat64, nullptr, ("IDictionaryStreamGetFloat64 (Pointer hObject)(Sys.Type.IString name)(Float64 defaultValue) -> (Float64 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamAppendString, nullptr, ("IDictionaryStreamAppendString (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString defaultString)(Sys.Type.IStringBuilder sb) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamClear, nullptr, ("IDictionaryStreamClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamProhibitOverwrite, nullptr, ("IDictionaryStreamProhibitOverwrite (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamLoadFrom, nullptr, ("IDictionaryStreamLoadFrom (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeMHostIDictionaryStreamSaveTo, nullptr, ("IDictionaryStreamSaveTo (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
	}
}

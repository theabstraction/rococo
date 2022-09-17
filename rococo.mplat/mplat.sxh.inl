namespace Rococo
{
	bool TryParse(const Rococo::fstring& s, ELayoutAlgorithm& value)
	{
		if (s ==  "ELayoutAlgorithm_None"_fstring)
		{
			value = ELayoutAlgorithm::None;
		}
		else if (s ==  "ELayoutAlgorithm_MaximizeOnlyChild"_fstring)
		{
			value = ELayoutAlgorithm::MaximizeOnlyChild;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, ELayoutAlgorithm& value)
	{
		if (s ==  "None"_fstring)
		{
			value = ELayoutAlgorithm::None;
		}
		else if (s ==  "MaximizeOnlyChild"_fstring)
		{
			value = ELayoutAlgorithm::MaximizeOnlyChild;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(ELayoutAlgorithm value)
	{
		switch(value)
		{
			case ELayoutAlgorithm::None:
				return "None"_fstring;
			case ELayoutAlgorithm::MaximizeOnlyChild:
				return "MaximizeOnlyChild"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.ELayoutAlgorithm

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s ==  "OrientationFlags_None"_fstring)
		{
			value = OrientationFlags::None;
		}
		else if (s ==  "OrientationFlags_Heading"_fstring)
		{
			value = OrientationFlags::Heading;
		}
		else if (s ==  "OrientationFlags_Elevation"_fstring)
		{
			value = OrientationFlags::Elevation;
		}
		else if (s ==  "OrientationFlags_Tilt"_fstring)
		{
			value = OrientationFlags::Tilt;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s ==  "None"_fstring)
		{
			value = OrientationFlags::None;
		}
		else if (s ==  "Heading"_fstring)
		{
			value = OrientationFlags::Heading;
		}
		else if (s ==  "Elevation"_fstring)
		{
			value = OrientationFlags::Elevation;
		}
		else if (s ==  "Tilt"_fstring)
		{
			value = OrientationFlags::Tilt;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(OrientationFlags value)
	{
		switch(value)
		{
			case OrientationFlags::None:
				return "None"_fstring;
			case OrientationFlags::Heading:
				return "Heading"_fstring;
			case OrientationFlags::Elevation:
				return "Elevation"_fstring;
			case OrientationFlags::Tilt:
				return "Tilt"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.OrientationFlags

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, MaterialCategory& value)
	{
		if (s ==  "MaterialCategory_Rock"_fstring)
		{
			value = MaterialCategory::Rock;
		}
		else if (s ==  "MaterialCategory_Stone"_fstring)
		{
			value = MaterialCategory::Stone;
		}
		else if (s ==  "MaterialCategory_Marble"_fstring)
		{
			value = MaterialCategory::Marble;
		}
		else if (s ==  "MaterialCategory_Metal"_fstring)
		{
			value = MaterialCategory::Metal;
		}
		else if (s ==  "MaterialCategory_Wood"_fstring)
		{
			value = MaterialCategory::Wood;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, MaterialCategory& value)
	{
		if (s ==  "Rock"_fstring)
		{
			value = MaterialCategory::Rock;
		}
		else if (s ==  "Stone"_fstring)
		{
			value = MaterialCategory::Stone;
		}
		else if (s ==  "Marble"_fstring)
		{
			value = MaterialCategory::Marble;
		}
		else if (s ==  "Metal"_fstring)
		{
			value = MaterialCategory::Metal;
		}
		else if (s ==  "Wood"_fstring)
		{
			value = MaterialCategory::Wood;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(MaterialCategory value)
	{
		switch(value)
		{
			case MaterialCategory::Rock:
				return "Rock"_fstring;
			case MaterialCategory::Stone:
				return "Stone"_fstring;
			case MaterialCategory::Marble:
				return "Marble"_fstring;
			case MaterialCategory::Metal:
				return "Metal"_fstring;
			case MaterialCategory::Wood:
				return "Wood"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.MaterialCategory

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, HQFont& value)
	{
		if (s ==  "HQFont_DebuggerFont"_fstring)
		{
			value = HQFont::DebuggerFont;
		}
		else if (s ==  "HQFont_EditorFont"_fstring)
		{
			value = HQFont::EditorFont;
		}
		else if (s ==  "HQFont_TitleFont"_fstring)
		{
			value = HQFont::TitleFont;
		}
		else if (s ==  "HQFont_EmperorFont"_fstring)
		{
			value = HQFont::EmperorFont;
		}
		else if (s ==  "HQFont_InfoFont"_fstring)
		{
			value = HQFont::InfoFont;
		}
		else if (s ==  "HQFont_MenuFont"_fstring)
		{
			value = HQFont::MenuFont;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, HQFont& value)
	{
		if (s ==  "DebuggerFont"_fstring)
		{
			value = HQFont::DebuggerFont;
		}
		else if (s ==  "EditorFont"_fstring)
		{
			value = HQFont::EditorFont;
		}
		else if (s ==  "TitleFont"_fstring)
		{
			value = HQFont::TitleFont;
		}
		else if (s ==  "EmperorFont"_fstring)
		{
			value = HQFont::EmperorFont;
		}
		else if (s ==  "InfoFont"_fstring)
		{
			value = HQFont::InfoFont;
		}
		else if (s ==  "MenuFont"_fstring)
		{
			value = HQFont::MenuFont;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(HQFont value)
	{
		switch(value)
		{
			case HQFont::DebuggerFont:
				return "DebuggerFont"_fstring;
			case HQFont::EditorFont:
				return "EditorFont"_fstring;
			case HQFont::TitleFont:
				return "TitleFont"_fstring;
			case HQFont::EmperorFont:
				return "EmperorFont"_fstring;
			case HQFont::InfoFont:
				return "InfoFont"_fstring;
			case HQFont::MenuFont:
				return "MenuFont"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.HQFont

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, SampleMethod& value)
	{
		if (s ==  "SampleMethod_Point"_fstring)
		{
			value = SampleMethod::Point;
		}
		else if (s ==  "SampleMethod_Linear"_fstring)
		{
			value = SampleMethod::Linear;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, SampleMethod& value)
	{
		if (s ==  "Point"_fstring)
		{
			value = SampleMethod::Point;
		}
		else if (s ==  "Linear"_fstring)
		{
			value = SampleMethod::Linear;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(SampleMethod value)
	{
		switch(value)
		{
			case SampleMethod::Point:
				return "Point"_fstring;
			case SampleMethod::Linear:
				return "Linear"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.SampleMethod

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, SampleFilter& value)
	{
		if (s ==  "SampleFilter_Border"_fstring)
		{
			value = SampleFilter::Border;
		}
		else if (s ==  "SampleFilter_Mirror"_fstring)
		{
			value = SampleFilter::Mirror;
		}
		else if (s ==  "SampleFilter_Wrap"_fstring)
		{
			value = SampleFilter::Wrap;
		}
		else if (s ==  "SampleFilter_Clamp"_fstring)
		{
			value = SampleFilter::Clamp;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, SampleFilter& value)
	{
		if (s ==  "Border"_fstring)
		{
			value = SampleFilter::Border;
		}
		else if (s ==  "Mirror"_fstring)
		{
			value = SampleFilter::Mirror;
		}
		else if (s ==  "Wrap"_fstring)
		{
			value = SampleFilter::Wrap;
		}
		else if (s ==  "Clamp"_fstring)
		{
			value = SampleFilter::Clamp;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(SampleFilter value)
	{
		switch(value)
		{
			case SampleFilter::Border:
				return "Border"_fstring;
			case SampleFilter::Mirror:
				return "Mirror"_fstring;
			case SampleFilter::Wrap:
				return "Wrap"_fstring;
			case SampleFilter::Clamp:
				return "Clamp"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.SampleFilter

namespace Rococo::Graphics
{
	bool TryParse(const Rococo::fstring& s, SampleIndex& value)
	{
		if (s ==  "SampleIndex_Fonts"_fstring)
		{
			value = SampleIndex::Fonts;
		}
		else if (s ==  "SampleIndex_ShadowMap"_fstring)
		{
			value = SampleIndex::ShadowMap;
		}
		else if (s ==  "SampleIndex_EnvironmentalMap"_fstring)
		{
			value = SampleIndex::EnvironmentalMap;
		}
		else if (s ==  "SampleIndex_TextureSelector"_fstring)
		{
			value = SampleIndex::TextureSelector;
		}
		else if (s ==  "SampleIndex_Materials"_fstring)
		{
			value = SampleIndex::Materials;
		}
		else if (s ==  "SampleIndex_Sprites"_fstring)
		{
			value = SampleIndex::Sprites;
		}
		else if (s ==  "SampleIndex_HQFontGlyphs"_fstring)
		{
			value = SampleIndex::HQFontGlyphs;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, SampleIndex& value)
	{
		if (s ==  "Fonts"_fstring)
		{
			value = SampleIndex::Fonts;
		}
		else if (s ==  "ShadowMap"_fstring)
		{
			value = SampleIndex::ShadowMap;
		}
		else if (s ==  "EnvironmentalMap"_fstring)
		{
			value = SampleIndex::EnvironmentalMap;
		}
		else if (s ==  "TextureSelector"_fstring)
		{
			value = SampleIndex::TextureSelector;
		}
		else if (s ==  "Materials"_fstring)
		{
			value = SampleIndex::Materials;
		}
		else if (s ==  "Sprites"_fstring)
		{
			value = SampleIndex::Sprites;
		}
		else if (s ==  "HQFontGlyphs"_fstring)
		{
			value = SampleIndex::HQFontGlyphs;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(SampleIndex value)
	{
		switch(value)
		{
			case SampleIndex::Fonts:
				return "Fonts"_fstring;
			case SampleIndex::ShadowMap:
				return "ShadowMap"_fstring;
			case SampleIndex::EnvironmentalMap:
				return "EnvironmentalMap"_fstring;
			case SampleIndex::TextureSelector:
				return "TextureSelector"_fstring;
			case SampleIndex::Materials:
				return "Materials"_fstring;
			case SampleIndex::Sprites:
				return "Sprites"_fstring;
			case SampleIndex::HQFontGlyphs:
				return "HQFontGlyphs"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.Graphics.SampleIndex

// BennyHill generated Sexy native functions for Rococo::IPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIPaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIPaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIPaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIPaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIPaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIPaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIPaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIPaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIPaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIPaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIPaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIPaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIPaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIPaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk1, nullptr, ("IPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk2, nullptr, ("IPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge1, nullptr, ("IPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge2, nullptr, ("IPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourFont, nullptr, ("IPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneIsVisible, nullptr, ("IPaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneIsNormalized, nullptr, ("IPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetVisible, nullptr, ("IPaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneGetRect, nullptr, ("IPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetRect, nullptr, ("IPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetBkImage, nullptr, ("IPaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneAlignLeftEdges, nullptr, ("IPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneAlignRightEdges, nullptr, ("IPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneAlignTopEdges, nullptr, ("IPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneAlignBottomEdges, nullptr, ("IPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneLayoutVertically, nullptr, ("IPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetCommand, nullptr, ("IPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneSetPopulator, nullptr, ("IPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IInventoryArray 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIInventoryArrayAddPaperDoll(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPathToImage;
		ReadInput(_pingPathToImage, _sf, -_offset);
		fstring pingPathToImage { _pingPathToImage->buffer, _pingPathToImage->length };


		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPaperDoll(*rect, pingPathToImage);
	}
	void NativeRococoIInventoryArrayClearDolls(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearDolls();
	}
	void NativeRococoIInventoryArrayDollCount(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 nDolls = _pObject->DollCount();
		_offset += sizeof(nDolls);
		WriteOutput(nDolls, _sf, -_offset);
	}
	void NativeRococoIInventoryArrayGetDoll(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::Textures::BitmapLocation* bitmap;
		_offset += sizeof(bitmap);
		ReadInput(bitmap, _sf, -_offset);

		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		int32 dollIndex;
		_offset += sizeof(dollIndex);
		ReadInput(dollIndex, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isOK = _pObject->GetDoll(dollIndex, *rect, _sbPopulator, *bitmap);
		_offset += sizeof(isOK);
		WriteOutput(isOK, _sf, -_offset);
	}
	void NativeRococoIInventoryArraySetDollBitmap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::Textures::BitmapLocation* bitmap;
		_offset += sizeof(bitmap);
		ReadInput(bitmap, _sf, -_offset);

		int32 dollIndex;
		_offset += sizeof(dollIndex);
		ReadInput(dollIndex, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetDollBitmap(dollIndex, *bitmap);
	}
	void NativeRococoIInventoryArrayFlags(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 flags = _pObject->Flags(index);
		_offset += sizeof(flags);
		WriteOutput(flags, _sf, -_offset);
	}
	void NativeRococoIInventoryArrayGetIndexAt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 index = _pObject->GetIndexAt(*pos);
		_offset += sizeof(index);
		WriteOutput(index, _sf, -_offset);
	}
	void NativeRococoIInventoryArrayGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(index, *rect);
	}
	void NativeRococoIInventoryArrayId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 flags = _pObject->Id(index);
		_offset += sizeof(flags);
		WriteOutput(flags, _sf, -_offset);
	}
	void NativeRococoIInventoryArrayItemCount(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 flags = _pObject->ItemCount(index);
		_offset += sizeof(flags);
		WriteOutput(flags, _sf, -_offset);
	}
	void NativeRococoIInventoryArrayLayoutAsRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::InventoryLayoutRules* rules;
		_offset += sizeof(rules);
		ReadInput(rules, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutAsRect(*rules);
	}
	void NativeRococoIInventoryArrayNumberOfItems(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 nItems = _pObject->NumberOfItems();
		_offset += sizeof(nItems);
		WriteOutput(nItems, _sf, -_offset);
	}
	void NativeRococoIInventoryArraySetFlags(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFlags(index, flags);
	}
	void NativeRococoIInventoryArraySetId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetId(index, id);
	}
	void NativeRococoIInventoryArraySetItemCount(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 count;
		_offset += sizeof(count);
		ReadInput(count, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetItemCount(index, count);
	}
	void NativeRococoIInventoryArraySetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(index, *rect);
	}
	void NativeRococoIInventoryArraySwap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Swap(i, j);
	}
	void NativeRococoIInventoryArrayComputeSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Rococo::InventoryLayoutRules* rules;
		_offset += sizeof(rules);
		ReadInput(rules, _sf, -_offset);

		Rococo::IInventoryArray* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ComputeSpan(*rules, *span);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIInventoryArray(Rococo::Script::IPublicScriptSystem& ss, Rococo::IInventoryArray* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayAddPaperDoll, nullptr, ("IInventoryArrayAddPaperDoll (Pointer hObject)(Sys.Maths.Recti rect)(Sys.Type.IString pingPathToImage) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayClearDolls, nullptr, ("IInventoryArrayClearDolls (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayDollCount, nullptr, ("IInventoryArrayDollCount (Pointer hObject) -> (Int32 nDolls)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayGetDoll, nullptr, ("IInventoryArrayGetDoll (Pointer hObject)(Int32 dollIndex)(Sys.Maths.Recti rect)(Sys.Type.IStringBuilder sb)(MPlat.BitmapLocation bitmap) -> (Bool isOK)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySetDollBitmap, nullptr, ("IInventoryArraySetDollBitmap (Pointer hObject)(Int32 dollIndex)(MPlat.BitmapLocation bitmap) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayFlags, nullptr, ("IInventoryArrayFlags (Pointer hObject)(Int32 index) -> (Int64 flags)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayGetIndexAt, nullptr, ("IInventoryArrayGetIndexAt (Pointer hObject)(Sys.Maths.Vec2i pos) -> (Int32 index)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayGetRect, nullptr, ("IInventoryArrayGetRect (Pointer hObject)(Int32 index)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayId, nullptr, ("IInventoryArrayId (Pointer hObject)(Int32 index) -> (Int64 flags)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayItemCount, nullptr, ("IInventoryArrayItemCount (Pointer hObject)(Int32 index) -> (Int64 flags)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayLayoutAsRect, nullptr, ("IInventoryArrayLayoutAsRect (Pointer hObject)(Rococo.InventoryLayoutRules rules) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayNumberOfItems, nullptr, ("IInventoryArrayNumberOfItems (Pointer hObject) -> (Int32 nItems)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySetFlags, nullptr, ("IInventoryArraySetFlags (Pointer hObject)(Int32 index)(Int64 flags) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySetId, nullptr, ("IInventoryArraySetId (Pointer hObject)(Int32 index)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySetItemCount, nullptr, ("IInventoryArraySetItemCount (Pointer hObject)(Int32 index)(Int64 count) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySetRect, nullptr, ("IInventoryArraySetRect (Pointer hObject)(Int32 index)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArraySwap, nullptr, ("IInventoryArraySwap (Pointer hObject)(Int32 i)(Int32 j) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInventoryArrayComputeSpan, nullptr, ("IInventoryArrayComputeSpan (Pointer hObject)(Rococo.InventoryLayoutRules rules)(Sys.Maths.Vec2i span) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IContextMenu 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIContextMenuAddString(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _shortcutKey;
		ReadInput(_shortcutKey, _sf, -_offset);
		fstring shortcutKey { _shortcutKey->buffer, _shortcutKey->length };


		_offset += sizeof(IString*);
		IString* _eventName;
		ReadInput(_eventName, _sf, -_offset);
		fstring eventName { _eventName->buffer, _eventName->length };


		_offset += sizeof(IString*);
		IString* _displayName;
		ReadInput(_displayName, _sf, -_offset);
		fstring displayName { _displayName->buffer, _displayName->length };


		int32 branchId;
		_offset += sizeof(branchId);
		ReadInput(branchId, _sf, -_offset);

		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddString(branchId, displayName, eventName, shortcutKey);
	}
	void NativeRococoIContextMenuAddSubMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 parentBrancId;
		_offset += sizeof(parentBrancId);
		ReadInput(parentBrancId, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _displayName;
		ReadInput(_displayName, _sf, -_offset);
		fstring displayName { _displayName->buffer, _displayName->length };


		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 newChildBranchId = _pObject->AddSubMenu(displayName, parentBrancId);
		_offset += sizeof(newChildBranchId);
		WriteOutput(newChildBranchId, _sf, -_offset);
	}
	void NativeRococoIContextMenuClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 branchId;
		_offset += sizeof(branchId);
		ReadInput(branchId, _sf, -_offset);

		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear(branchId);
	}
	void NativeRococoIContextMenuSetNextBackColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		int32 normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetNextBackColour(normal, hilight);
	}
	void NativeRococoIContextMenuSetNextStringColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		int32 normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetNextStringColour(normal, hilight);
	}
	void NativeRococoIContextMenuSetPopupPoint(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		Rococo::IContextMenu* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopupPoint(*position);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIContextMenu(Rococo::Script::IPublicScriptSystem& ss, Rococo::IContextMenu* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIContextMenuAddString, nullptr, ("IContextMenuAddString (Pointer hObject)(Int32 branchId)(Sys.Type.IString displayName)(Sys.Type.IString eventName)(Sys.Type.IString shortcutKey) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuAddSubMenu, nullptr, ("IContextMenuAddSubMenu (Pointer hObject)(Sys.Type.IString displayName)(Int32 parentBrancId) -> (Int32 newChildBranchId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuClear, nullptr, ("IContextMenuClear (Pointer hObject)(Int32 branchId) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuSetNextBackColour, nullptr, ("IContextMenuSetNextBackColour (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuSetNextStringColour, nullptr, ("IContextMenuSetNextStringColour (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuSetPopupPoint, nullptr, ("IContextMenuSetPopupPoint (Pointer hObject)(Sys.Maths.Vec2i position) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Entities::IRigBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoEntitiesIRigBuilderClearBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearBuilder();
	}
	void NativeRococoEntitiesIRigBuilderClearPoses(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearPoses();
	}
	void NativeRococoEntitiesIRigBuilderClearSkeletons(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearSkeletons();
	}
	void NativeRococoEntitiesIRigBuilderAddBone(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBone(name);
	}
	void NativeRococoEntitiesIRigBuilderAddBoneX(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Degrees rZ;
		_offset += sizeof(rZ);
		ReadInput(rZ, _sf, -_offset);

		Degrees rY;
		_offset += sizeof(rY);
		ReadInput(rY, _sf, -_offset);

		Degrees rX;
		_offset += sizeof(rX);
		ReadInput(rX, _sf, -_offset);

		float dz;
		_offset += sizeof(dz);
		ReadInput(dz, _sf, -_offset);

		float dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		float dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _parent;
		ReadInput(_parent, _sf, -_offset);
		fstring parent { _parent->buffer, _parent->length };


		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBoneX(name, parent, length, dx, dy, dz, rX, rY, rZ);
	}
	void NativeRococoEntitiesIRigBuilderSetLength(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetLength(name, length);
	}
	void NativeRococoEntitiesIRigBuilderSetScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float sz;
		_offset += sizeof(sz);
		ReadInput(sz, _sf, -_offset);

		float sy;
		_offset += sizeof(sy);
		ReadInput(sy, _sf, -_offset);

		float sx;
		_offset += sizeof(sx);
		ReadInput(sx, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScale(name, sx, sy, sz);
	}
	void NativeRococoEntitiesIRigBuilderSetVec3OffsetFromParent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* positionOffset;
		_offset += sizeof(positionOffset);
		ReadInput(positionOffset, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVec3OffsetFromParent(name, *positionOffset);
	}
	void NativeRococoEntitiesIRigBuilderSetOffsetFromParent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float dz;
		_offset += sizeof(dz);
		ReadInput(dz, _sf, -_offset);

		float dy;
		_offset += sizeof(dy);
		ReadInput(dy, _sf, -_offset);

		float dx;
		_offset += sizeof(dx);
		ReadInput(dx, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOffsetFromParent(name, dx, dy, dz);
	}
	void NativeRococoEntitiesIRigBuilderSetQuatFromParent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 validateQuat;
		_offset += sizeof(validateQuat);
		ReadInput(validateQuat, _sf, -_offset);

		Quat* quatFromParent;
		_offset += sizeof(quatFromParent);
		ReadInput(quatFromParent, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetQuatFromParent(name, *quatFromParent, validateQuat);
	}
	void NativeRococoEntitiesIRigBuilderSetRotationFromParent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Degrees rZ;
		_offset += sizeof(rZ);
		ReadInput(rZ, _sf, -_offset);

		Degrees rY;
		_offset += sizeof(rY);
		ReadInput(rY, _sf, -_offset);

		Degrees rX;
		_offset += sizeof(rX);
		ReadInput(rX, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRotationFromParent(name, rX, rY, rZ);
	}
	void NativeRococoEntitiesIRigBuilderSetParentOfChild(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _ofChild;
		ReadInput(_ofChild, _sf, -_offset);
		fstring ofChild { _ofChild->buffer, _ofChild->length };


		_offset += sizeof(IString*);
		IString* _parent;
		ReadInput(_parent, _sf, -_offset);
		fstring parent { _parent->buffer, _parent->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetParentOfChild(parent, ofChild);
	}
	void NativeRococoEntitiesIRigBuilderCommitToSkeleton(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_SKELETON id = _pObject->CommitToSkeleton(name);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoEntitiesIRigBuilderCommitToPose(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Entities::IRigBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_POSE id = _pObject->CommitToPose(name);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}

	void NativeGetHandleForRococoEntitiesRigBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IRigs* nceContext = reinterpret_cast<Rococo::Entities::IRigs*>(_nce.context);
		// Uses: Rococo::Entities::IRigBuilder* FactoryConstructRococoEntitiesRigBuilder(Rococo::Entities::IRigs* _context);
		Rococo::Entities::IRigBuilder* pObject = FactoryConstructRococoEntitiesRigBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Entities
{
	void AddNativeCalls_RococoEntitiesIRigBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Entities::IRigs* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Entities.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesRigBuilder, _nceContext, ("GetHandleForIRigBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderClearBuilder, nullptr, ("IRigBuilderClearBuilder (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderClearPoses, nullptr, ("IRigBuilderClearPoses (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderClearSkeletons, nullptr, ("IRigBuilderClearSkeletons (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderAddBone, nullptr, ("IRigBuilderAddBone (Pointer hObject)(Sys.Type.IString name) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderAddBoneX, nullptr, ("IRigBuilderAddBoneX (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString parent)(Sys.SI.Metres length)(Float32 dx)(Float32 dy)(Float32 dz)(Sys.Maths.Degrees rX)(Sys.Maths.Degrees rY)(Sys.Maths.Degrees rZ) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetLength, nullptr, ("IRigBuilderSetLength (Pointer hObject)(Sys.Type.IString name)(Sys.SI.Metres length) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetScale, nullptr, ("IRigBuilderSetScale (Pointer hObject)(Sys.Type.IString name)(Float32 sx)(Float32 sy)(Float32 sz) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetVec3OffsetFromParent, nullptr, ("IRigBuilderSetVec3OffsetFromParent (Pointer hObject)(Sys.Type.IString name)(Sys.Maths.Vec3 positionOffset) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetOffsetFromParent, nullptr, ("IRigBuilderSetOffsetFromParent (Pointer hObject)(Sys.Type.IString name)(Float32 dx)(Float32 dy)(Float32 dz) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetQuatFromParent, nullptr, ("IRigBuilderSetQuatFromParent (Pointer hObject)(Sys.Type.IString name)(Sys.Maths.Quat quatFromParent)(Bool validateQuat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetRotationFromParent, nullptr, ("IRigBuilderSetRotationFromParent (Pointer hObject)(Sys.Type.IString name)(Sys.Maths.Degrees rX)(Sys.Maths.Degrees rY)(Sys.Maths.Degrees rZ) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderSetParentOfChild, nullptr, ("IRigBuilderSetParentOfChild (Pointer hObject)(Sys.Type.IString parent)(Sys.Type.IString ofChild) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderCommitToSkeleton, nullptr, ("IRigBuilderCommitToSkeleton (Pointer hObject)(Sys.Type.IString name) -> (Int64 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIRigBuilderCommitToPose, nullptr, ("IRigBuilderCommitToPose (Pointer hObject)(Sys.Type.IString name) -> (Int64 id)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IContextMenuPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIContextMenuPaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIContextMenuPaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIContextMenuPaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIContextMenuPaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIContextMenuPaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIContextMenuPaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIContextMenuPaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIContextMenuPaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIContextMenuPaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIContextMenuPaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIContextMenuPaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIContextMenuPaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIContextMenuPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIContextMenuPaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIContextMenuPaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIContextMenuPaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIContextMenuPaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIContextMenuPaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIContextMenuPaneNoOperation(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IContextMenuPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->NoOperation();
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIContextMenuPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IContextMenuPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetColourBk1, nullptr, ("IContextMenuPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetColourBk2, nullptr, ("IContextMenuPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetColourEdge1, nullptr, ("IContextMenuPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetColourEdge2, nullptr, ("IContextMenuPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetColourFont, nullptr, ("IContextMenuPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneIsVisible, nullptr, ("IContextMenuPaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneIsNormalized, nullptr, ("IContextMenuPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetVisible, nullptr, ("IContextMenuPaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneGetRect, nullptr, ("IContextMenuPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetRect, nullptr, ("IContextMenuPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetBkImage, nullptr, ("IContextMenuPaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneAlignLeftEdges, nullptr, ("IContextMenuPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneAlignRightEdges, nullptr, ("IContextMenuPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneAlignTopEdges, nullptr, ("IContextMenuPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneAlignBottomEdges, nullptr, ("IContextMenuPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneLayoutVertically, nullptr, ("IContextMenuPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetCommand, nullptr, ("IContextMenuPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneSetPopulator, nullptr, ("IContextMenuPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIContextMenuPaneNoOperation, nullptr, ("IContextMenuPaneNoOperation (Pointer hObject) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Audio::IAudio 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoAudioIAudioSetMusic(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _musicFile;
		ReadInput(_musicFile, _sf, -_offset);
		fstring musicFile { _musicFile->buffer, _musicFile->length };


		Rococo::Audio::IAudio* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMusic(musicFile);
	}

	void NativeGetHandleForRococoAudioGetAudio(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Audio::IAudio* nceContext = reinterpret_cast<Rococo::Audio::IAudio*>(_nce.context);
		// Uses: Rococo::Audio::IAudio* FactoryConstructRococoAudioGetAudio(Rococo::Audio::IAudio* _context);
		Rococo::Audio::IAudio* pObject = FactoryConstructRococoAudioGetAudio(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Audio
{
	void AddNativeCalls_RococoAudioIAudio(Rococo::Script::IPublicScriptSystem& ss, Rococo::Audio::IAudio* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Audio.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoAudioGetAudio, _nceContext, ("GetHandleForIAudio0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoAudioIAudioSetMusic, nullptr, ("IAudioSetMusic (Pointer hObject)(Sys.Type.IString musicFile) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IEnumListPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIEnumListPaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIEnumListPaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIEnumListPaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIEnumListPaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIEnumListPaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIEnumListPaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIEnumListPaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIEnumListPaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIEnumListPaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIEnumListPaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIEnumListPaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIEnumListPaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIEnumListPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIEnumListPaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIEnumListPaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIEnumListPaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIEnumListPaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIEnumListPaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIEnumListPaneAddEnumCategory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddEnumCategory(key, value);
	}
	void NativeRococoIEnumListPaneSetArrowColours(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb disabled;
		_offset += sizeof(disabled);
		ReadInput(disabled, _sf, -_offset);

		RGBAb hilighted;
		_offset += sizeof(hilighted);
		ReadInput(hilighted, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetArrowColours(normal, hilighted, disabled);
	}
	void NativeRococoIEnumListPaneSetFocusColours(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb col2;
		_offset += sizeof(col2);
		ReadInput(col2, _sf, -_offset);

		RGBAb col1;
		_offset += sizeof(col1);
		ReadInput(col1, _sf, -_offset);

		Rococo::IEnumListPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFocusColours(col1, col2);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIEnumListPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IEnumListPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetColourBk1, nullptr, ("IEnumListPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetColourBk2, nullptr, ("IEnumListPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetColourEdge1, nullptr, ("IEnumListPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetColourEdge2, nullptr, ("IEnumListPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetColourFont, nullptr, ("IEnumListPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneIsVisible, nullptr, ("IEnumListPaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneIsNormalized, nullptr, ("IEnumListPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetVisible, nullptr, ("IEnumListPaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneGetRect, nullptr, ("IEnumListPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetRect, nullptr, ("IEnumListPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetBkImage, nullptr, ("IEnumListPaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneAlignLeftEdges, nullptr, ("IEnumListPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneAlignRightEdges, nullptr, ("IEnumListPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneAlignTopEdges, nullptr, ("IEnumListPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneAlignBottomEdges, nullptr, ("IEnumListPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneLayoutVertically, nullptr, ("IEnumListPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetCommand, nullptr, ("IEnumListPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetPopulator, nullptr, ("IEnumListPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneAddEnumCategory, nullptr, ("IEnumListPaneAddEnumCategory (Pointer hObject)(Sys.Type.IString key)(Int32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetArrowColours, nullptr, ("IEnumListPaneSetArrowColours (Pointer hObject)(Int32 normal)(Int32 hilighted)(Int32 disabled) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIEnumListPaneSetFocusColours, nullptr, ("IEnumListPaneSetFocusColours (Pointer hObject)(Int32 col1)(Int32 col2) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IArrayBox 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIArrayBoxSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIArrayBoxSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIArrayBoxSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIArrayBoxSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIArrayBoxSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIArrayBoxIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIArrayBoxIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIArrayBoxSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIArrayBoxGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIArrayBoxSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIArrayBoxSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIArrayBoxAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIArrayBoxAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIArrayBoxAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIArrayBoxAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIArrayBoxLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIArrayBoxSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIArrayBoxSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIArrayBoxSetFocusColours(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb col2;
		_offset += sizeof(col2);
		ReadInput(col2, _sf, -_offset);

		RGBAb col1;
		_offset += sizeof(col1);
		ReadInput(col1, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFocusColours(col1, col2);
	}
	void NativeRococoIArrayBoxSetLineBorders(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 bottom;
		_offset += sizeof(bottom);
		ReadInput(bottom, _sf, -_offset);

		int32 right;
		_offset += sizeof(right);
		ReadInput(right, _sf, -_offset);

		int32 top;
		_offset += sizeof(top);
		ReadInput(top, _sf, -_offset);

		int32 left;
		_offset += sizeof(left);
		ReadInput(left, _sf, -_offset);

		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetLineBorders(left, top, right, bottom);
	}
	void NativeRococoIArrayBoxSetItemSelectEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _eventText;
		ReadInput(_eventText, _sf, -_offset);
		fstring eventText { _eventText->buffer, _eventText->length };


		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetItemSelectEvent(eventText);
	}
	void NativeRococoIArrayBoxSetScrollToItemEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _eventText;
		ReadInput(_eventText, _sf, -_offset);
		fstring eventText { _eventText->buffer, _eventText->length };


		Rococo::IArrayBox* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScrollToItemEvent(eventText);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIArrayBox(Rococo::Script::IPublicScriptSystem& ss, Rococo::IArrayBox* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetColourBk1, nullptr, ("IArrayBoxSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetColourBk2, nullptr, ("IArrayBoxSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetColourEdge1, nullptr, ("IArrayBoxSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetColourEdge2, nullptr, ("IArrayBoxSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetColourFont, nullptr, ("IArrayBoxSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxIsVisible, nullptr, ("IArrayBoxIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxIsNormalized, nullptr, ("IArrayBoxIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetVisible, nullptr, ("IArrayBoxSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxGetRect, nullptr, ("IArrayBoxGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetRect, nullptr, ("IArrayBoxSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetBkImage, nullptr, ("IArrayBoxSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxAlignLeftEdges, nullptr, ("IArrayBoxAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxAlignRightEdges, nullptr, ("IArrayBoxAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxAlignTopEdges, nullptr, ("IArrayBoxAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxAlignBottomEdges, nullptr, ("IArrayBoxAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxLayoutVertically, nullptr, ("IArrayBoxLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetCommand, nullptr, ("IArrayBoxSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetPopulator, nullptr, ("IArrayBoxSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetFocusColours, nullptr, ("IArrayBoxSetFocusColours (Pointer hObject)(Int32 col1)(Int32 col2) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetLineBorders, nullptr, ("IArrayBoxSetLineBorders (Pointer hObject)(Int32 left)(Int32 top)(Int32 right)(Int32 bottom) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetItemSelectEvent, nullptr, ("IArrayBoxSetItemSelectEvent (Pointer hObject)(Sys.Type.IString eventText) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArrayBoxSetScrollToItemEvent, nullptr, ("IArrayBoxSetScrollToItemEvent (Pointer hObject)(Sys.Type.IString eventText) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IPaneContainer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIPaneContainerSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIPaneContainerSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIPaneContainerSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIPaneContainerSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIPaneContainerSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIPaneContainerIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIPaneContainerIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIPaneContainerSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIPaneContainerGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIPaneContainerSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIPaneContainerSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIPaneContainerAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneContainerAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneContainerAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneContainerAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIPaneContainerLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIPaneContainerSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIPaneContainerSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIPaneContainerAddArrayBox(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _populatorEvent;
		ReadInput(_populatorEvent, _sf, -_offset);
		fstring populatorEvent { _populatorEvent->buffer, _populatorEvent->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IArrayBox* box = _pObject->AddArrayBox(fontIndex, populatorEvent, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _boxStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IArrayBox"), ("ProxyIArrayBox"), _nce.ss);
		CReflectedClass* _sxybox = _nce.ss.Represent(_boxStruct, box);
		WriteOutput(&_sxybox->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddContainer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPaneContainer* container = _pObject->AddContainer(*rect);
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IPaneContainer"), ("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddFrame(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IFramePane* frame = _pObject->AddFrame(*rect);
		_offset += sizeof(CReflectedClass*);
		auto& _frameStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IFramePane"), ("ProxyIFramePane"), _nce.ss);
		CReflectedClass* _sxyframe = _nce.ss.Represent(_frameStruct, frame);
		WriteOutput(&_sxyframe->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddTabContainer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		int32 tabHeight;
		_offset += sizeof(tabHeight);
		ReadInput(tabHeight, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ITabContainer* container = _pObject->AddTabContainer(tabHeight, fontIndex, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ITabContainer"), ("ProxyITabContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddLabel(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ILabelPane* label = _pObject->AddLabel(fontIndex, text, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _labelStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ILabelPane"), ("ProxyILabelPane"), _nce.ss);
		CReflectedClass* _sxylabel = _nce.ss.Represent(_labelStruct, label);
		WriteOutput(&_sxylabel->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddSlider(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ISlider* slider = _pObject->AddSlider(fontIndex, text, *rect, minValue, maxValue);
		_offset += sizeof(CReflectedClass*);
		auto& _sliderStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ISlider"), ("ProxyISlider"), _nce.ss);
		CReflectedClass* _sxyslider = _nce.ss.Represent(_sliderStruct, slider);
		WriteOutput(&_sxyslider->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddScroller(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVertical;
		_offset += sizeof(isVertical);
		ReadInput(isVertical, _sf, -_offset);

		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IScroller* scroller = _pObject->AddScroller(key, *rect, isVertical);
		_offset += sizeof(CReflectedClass*);
		auto& _scrollerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IScroller"), ("ProxyIScroller"), _nce.ss);
		CReflectedClass* _sxyscroller = _nce.ss.Represent(_scrollerStruct, scroller);
		WriteOutput(&_sxyscroller->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddTextOutput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ITextOutputPane* textBox = _pObject->AddTextOutput(fontIndex, key, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _textBoxStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ITextOutputPane"), ("ProxyITextOutputPane"), _nce.ss);
		CReflectedClass* _sxytextBox = _nce.ss.Represent(_textBoxStruct, textBox);
		WriteOutput(&_sxytextBox->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddRadioButton(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _value;
		ReadInput(_value, _sf, -_offset);
		fstring value { _value->buffer, _value->length };


		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IRadioButton* radio = _pObject->AddRadioButton(fontIndex, text, key, value, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _radioStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IRadioButton"), ("ProxyIRadioButton"), _nce.ss);
		CReflectedClass* _sxyradio = _nce.ss.Represent(_radioStruct, radio);
		WriteOutput(&_sxyradio->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddContextMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IContextMenuPane* menu = _pObject->AddContextMenu(key, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _menuStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IContextMenuPane"), ("ProxyIContextMenuPane"), _nce.ss);
		CReflectedClass* _sxymenu = _nce.ss.Represent(_menuStruct, menu);
		WriteOutput(&_sxymenu->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerAddEnumList(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _populateEvent;
		ReadInput(_populateEvent, _sf, -_offset);
		fstring populateEvent { _populateEvent->buffer, _populateEvent->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IEnumListPane* enumList = _pObject->AddEnumList(fontIndex, populateEvent, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _enumListStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IEnumListPane"), ("ProxyIEnumListPane"), _nce.ss);
		CReflectedClass* _sxyenumList = _nce.ss.Represent(_enumListStruct, enumList);
		WriteOutput(&_sxyenumList->header.pVTables[0], _sf, -_offset);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIPaneContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPaneContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourBk1, nullptr, ("IPaneContainerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourBk2, nullptr, ("IPaneContainerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourEdge1, nullptr, ("IPaneContainerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourEdge2, nullptr, ("IPaneContainerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourFont, nullptr, ("IPaneContainerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerIsVisible, nullptr, ("IPaneContainerIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerIsNormalized, nullptr, ("IPaneContainerIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetVisible, nullptr, ("IPaneContainerSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerGetRect, nullptr, ("IPaneContainerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetRect, nullptr, ("IPaneContainerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetBkImage, nullptr, ("IPaneContainerSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignLeftEdges, nullptr, ("IPaneContainerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignRightEdges, nullptr, ("IPaneContainerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignTopEdges, nullptr, ("IPaneContainerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignBottomEdges, nullptr, ("IPaneContainerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerLayoutVertically, nullptr, ("IPaneContainerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetCommand, nullptr, ("IPaneContainerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetPopulator, nullptr, ("IPaneContainerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddArrayBox, nullptr, ("IPaneContainerAddArrayBox (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString populatorEvent)(Sys.Maths.Recti rect) -> (Rococo.IArrayBox box)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddContainer, nullptr, ("IPaneContainerAddContainer (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IPaneContainer container)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddFrame, nullptr, ("IPaneContainerAddFrame (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IFramePane frame)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTabContainer, nullptr, ("IPaneContainerAddTabContainer (Pointer hObject)(Int32 tabHeight)(Int32 fontIndex)(Sys.Maths.Recti rect) -> (Rococo.ITabContainer container)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddLabel, nullptr, ("IPaneContainerAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddSlider, nullptr, ("IPaneContainerAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddScroller, nullptr, ("IPaneContainerAddScroller (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect)(Bool isVertical) -> (Rococo.IScroller scroller)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTextOutput, nullptr, ("IPaneContainerAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddRadioButton, nullptr, ("IPaneContainerAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddContextMenu, nullptr, ("IPaneContainerAddContextMenu (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.IContextMenuPane menu)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddEnumList, nullptr, ("IPaneContainerAddEnumList (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString populateEvent)(Sys.Maths.Recti rect) -> (Rococo.IEnumListPane enumList)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::ITabContainer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoITabContainerSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoITabContainerSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoITabContainerSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoITabContainerSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoITabContainerSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoITabContainerIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoITabContainerIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoITabContainerSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoITabContainerGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoITabContainerSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoITabContainerSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoITabContainerAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITabContainerAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITabContainerAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITabContainerAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITabContainerLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoITabContainerSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoITabContainerSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoITabContainerAddTab(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _panelText;
		ReadInput(_panelText, _sf, -_offset);
		fstring panelText { _panelText->buffer, _panelText->length };


		_offset += sizeof(IString*);
		IString* _caption;
		ReadInput(_caption, _sf, -_offset);
		fstring caption { _caption->buffer, _caption->length };


		int32 tabWidth;
		_offset += sizeof(tabWidth);
		ReadInput(tabWidth, _sf, -_offset);

		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTab(tabWidth, caption, panelText);
	}
	void NativeRococoITabContainerSetTabPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		Rococo::ITabContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTabPopulator(populatorName);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoITabContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITabContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourBk1, nullptr, ("ITabContainerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourBk2, nullptr, ("ITabContainerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourEdge1, nullptr, ("ITabContainerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourEdge2, nullptr, ("ITabContainerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourFont, nullptr, ("ITabContainerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerIsVisible, nullptr, ("ITabContainerIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerIsNormalized, nullptr, ("ITabContainerIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetVisible, nullptr, ("ITabContainerSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerGetRect, nullptr, ("ITabContainerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetRect, nullptr, ("ITabContainerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetBkImage, nullptr, ("ITabContainerSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignLeftEdges, nullptr, ("ITabContainerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignRightEdges, nullptr, ("ITabContainerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignTopEdges, nullptr, ("ITabContainerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignBottomEdges, nullptr, ("ITabContainerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerLayoutVertically, nullptr, ("ITabContainerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetCommand, nullptr, ("ITabContainerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetPopulator, nullptr, ("ITabContainerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerAddTab, nullptr, ("ITabContainerAddTab (Pointer hObject)(Int32 tabWidth)(Sys.Type.IString caption)(Sys.Type.IString panelText) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITabContainerSetTabPopulator, nullptr, ("ITabContainerSetTabPopulator (Pointer hObject)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IFramePane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIFramePaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIFramePaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIFramePaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIFramePaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIFramePaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIFramePaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIFramePaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIFramePaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIFramePaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIFramePaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIFramePaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIFramePaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIFramePaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIFramePaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIFramePaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIFramePaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIFramePaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIFramePaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIFramePaneAddArrayBox(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _populatorEvent;
		ReadInput(_populatorEvent, _sf, -_offset);
		fstring populatorEvent { _populatorEvent->buffer, _populatorEvent->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IArrayBox* box = _pObject->AddArrayBox(fontIndex, populatorEvent, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _boxStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IArrayBox"), ("ProxyIArrayBox"), _nce.ss);
		CReflectedClass* _sxybox = _nce.ss.Represent(_boxStruct, box);
		WriteOutput(&_sxybox->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddContainer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPaneContainer* container = _pObject->AddContainer(*rect);
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IPaneContainer"), ("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddFrame(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IFramePane* frame = _pObject->AddFrame(*rect);
		_offset += sizeof(CReflectedClass*);
		auto& _frameStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IFramePane"), ("ProxyIFramePane"), _nce.ss);
		CReflectedClass* _sxyframe = _nce.ss.Represent(_frameStruct, frame);
		WriteOutput(&_sxyframe->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddTabContainer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		int32 tabHeight;
		_offset += sizeof(tabHeight);
		ReadInput(tabHeight, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ITabContainer* container = _pObject->AddTabContainer(tabHeight, fontIndex, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ITabContainer"), ("ProxyITabContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddLabel(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ILabelPane* label = _pObject->AddLabel(fontIndex, text, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _labelStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ILabelPane"), ("ProxyILabelPane"), _nce.ss);
		CReflectedClass* _sxylabel = _nce.ss.Represent(_labelStruct, label);
		WriteOutput(&_sxylabel->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddSlider(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ISlider* slider = _pObject->AddSlider(fontIndex, text, *rect, minValue, maxValue);
		_offset += sizeof(CReflectedClass*);
		auto& _sliderStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ISlider"), ("ProxyISlider"), _nce.ss);
		CReflectedClass* _sxyslider = _nce.ss.Represent(_sliderStruct, slider);
		WriteOutput(&_sxyslider->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddScroller(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVertical;
		_offset += sizeof(isVertical);
		ReadInput(isVertical, _sf, -_offset);

		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IScroller* scroller = _pObject->AddScroller(key, *rect, isVertical);
		_offset += sizeof(CReflectedClass*);
		auto& _scrollerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IScroller"), ("ProxyIScroller"), _nce.ss);
		CReflectedClass* _sxyscroller = _nce.ss.Represent(_scrollerStruct, scroller);
		WriteOutput(&_sxyscroller->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddTextOutput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::ITextOutputPane* textBox = _pObject->AddTextOutput(fontIndex, key, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _textBoxStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("ITextOutputPane"), ("ProxyITextOutputPane"), _nce.ss);
		CReflectedClass* _sxytextBox = _nce.ss.Represent(_textBoxStruct, textBox);
		WriteOutput(&_sxytextBox->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddRadioButton(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _value;
		ReadInput(_value, _sf, -_offset);
		fstring value { _value->buffer, _value->length };


		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IRadioButton* radio = _pObject->AddRadioButton(fontIndex, text, key, value, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _radioStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IRadioButton"), ("ProxyIRadioButton"), _nce.ss);
		CReflectedClass* _sxyradio = _nce.ss.Represent(_radioStruct, radio);
		WriteOutput(&_sxyradio->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddContextMenu(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IContextMenuPane* menu = _pObject->AddContextMenu(key, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _menuStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IContextMenuPane"), ("ProxyIContextMenuPane"), _nce.ss);
		CReflectedClass* _sxymenu = _nce.ss.Represent(_menuStruct, menu);
		WriteOutput(&_sxymenu->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneAddEnumList(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _populateEvent;
		ReadInput(_populateEvent, _sf, -_offset);
		fstring populateEvent { _populateEvent->buffer, _populateEvent->length };


		int32 fontIndex;
		_offset += sizeof(fontIndex);
		ReadInput(fontIndex, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IEnumListPane* enumList = _pObject->AddEnumList(fontIndex, populateEvent, *rect);
		_offset += sizeof(CReflectedClass*);
		auto& _enumListStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IEnumListPane"), ("ProxyIEnumListPane"), _nce.ss);
		CReflectedClass* _sxyenumList = _nce.ss.Represent(_enumListStruct, enumList);
		WriteOutput(&_sxyenumList->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneGetChildRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetChildRect(*rect);
	}
	void NativeRococoIFramePaneContainer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPaneContainer* container = _pObject->Container();
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IPaneContainer"), ("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoIFramePaneSetLayoutAlgorithm(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ELayoutAlgorithm layout;
		_offset += sizeof(layout);
		ReadInput(layout, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetLayoutAlgorithm(layout);
	}
	void NativeRococoIFramePaneSetMinMaxSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 maxDY;
		_offset += sizeof(maxDY);
		ReadInput(maxDY, _sf, -_offset);

		int32 maxDX;
		_offset += sizeof(maxDX);
		ReadInput(maxDX, _sf, -_offset);

		int32 minDY;
		_offset += sizeof(minDY);
		ReadInput(minDY, _sf, -_offset);

		int32 minDX;
		_offset += sizeof(minDX);
		ReadInput(minDX, _sf, -_offset);

		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMinMaxSpan(minDX, minDY, maxDX, maxDY);
	}
	void NativeRococoIFramePaneSetCaption(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _caption;
		ReadInput(_caption, _sf, -_offset);
		fstring caption { _caption->buffer, _caption->length };


		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCaption(caption);
	}
	void NativeRococoIFramePaneSetCaptionEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _eventId;
		ReadInput(_eventId, _sf, -_offset);
		fstring eventId { _eventId->buffer, _eventId->length };


		Rococo::IFramePane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCaptionEvent(eventId);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIFramePane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IFramePane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourBk1, nullptr, ("IFramePaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourBk2, nullptr, ("IFramePaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourEdge1, nullptr, ("IFramePaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourEdge2, nullptr, ("IFramePaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourFont, nullptr, ("IFramePaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneIsVisible, nullptr, ("IFramePaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneIsNormalized, nullptr, ("IFramePaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetVisible, nullptr, ("IFramePaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneGetRect, nullptr, ("IFramePaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetRect, nullptr, ("IFramePaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetBkImage, nullptr, ("IFramePaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignLeftEdges, nullptr, ("IFramePaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignRightEdges, nullptr, ("IFramePaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignTopEdges, nullptr, ("IFramePaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignBottomEdges, nullptr, ("IFramePaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneLayoutVertically, nullptr, ("IFramePaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetCommand, nullptr, ("IFramePaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetPopulator, nullptr, ("IFramePaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddArrayBox, nullptr, ("IFramePaneAddArrayBox (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString populatorEvent)(Sys.Maths.Recti rect) -> (Rococo.IArrayBox box)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddContainer, nullptr, ("IFramePaneAddContainer (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IPaneContainer container)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddFrame, nullptr, ("IFramePaneAddFrame (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IFramePane frame)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddTabContainer, nullptr, ("IFramePaneAddTabContainer (Pointer hObject)(Int32 tabHeight)(Int32 fontIndex)(Sys.Maths.Recti rect) -> (Rococo.ITabContainer container)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddLabel, nullptr, ("IFramePaneAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddSlider, nullptr, ("IFramePaneAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddScroller, nullptr, ("IFramePaneAddScroller (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect)(Bool isVertical) -> (Rococo.IScroller scroller)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddTextOutput, nullptr, ("IFramePaneAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddRadioButton, nullptr, ("IFramePaneAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddContextMenu, nullptr, ("IFramePaneAddContextMenu (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.IContextMenuPane menu)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddEnumList, nullptr, ("IFramePaneAddEnumList (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString populateEvent)(Sys.Maths.Recti rect) -> (Rococo.IEnumListPane enumList)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneGetChildRect, nullptr, ("IFramePaneGetChildRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneContainer, nullptr, ("IFramePaneContainer (Pointer hObject) -> (Rococo.IPaneContainer container)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetLayoutAlgorithm, nullptr, ("IFramePaneSetLayoutAlgorithm (Pointer hObject)(Int32 layout) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetMinMaxSpan, nullptr, ("IFramePaneSetMinMaxSpan (Pointer hObject)(Int32 minDX)(Int32 minDY)(Int32 maxDX)(Int32 maxDY) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetCaption, nullptr, ("IFramePaneSetCaption (Pointer hObject)(Sys.Type.IString caption) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetCaptionEvent, nullptr, ("IFramePaneSetCaptionEvent (Pointer hObject)(Sys.Type.IString eventId) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IRadioButton 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIRadioButtonSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIRadioButtonSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIRadioButtonSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIRadioButtonSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIRadioButtonSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIRadioButtonIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIRadioButtonIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIRadioButtonSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIRadioButtonGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIRadioButtonSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIRadioButtonSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIRadioButtonAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIRadioButtonAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIRadioButtonAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIRadioButtonAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIRadioButtonLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIRadioButtonSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIRadioButtonSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoIRadioButtonSetAlignment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 paddingY;
		_offset += sizeof(paddingY);
		ReadInput(paddingY, _sf, -_offset);

		int32 paddingX;
		_offset += sizeof(paddingX);
		ReadInput(paddingX, _sf, -_offset);

		int32 vert;
		_offset += sizeof(vert);
		ReadInput(vert, _sf, -_offset);

		int32 horz;
		_offset += sizeof(horz);
		ReadInput(horz, _sf, -_offset);

		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAlignment(horz, vert, paddingX, paddingY);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIRadioButton(Rococo::Script::IPublicScriptSystem& ss, Rococo::IRadioButton* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourBk1, nullptr, ("IRadioButtonSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourBk2, nullptr, ("IRadioButtonSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourEdge1, nullptr, ("IRadioButtonSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourEdge2, nullptr, ("IRadioButtonSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourFont, nullptr, ("IRadioButtonSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonIsVisible, nullptr, ("IRadioButtonIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonIsNormalized, nullptr, ("IRadioButtonIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetVisible, nullptr, ("IRadioButtonSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonGetRect, nullptr, ("IRadioButtonGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetRect, nullptr, ("IRadioButtonSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetBkImage, nullptr, ("IRadioButtonSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignLeftEdges, nullptr, ("IRadioButtonAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignRightEdges, nullptr, ("IRadioButtonAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignTopEdges, nullptr, ("IRadioButtonAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignBottomEdges, nullptr, ("IRadioButtonAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonLayoutVertically, nullptr, ("IRadioButtonLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetCommand, nullptr, ("IRadioButtonSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetPopulator, nullptr, ("IRadioButtonSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetAlignment, nullptr, ("IRadioButtonSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::ILabelPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoILabelPaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoILabelPaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoILabelPaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoILabelPaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoILabelPaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoILabelPaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoILabelPaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoILabelPaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoILabelPaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoILabelPaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoILabelPaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoILabelPaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoILabelPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoILabelPaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoILabelPaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoILabelPaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoILabelPaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoILabelPaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoILabelPaneSetAlignment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 paddingY;
		_offset += sizeof(paddingY);
		ReadInput(paddingY, _sf, -_offset);

		int32 paddingX;
		_offset += sizeof(paddingX);
		ReadInput(paddingX, _sf, -_offset);

		int32 vert;
		_offset += sizeof(vert);
		ReadInput(vert, _sf, -_offset);

		int32 horz;
		_offset += sizeof(horz);
		ReadInput(horz, _sf, -_offset);

		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAlignment(horz, vert, paddingX, paddingY);
	}
	void NativeRococoILabelPaneSetEnableEvent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb grey2;
		_offset += sizeof(grey2);
		ReadInput(grey2, _sf, -_offset);

		RGBAb grey1;
		_offset += sizeof(grey1);
		ReadInput(grey1, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _enablerEventName;
		ReadInput(_enablerEventName, _sf, -_offset);
		fstring enablerEventName { _enablerEventName->buffer, _enablerEventName->length };


		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetEnableEvent(enablerEventName, grey1, grey2);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoILabelPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ILabelPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourBk1, nullptr, ("ILabelPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourBk2, nullptr, ("ILabelPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourEdge1, nullptr, ("ILabelPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourEdge2, nullptr, ("ILabelPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourFont, nullptr, ("ILabelPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneIsVisible, nullptr, ("ILabelPaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneIsNormalized, nullptr, ("ILabelPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetVisible, nullptr, ("ILabelPaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneGetRect, nullptr, ("ILabelPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetRect, nullptr, ("ILabelPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetBkImage, nullptr, ("ILabelPaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignLeftEdges, nullptr, ("ILabelPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignRightEdges, nullptr, ("ILabelPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignTopEdges, nullptr, ("ILabelPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignBottomEdges, nullptr, ("ILabelPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneLayoutVertically, nullptr, ("ILabelPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetCommand, nullptr, ("ILabelPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetPopulator, nullptr, ("ILabelPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetAlignment, nullptr, ("ILabelPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetEnableEvent, nullptr, ("ILabelPaneSetEnableEvent (Pointer hObject)(Sys.Type.IString enablerEventName)(Int32 grey1)(Int32 grey2) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::ISlider 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoISliderSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoISliderSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoISliderSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoISliderSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoISliderSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoISliderIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoISliderIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoISliderSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoISliderGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoISliderSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoISliderSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoISliderAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoISliderAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoISliderAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoISliderAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoISliderLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoISliderSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoISliderSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoISlider(Rococo::Script::IPublicScriptSystem& ss, Rococo::ISlider* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoISliderSetColourBk1, nullptr, ("ISliderSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetColourBk2, nullptr, ("ISliderSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetColourEdge1, nullptr, ("ISliderSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetColourEdge2, nullptr, ("ISliderSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetColourFont, nullptr, ("ISliderSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderIsVisible, nullptr, ("ISliderIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderIsNormalized, nullptr, ("ISliderIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetVisible, nullptr, ("ISliderSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderGetRect, nullptr, ("ISliderGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetRect, nullptr, ("ISliderSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetBkImage, nullptr, ("ISliderSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderAlignLeftEdges, nullptr, ("ISliderAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderAlignRightEdges, nullptr, ("ISliderAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderAlignTopEdges, nullptr, ("ISliderAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderAlignBottomEdges, nullptr, ("ISliderAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderLayoutVertically, nullptr, ("ISliderLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetCommand, nullptr, ("ISliderSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoISliderSetPopulator, nullptr, ("ISliderSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IScroller 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIScrollerSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoIScrollerSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoIScrollerSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoIScrollerSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoIScrollerSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoIScrollerIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoIScrollerIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoIScrollerSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoIScrollerGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoIScrollerSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoIScrollerSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoIScrollerAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIScrollerAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIScrollerAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIScrollerAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoIScrollerLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoIScrollerSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoIScrollerSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::IScroller* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoIScroller(Rococo::Script::IPublicScriptSystem& ss, Rococo::IScroller* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourBk1, nullptr, ("IScrollerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourBk2, nullptr, ("IScrollerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourEdge1, nullptr, ("IScrollerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourEdge2, nullptr, ("IScrollerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourFont, nullptr, ("IScrollerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerIsVisible, nullptr, ("IScrollerIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerIsNormalized, nullptr, ("IScrollerIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetVisible, nullptr, ("IScrollerSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerGetRect, nullptr, ("IScrollerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetRect, nullptr, ("IScrollerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetBkImage, nullptr, ("IScrollerSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignLeftEdges, nullptr, ("IScrollerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignRightEdges, nullptr, ("IScrollerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignTopEdges, nullptr, ("IScrollerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignBottomEdges, nullptr, ("IScrollerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerLayoutVertically, nullptr, ("IScrollerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetCommand, nullptr, ("IScrollerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIScrollerSetPopulator, nullptr, ("IScrollerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::ITextOutputPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoITextOutputPaneSetColourBk1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk1(normal, hilight);
	}
	void NativeRococoITextOutputPaneSetColourBk2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourBk2(normal, hilight);
	}
	void NativeRococoITextOutputPaneSetColourEdge1(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge1(normal, hilight);
	}
	void NativeRococoITextOutputPaneSetColourEdge2(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourEdge2(normal, hilight);
	}
	void NativeRococoITextOutputPaneSetColourFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb hilight;
		_offset += sizeof(hilight);
		ReadInput(hilight, _sf, -_offset);

		RGBAb normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetColourFont(normal, hilight);
	}
	void NativeRococoITextOutputPaneIsVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isVisible = _pObject->IsVisible();
		_offset += sizeof(isVisible);
		WriteOutput(isVisible, _sf, -_offset);
	}
	void NativeRococoITextOutputPaneIsNormalized(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isNormalized = _pObject->IsNormalized();
		_offset += sizeof(isNormalized);
		WriteOutput(isNormalized, _sf, -_offset);
	}
	void NativeRococoITextOutputPaneSetVisible(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isVisible;
		_offset += sizeof(isVisible);
		ReadInput(isVisible, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVisible(isVisible);
	}
	void NativeRococoITextOutputPaneGetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRect(*rect);
	}
	void NativeRococoITextOutputPaneSetRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRect* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRect(*rect);
	}
	void NativeRococoITextOutputPaneSetBkImage(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBkImage(pingPath);
	}
	void NativeRococoITextOutputPaneAlignLeftEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITextOutputPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITextOutputPaneAlignTopEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignTopEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITextOutputPaneAlignBottomEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 borderPixels;
		_offset += sizeof(borderPixels);
		ReadInput(borderPixels, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignBottomEdges(borderPixels, preserveSpan);
	}
	void NativeRococoITextOutputPaneLayoutVertically(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vertSpacing;
		_offset += sizeof(vertSpacing);
		ReadInput(vertSpacing, _sf, -_offset);

		int32 vertBorder;
		_offset += sizeof(vertBorder);
		ReadInput(vertBorder, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LayoutVertically(vertBorder, vertSpacing);
	}
	void NativeRococoITextOutputPaneSetCommand(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		boolean32 deferAction;
		_offset += sizeof(deferAction);
		ReadInput(deferAction, _sf, -_offset);

		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetCommand(stateIndex, deferAction, text);
	}
	void NativeRococoITextOutputPaneSetPopulator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _populatorName;
		ReadInput(_populatorName, _sf, -_offset);
		fstring populatorName { _populatorName->buffer, _populatorName->length };


		int32 stateIndex;
		_offset += sizeof(stateIndex);
		ReadInput(stateIndex, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPopulator(stateIndex, populatorName);
	}
	void NativeRococoITextOutputPaneSetAlignment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 paddingY;
		_offset += sizeof(paddingY);
		ReadInput(paddingY, _sf, -_offset);

		int32 paddingX;
		_offset += sizeof(paddingX);
		ReadInput(paddingX, _sf, -_offset);

		int32 vert;
		_offset += sizeof(vert);
		ReadInput(vert, _sf, -_offset);

		int32 horz;
		_offset += sizeof(horz);
		ReadInput(horz, _sf, -_offset);

		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAlignment(horz, vert, paddingX, paddingY);
	}
	void NativeRococoITextOutputPaneSetActivateKey(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetActivateKey(key);
	}

}

namespace Rococo
{
	void AddNativeCalls_RococoITextOutputPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITextOutputPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourBk1, nullptr, ("ITextOutputPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourBk2, nullptr, ("ITextOutputPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourEdge1, nullptr, ("ITextOutputPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourEdge2, nullptr, ("ITextOutputPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourFont, nullptr, ("ITextOutputPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneIsVisible, nullptr, ("ITextOutputPaneIsVisible (Pointer hObject) -> (Bool isVisible)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneIsNormalized, nullptr, ("ITextOutputPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetVisible, nullptr, ("ITextOutputPaneSetVisible (Pointer hObject)(Bool isVisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneGetRect, nullptr, ("ITextOutputPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetRect, nullptr, ("ITextOutputPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetBkImage, nullptr, ("ITextOutputPaneSetBkImage (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignLeftEdges, nullptr, ("ITextOutputPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignRightEdges, nullptr, ("ITextOutputPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignTopEdges, nullptr, ("ITextOutputPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignBottomEdges, nullptr, ("ITextOutputPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneLayoutVertically, nullptr, ("ITextOutputPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetCommand, nullptr, ("ITextOutputPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetPopulator, nullptr, ("ITextOutputPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetAlignment, nullptr, ("ITextOutputPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetActivateKey, nullptr, ("ITextOutputPaneSetActivateKey (Pointer hObject)(Sys.Type.IString key) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IPaneBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIPaneBuilderRoot(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPaneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPaneContainer* container = _pObject->Root();
		_offset += sizeof(CReflectedClass*);
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(("Rococo"),("IPaneContainer"), ("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header.pVTables[0], _sf, -_offset);
	}

	void NativeGetHandleForRococoPaneBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPaneBuilder* nceContext = reinterpret_cast<Rococo::IPaneBuilder*>(_nce.context);
		// Uses: Rococo::IPaneBuilder* FactoryConstructRococoPaneBuilder(Rococo::IPaneBuilder* _context);
		Rococo::IPaneBuilder* pObject = FactoryConstructRococoPaneBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIPaneBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPaneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoPaneBuilder, _nceContext, ("GetHandleForIPaneBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIPaneBuilderRoot, nullptr, ("IPaneBuilderRoot (Pointer hObject) -> (Rococo.IPaneContainer container)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IKeyboard 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIKeyboardClearActions(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearActions();
	}
	void NativeRococoIKeyboardGetVKeyFromName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 vkCode = _pObject->GetVKeyFromName(name);
		_offset += sizeof(vkCode);
		WriteOutput(vkCode, _sf, -_offset);
	}
	void NativeRococoIKeyboardSetKeyName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 vkeyCode;
		_offset += sizeof(vkeyCode);
		ReadInput(vkeyCode, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetKeyName(name, vkeyCode);
	}
	void NativeRococoIKeyboardBindAction(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _actionName;
		ReadInput(_actionName, _sf, -_offset);
		fstring actionName { _actionName->buffer, _actionName->length };


		_offset += sizeof(IString*);
		IString* _keyName;
		ReadInput(_keyName, _sf, -_offset);
		fstring keyName { _keyName->buffer, _keyName->length };


		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->BindAction(keyName, actionName);
	}
	void NativeRococoIKeyboardSaveCppHeader(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SaveCppHeader();
	}

	void NativeGetHandleForRococoKeyboard(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IKeyboard* nceContext = reinterpret_cast<Rococo::IKeyboard*>(_nce.context);
		// Uses: Rococo::IKeyboard* FactoryConstructRococoKeyboard(Rococo::IKeyboard* _context);
		Rococo::IKeyboard* pObject = FactoryConstructRococoKeyboard(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIKeyboard(Rococo::Script::IPublicScriptSystem& ss, Rococo::IKeyboard* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoKeyboard, _nceContext, ("GetHandleForIKeyboard0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIKeyboardClearActions, nullptr, ("IKeyboardClearActions (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIKeyboardGetVKeyFromName, nullptr, ("IKeyboardGetVKeyFromName (Pointer hObject)(Sys.Type.IString name) -> (Int32 vkCode)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIKeyboardSetKeyName, nullptr, ("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 vkeyCode) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIKeyboardBindAction, nullptr, ("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIKeyboardSaveCppHeader, nullptr, ("IKeyboardSaveCppHeader (Pointer hObject) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IConfig 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIConfigInt(NativeCallEnvironment& _nce)
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


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Int(name, value);
	}
	void NativeRococoIConfigFloat(NativeCallEnvironment& _nce)
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


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Float(name, value);
	}
	void NativeRococoIConfigBool(NativeCallEnvironment& _nce)
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


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Bool(name, value);
	}
	void NativeRococoIConfigText(NativeCallEnvironment& _nce)
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


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Text(name, value);
	}
	void NativeRococoIConfigGetInt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 value = _pObject->GetInt(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeRococoIConfigGetFloat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->GetFloat(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeRococoIConfigGetBool(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 value = _pObject->GetBool(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeRococoIConfigGetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** text;
		ReadInput(text, _sf, -_offset);
		Rococo::Helpers::StringPopulator _textPopulator(_nce, text);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetText(name, _textPopulator);
	}

	void NativeGetHandleForRococoConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IConfig* nceContext = reinterpret_cast<Rococo::IConfig*>(_nce.context);
		// Uses: Rococo::IConfig* FactoryConstructRococoConfig(Rococo::IConfig* _context);
		Rococo::IConfig* pObject = FactoryConstructRococoConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIConfig(Rococo::Script::IPublicScriptSystem& ss, Rococo::IConfig* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoConfig, _nceContext, ("GetHandleForIConfig0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigInt, nullptr, ("IConfigInt (Pointer hObject)(Sys.Type.IString name)(Int32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigFloat, nullptr, ("IConfigFloat (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigBool, nullptr, ("IConfigBool (Pointer hObject)(Sys.Type.IString name)(Bool value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigText, nullptr, ("IConfigText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigGetInt, nullptr, ("IConfigGetInt (Pointer hObject)(Sys.Type.IString name) -> (Int32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigGetFloat, nullptr, ("IConfigGetFloat (Pointer hObject)(Sys.Type.IString name) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigGetBool, nullptr, ("IConfigGetBool (Pointer hObject)(Sys.Type.IString name) -> (Bool value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIConfigGetText, nullptr, ("IConfigGetText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IStringBuilder text) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ISpriteBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsISpriteBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISpriteBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsISpriteBuilderAddSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };


		Rococo::Graphics::ISpriteBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddSprite(resourceName);
	}
	void NativeRococoGraphicsISpriteBuilderAddEachSpriteInDirectory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _directoryName;
		ReadInput(_directoryName, _sf, -_offset);
		fstring directoryName { _directoryName->buffer, _directoryName->length };


		Rococo::Graphics::ISpriteBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddEachSpriteInDirectory(directoryName);
	}
	void NativeRococoGraphicsISpriteBuilderLoadAllSprites(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISpriteBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadAllSprites();
	}

	void NativeGetHandleForRococoGraphicsSpriteBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISpriteBuilder* nceContext = reinterpret_cast<Rococo::Graphics::ISpriteBuilder*>(_nce.context);
		// Uses: Rococo::Graphics::ISpriteBuilder* FactoryConstructRococoGraphicsSpriteBuilder(Rococo::Graphics::ISpriteBuilder* _context);
		Rococo::Graphics::ISpriteBuilder* pObject = FactoryConstructRococoGraphicsSpriteBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsISpriteBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ISpriteBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSpriteBuilder, _nceContext, ("GetHandleForISpriteBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpriteBuilderClear, nullptr, ("ISpriteBuilderClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpriteBuilderAddSprite, nullptr, ("ISpriteBuilderAddSprite (Pointer hObject)(Sys.Type.IString resourceName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpriteBuilderAddEachSpriteInDirectory, nullptr, ("ISpriteBuilderAddEachSpriteInDirectory (Pointer hObject)(Sys.Type.IString directoryName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpriteBuilderLoadAllSprites, nullptr, ("ISpriteBuilderLoadAllSprites (Pointer hObject) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ISprites 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsISpritesTryGetId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_SPRITE id = _pObject->TryGetId(pingPath);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoGraphicsISpritesAppendPingPath(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** sb;
		ReadInput(sb, _sf, -_offset);
		Rococo::Helpers::StringPopulator _sbPopulator(_nce, sb);
		ID_SPRITE id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendPingPath(id, _sbPopulator);
	}

	void NativeGetHandleForRococoGraphicsSprites(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISprites* nceContext = reinterpret_cast<Rococo::Graphics::ISprites*>(_nce.context);
		// Uses: Rococo::Graphics::ISprites* FactoryConstructRococoGraphicsSprites(Rococo::Graphics::ISprites* _context);
		Rococo::Graphics::ISprites* pObject = FactoryConstructRococoGraphicsSprites(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsISprites(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ISprites* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSprites, _nceContext, ("GetHandleForISprites0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesTryGetId, nullptr, ("ISpritesTryGetId (Pointer hObject)(Sys.Type.IString pingPath) -> (Int64 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesAppendPingPath, nullptr, ("ISpritesAppendPingPath (Pointer hObject)(Int64 id)(Sys.Type.IStringBuilder sb) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Entities::IMobiles 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoEntitiesIMobilesLink(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Link(id);
	}
	void NativeRococoEntitiesIMobilesGetAngles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		FPSAngles* angles;
		_offset += sizeof(angles);
		ReadInput(angles, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetAngles(id, *angles);
	}
	void NativeRococoEntitiesIMobilesSetAngles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		FPSAngles* angles;
		_offset += sizeof(angles);
		ReadInput(angles, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAngles(id, *angles);
	}

	void NativeGetHandleForRococoEntitiesMobiles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IMobiles* nceContext = reinterpret_cast<Rococo::Entities::IMobiles*>(_nce.context);
		// Uses: Rococo::Entities::IMobiles* FactoryConstructRococoEntitiesMobiles(Rococo::Entities::IMobiles* _context);
		Rococo::Entities::IMobiles* pObject = FactoryConstructRococoEntitiesMobiles(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Entities
{
	void AddNativeCalls_RococoEntitiesIMobiles(Rococo::Script::IPublicScriptSystem& ss, Rococo::Entities::IMobiles* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Entities.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesMobiles, _nceContext, ("GetHandleForIMobiles0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesLink, nullptr, ("IMobilesLink (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesGetAngles, nullptr, ("IMobilesGetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesSetAngles, nullptr, ("IMobilesSetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Entities::IParticleSystem 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoEntitiesIParticleSystemApplySpectrum(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ApplySpectrum(id);
	}
	void NativeRococoEntitiesIParticleSystemClearSpectrum(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearSpectrum();
	}
	void NativeRococoEntitiesIParticleSystemSetSpectrum(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float relativeLifeTime;
		_offset += sizeof(relativeLifeTime);
		ReadInput(relativeLifeTime, _sf, -_offset);

		RGBA* colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSpectrum(*colour, relativeLifeTime);
	}
	void NativeRococoEntitiesIParticleSystemAddDust(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Metres maxHeight;
		_offset += sizeof(maxHeight);
		ReadInput(maxHeight, _sf, -_offset);

		Metres minHeight;
		_offset += sizeof(minHeight);
		ReadInput(minHeight, _sf, -_offset);

		Metres range;
		_offset += sizeof(range);
		ReadInput(range, _sf, -_offset);

		Metres meanParticleSize;
		_offset += sizeof(meanParticleSize);
		ReadInput(meanParticleSize, _sf, -_offset);

		int32 particles;
		_offset += sizeof(particles);
		ReadInput(particles, _sf, -_offset);

		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddDust(particles, meanParticleSize, range, minHeight, maxHeight, colour, id);
	}
	void NativeRococoEntitiesIParticleSystemAddVerticalFlame(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		FlameDef* flameDef;
		_offset += sizeof(flameDef);
		ReadInput(flameDef, _sf, -_offset);

		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddVerticalFlame(*flameDef, id);
	}
	void NativeRococoEntitiesIParticleSystemClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoEntitiesIParticleSystemSnuff(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IParticleSystem* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Snuff(id);
	}

	void NativeGetHandleForRococoEntitiesParticleSystem(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Entities::IParticleSystem* FactoryConstructRococoEntitiesParticleSystem(Rococo::Platform* _context);
		Rococo::Entities::IParticleSystem* pObject = FactoryConstructRococoEntitiesParticleSystem(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Entities
{
	void AddNativeCalls_RococoEntitiesIParticleSystem(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Entities.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesParticleSystem, _nceContext, ("GetHandleForIParticleSystem0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemApplySpectrum, nullptr, ("IParticleSystemApplySpectrum (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemClearSpectrum, nullptr, ("IParticleSystemClearSpectrum (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemSetSpectrum, nullptr, ("IParticleSystemSetSpectrum (Pointer hObject)(Sys.Maths.Vec4 colour)(Float32 relativeLifeTime) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemAddDust, nullptr, ("IParticleSystemAddDust (Pointer hObject)(Int32 particles)(Sys.SI.Metres meanParticleSize)(Sys.SI.Metres range)(Sys.SI.Metres minHeight)(Sys.SI.Metres maxHeight)(Int32 colour)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemAddVerticalFlame, nullptr, ("IParticleSystemAddVerticalFlame (Pointer hObject)(Rococo.FlameDef flameDef)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemClear, nullptr, ("IParticleSystemClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemSnuff, nullptr, ("IParticleSystemSnuff (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ICamera 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsICameraClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsICameraSetRHProjection(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float far;
		_offset += sizeof(far);
		ReadInput(far, _sf, -_offset);

		float near;
		_offset += sizeof(near);
		ReadInput(near, _sf, -_offset);

		Degrees fov;
		_offset += sizeof(fov);
		ReadInput(fov, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRHProjection(fov, near, far);
	}
	void NativeRococoGraphicsICameraSetPosition(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPosition(*position);
	}
	void NativeRococoGraphicsICameraSetOrientation(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOrientation(*orientation);
	}
	void NativeRococoGraphicsICameraFollowEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FollowEntity(id);
	}
	void NativeRococoGraphicsICameraMoveToEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveToEntity(id);
	}
	void NativeRococoGraphicsICameraOrientateWithEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->OrientateWithEntity(id, flags);
	}
	void NativeRococoGraphicsICameraOrientateToEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->OrientateToEntity(id, flags);
	}
	void NativeRococoGraphicsICameraGetPosition(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetPosition(*position);
	}
	void NativeRococoGraphicsICameraGetOrientation(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetOrientation(*orientation);
	}
	void NativeRococoGraphicsICameraGetWorld(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* world;
		_offset += sizeof(world);
		ReadInput(world, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetWorld(*world);
	}
	void NativeRococoGraphicsICameraGetWorldAndProj(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* worldAndProj;
		_offset += sizeof(worldAndProj);
		ReadInput(worldAndProj, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetWorldAndProj(*worldAndProj);
	}
	void NativeRococoGraphicsICameraGetProjection(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* proj;
		_offset += sizeof(proj);
		ReadInput(proj, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetProjection(*proj);
	}
	void NativeRococoGraphicsICameraAspectRatio(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float widthOverHeight = _pObject->AspectRatio();
		_offset += sizeof(widthOverHeight);
		WriteOutput(widthOverHeight, _sf, -_offset);
	}

	void NativeGetHandleForRococoGraphicsCamera(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ICamera* nceContext = reinterpret_cast<Rococo::Graphics::ICamera*>(_nce.context);
		// Uses: Rococo::Graphics::ICamera* FactoryConstructRococoGraphicsCamera(Rococo::Graphics::ICamera* _context);
		Rococo::Graphics::ICamera* pObject = FactoryConstructRococoGraphicsCamera(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsICamera(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ICamera* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsCamera, _nceContext, ("GetHandleForICamera0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraClear, nullptr, ("ICameraClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetRHProjection, nullptr, ("ICameraSetRHProjection (Pointer hObject)(Sys.Maths.Degrees fov)(Float32 near)(Float32 far) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetPosition, nullptr, ("ICameraSetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetOrientation, nullptr, ("ICameraSetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraFollowEntity, nullptr, ("ICameraFollowEntity (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraMoveToEntity, nullptr, ("ICameraMoveToEntity (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateWithEntity, nullptr, ("ICameraOrientateWithEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateToEntity, nullptr, ("ICameraOrientateToEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetPosition, nullptr, ("ICameraGetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetOrientation, nullptr, ("ICameraGetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorld, nullptr, ("ICameraGetWorld (Pointer hObject)(Sys.Maths.Matrix4x4 world) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorldAndProj, nullptr, ("ICameraGetWorldAndProj (Pointer hObject)(Sys.Maths.Matrix4x4 worldAndProj) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetProjection, nullptr, ("ICameraGetProjection (Pointer hObject)(Sys.Maths.Matrix4x4 proj) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraAspectRatio, nullptr, ("ICameraAspectRatio (Pointer hObject) -> (Float32 widthOverHeight)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ISceneBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsISceneBuilderAddStatics(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddStatics(entityId);
	}
	void NativeRococoGraphicsISceneBuilderAddDebugObject(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddDebugObject(entityId);
	}
	void NativeRococoGraphicsISceneBuilderAddDynamicObject(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddDynamicObject(entityId);
	}
	void NativeRococoGraphicsISceneBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsISceneBuilderClearLights(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearLights();
	}
	void NativeRococoGraphicsISceneBuilderSetClearColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float alpha;
		_offset += sizeof(alpha);
		ReadInput(alpha, _sf, -_offset);

		float blue;
		_offset += sizeof(blue);
		ReadInput(blue, _sf, -_offset);

		float green;
		_offset += sizeof(green);
		ReadInput(green, _sf, -_offset);

		float red;
		_offset += sizeof(red);
		ReadInput(red, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetClearColour(red, green, blue, alpha);
	}
	void NativeRococoGraphicsISceneBuilderSetLight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		LightSpec* light;
		_offset += sizeof(light);
		ReadInput(light, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetLight(*light, index);
	}
	void NativeRococoGraphicsISceneBuilderSetSkyBox(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_CUBE_TEXTURE cubeId;
		_offset += sizeof(cubeId);
		ReadInput(cubeId, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSkyBox(cubeId);
	}

	void NativeGetHandleForRococoGraphicsSceneBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISceneBuilder* nceContext = reinterpret_cast<Rococo::Graphics::ISceneBuilder*>(_nce.context);
		// Uses: Rococo::Graphics::ISceneBuilder* FactoryConstructRococoGraphicsSceneBuilder(Rococo::Graphics::ISceneBuilder* _context);
		Rococo::Graphics::ISceneBuilder* pObject = FactoryConstructRococoGraphicsSceneBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsISceneBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ISceneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSceneBuilder, _nceContext, ("GetHandleForISceneBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderAddStatics, nullptr, ("ISceneBuilderAddStatics (Pointer hObject)(Int64 entityId) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderAddDebugObject, nullptr, ("ISceneBuilderAddDebugObject (Pointer hObject)(Int64 entityId) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderAddDynamicObject, nullptr, ("ISceneBuilderAddDynamicObject (Pointer hObject)(Int64 entityId) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderClear, nullptr, ("ISceneBuilderClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderClearLights, nullptr, ("ISceneBuilderClearLights (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetClearColour, nullptr, ("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue)(Float32 alpha) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetLight, nullptr, ("ISceneBuilderSetLight (Pointer hObject)(Rococo.LightSpec light)(Int32 index) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetSkyBox, nullptr, ("ISceneBuilderSetSkyBox (Pointer hObject)(Int64 cubeId) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IMessaging 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIMessagingLog(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _message;
		ReadInput(_message, _sf, -_offset);
		fstring message { _message->buffer, _message->length };


		Rococo::Graphics::IMessaging* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Log(message);
	}

	void NativeGetHandleForRococoGraphicsMessaging(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::IMessaging* FactoryConstructRococoGraphicsMessaging(Rococo::Platform* _context);
		Rococo::Graphics::IMessaging* pObject = FactoryConstructRococoGraphicsMessaging(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIMessaging(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsMessaging, _nceContext, ("GetHandleForIMessaging0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMessagingLog, nullptr, ("IMessagingLog (Pointer hObject)(Sys.Type.IString message) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Entities::IInstances 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoEntitiesIInstancesAddBody(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY parentId;
		_offset += sizeof(parentId);
		ReadInput(parentId, _sf, -_offset);

		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _modelName;
		ReadInput(_modelName, _sf, -_offset);
		fstring modelName { _modelName->buffer, _modelName->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddBody(modelName, *model, *scale, parentId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesAddGhost(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY parentId;
		_offset += sizeof(parentId);
		ReadInput(parentId, _sf, -_offset);

		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddGhost(*model, parentId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesAddSkeleton(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _skeleton;
		ReadInput(_skeleton, _sf, -_offset);
		fstring skeleton { _skeleton->buffer, _skeleton->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddSkeleton(skeleton, *model);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesAddAnimationFrame(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 loop;
		_offset += sizeof(loop);
		ReadInput(loop, _sf, -_offset);

		Seconds duration;
		_offset += sizeof(duration);
		ReadInput(duration, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _frameName;
		ReadInput(_frameName, _sf, -_offset);
		fstring frameName { _frameName->buffer, _frameName->length };


		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddAnimationFrame(id, frameName, duration, loop);
	}
	void NativeRococoEntitiesIInstancesBindSkeletonToBody(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY idBody;
		_offset += sizeof(idBody);
		ReadInput(idBody, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _skeleton;
		ReadInput(_skeleton, _sf, -_offset);
		fstring skeleton { _skeleton->buffer, _skeleton->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->BindSkeletonToBody(skeleton, idBody);
	}
	void NativeRococoEntitiesIInstancesCreateCubeTexture(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _extension;
		ReadInput(_extension, _sf, -_offset);
		fstring extension { _extension->buffer, _extension->length };


		_offset += sizeof(IString*);
		IString* _folder;
		ReadInput(_folder, _sf, -_offset);
		fstring folder { _folder->buffer, _folder->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_CUBE_TEXTURE cubeId = _pObject->CreateCubeTexture(folder, extension);
		_offset += sizeof(cubeId);
		WriteOutput(cubeId, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesDelete(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Delete(id);
	}
	void NativeRococoEntitiesIInstancesLoadMaterialArray(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 txWidth;
		_offset += sizeof(txWidth);
		ReadInput(txWidth, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _folder;
		ReadInput(_folder, _sf, -_offset);
		fstring folder { _folder->buffer, _folder->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadMaterialArray(folder, txWidth);
	}
	void NativeRococoEntitiesIInstancesGetMaterialCateogry(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialId id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Graphics::MaterialCategory category = _pObject->GetMaterialCateogry(id);
		_offset += sizeof(category);
		WriteOutput(category, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesCountMaterialsInCategory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::MaterialCategory category;
		_offset += sizeof(category);
		ReadInput(category, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->CountMaterialsInCategory(category);
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesGetMaterialId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::MaterialCategory category;
		_offset += sizeof(category);
		ReadInput(category, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		MaterialId id = _pObject->GetMaterialId(category, index);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesGetMaterialDirect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		MaterialId id = _pObject->GetMaterialDirect(pingPath);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesGetRandomMaterialId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::MaterialCategory category;
		_offset += sizeof(category);
		ReadInput(category, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		MaterialId id = _pObject->GetRandomMaterialId(category);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesGetScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetScale(entityId, *scale);
	}
	void NativeRococoEntitiesIInstancesSetMaterialMacro(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPath;
		ReadInput(_pingPath, _sf, -_offset);
		fstring pingPath { _pingPath->buffer, _pingPath->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaterialMacro(pingPath);
	}
	void NativeRococoEntitiesIInstancesSetScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScale(entityId, *scale);
	}
	void NativeRococoEntitiesIInstancesTryGetModelToWorldMatrix(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 existant = _pObject->TryGetModelToWorldMatrix(entityId, *position);
		_offset += sizeof(existant);
		WriteOutput(existant, _sf, -_offset);
	}
	void NativeRococoEntitiesIInstancesClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}

	void NativeGetHandleForRococoEntitiesInstances(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Entities::IInstances* nceContext = reinterpret_cast<Rococo::Entities::IInstances*>(_nce.context);
		// Uses: Rococo::Entities::IInstances* FactoryConstructRococoEntitiesInstances(Rococo::Entities::IInstances* _context);
		Rococo::Entities::IInstances* pObject = FactoryConstructRococoEntitiesInstances(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Entities
{
	void AddNativeCalls_RococoEntitiesIInstances(Rococo::Script::IPublicScriptSystem& ss, Rococo::Entities::IInstances* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Entities.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesInstances, _nceContext, ("GetHandleForIInstances0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddBody, nullptr, ("IInstancesAddBody (Pointer hObject)(Sys.Type.IString modelName)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddGhost, nullptr, ("IInstancesAddGhost (Pointer hObject)(Sys.Maths.Matrix4x4 model)(Int64 parentId) -> (Int64 entityId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddSkeleton, nullptr, ("IInstancesAddSkeleton (Pointer hObject)(Sys.Type.IString skeleton)(Sys.Maths.Matrix4x4 model) -> (Int64 entityId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddAnimationFrame, nullptr, ("IInstancesAddAnimationFrame (Pointer hObject)(Int64 id)(Sys.Type.IString frameName)(Sys.SI.Seconds duration)(Bool loop) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesBindSkeletonToBody, nullptr, ("IInstancesBindSkeletonToBody (Pointer hObject)(Sys.Type.IString skeleton)(Int64 idBody) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesCreateCubeTexture, nullptr, ("IInstancesCreateCubeTexture (Pointer hObject)(Sys.Type.IString folder)(Sys.Type.IString extension) -> (Int64 cubeId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesDelete, nullptr, ("IInstancesDelete (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesLoadMaterialArray, nullptr, ("IInstancesLoadMaterialArray (Pointer hObject)(Sys.Type.IString folder)(Int32 txWidth) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetMaterialCateogry, nullptr, ("IInstancesGetMaterialCateogry (Pointer hObject)(Float32 id) -> (Int32 category)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesCountMaterialsInCategory, nullptr, ("IInstancesCountMaterialsInCategory (Pointer hObject)(Int32 category) -> (Int32 count)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetMaterialId, nullptr, ("IInstancesGetMaterialId (Pointer hObject)(Int32 category)(Int32 index) -> (Float32 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetMaterialDirect, nullptr, ("IInstancesGetMaterialDirect (Pointer hObject)(Sys.Type.IString pingPath) -> (Float32 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetRandomMaterialId, nullptr, ("IInstancesGetRandomMaterialId (Pointer hObject)(Int32 category) -> (Float32 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetScale, nullptr, ("IInstancesGetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesSetMaterialMacro, nullptr, ("IInstancesSetMaterialMacro (Pointer hObject)(Sys.Type.IString pingPath) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesSetScale, nullptr, ("IInstancesSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesTryGetModelToWorldMatrix, nullptr, ("IInstancesTryGetModelToWorldMatrix (Pointer hObject)(Int64 entityId)(Sys.Maths.Matrix4x4 position) -> (Bool existant)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesClear, nullptr, ("IInstancesClear (Pointer hObject) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IMeshBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIMeshBuilderAddMesh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _sourceName;
		ReadInput(_sourceName, _sf, -_offset);
		fstring sourceName { _sourceName->buffer, _sourceName->length };


		Matrix4x4* transform;
		_offset += sizeof(transform);
		ReadInput(transform, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddMesh(*transform, sourceName);
	}
	void NativeRococoGraphicsIMeshBuilderAddTriangleEx(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangleEx(*t);
	}
	void NativeRococoGraphicsIMeshBuilderAddTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ObjectVertex* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		ObjectVertex* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		ObjectVertex* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*a, *b, *c);
	}
	void NativeRococoGraphicsIMeshBuilderAddBoneWeights(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		BoneWeights* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		BoneWeights* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		BoneWeights* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBoneWeights(*a, *b, *c);
	}
	void NativeRococoGraphicsIMeshBuilderAddPhysicsHull(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Triangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPhysicsHull(*t);
	}
	void NativeRococoGraphicsIMeshBuilderBegin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _meshName;
		ReadInput(_meshName, _sf, -_offset);
		fstring meshName { _meshName->buffer, _meshName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Begin(meshName);
	}
	void NativeRococoGraphicsIMeshBuilderEnd(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 invisible;
		_offset += sizeof(invisible);
		ReadInput(invisible, _sf, -_offset);

		boolean32 preserveCopy;
		_offset += sizeof(preserveCopy);
		ReadInput(preserveCopy, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->End(preserveCopy, invisible);
	}
	void NativeRococoGraphicsIMeshBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsIMeshBuilderDelete(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Delete(fqName);
	}
	void NativeRococoGraphicsIMeshBuilderSetShadowCasting(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isActive;
		_offset += sizeof(isActive);
		ReadInput(isActive, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetShadowCasting(fqName, isActive);
	}
	void NativeRococoGraphicsIMeshBuilderSetSpecialAmbientShader(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 alphaBlending;
		_offset += sizeof(alphaBlending);
		ReadInput(alphaBlending, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _psAmbientPingPath;
		ReadInput(_psAmbientPingPath, _sf, -_offset);
		fstring psAmbientPingPath { _psAmbientPingPath->buffer, _psAmbientPingPath->length };


		_offset += sizeof(IString*);
		IString* _vsAmbientPingPath;
		ReadInput(_vsAmbientPingPath, _sf, -_offset);
		fstring vsAmbientPingPath { _vsAmbientPingPath->buffer, _vsAmbientPingPath->length };


		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSpecialAmbientShader(fqName, vsAmbientPingPath, psAmbientPingPath, alphaBlending);
	}
	void NativeRococoGraphicsIMeshBuilderSetSpecialSpotlightShader(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 alphaBlending;
		_offset += sizeof(alphaBlending);
		ReadInput(alphaBlending, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _psSpotlightPingPath;
		ReadInput(_psSpotlightPingPath, _sf, -_offset);
		fstring psSpotlightPingPath { _psSpotlightPingPath->buffer, _psSpotlightPingPath->length };


		_offset += sizeof(IString*);
		IString* _vsSpotlightPingPath;
		ReadInput(_vsSpotlightPingPath, _sf, -_offset);
		fstring vsSpotlightPingPath { _vsSpotlightPingPath->buffer, _vsSpotlightPingPath->length };


		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSpecialSpotlightShader(fqName, vsSpotlightPingPath, psSpotlightPingPath, alphaBlending);
	}
	void NativeRococoGraphicsIMeshBuilderSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Vec3* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Span(*span, fqName);
	}

	void NativeGetHandleForRococoGraphicsMeshBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IMeshBuilder* nceContext = reinterpret_cast<Rococo::Graphics::IMeshBuilder*>(_nce.context);
		// Uses: Rococo::Graphics::IMeshBuilder* FactoryConstructRococoGraphicsMeshBuilder(Rococo::Graphics::IMeshBuilder* _context);
		Rococo::Graphics::IMeshBuilder* pObject = FactoryConstructRococoGraphicsMeshBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIMeshBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IMeshBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsMeshBuilder, _nceContext, ("GetHandleForIMeshBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddMesh, nullptr, ("IMeshBuilderAddMesh (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Sys.Type.IString sourceName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddTriangleEx, nullptr, ("IMeshBuilderAddTriangleEx (Pointer hObject)(Rococo.VertexTriangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddTriangle, nullptr, ("IMeshBuilderAddTriangle (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddBoneWeights, nullptr, ("IMeshBuilderAddBoneWeights (Pointer hObject)(Rococo.BoneWeights a)(Rococo.BoneWeights b)(Rococo.BoneWeights c) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddPhysicsHull, nullptr, ("IMeshBuilderAddPhysicsHull (Pointer hObject)(Sys.Maths.Triangle t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderBegin, nullptr, ("IMeshBuilderBegin (Pointer hObject)(Sys.Type.IString meshName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderEnd, nullptr, ("IMeshBuilderEnd (Pointer hObject)(Bool preserveCopy)(Bool invisible) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderClear, nullptr, ("IMeshBuilderClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderDelete, nullptr, ("IMeshBuilderDelete (Pointer hObject)(Sys.Type.IString fqName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSetShadowCasting, nullptr, ("IMeshBuilderSetShadowCasting (Pointer hObject)(Sys.Type.IString fqName)(Bool isActive) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSetSpecialAmbientShader, nullptr, ("IMeshBuilderSetSpecialAmbientShader (Pointer hObject)(Sys.Type.IString fqName)(Sys.Type.IString vsAmbientPingPath)(Sys.Type.IString psAmbientPingPath)(Bool alphaBlending) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSetSpecialSpotlightShader, nullptr, ("IMeshBuilderSetSpecialSpotlightShader (Pointer hObject)(Sys.Type.IString fqName)(Sys.Type.IString vsSpotlightPingPath)(Sys.Type.IString psSpotlightPingPath)(Bool alphaBlending) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSpan, nullptr, ("IMeshBuilderSpan (Pointer hObject)(Sys.Maths.Vec3 span)(Sys.Type.IString fqName) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IRimTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIRimTesselatorAddPoint(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* p;
		_offset += sizeof(p);
		ReadInput(p, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPoint(*p);
	}
	void NativeRococoGraphicsIRimTesselatorAddPointXY(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPointXY(x, y);
	}
	void NativeRococoGraphicsIRimTesselatorCloseLoop(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CloseLoop();
	}
	void NativeRococoGraphicsIRimTesselatorMakeElipse(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float sy;
		_offset += sizeof(sy);
		ReadInput(sy, _sf, -_offset);

		float sx;
		_offset += sizeof(sx);
		ReadInput(sx, _sf, -_offset);

		int32 numberOfSides;
		_offset += sizeof(numberOfSides);
		ReadInput(numberOfSides, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MakeElipse(numberOfSides, sx, sy);
	}
	void NativeRococoGraphicsIRimTesselatorClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsIRimTesselatorClearFaces(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearFaces();
	}
	void NativeRococoGraphicsIRimTesselatorScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float sy;
		_offset += sizeof(sy);
		ReadInput(sy, _sf, -_offset);

		float sx;
		_offset += sizeof(sx);
		ReadInput(sx, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Scale(sx, sy);
	}
	void NativeRococoGraphicsIRimTesselatorPerimeterVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->PerimeterVertices();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeRococoGraphicsIRimTesselatorGetRimQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quad* quad;
		_offset += sizeof(quad);
		ReadInput(quad, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		float zLow;
		_offset += sizeof(zLow);
		ReadInput(zLow, _sf, -_offset);

		float zHigh;
		_offset += sizeof(zHigh);
		ReadInput(zHigh, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRimQuad(zHigh, zLow, index, *quad);
	}
	void NativeRococoGraphicsIRimTesselatorGetRimVertex(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* p;
		_offset += sizeof(p);
		ReadInput(p, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetRimVertex(index, *p);
	}
	void NativeRococoGraphicsIRimTesselatorTesselateUniform(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 nTrianglesPerFace = _pObject->TesselateUniform();
		_offset += sizeof(nTrianglesPerFace);
		WriteOutput(nTrianglesPerFace, _sf, -_offset);
	}
	void NativeRococoGraphicsIRimTesselatorGetBottomTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float z;
		_offset += sizeof(z);
		ReadInput(z, _sf, -_offset);

		Triangle2d* uv;
		_offset += sizeof(uv);
		ReadInput(uv, _sf, -_offset);

		Triangle* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBottomTriangle(index, *pos, *uv, z);
	}
	void NativeRococoGraphicsIRimTesselatorGetTopTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float z;
		_offset += sizeof(z);
		ReadInput(z, _sf, -_offset);

		Triangle2d* uv;
		_offset += sizeof(uv);
		ReadInput(uv, _sf, -_offset);

		Triangle* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetTopTriangle(index, *pos, *uv, z);
	}
	void NativeRococoGraphicsIRimTesselatorSetTransform(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* transform;
		_offset += sizeof(transform);
		ReadInput(transform, _sf, -_offset);

		Rococo::Graphics::IRimTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTransform(*transform);
	}

	void NativeGetHandleForRococoGraphicsRimTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRimTesselator* nceContext = reinterpret_cast<Rococo::Graphics::IRimTesselator*>(_nce.context);
		// Uses: Rococo::Graphics::IRimTesselator* FactoryConstructRococoGraphicsRimTesselator(Rococo::Graphics::IRimTesselator* _context);
		Rococo::Graphics::IRimTesselator* pObject = FactoryConstructRococoGraphicsRimTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIRimTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IRimTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRimTesselator, _nceContext, ("GetHandleForIRimTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPoint, nullptr, ("IRimTesselatorAddPoint (Pointer hObject)(Sys.Maths.Vec2 p) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPointXY, nullptr, ("IRimTesselatorAddPointXY (Pointer hObject)(Float32 x)(Float32 y) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorCloseLoop, nullptr, ("IRimTesselatorCloseLoop (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorMakeElipse, nullptr, ("IRimTesselatorMakeElipse (Pointer hObject)(Int32 numberOfSides)(Float32 sx)(Float32 sy) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClear, nullptr, ("IRimTesselatorClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClearFaces, nullptr, ("IRimTesselatorClearFaces (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorScale, nullptr, ("IRimTesselatorScale (Pointer hObject)(Float32 sx)(Float32 sy) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorPerimeterVertices, nullptr, ("IRimTesselatorPerimeterVertices (Pointer hObject) -> (Int32 count)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimQuad, nullptr, ("IRimTesselatorGetRimQuad (Pointer hObject)(Float32 zHigh)(Float32 zLow)(Int32 index)(Sys.Maths.Quadf quad) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimVertex, nullptr, ("IRimTesselatorGetRimVertex (Pointer hObject)(Int32 index)(Sys.Maths.Vec2 p) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorTesselateUniform, nullptr, ("IRimTesselatorTesselateUniform (Pointer hObject) -> (Int32 nTrianglesPerFace)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetBottomTriangle, nullptr, ("IRimTesselatorGetBottomTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetTopTriangle, nullptr, ("IRimTesselatorGetTopTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorSetTransform, nullptr, ("IRimTesselatorSetTransform (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IFieldTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIFieldTesselatorDestruct(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Destruct();
	}
	void NativeRococoGraphicsIFieldTesselatorInitByFixedCellWidth(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxCellHeight;
		_offset += sizeof(maxCellHeight);
		ReadInput(maxCellHeight, _sf, -_offset);

		float maxCellWidth;
		_offset += sizeof(maxCellWidth);
		ReadInput(maxCellWidth, _sf, -_offset);

		Quad* positions;
		_offset += sizeof(positions);
		ReadInput(positions, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->InitByFixedCellWidth(*positions, maxCellWidth, maxCellHeight);
	}
	void NativeRococoGraphicsIFieldTesselatorInitByDivisions(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 yDivs;
		_offset += sizeof(yDivs);
		ReadInput(yDivs, _sf, -_offset);

		int32 xDivs;
		_offset += sizeof(xDivs);
		ReadInput(xDivs, _sf, -_offset);

		Quad* positions;
		_offset += sizeof(positions);
		ReadInput(positions, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->InitByDivisions(*positions, xDivs, yDivs);
	}
	void NativeRococoGraphicsIFieldTesselatorSetUV(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* uvC;
		_offset += sizeof(uvC);
		ReadInput(uvC, _sf, -_offset);

		Vec2* uvA;
		_offset += sizeof(uvA);
		ReadInput(uvA, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetUV(*uvA, *uvC);
	}
	void NativeRococoGraphicsIFieldTesselatorNumberOfColumns(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 cols = _pObject->NumberOfColumns();
		_offset += sizeof(cols);
		WriteOutput(cols, _sf, -_offset);
	}
	void NativeRococoGraphicsIFieldTesselatorNumberOfRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 rows = _pObject->NumberOfRows();
		_offset += sizeof(rows);
		WriteOutput(rows, _sf, -_offset);
	}
	void NativeRococoGraphicsIFieldTesselatorGetFlatSubQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* subQuad;
		_offset += sizeof(subQuad);
		ReadInput(subQuad, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetFlatSubQuad(i, j, *subQuad);
	}
	void NativeRococoGraphicsIFieldTesselatorGetPerturbedSubQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetPerturbedSubQuad(i, j, *q);
	}
	void NativeRococoGraphicsIFieldTesselatorGetStackBondedBrick(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres cementWidth;
		_offset += sizeof(cementWidth);
		ReadInput(cementWidth, _sf, -_offset);

		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetStackBondedBrick(i, j, *q, cementWidth);
	}
	void NativeRococoGraphicsIFieldTesselatorGetStretchBondedBrick(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres extrusionBase;
		_offset += sizeof(extrusionBase);
		ReadInput(extrusionBase, _sf, -_offset);

		Metres cementWidth;
		_offset += sizeof(cementWidth);
		ReadInput(cementWidth, _sf, -_offset);

		QuadVertices* bottom;
		_offset += sizeof(bottom);
		ReadInput(bottom, _sf, -_offset);

		QuadVertices* right;
		_offset += sizeof(right);
		ReadInput(right, _sf, -_offset);

		QuadVertices* left;
		_offset += sizeof(left);
		ReadInput(left, _sf, -_offset);

		QuadVertices* top;
		_offset += sizeof(top);
		ReadInput(top, _sf, -_offset);

		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetStretchBondedBrick(i, j, *q, *top, *left, *right, *bottom, cementWidth, extrusionBase);
	}
	void NativeRococoGraphicsIFieldTesselatorGetBrickJoinRight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres cementWidth;
		_offset += sizeof(cementWidth);
		ReadInput(cementWidth, _sf, -_offset);

		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBrickJoinRight(i, j, *q, cementWidth);
	}
	void NativeRococoGraphicsIFieldTesselatorGetBrickBedTop(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres cementWidth;
		_offset += sizeof(cementWidth);
		ReadInput(cementWidth, _sf, -_offset);

		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBrickBedTop(row, *q, cementWidth);
	}
	void NativeRococoGraphicsIFieldTesselatorPerturbField(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float dH;
		_offset += sizeof(dH);
		ReadInput(dH, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PerturbField(i, j, dH);
	}
	void NativeRococoGraphicsIFieldTesselatorLevelField(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float dH;
		_offset += sizeof(dH);
		ReadInput(dH, _sf, -_offset);

		int32 j1;
		_offset += sizeof(j1);
		ReadInput(j1, _sf, -_offset);

		int32 i1;
		_offset += sizeof(i1);
		ReadInput(i1, _sf, -_offset);

		int32 j0;
		_offset += sizeof(j0);
		ReadInput(j0, _sf, -_offset);

		int32 i0;
		_offset += sizeof(i0);
		ReadInput(i0, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LevelField(i0, j0, i1, j1, dH);
	}
	void NativeRococoGraphicsIFieldTesselatorRandomizeField(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		int32 j;
		_offset += sizeof(j);
		ReadInput(j, _sf, -_offset);

		int32 i;
		_offset += sizeof(i);
		ReadInput(i, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RandomizeField(i, j, minValue, maxValue);
	}
	void NativeRococoGraphicsIFieldTesselatorGetBasis(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* transform;
		_offset += sizeof(transform);
		ReadInput(transform, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBasis(*transform);
	}

	void NativeGetHandleForRococoGraphicsFieldTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IFieldTesselator* nceContext = reinterpret_cast<Rococo::Graphics::IFieldTesselator*>(_nce.context);
		// Uses: Rococo::Graphics::IFieldTesselator* FactoryConstructRococoGraphicsFieldTesselator(Rococo::Graphics::IFieldTesselator* _context);
		Rococo::Graphics::IFieldTesselator* pObject = FactoryConstructRococoGraphicsFieldTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIFieldTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IFieldTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsFieldTesselator, _nceContext, ("GetHandleForIFieldTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorDestruct, nullptr, ("IFieldTesselatorDestruct (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByFixedCellWidth, nullptr, ("IFieldTesselatorInitByFixedCellWidth (Pointer hObject)(Sys.Maths.Quadf positions)(Float32 maxCellWidth)(Float32 maxCellHeight) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByDivisions, nullptr, ("IFieldTesselatorInitByDivisions (Pointer hObject)(Sys.Maths.Quadf positions)(Int32 xDivs)(Int32 yDivs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorSetUV, nullptr, ("IFieldTesselatorSetUV (Pointer hObject)(Sys.Maths.Vec2 uvA)(Sys.Maths.Vec2 uvC) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfColumns, nullptr, ("IFieldTesselatorNumberOfColumns (Pointer hObject) -> (Int32 cols)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfRows, nullptr, ("IFieldTesselatorNumberOfRows (Pointer hObject) -> (Int32 rows)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetFlatSubQuad, nullptr, ("IFieldTesselatorGetFlatSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices subQuad) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetPerturbedSubQuad, nullptr, ("IFieldTesselatorGetPerturbedSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStackBondedBrick, nullptr, ("IFieldTesselatorGetStackBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStretchBondedBrick, nullptr, ("IFieldTesselatorGetStretchBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Rococo.QuadVertices top)(Rococo.QuadVertices left)(Rococo.QuadVertices right)(Rococo.QuadVertices bottom)(Sys.SI.Metres cementWidth)(Sys.SI.Metres extrusionBase) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickJoinRight, nullptr, ("IFieldTesselatorGetBrickJoinRight (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickBedTop, nullptr, ("IFieldTesselatorGetBrickBedTop (Pointer hObject)(Int32 row)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorPerturbField, nullptr, ("IFieldTesselatorPerturbField (Pointer hObject)(Int32 i)(Int32 j)(Float32 dH) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorLevelField, nullptr, ("IFieldTesselatorLevelField (Pointer hObject)(Int32 i0)(Int32 j0)(Int32 i1)(Int32 j1)(Float32 dH) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorRandomizeField, nullptr, ("IFieldTesselatorRandomizeField (Pointer hObject)(Int32 i)(Int32 j)(Float32 minValue)(Float32 maxValue) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBasis, nullptr, ("IFieldTesselatorGetBasis (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IQuadStackTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIQuadStackTesselatorAddCuboid(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* rodMat;
		_offset += sizeof(rodMat);
		ReadInput(rodMat, _sf, -_offset);

		float uvScale;
		_offset += sizeof(uvScale);
		ReadInput(uvScale, _sf, -_offset);

		float thickness;
		_offset += sizeof(thickness);
		ReadInput(thickness, _sf, -_offset);

		float t1;
		_offset += sizeof(t1);
		ReadInput(t1, _sf, -_offset);

		float t0;
		_offset += sizeof(t0);
		ReadInput(t0, _sf, -_offset);

		float v1;
		_offset += sizeof(v1);
		ReadInput(v1, _sf, -_offset);

		float v0;
		_offset += sizeof(v0);
		ReadInput(v0, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddCuboid(v0, v1, t0, t1, thickness, uvScale, *rodMat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorAddCuboidAbs(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* rodMat;
		_offset += sizeof(rodMat);
		ReadInput(rodMat, _sf, -_offset);

		float uvScale;
		_offset += sizeof(uvScale);
		ReadInput(uvScale, _sf, -_offset);

		Metres thickness;
		_offset += sizeof(thickness);
		ReadInput(thickness, _sf, -_offset);

		Metres dy1;
		_offset += sizeof(dy1);
		ReadInput(dy1, _sf, -_offset);

		Metres dx1;
		_offset += sizeof(dx1);
		ReadInput(dx1, _sf, -_offset);

		Metres dy0;
		_offset += sizeof(dy0);
		ReadInput(dy0, _sf, -_offset);

		Metres dx0;
		_offset += sizeof(dx0);
		ReadInput(dx0, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddCuboidAbs(dx0, dy0, dx1, dy1, thickness, uvScale, *rodMat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsIQuadStackTesselatorClearInput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearInput();
	}
	void NativeRococoGraphicsIQuadStackTesselatorClearOutput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearOutput();
	}
	void NativeRococoGraphicsIQuadStackTesselatorCopyInputToOutput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CopyInputToOutput();
	}
	void NativeRococoGraphicsIQuadStackTesselatorDestruct(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Destruct();
	}
	void NativeRococoGraphicsIQuadStackTesselatorIntrude(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* innerMat;
		_offset += sizeof(innerMat);
		ReadInput(innerMat, _sf, -_offset);

		MaterialVertexData* rimMat;
		_offset += sizeof(rimMat);
		ReadInput(rimMat, _sf, -_offset);

		float depthUvScale;
		_offset += sizeof(depthUvScale);
		ReadInput(depthUvScale, _sf, -_offset);

		float depth;
		_offset += sizeof(depth);
		ReadInput(depth, _sf, -_offset);

		GuiRectf* window;
		_offset += sizeof(window);
		ReadInput(window, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Intrude(*window, depth, depthUvScale, *rimMat, *innerMat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveInputToOutput();
	}
	void NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInput(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveOutputToInput();
	}
	void NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithMat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveOutputToInputWithMat(*mat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithNormalDotRange(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxDot;
		_offset += sizeof(maxDot);
		ReadInput(maxDot, _sf, -_offset);

		float minDot;
		_offset += sizeof(minDot);
		ReadInput(minDot, _sf, -_offset);

		Vec3* normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveOutputToInputWithNormalDotRange(*normal, minDot, maxDot);
	}
	void NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutputWithNormalDotRange(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxDot;
		_offset += sizeof(maxDot);
		ReadInput(maxDot, _sf, -_offset);

		float minDot;
		_offset += sizeof(minDot);
		ReadInput(minDot, _sf, -_offset);

		Vec3* normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveInputToOutputWithNormalDotRange(*normal, minDot, maxDot);
	}
	void NativeRococoGraphicsIQuadStackTesselatorPushQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* material;
		_offset += sizeof(material);
		ReadInput(material, _sf, -_offset);

		QuadVertices* quad;
		_offset += sizeof(quad);
		ReadInput(quad, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PushQuad(*quad, *material);
	}
	void NativeRococoGraphicsIQuadStackTesselatorPopOutputAsTriangles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* bottomLeft;
		_offset += sizeof(bottomLeft);
		ReadInput(bottomLeft, _sf, -_offset);

		VertexTriangle* topRight;
		_offset += sizeof(topRight);
		ReadInput(topRight, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasPopped = _pObject->PopOutputAsTriangles(*topRight, *bottomLeft);
		_offset += sizeof(wasPopped);
		WriteOutput(wasPopped, _sf, -_offset);
	}
	void NativeRococoGraphicsIQuadStackTesselatorScaleEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveUVs;
		_offset += sizeof(preserveUVs);
		ReadInput(preserveUVs, _sf, -_offset);

		float high;
		_offset += sizeof(high);
		ReadInput(high, _sf, -_offset);

		float low;
		_offset += sizeof(low);
		ReadInput(low, _sf, -_offset);

		float right;
		_offset += sizeof(right);
		ReadInput(right, _sf, -_offset);

		float left;
		_offset += sizeof(left);
		ReadInput(left, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ScaleEdges(left, right, low, high, preserveUVs);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSetBasis(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* vertical;
		_offset += sizeof(vertical);
		ReadInput(vertical, _sf, -_offset);

		Vec3* normal;
		_offset += sizeof(normal);
		ReadInput(normal, _sf, -_offset);

		Vec3* tangent;
		_offset += sizeof(tangent);
		ReadInput(tangent, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBasis(*tangent, *normal, *vertical);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaterial(*mat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSetTextureRect(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTextureRect(*rect);
	}
	void NativeRococoGraphicsIQuadStackTesselatorShrink(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		GuiRectf* rect;
		_offset += sizeof(rect);
		ReadInput(rect, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Shrink(*rect);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSplitThreeColumns(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float x1;
		_offset += sizeof(x1);
		ReadInput(x1, _sf, -_offset);

		float x0;
		_offset += sizeof(x0);
		ReadInput(x0, _sf, -_offset);

		MaterialVertexData* c3;
		_offset += sizeof(c3);
		ReadInput(c3, _sf, -_offset);

		MaterialVertexData* c2;
		_offset += sizeof(c2);
		ReadInput(c2, _sf, -_offset);

		MaterialVertexData* c1;
		_offset += sizeof(c1);
		ReadInput(c1, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SplitThreeColumns(*c1, *c2, *c3, x0, x1);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSplitThreeRows(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float y1;
		_offset += sizeof(y1);
		ReadInput(y1, _sf, -_offset);

		float y0;
		_offset += sizeof(y0);
		ReadInput(y0, _sf, -_offset);

		MaterialVertexData* r3;
		_offset += sizeof(r3);
		ReadInput(r3, _sf, -_offset);

		MaterialVertexData* r2;
		_offset += sizeof(r2);
		ReadInput(r2, _sf, -_offset);

		MaterialVertexData* r1;
		_offset += sizeof(r1);
		ReadInput(r1, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SplitThreeRows(*r1, *r2, *r3, y0, y1);
	}
	void NativeRococoGraphicsIQuadStackTesselatorSplitAcrossTangent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* bottomMat;
		_offset += sizeof(bottomMat);
		ReadInput(bottomMat, _sf, -_offset);

		MaterialVertexData* topMat;
		_offset += sizeof(topMat);
		ReadInput(topMat, _sf, -_offset);

		RGBAb lowColour;
		_offset += sizeof(lowColour);
		ReadInput(lowColour, _sf, -_offset);

		RGBAb middleColour;
		_offset += sizeof(middleColour);
		ReadInput(middleColour, _sf, -_offset);

		RGBAb topColour;
		_offset += sizeof(topColour);
		ReadInput(topColour, _sf, -_offset);

		float v;
		_offset += sizeof(v);
		ReadInput(v, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SplitAcrossTangent(v, topColour, middleColour, lowColour, *topMat, *bottomMat);
	}
	void NativeRococoGraphicsIQuadStackTesselatorTileMosaic(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres roughSize;
		_offset += sizeof(roughSize);
		ReadInput(roughSize, _sf, -_offset);

		GuiRectf* uvRect;
		_offset += sizeof(uvRect);
		ReadInput(uvRect, _sf, -_offset);

		MaterialVertexData* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		MaterialVertexData* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->TileMosaic(*a, *b, *uvRect, roughSize);
	}
	void NativeRococoGraphicsIQuadStackTesselatorTranslate(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* v;
		_offset += sizeof(v);
		ReadInput(v, _sf, -_offset);

		Rococo::Graphics::IQuadStackTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Translate(*v);
	}

	void NativeGetHandleForRococoGraphicsQuadStackTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IQuadStackTesselator* nceContext = reinterpret_cast<Rococo::Graphics::IQuadStackTesselator*>(_nce.context);
		// Uses: Rococo::Graphics::IQuadStackTesselator* FactoryConstructRococoGraphicsQuadStackTesselator(Rococo::Graphics::IQuadStackTesselator* _context);
		Rococo::Graphics::IQuadStackTesselator* pObject = FactoryConstructRococoGraphicsQuadStackTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIQuadStackTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IQuadStackTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsQuadStackTesselator, _nceContext, ("GetHandleForIQuadStackTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorAddCuboid, nullptr, ("IQuadStackTesselatorAddCuboid (Pointer hObject)(Float32 v0)(Float32 v1)(Float32 t0)(Float32 t1)(Float32 thickness)(Float32 uvScale)(Rococo.MaterialVertexData rodMat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorAddCuboidAbs, nullptr, ("IQuadStackTesselatorAddCuboidAbs (Pointer hObject)(Sys.SI.Metres dx0)(Sys.SI.Metres dy0)(Sys.SI.Metres dx1)(Sys.SI.Metres dy1)(Sys.SI.Metres thickness)(Float32 uvScale)(Rococo.MaterialVertexData rodMat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClear, nullptr, ("IQuadStackTesselatorClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClearInput, nullptr, ("IQuadStackTesselatorClearInput (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClearOutput, nullptr, ("IQuadStackTesselatorClearOutput (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorCopyInputToOutput, nullptr, ("IQuadStackTesselatorCopyInputToOutput (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorDestruct, nullptr, ("IQuadStackTesselatorDestruct (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorIntrude, nullptr, ("IQuadStackTesselatorIntrude (Pointer hObject)(Sys.Maths.Rectf window)(Float32 depth)(Float32 depthUvScale)(Rococo.MaterialVertexData rimMat)(Rococo.MaterialVertexData innerMat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutput, nullptr, ("IQuadStackTesselatorMoveInputToOutput (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInput, nullptr, ("IQuadStackTesselatorMoveOutputToInput (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithMat, nullptr, ("IQuadStackTesselatorMoveOutputToInputWithMat (Pointer hObject)(Rococo.MaterialVertexData mat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithNormalDotRange, nullptr, ("IQuadStackTesselatorMoveOutputToInputWithNormalDotRange (Pointer hObject)(Sys.Maths.Vec3 normal)(Float32 minDot)(Float32 maxDot) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutputWithNormalDotRange, nullptr, ("IQuadStackTesselatorMoveInputToOutputWithNormalDotRange (Pointer hObject)(Sys.Maths.Vec3 normal)(Float32 minDot)(Float32 maxDot) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorPushQuad, nullptr, ("IQuadStackTesselatorPushQuad (Pointer hObject)(Rococo.QuadVertices quad)(Rococo.MaterialVertexData material) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorPopOutputAsTriangles, nullptr, ("IQuadStackTesselatorPopOutputAsTriangles (Pointer hObject)(Rococo.VertexTriangle topRight)(Rococo.VertexTriangle bottomLeft) -> (Bool wasPopped)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorScaleEdges, nullptr, ("IQuadStackTesselatorScaleEdges (Pointer hObject)(Float32 left)(Float32 right)(Float32 low)(Float32 high)(Bool preserveUVs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetBasis, nullptr, ("IQuadStackTesselatorSetBasis (Pointer hObject)(Sys.Maths.Vec3 tangent)(Sys.Maths.Vec3 normal)(Sys.Maths.Vec3 vertical) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetMaterial, nullptr, ("IQuadStackTesselatorSetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetTextureRect, nullptr, ("IQuadStackTesselatorSetTextureRect (Pointer hObject)(Sys.Maths.Rectf rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorShrink, nullptr, ("IQuadStackTesselatorShrink (Pointer hObject)(Sys.Maths.Rectf rect) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitThreeColumns, nullptr, ("IQuadStackTesselatorSplitThreeColumns (Pointer hObject)(Rococo.MaterialVertexData c1)(Rococo.MaterialVertexData c2)(Rococo.MaterialVertexData c3)(Float32 x0)(Float32 x1) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitThreeRows, nullptr, ("IQuadStackTesselatorSplitThreeRows (Pointer hObject)(Rococo.MaterialVertexData r1)(Rococo.MaterialVertexData r2)(Rococo.MaterialVertexData r3)(Float32 y0)(Float32 y1) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitAcrossTangent, nullptr, ("IQuadStackTesselatorSplitAcrossTangent (Pointer hObject)(Float32 v)(Int32 topColour)(Int32 middleColour)(Int32 lowColour)(Rococo.MaterialVertexData topMat)(Rococo.MaterialVertexData bottomMat) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorTileMosaic, nullptr, ("IQuadStackTesselatorTileMosaic (Pointer hObject)(Rococo.MaterialVertexData a)(Rococo.MaterialVertexData b)(Sys.Maths.Rectf uvRect)(Sys.SI.Metres roughSize) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorTranslate, nullptr, ("IQuadStackTesselatorTranslate (Pointer hObject)(Sys.Maths.Vec3 v) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IRodTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIRodTesselatorAddBox(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* d;
		_offset += sizeof(d);
		ReadInput(d, _sf, -_offset);

		Vec2* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBox(length, *a, *b, *c, *d);
	}
	void NativeRococoGraphicsIRodTesselatorAddPyramid(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* d;
		_offset += sizeof(d);
		ReadInput(d, _sf, -_offset);

		Vec2* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPyramid(length, *a, *b, *c, *d);
	}
	void NativeRococoGraphicsIRodTesselatorAddPrism(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		Vec2* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		Vec2* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddPrism(length, *a, *b, *c);
	}
	void NativeRococoGraphicsIRodTesselatorAddSphere(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 nDivs;
		_offset += sizeof(nDivs);
		ReadInput(nDivs, _sf, -_offset);

		int32 nRings;
		_offset += sizeof(nRings);
		ReadInput(nRings, _sf, -_offset);

		Metres radius;
		_offset += sizeof(radius);
		ReadInput(radius, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddSphere(radius, nRings, nDivs);
	}
	void NativeRococoGraphicsIRodTesselatorAddTorus(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 nDivs;
		_offset += sizeof(nDivs);
		ReadInput(nDivs, _sf, -_offset);

		int32 nRings;
		_offset += sizeof(nRings);
		ReadInput(nRings, _sf, -_offset);

		Metres outerRadius;
		_offset += sizeof(outerRadius);
		ReadInput(outerRadius, _sf, -_offset);

		Metres innerRadius;
		_offset += sizeof(innerRadius);
		ReadInput(innerRadius, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTorus(innerRadius, outerRadius, nRings, nDivs);
	}
	void NativeRococoGraphicsIRodTesselatorAddBowl(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 nDivs;
		_offset += sizeof(nDivs);
		ReadInput(nDivs, _sf, -_offset);

		int32 endRing;
		_offset += sizeof(endRing);
		ReadInput(endRing, _sf, -_offset);

		int32 startRing;
		_offset += sizeof(startRing);
		ReadInput(startRing, _sf, -_offset);

		int32 nRings;
		_offset += sizeof(nRings);
		ReadInput(nRings, _sf, -_offset);

		Metres radius2;
		_offset += sizeof(radius2);
		ReadInput(radius2, _sf, -_offset);

		Metres radius1;
		_offset += sizeof(radius1);
		ReadInput(radius1, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBowl(radius1, radius2, nRings, startRing, endRing, nDivs);
	}
	void NativeRococoGraphicsIRodTesselatorAddTube(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 nDivs;
		_offset += sizeof(nDivs);
		ReadInput(nDivs, _sf, -_offset);

		Metres topRadius;
		_offset += sizeof(topRadius);
		ReadInput(topRadius, _sf, -_offset);

		Metres bottomRadius;
		_offset += sizeof(bottomRadius);
		ReadInput(bottomRadius, _sf, -_offset);

		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTube(length, bottomRadius, topRadius, nDivs);
	}
	void NativeRococoGraphicsIRodTesselatorAddVertex(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* v;
		_offset += sizeof(v);
		ReadInput(v, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddVertex(*v);
	}
	void NativeRococoGraphicsIRodTesselatorAdvance(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres distance;
		_offset += sizeof(distance);
		ReadInput(distance, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Advance(distance);
	}
	void NativeRococoGraphicsIRodTesselatorClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsIRodTesselatorClearVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearVertices();
	}
	void NativeRococoGraphicsIRodTesselatorCloseLoop(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CloseLoop();
	}
	void NativeRococoGraphicsIRodTesselatorCopyToMeshBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 castsShadows;
		_offset += sizeof(castsShadows);
		ReadInput(castsShadows, _sf, -_offset);

		boolean32 invisible;
		_offset += sizeof(invisible);
		ReadInput(invisible, _sf, -_offset);

		boolean32 preserveMesh;
		_offset += sizeof(preserveMesh);
		ReadInput(preserveMesh, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _meshName;
		ReadInput(_meshName, _sf, -_offset);
		fstring meshName { _meshName->buffer, _meshName->length };


		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CopyToMeshBuilder(meshName, preserveMesh, invisible, castsShadows);
	}
	void NativeRococoGraphicsIRodTesselatorDestruct(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Destruct();
	}
	void NativeRococoGraphicsIRodTesselatorGetOrigin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* origin;
		_offset += sizeof(origin);
		ReadInput(origin, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetOrigin(*origin);
	}
	void NativeRococoGraphicsIRodTesselatorPopNextTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasPopped = _pObject->PopNextTriangle(*t);
		_offset += sizeof(wasPopped);
		WriteOutput(wasPopped, _sf, -_offset);
	}
	void NativeRococoGraphicsIRodTesselatorRaiseBox(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RaiseBox(length);
	}
	void NativeRococoGraphicsIRodTesselatorRaisePyramid(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres length;
		_offset += sizeof(length);
		ReadInput(length, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RaisePyramid(length);
	}
	void NativeRococoGraphicsIRodTesselatorScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float sz;
		_offset += sizeof(sz);
		ReadInput(sz, _sf, -_offset);

		float sy;
		_offset += sizeof(sy);
		ReadInput(sy, _sf, -_offset);

		float sx;
		_offset += sizeof(sx);
		ReadInput(sx, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Scale(sx, sy, sz);
	}
	void NativeRococoGraphicsIRodTesselatorSetBlendWeightByHeight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float upperValue;
		_offset += sizeof(upperValue);
		ReadInput(upperValue, _sf, -_offset);

		float lowerValue;
		_offset += sizeof(lowerValue);
		ReadInput(lowerValue, _sf, -_offset);

		int32 boneIndex;
		_offset += sizeof(boneIndex);
		ReadInput(boneIndex, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBlendWeightByHeight(boneIndex, lowerValue, upperValue);
	}
	void NativeRococoGraphicsIRodTesselatorSetMaterialBottom(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* bottom;
		_offset += sizeof(bottom);
		ReadInput(bottom, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaterialBottom(*bottom);
	}
	void NativeRococoGraphicsIRodTesselatorSetMaterialMiddle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* middle;
		_offset += sizeof(middle);
		ReadInput(middle, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaterialMiddle(*middle);
	}
	void NativeRococoGraphicsIRodTesselatorSetMaterialTop(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		MaterialVertexData* top;
		_offset += sizeof(top);
		ReadInput(top, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMaterialTop(*top);
	}
	void NativeRococoGraphicsIRodTesselatorSetOrigin(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* origin;
		_offset += sizeof(origin);
		ReadInput(origin, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOrigin(*origin);
	}
	void NativeRococoGraphicsIRodTesselatorSetUVScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float sUV;
		_offset += sizeof(sUV);
		ReadInput(sUV, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetUVScale(sUV);
	}
	void NativeRococoGraphicsIRodTesselatorTransformVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* m;
		_offset += sizeof(m);
		ReadInput(m, _sf, -_offset);

		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->TransformVertices(*m);
	}
	void NativeRococoGraphicsIRodTesselatorUseFaceNormals(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->UseFaceNormals();
	}
	void NativeRococoGraphicsIRodTesselatorUseSmoothNormals(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IRodTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->UseSmoothNormals();
	}

	void NativeGetHandleForRococoGraphicsRodTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::IRodTesselator* FactoryConstructRococoGraphicsRodTesselator(Rococo::Platform* _context);
		Rococo::Graphics::IRodTesselator* pObject = FactoryConstructRococoGraphicsRodTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIRodTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRodTesselator, _nceContext, ("GetHandleForIRodTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddBox, nullptr, ("IRodTesselatorAddBox (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c)(Sys.Maths.Vec2 d) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddPyramid, nullptr, ("IRodTesselatorAddPyramid (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c)(Sys.Maths.Vec2 d) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddPrism, nullptr, ("IRodTesselatorAddPrism (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddSphere, nullptr, ("IRodTesselatorAddSphere (Pointer hObject)(Sys.SI.Metres radius)(Int32 nRings)(Int32 nDivs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddTorus, nullptr, ("IRodTesselatorAddTorus (Pointer hObject)(Sys.SI.Metres innerRadius)(Sys.SI.Metres outerRadius)(Int32 nRings)(Int32 nDivs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddBowl, nullptr, ("IRodTesselatorAddBowl (Pointer hObject)(Sys.SI.Metres radius1)(Sys.SI.Metres radius2)(Int32 nRings)(Int32 startRing)(Int32 endRing)(Int32 nDivs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddTube, nullptr, ("IRodTesselatorAddTube (Pointer hObject)(Sys.SI.Metres length)(Sys.SI.Metres bottomRadius)(Sys.SI.Metres topRadius)(Int32 nDivs) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddVertex, nullptr, ("IRodTesselatorAddVertex (Pointer hObject)(Sys.Maths.Vec2 v) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAdvance, nullptr, ("IRodTesselatorAdvance (Pointer hObject)(Sys.SI.Metres distance) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorClear, nullptr, ("IRodTesselatorClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorClearVertices, nullptr, ("IRodTesselatorClearVertices (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorCloseLoop, nullptr, ("IRodTesselatorCloseLoop (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorCopyToMeshBuilder, nullptr, ("IRodTesselatorCopyToMeshBuilder (Pointer hObject)(Sys.Type.IString meshName)(Bool preserveMesh)(Bool invisible)(Bool castsShadows) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorDestruct, nullptr, ("IRodTesselatorDestruct (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorGetOrigin, nullptr, ("IRodTesselatorGetOrigin (Pointer hObject)(Sys.Maths.Vec3 origin) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorPopNextTriangle, nullptr, ("IRodTesselatorPopNextTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> (Bool wasPopped)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorRaiseBox, nullptr, ("IRodTesselatorRaiseBox (Pointer hObject)(Sys.SI.Metres length) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorRaisePyramid, nullptr, ("IRodTesselatorRaisePyramid (Pointer hObject)(Sys.SI.Metres length) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorScale, nullptr, ("IRodTesselatorScale (Pointer hObject)(Float32 sx)(Float32 sy)(Float32 sz) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetBlendWeightByHeight, nullptr, ("IRodTesselatorSetBlendWeightByHeight (Pointer hObject)(Int32 boneIndex)(Float32 lowerValue)(Float32 upperValue) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialBottom, nullptr, ("IRodTesselatorSetMaterialBottom (Pointer hObject)(Rococo.MaterialVertexData bottom) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialMiddle, nullptr, ("IRodTesselatorSetMaterialMiddle (Pointer hObject)(Rococo.MaterialVertexData middle) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialTop, nullptr, ("IRodTesselatorSetMaterialTop (Pointer hObject)(Rococo.MaterialVertexData top) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetOrigin, nullptr, ("IRodTesselatorSetOrigin (Pointer hObject)(Sys.Maths.Vec3 origin) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetUVScale, nullptr, ("IRodTesselatorSetUVScale (Pointer hObject)(Float32 sUV) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorTransformVertices, nullptr, ("IRodTesselatorTransformVertices (Pointer hObject)(Sys.Maths.Matrix4x4 m) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorUseFaceNormals, nullptr, ("IRodTesselatorUseFaceNormals (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorUseSmoothNormals, nullptr, ("IRodTesselatorUseSmoothNormals (Pointer hObject) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ILandscapeTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsILandscapeTesselatorAddQuadField(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		int32 base2exponentDivisions;
		_offset += sizeof(base2exponentDivisions);
		ReadInput(base2exponentDivisions, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddQuadField(base2exponentDivisions, span);
	}
	void NativeRococoGraphicsILandscapeTesselatorClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsILandscapeTesselatorCommitToMesh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _meshName;
		ReadInput(_meshName, _sf, -_offset);
		fstring meshName { _meshName->buffer, _meshName->length };


		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_MESH id = _pObject->CommitToMesh(meshName);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoGraphicsILandscapeTesselatorGenerateByRecursiveSubdivision(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres maxAltitude;
		_offset += sizeof(maxAltitude);
		ReadInput(maxAltitude, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GenerateByRecursiveSubdivision(maxAltitude);
	}
	void NativeRococoGraphicsILandscapeTesselatorGetBounds(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* maxPoint;
		_offset += sizeof(maxPoint);
		ReadInput(maxPoint, _sf, -_offset);

		Vec3* minPoint;
		_offset += sizeof(minPoint);
		ReadInput(minPoint, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBounds(*minPoint, *maxPoint);
	}
	void NativeRococoGraphicsILandscapeTesselatorRaiseMountain(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres spread;
		_offset += sizeof(spread);
		ReadInput(spread, _sf, -_offset);

		Metres deltaHeight;
		_offset += sizeof(deltaHeight);
		ReadInput(deltaHeight, _sf, -_offset);

		Vec3* atPosition;
		_offset += sizeof(atPosition);
		ReadInput(atPosition, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RaiseMountain(*atPosition, deltaHeight, spread);
	}
	void NativeRococoGraphicsILandscapeTesselatorSetHeights(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres height;
		_offset += sizeof(height);
		ReadInput(height, _sf, -_offset);

		Vec2i* p1;
		_offset += sizeof(p1);
		ReadInput(p1, _sf, -_offset);

		Vec2i* p0;
		_offset += sizeof(p0);
		ReadInput(p0, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetHeights(*p0, *p1, height);
	}
	void NativeRococoGraphicsILandscapeTesselatorSetSeed(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int64 seedNumber;
		_offset += sizeof(seedNumber);
		ReadInput(seedNumber, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSeed(seedNumber);
	}
	void NativeRococoGraphicsILandscapeTesselatorTranslateEachCell(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* delta;
		_offset += sizeof(delta);
		ReadInput(delta, _sf, -_offset);

		Rococo::Graphics::ILandscapeTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->TranslateEachCell(*delta);
	}

	void NativeGetHandleForRococoGraphicsLandscapeTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::ILandscapeTesselator* FactoryConstructRococoGraphicsLandscapeTesselator(Rococo::Platform* _context);
		Rococo::Graphics::ILandscapeTesselator* pObject = FactoryConstructRococoGraphicsLandscapeTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsILandscapeTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsLandscapeTesselator, _nceContext, ("GetHandleForILandscapeTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorAddQuadField, nullptr, ("ILandscapeTesselatorAddQuadField (Pointer hObject)(Int32 base2exponentDivisions)(Sys.SI.Metres span) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorClear, nullptr, ("ILandscapeTesselatorClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorCommitToMesh, nullptr, ("ILandscapeTesselatorCommitToMesh (Pointer hObject)(Sys.Type.IString meshName) -> (Int32 id)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorGenerateByRecursiveSubdivision, nullptr, ("ILandscapeTesselatorGenerateByRecursiveSubdivision (Pointer hObject)(Sys.SI.Metres maxAltitude) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorGetBounds, nullptr, ("ILandscapeTesselatorGetBounds (Pointer hObject)(Sys.Maths.Vec3 minPoint)(Sys.Maths.Vec3 maxPoint) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorRaiseMountain, nullptr, ("ILandscapeTesselatorRaiseMountain (Pointer hObject)(Sys.Maths.Vec3 atPosition)(Sys.SI.Metres deltaHeight)(Sys.SI.Metres spread) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorSetHeights, nullptr, ("ILandscapeTesselatorSetHeights (Pointer hObject)(Sys.Maths.Vec2i p0)(Sys.Maths.Vec2i p1)(Sys.SI.Metres height) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorSetSeed, nullptr, ("ILandscapeTesselatorSetSeed (Pointer hObject)(Int64 seedNumber) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsILandscapeTesselatorTranslateEachCell, nullptr, ("ILandscapeTesselatorTranslateEachCell (Pointer hObject)(Sys.Maths.Vec3 delta) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ITextTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsITextTesselatorAddBlankQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb paperColour;
		_offset += sizeof(paperColour);
		ReadInput(paperColour, _sf, -_offset);

		Quad* positions;
		_offset += sizeof(positions);
		ReadInput(positions, _sf, -_offset);

		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddBlankQuad(*positions, paperColour);
	}
	void NativeRococoGraphicsITextTesselatorAddLeftAlignedText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddLeftAlignedText(colour, text);
	}
	void NativeRococoGraphicsITextTesselatorClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsITextTesselatorSaveMesh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _meshName;
		ReadInput(_meshName, _sf, -_offset);
		fstring meshName { _meshName->buffer, _meshName->length };


		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SaveMesh(meshName);
	}
	void NativeRococoGraphicsITextTesselatorSetUVScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float scaleFactor;
		_offset += sizeof(scaleFactor);
		ReadInput(scaleFactor, _sf, -_offset);

		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetUVScale(scaleFactor);
	}
	void NativeRococoGraphicsITextTesselatorSetFormatQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quad* positions;
		_offset += sizeof(positions);
		ReadInput(positions, _sf, -_offset);

		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFormatQuad(*positions);
	}
	void NativeRococoGraphicsITextTesselatorTrySetFontIndex(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSet = _pObject->TrySetFontIndex(index);
		_offset += sizeof(isSet);
		WriteOutput(isSet, _sf, -_offset);
	}
	void NativeRococoGraphicsITextTesselatorTrySetFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float dotSize;
		_offset += sizeof(dotSize);
		ReadInput(dotSize, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Graphics::ITextTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSet = _pObject->TrySetFont(name, dotSize);
		_offset += sizeof(isSet);
		WriteOutput(isSet, _sf, -_offset);
	}

	void NativeGetHandleForRococoGraphicsTextTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::ITextTesselator* FactoryConstructRococoGraphicsTextTesselator(Rococo::Platform* _context);
		Rococo::Graphics::ITextTesselator* pObject = FactoryConstructRococoGraphicsTextTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsITextTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsTextTesselator, _nceContext, ("GetHandleForITextTesselator0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorAddBlankQuad, nullptr, ("ITextTesselatorAddBlankQuad (Pointer hObject)(Sys.Maths.Quadf positions)(Int32 paperColour) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorAddLeftAlignedText, nullptr, ("ITextTesselatorAddLeftAlignedText (Pointer hObject)(Int32 colour)(Sys.Type.IString text) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorClear, nullptr, ("ITextTesselatorClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSaveMesh, nullptr, ("ITextTesselatorSaveMesh (Pointer hObject)(Sys.Type.IString meshName) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSetUVScale, nullptr, ("ITextTesselatorSetUVScale (Pointer hObject)(Float32 scaleFactor) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSetFormatQuad, nullptr, ("ITextTesselatorSetFormatQuad (Pointer hObject)(Sys.Maths.Quadf positions) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorTrySetFontIndex, nullptr, ("ITextTesselatorTrySetFontIndex (Pointer hObject)(Int32 index) -> (Bool isSet)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorTrySetFont, nullptr, ("ITextTesselatorTrySetFont (Pointer hObject)(Sys.Type.IString name)(Float32 dotSize) -> (Bool isSet)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IHQFonts 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIHQFontsBuild(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::HQFont font;
		_offset += sizeof(font);
		ReadInput(font, _sf, -_offset);

		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Build(font);
	}
	void NativeRococoGraphicsIHQFontsClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsIHQFontsSetFaceName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFaceName(name);
	}
	void NativeRococoGraphicsIHQFontsSetHeight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 dyPixels;
		_offset += sizeof(dyPixels);
		ReadInput(dyPixels, _sf, -_offset);

		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetHeight(dyPixels);
	}
	void NativeRococoGraphicsIHQFontsMakeItalics(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MakeItalics();
	}
	void NativeRococoGraphicsIHQFontsMakeBold(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MakeBold();
	}
	void NativeRococoGraphicsIHQFontsAddUnicode32Char(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 unicodeValue;
		_offset += sizeof(unicodeValue);
		ReadInput(unicodeValue, _sf, -_offset);

		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddUnicode32Char(unicodeValue);
	}
	void NativeRococoGraphicsIHQFontsAddCharacters(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _s;
		ReadInput(_s, _sf, -_offset);
		fstring s { _s->buffer, _s->length };


		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddCharacters(s);
	}
	void NativeRococoGraphicsIHQFontsAddRange(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 unicodeEndChar;
		_offset += sizeof(unicodeEndChar);
		ReadInput(unicodeEndChar, _sf, -_offset);

		int32 unicodeStarChar;
		_offset += sizeof(unicodeStarChar);
		ReadInput(unicodeStarChar, _sf, -_offset);

		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddRange(unicodeStarChar, unicodeEndChar);
	}
	void NativeRococoGraphicsIHQFontsCommit(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_FONT fontId = _pObject->Commit();
		_offset += sizeof(fontId);
		WriteOutput(fontId, _sf, -_offset);
	}
	void NativeRococoGraphicsIHQFontsGetSysFont(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::HQFont font;
		_offset += sizeof(font);
		ReadInput(font, _sf, -_offset);

		Rococo::Graphics::IHQFonts* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_FONT fontId = _pObject->GetSysFont(font);
		_offset += sizeof(fontId);
		WriteOutput(fontId, _sf, -_offset);
	}

	void NativeGetHandleForRococoGraphicsHQFonts(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::IHQFonts* FactoryConstructRococoGraphicsHQFonts(Rococo::Platform* _context);
		Rococo::Graphics::IHQFonts* pObject = FactoryConstructRococoGraphicsHQFonts(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIHQFonts(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsHQFonts, _nceContext, ("GetHandleForIHQFonts0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsBuild, nullptr, ("IHQFontsBuild (Pointer hObject)(Int32 font) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsClear, nullptr, ("IHQFontsClear (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsSetFaceName, nullptr, ("IHQFontsSetFaceName (Pointer hObject)(Sys.Type.IString name) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsSetHeight, nullptr, ("IHQFontsSetHeight (Pointer hObject)(Int32 dyPixels) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsMakeItalics, nullptr, ("IHQFontsMakeItalics (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsMakeBold, nullptr, ("IHQFontsMakeBold (Pointer hObject) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsAddUnicode32Char, nullptr, ("IHQFontsAddUnicode32Char (Pointer hObject)(Int32 unicodeValue) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsAddCharacters, nullptr, ("IHQFontsAddCharacters (Pointer hObject)(Sys.Type.IString s) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsAddRange, nullptr, ("IHQFontsAddRange (Pointer hObject)(Int32 unicodeStarChar)(Int32 unicodeEndChar) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsCommit, nullptr, ("IHQFontsCommit (Pointer hObject) -> (Int32 fontId)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIHQFontsGetSysFont, nullptr, ("IHQFontsGetSysFont (Pointer hObject)(Int32 font) -> (Int32 fontId)"), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IRendererConfig 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIRendererConfigSetSampler(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::SampleIndex index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		Rococo::Graphics::SampleStateDef* ssd;
		_offset += sizeof(ssd);
		ReadInput(ssd, _sf, -_offset);

		Rococo::Graphics::IRendererConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSampler(*ssd, index);
	}

	void NativeGetHandleForRococoGraphicsRendererConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Graphics::IRendererConfig* FactoryConstructRococoGraphicsRendererConfig(Rococo::Platform* _context);
		Rococo::Graphics::IRendererConfig* pObject = FactoryConstructRococoGraphicsRendererConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Graphics
{
	void AddNativeCalls_RococoGraphicsIRendererConfig(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Graphics.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRendererConfig, _nceContext, ("GetHandleForIRendererConfig0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoGraphicsIRendererConfigSetSampler, nullptr, ("IRendererConfigSetSampler (Pointer hObject)(Rococo.SampleStateDef ssd)(Int32 index) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IArchive 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIArchiveLoadF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float defaultValue;
		_offset += sizeof(defaultValue);
		ReadInput(defaultValue, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IArchive* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->LoadF32(key, defaultValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeRococoIArchiveSaveF32(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IArchive* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SaveF32(key, value);
	}
	void NativeRococoIArchiveLoadVec3(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float defaultZ;
		_offset += sizeof(defaultZ);
		ReadInput(defaultZ, _sf, -_offset);

		float defaultY;
		_offset += sizeof(defaultY);
		ReadInput(defaultY, _sf, -_offset);

		float defaultX;
		_offset += sizeof(defaultX);
		ReadInput(defaultX, _sf, -_offset);

		Vec3* targetVariable;
		_offset += sizeof(targetVariable);
		ReadInput(targetVariable, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IArchive* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadVec3(key, *targetVariable, defaultX, defaultY, defaultZ);
	}
	void NativeRococoIArchiveSaveVec3(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* value;
		_offset += sizeof(value);
		ReadInput(value, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IArchive* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SaveVec3(key, *value);
	}

	void NativeGetHandleForRococoGetArchive(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::IArchive* FactoryConstructRococoGetArchive(Rococo::Platform* _context);
		Rococo::IArchive* pObject = FactoryConstructRococoGetArchive(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIArchive(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoGetArchive, _nceContext, ("GetHandleForIArchive0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArchiveLoadF32, nullptr, ("IArchiveLoadF32 (Pointer hObject)(Sys.Type.IString key)(Float32 defaultValue) -> (Float32 value)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArchiveSaveF32, nullptr, ("IArchiveSaveF32 (Pointer hObject)(Sys.Type.IString key)(Float32 value) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArchiveLoadVec3, nullptr, ("IArchiveLoadVec3 (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Vec3 targetVariable)(Float32 defaultX)(Float32 defaultY)(Float32 defaultZ) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIArchiveSaveVec3, nullptr, ("IArchiveSaveVec3 (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Vec3 value) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IInstallationManager 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIInstallationManagerSetPingPathMacro(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _pingPathValue;
		ReadInput(_pingPathValue, _sf, -_offset);
		fstring pingPathValue { _pingPathValue->buffer, _pingPathValue->length };


		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::IInstallationManager* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPingPathMacro(key, pingPathValue);
	}

	void NativeGetHandleForRococoInstallation(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::IInstallationManager* FactoryConstructRococoInstallation(Rococo::Platform* _context);
		Rococo::IInstallationManager* pObject = FactoryConstructRococoInstallation(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIInstallationManager(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoInstallation, _nceContext, ("GetHandleForIInstallation0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIInstallationManagerSetPingPathMacro, nullptr, ("IInstallationSetPingPathMacro (Pointer hObject)(Sys.Type.IString key)(Sys.Type.IString pingPathValue) -> "), __FILE__, __LINE__);
	}
}
// BennyHill generated Sexy native functions for Rococo::IWorldBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIWorldBuilderAddMeshToQuadtree(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::IWorldBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddMeshToQuadtree(id);
	}
	void NativeRococoIWorldBuilderGetHeightAt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::TriangleScan* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		float y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		Rococo::IWorldBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float height = _pObject->GetHeightAt(x, y, *t);
		_offset += sizeof(height);
		WriteOutput(height, _sf, -_offset);
	}
	void NativeRococoIWorldBuilderGetTriangleAt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::TriangleScan* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		Vec2* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		Rococo::IWorldBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetTriangleAt(*position, *t);
	}
	void NativeRococoIWorldBuilderNew(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres minSpan;
		_offset += sizeof(minSpan);
		ReadInput(minSpan, _sf, -_offset);

		Metres span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		Rococo::IWorldBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->New(span, minSpan);
	}

	void NativeGetHandleForRococoWorldBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::IWorldBuilder* FactoryConstructRococoWorldBuilder(Rococo::Platform* _context);
		Rococo::IWorldBuilder* pObject = FactoryConstructRococoWorldBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo
{
	void AddNativeCalls_RococoIWorldBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoWorldBuilder, _nceContext, ("GetHandleForIWorldBuilder0  -> (Pointer hObject)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIWorldBuilderAddMeshToQuadtree, nullptr, ("IWorldBuilderAddMeshToQuadtree (Pointer hObject)(Int64 id) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIWorldBuilderGetHeightAt, nullptr, ("IWorldBuilderGetHeightAt (Pointer hObject)(Float32 x)(Float32 y)(Rococo.TriangleScan t) -> (Float32 height)"), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIWorldBuilderGetTriangleAt, nullptr, ("IWorldBuilderGetTriangleAt (Pointer hObject)(Sys.Maths.Vec2 position)(Rococo.TriangleScan t) -> "), __FILE__, __LINE__);
		ss.AddNativeCall(ns, NativeRococoIWorldBuilderNew, nullptr, ("IWorldBuilderNew (Pointer hObject)(Sys.SI.Metres span)(Sys.SI.Metres minSpan) -> "), __FILE__, __LINE__);
	}
}

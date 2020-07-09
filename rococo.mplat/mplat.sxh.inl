namespace Rococo { 
	bool TryParse(const Rococo::fstring& s, ELayoutAlgorithm& value)
	{
		if (s ==  "ELayoutAlgorithm_None"_fstring)
		{
			value = ELayoutAlgorithm_None;
		}
		else if (s ==  "ELayoutAlgorithm_MaximizeOnlyChild"_fstring)
		{
			value = ELayoutAlgorithm_MaximizeOnlyChild;
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
			value = ELayoutAlgorithm_None;
		}
		else if (s ==  "MaximizeOnlyChild"_fstring)
		{
			value = ELayoutAlgorithm_MaximizeOnlyChild;
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
			case ELayoutAlgorithm_None:
				return "None"_fstring;
			case ELayoutAlgorithm_MaximizeOnlyChild:
				return "MaximizeOnlyChild"_fstring;
			default:
				return {"",0};
		}
	}
}// Rococo.ELayoutAlgorithm

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s ==  "OrientationFlags_None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s ==  "OrientationFlags_Heading"_fstring)
		{
			value = OrientationFlags_Heading;
		}
		else if (s ==  "OrientationFlags_Elevation"_fstring)
		{
			value = OrientationFlags_Elevation;
		}
		else if (s ==  "OrientationFlags_Tilt"_fstring)
		{
			value = OrientationFlags_Tilt;
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
			value = OrientationFlags_None;
		}
		else if (s ==  "Heading"_fstring)
		{
			value = OrientationFlags_Heading;
		}
		else if (s ==  "Elevation"_fstring)
		{
			value = OrientationFlags_Elevation;
		}
		else if (s ==  "Tilt"_fstring)
		{
			value = OrientationFlags_Tilt;
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
			case OrientationFlags_None:
				return "None"_fstring;
			case OrientationFlags_Heading:
				return "Heading"_fstring;
			case OrientationFlags_Elevation:
				return "Elevation"_fstring;
			case OrientationFlags_Tilt:
				return "Tilt"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.OrientationFlags

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, MaterialCategory& value)
	{
		if (s ==  "MaterialCategory_Rock"_fstring)
		{
			value = MaterialCategory_Rock;
		}
		else if (s ==  "MaterialCategory_Stone"_fstring)
		{
			value = MaterialCategory_Stone;
		}
		else if (s ==  "MaterialCategory_Marble"_fstring)
		{
			value = MaterialCategory_Marble;
		}
		else if (s ==  "MaterialCategory_Metal"_fstring)
		{
			value = MaterialCategory_Metal;
		}
		else if (s ==  "MaterialCategory_Wood"_fstring)
		{
			value = MaterialCategory_Wood;
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
			value = MaterialCategory_Rock;
		}
		else if (s ==  "Stone"_fstring)
		{
			value = MaterialCategory_Stone;
		}
		else if (s ==  "Marble"_fstring)
		{
			value = MaterialCategory_Marble;
		}
		else if (s ==  "Metal"_fstring)
		{
			value = MaterialCategory_Metal;
		}
		else if (s ==  "Wood"_fstring)
		{
			value = MaterialCategory_Wood;
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
			case MaterialCategory_Rock:
				return "Rock"_fstring;
			case MaterialCategory_Stone:
				return "Stone"_fstring;
			case MaterialCategory_Marble:
				return "Marble"_fstring;
			case MaterialCategory_Metal:
				return "Metal"_fstring;
			case MaterialCategory_Wood:
				return "Wood"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.MaterialCategory

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, SampleMethod& value)
	{
		if (s ==  "SampleMethod_Point"_fstring)
		{
			value = SampleMethod_Point;
		}
		else if (s ==  "SampleMethod_Linear"_fstring)
		{
			value = SampleMethod_Linear;
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
			value = SampleMethod_Point;
		}
		else if (s ==  "Linear"_fstring)
		{
			value = SampleMethod_Linear;
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
			case SampleMethod_Point:
				return "Point"_fstring;
			case SampleMethod_Linear:
				return "Linear"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.SampleMethod

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, SampleFilter& value)
	{
		if (s ==  "SampleFilter_Border"_fstring)
		{
			value = SampleFilter_Border;
		}
		else if (s ==  "SampleFilter_Mirror"_fstring)
		{
			value = SampleFilter_Mirror;
		}
		else if (s ==  "SampleFilter_Wrap"_fstring)
		{
			value = SampleFilter_Wrap;
		}
		else if (s ==  "SampleFilter_Clamp"_fstring)
		{
			value = SampleFilter_Clamp;
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
			value = SampleFilter_Border;
		}
		else if (s ==  "Mirror"_fstring)
		{
			value = SampleFilter_Mirror;
		}
		else if (s ==  "Wrap"_fstring)
		{
			value = SampleFilter_Wrap;
		}
		else if (s ==  "Clamp"_fstring)
		{
			value = SampleFilter_Clamp;
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
			case SampleFilter_Border:
				return "Border"_fstring;
			case SampleFilter_Mirror:
				return "Mirror"_fstring;
			case SampleFilter_Wrap:
				return "Wrap"_fstring;
			case SampleFilter_Clamp:
				return "Clamp"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.SampleFilter

namespace Rococo { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, SampleIndex& value)
	{
		if (s ==  "SampleIndex_Fonts"_fstring)
		{
			value = SampleIndex_Fonts;
		}
		else if (s ==  "SampleIndex_Sprites"_fstring)
		{
			value = SampleIndex_Sprites;
		}
		else if (s ==  "SampleIndex_Materials"_fstring)
		{
			value = SampleIndex_Materials;
		}
		else if (s ==  "SampleIndex_EnvironmentalMap"_fstring)
		{
			value = SampleIndex_EnvironmentalMap;
		}
		else if (s ==  "SampleIndex_ShadowMap"_fstring)
		{
			value = SampleIndex_ShadowMap;
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
			value = SampleIndex_Fonts;
		}
		else if (s ==  "Sprites"_fstring)
		{
			value = SampleIndex_Sprites;
		}
		else if (s ==  "Materials"_fstring)
		{
			value = SampleIndex_Materials;
		}
		else if (s ==  "EnvironmentalMap"_fstring)
		{
			value = SampleIndex_EnvironmentalMap;
		}
		else if (s ==  "ShadowMap"_fstring)
		{
			value = SampleIndex_ShadowMap;
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
			case SampleIndex_Fonts:
				return "Fonts"_fstring;
			case SampleIndex_Sprites:
				return "Sprites"_fstring;
			case SampleIndex_Materials:
				return "Materials"_fstring;
			case SampleIndex_EnvironmentalMap:
				return "EnvironmentalMap"_fstring;
			case SampleIndex_ShadowMap:
				return "ShadowMap"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Graphics.SampleIndex

namespace Rococo { namespace Audio { 
	bool TryParse(const Rococo::fstring& s, ELegacySoundShape& value)
	{
		if (s ==  "ELegacySoundShape_Sine"_fstring)
		{
			value = ELegacySoundShape_Sine;
		}
		else if (s ==  "ELegacySoundShape_Square"_fstring)
		{
			value = ELegacySoundShape_Square;
		}
		else if (s ==  "ELegacySoundShape_Triangle"_fstring)
		{
			value = ELegacySoundShape_Triangle;
		}
		else if (s ==  "ELegacySoundShape_Saw"_fstring)
		{
			value = ELegacySoundShape_Saw;
		}
		else if (s ==  "ELegacySoundShape_Noise"_fstring)
		{
			value = ELegacySoundShape_Noise;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, ELegacySoundShape& value)
	{
		if (s ==  "Sine"_fstring)
		{
			value = ELegacySoundShape_Sine;
		}
		else if (s ==  "Square"_fstring)
		{
			value = ELegacySoundShape_Square;
		}
		else if (s ==  "Triangle"_fstring)
		{
			value = ELegacySoundShape_Triangle;
		}
		else if (s ==  "Saw"_fstring)
		{
			value = ELegacySoundShape_Saw;
		}
		else if (s ==  "Noise"_fstring)
		{
			value = ELegacySoundShape_Noise;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(ELegacySoundShape value)
	{
		switch(value)
		{
			case ELegacySoundShape_Sine:
				return "Sine"_fstring;
			case ELegacySoundShape_Square:
				return "Square"_fstring;
			case ELegacySoundShape_Triangle:
				return "Triangle"_fstring;
			case ELegacySoundShape_Saw:
				return "Saw"_fstring;
			case ELegacySoundShape_Noise:
				return "Noise"_fstring;
			default:
				return {"",0};
		}
	}
}}// Rococo.Audio.ELegacySoundShape

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

namespace Rococo { 
	void AddNativeCalls_RococoIPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk1, nullptr, ("IPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk2, nullptr, ("IPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge1, nullptr, ("IPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge2, nullptr, ("IPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourFont, nullptr, ("IPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneIsVisible, nullptr, ("IPaneIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIPaneIsNormalized, nullptr, ("IPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIPaneSetVisible, nullptr, ("IPaneSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneGetRect, nullptr, ("IPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetRect, nullptr, ("IPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignLeftEdges, nullptr, ("IPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignRightEdges, nullptr, ("IPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignTopEdges, nullptr, ("IPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignBottomEdges, nullptr, ("IPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneLayoutVertically, nullptr, ("IPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetCommand, nullptr, ("IPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetPopulator, nullptr, ("IPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
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

}

namespace Rococo { 
	void AddNativeCalls_RococoIPaneContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPaneContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourBk1, nullptr, ("IPaneContainerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourBk2, nullptr, ("IPaneContainerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourEdge1, nullptr, ("IPaneContainerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourEdge2, nullptr, ("IPaneContainerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetColourFont, nullptr, ("IPaneContainerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerIsVisible, nullptr, ("IPaneContainerIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerIsNormalized, nullptr, ("IPaneContainerIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetVisible, nullptr, ("IPaneContainerSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerGetRect, nullptr, ("IPaneContainerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetRect, nullptr, ("IPaneContainerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignLeftEdges, nullptr, ("IPaneContainerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignRightEdges, nullptr, ("IPaneContainerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignTopEdges, nullptr, ("IPaneContainerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAlignBottomEdges, nullptr, ("IPaneContainerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerLayoutVertically, nullptr, ("IPaneContainerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetCommand, nullptr, ("IPaneContainerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerSetPopulator, nullptr, ("IPaneContainerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddContainer, nullptr, ("IPaneContainerAddContainer (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IPaneContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddFrame, nullptr, ("IPaneContainerAddFrame (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IFramePane frame)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTabContainer, nullptr, ("IPaneContainerAddTabContainer (Pointer hObject)(Int32 tabHeight)(Int32 fontIndex)(Sys.Maths.Recti rect) -> (Rococo.ITabContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddLabel, nullptr, ("IPaneContainerAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddSlider, nullptr, ("IPaneContainerAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddScroller, nullptr, ("IPaneContainerAddScroller (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect)(Bool isVertical) -> (Rococo.IScroller scroller)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTextOutput, nullptr, ("IPaneContainerAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddRadioButton, nullptr, ("IPaneContainerAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"));
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

namespace Rococo { 
	void AddNativeCalls_RococoITabContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITabContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourBk1, nullptr, ("ITabContainerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourBk2, nullptr, ("ITabContainerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourEdge1, nullptr, ("ITabContainerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourEdge2, nullptr, ("ITabContainerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetColourFont, nullptr, ("ITabContainerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerIsVisible, nullptr, ("ITabContainerIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoITabContainerIsNormalized, nullptr, ("ITabContainerIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetVisible, nullptr, ("ITabContainerSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerGetRect, nullptr, ("ITabContainerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetRect, nullptr, ("ITabContainerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignLeftEdges, nullptr, ("ITabContainerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignRightEdges, nullptr, ("ITabContainerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignTopEdges, nullptr, ("ITabContainerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerAlignBottomEdges, nullptr, ("ITabContainerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerLayoutVertically, nullptr, ("ITabContainerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetCommand, nullptr, ("ITabContainerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetPopulator, nullptr, ("ITabContainerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerAddTab, nullptr, ("ITabContainerAddTab (Pointer hObject)(Int32 tabWidth)(Sys.Type.IString caption)(Sys.Type.IString panelText) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerSetTabPopulator, nullptr, ("ITabContainerSetTabPopulator (Pointer hObject)(Sys.Type.IString populatorName) -> "));
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

}

namespace Rococo { 
	void AddNativeCalls_RococoIFramePane(Rococo::Script::IPublicScriptSystem& ss, Rococo::IFramePane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourBk1, nullptr, ("IFramePaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourBk2, nullptr, ("IFramePaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourEdge1, nullptr, ("IFramePaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourEdge2, nullptr, ("IFramePaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetColourFont, nullptr, ("IFramePaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneIsVisible, nullptr, ("IFramePaneIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneIsNormalized, nullptr, ("IFramePaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetVisible, nullptr, ("IFramePaneSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneGetRect, nullptr, ("IFramePaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetRect, nullptr, ("IFramePaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignLeftEdges, nullptr, ("IFramePaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignRightEdges, nullptr, ("IFramePaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignTopEdges, nullptr, ("IFramePaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAlignBottomEdges, nullptr, ("IFramePaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneLayoutVertically, nullptr, ("IFramePaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetCommand, nullptr, ("IFramePaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetPopulator, nullptr, ("IFramePaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddContainer, nullptr, ("IFramePaneAddContainer (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IPaneContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddFrame, nullptr, ("IFramePaneAddFrame (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IFramePane frame)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddTabContainer, nullptr, ("IFramePaneAddTabContainer (Pointer hObject)(Int32 tabHeight)(Int32 fontIndex)(Sys.Maths.Recti rect) -> (Rococo.ITabContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddLabel, nullptr, ("IFramePaneAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddSlider, nullptr, ("IFramePaneAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddScroller, nullptr, ("IFramePaneAddScroller (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect)(Bool isVertical) -> (Rococo.IScroller scroller)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddTextOutput, nullptr, ("IFramePaneAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneAddRadioButton, nullptr, ("IFramePaneAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneGetChildRect, nullptr, ("IFramePaneGetChildRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneContainer, nullptr, ("IFramePaneContainer (Pointer hObject) -> (Rococo.IPaneContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetLayoutAlgorithm, nullptr, ("IFramePaneSetLayoutAlgorithm (Pointer hObject)(Int32 layout) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetMinMaxSpan, nullptr, ("IFramePaneSetMinMaxSpan (Pointer hObject)(Int32 minDX)(Int32 minDY)(Int32 maxDX)(Int32 maxDY) -> "));
		ss.AddNativeCall(ns, NativeRococoIFramePaneSetCaption, nullptr, ("IFramePaneSetCaption (Pointer hObject)(Sys.Type.IString caption) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoIRadioButton(Rococo::Script::IPublicScriptSystem& ss, Rococo::IRadioButton* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourBk1, nullptr, ("IRadioButtonSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourBk2, nullptr, ("IRadioButtonSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourEdge1, nullptr, ("IRadioButtonSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourEdge2, nullptr, ("IRadioButtonSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetColourFont, nullptr, ("IRadioButtonSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonIsVisible, nullptr, ("IRadioButtonIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonIsNormalized, nullptr, ("IRadioButtonIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetVisible, nullptr, ("IRadioButtonSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonGetRect, nullptr, ("IRadioButtonGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetRect, nullptr, ("IRadioButtonSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignLeftEdges, nullptr, ("IRadioButtonAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignRightEdges, nullptr, ("IRadioButtonAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignTopEdges, nullptr, ("IRadioButtonAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonAlignBottomEdges, nullptr, ("IRadioButtonAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonLayoutVertically, nullptr, ("IRadioButtonLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetCommand, nullptr, ("IRadioButtonSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetPopulator, nullptr, ("IRadioButtonSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetAlignment, nullptr, ("IRadioButtonSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
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

}

namespace Rococo { 
	void AddNativeCalls_RococoILabelPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ILabelPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourBk1, nullptr, ("ILabelPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourBk2, nullptr, ("ILabelPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourEdge1, nullptr, ("ILabelPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourEdge2, nullptr, ("ILabelPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetColourFont, nullptr, ("ILabelPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneIsVisible, nullptr, ("ILabelPaneIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoILabelPaneIsNormalized, nullptr, ("ILabelPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetVisible, nullptr, ("ILabelPaneSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneGetRect, nullptr, ("ILabelPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetRect, nullptr, ("ILabelPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignLeftEdges, nullptr, ("ILabelPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignRightEdges, nullptr, ("ILabelPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignTopEdges, nullptr, ("ILabelPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneAlignBottomEdges, nullptr, ("ILabelPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneLayoutVertically, nullptr, ("ILabelPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetCommand, nullptr, ("ILabelPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetPopulator, nullptr, ("ILabelPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetAlignment, nullptr, ("ILabelPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoISlider(Rococo::Script::IPublicScriptSystem& ss, Rococo::ISlider* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoISliderSetColourBk1, nullptr, ("ISliderSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetColourBk2, nullptr, ("ISliderSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetColourEdge1, nullptr, ("ISliderSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetColourEdge2, nullptr, ("ISliderSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetColourFont, nullptr, ("ISliderSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderIsVisible, nullptr, ("ISliderIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoISliderIsNormalized, nullptr, ("ISliderIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoISliderSetVisible, nullptr, ("ISliderSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderGetRect, nullptr, ("ISliderGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetRect, nullptr, ("ISliderSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderAlignLeftEdges, nullptr, ("ISliderAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderAlignRightEdges, nullptr, ("ISliderAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderAlignTopEdges, nullptr, ("ISliderAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderAlignBottomEdges, nullptr, ("ISliderAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderLayoutVertically, nullptr, ("ISliderLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetCommand, nullptr, ("ISliderSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoISliderSetPopulator, nullptr, ("ISliderSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoIScroller(Rococo::Script::IPublicScriptSystem& ss, Rococo::IScroller* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourBk1, nullptr, ("IScrollerSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourBk2, nullptr, ("IScrollerSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourEdge1, nullptr, ("IScrollerSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourEdge2, nullptr, ("IScrollerSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetColourFont, nullptr, ("IScrollerSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerIsVisible, nullptr, ("IScrollerIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIScrollerIsNormalized, nullptr, ("IScrollerIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetVisible, nullptr, ("IScrollerSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerGetRect, nullptr, ("IScrollerGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetRect, nullptr, ("IScrollerSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignLeftEdges, nullptr, ("IScrollerAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignRightEdges, nullptr, ("IScrollerAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignTopEdges, nullptr, ("IScrollerAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerAlignBottomEdges, nullptr, ("IScrollerAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerLayoutVertically, nullptr, ("IScrollerLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetCommand, nullptr, ("IScrollerSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIScrollerSetPopulator, nullptr, ("IScrollerSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoITextOutputPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITextOutputPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourBk1, nullptr, ("ITextOutputPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourBk2, nullptr, ("ITextOutputPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourEdge1, nullptr, ("ITextOutputPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourEdge2, nullptr, ("ITextOutputPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetColourFont, nullptr, ("ITextOutputPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneIsVisible, nullptr, ("ITextOutputPaneIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneIsNormalized, nullptr, ("ITextOutputPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetVisible, nullptr, ("ITextOutputPaneSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneGetRect, nullptr, ("ITextOutputPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetRect, nullptr, ("ITextOutputPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignLeftEdges, nullptr, ("ITextOutputPaneAlignLeftEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignRightEdges, nullptr, ("ITextOutputPaneAlignRightEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignTopEdges, nullptr, ("ITextOutputPaneAlignTopEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneAlignBottomEdges, nullptr, ("ITextOutputPaneAlignBottomEdges (Pointer hObject)(Int32 borderPixels)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneLayoutVertically, nullptr, ("ITextOutputPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetCommand, nullptr, ("ITextOutputPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetPopulator, nullptr, ("ITextOutputPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetAlignment, nullptr, ("ITextOutputPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetActivateKey, nullptr, ("ITextOutputPaneSetActivateKey (Pointer hObject)(Sys.Type.IString key) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoIPaneBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPaneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoPaneBuilder, _nceContext, ("GetHandleForIPaneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIPaneBuilderRoot, nullptr, ("IPaneBuilderRoot (Pointer hObject) -> (Rococo.IPaneContainer container)"));
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

namespace Rococo { 
	void AddNativeCalls_RococoIKeyboard(Rococo::Script::IPublicScriptSystem& ss, Rococo::IKeyboard* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoKeyboard, _nceContext, ("GetHandleForIKeyboard0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIKeyboardClearActions, nullptr, ("IKeyboardClearActions (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardGetVKeyFromName, nullptr, ("IKeyboardGetVKeyFromName (Pointer hObject)(Sys.Type.IString name) -> (Int32 vkCode)"));
		ss.AddNativeCall(ns, NativeRococoIKeyboardSetKeyName, nullptr, ("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 vkeyCode) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardBindAction, nullptr, ("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardSaveCppHeader, nullptr, ("IKeyboardSaveCppHeader (Pointer hObject) -> "));
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

namespace Rococo { 
	void AddNativeCalls_RococoIConfig(Rococo::Script::IPublicScriptSystem& ss, Rococo::IConfig* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoConfig, _nceContext, ("GetHandleForIConfig0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIConfigInt, nullptr, ("IConfigInt (Pointer hObject)(Sys.Type.IString name)(Int32 value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigFloat, nullptr, ("IConfigFloat (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigBool, nullptr, ("IConfigBool (Pointer hObject)(Sys.Type.IString name)(Bool value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigText, nullptr, ("IConfigText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigGetInt, nullptr, ("IConfigGetInt (Pointer hObject)(Sys.Type.IString name) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetFloat, nullptr, ("IConfigGetFloat (Pointer hObject)(Sys.Type.IString name) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetBool, nullptr, ("IConfigGetBool (Pointer hObject)(Sys.Type.IString name) -> (Bool value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetText, nullptr, ("IConfigGetText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IStringBuilder text) -> "));
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::ISprites 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsISpritesClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeRococoGraphicsISpritesAddSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };


		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddSprite(resourceName);
	}
	void NativeRococoGraphicsISpritesAddEachSpriteInDirectory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _directoryName;
		ReadInput(_directoryName, _sf, -_offset);
		fstring directoryName { _directoryName->buffer, _directoryName->length };


		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddEachSpriteInDirectory(directoryName);
	}
	void NativeRococoGraphicsISpritesLoadAllSprites(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadAllSprites();
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsISprites(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ISprites* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSprites, _nceContext, ("GetHandleForISprites0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesClear, nullptr, ("ISpritesClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesAddSprite, nullptr, ("ISpritesAddSprite (Pointer hObject)(Sys.Type.IString resourceName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesAddEachSpriteInDirectory, nullptr, ("ISpritesAddEachSpriteInDirectory (Pointer hObject)(Sys.Type.IString directoryName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesLoadAllSprites, nullptr, ("ISpritesLoadAllSprites (Pointer hObject) -> "));
	}
}}
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

namespace Rococo { namespace Entities { 
	void AddNativeCalls_RococoEntitiesIMobiles(Rococo::Script::IPublicScriptSystem& ss, Rococo::Entities::IMobiles* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesMobiles, _nceContext, ("GetHandleForIMobiles0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesLink, nullptr, ("IMobilesLink (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesGetAngles, nullptr, ("IMobilesGetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesSetAngles, nullptr, ("IMobilesSetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
	}
}}
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

namespace Rococo { namespace Entities { 
	void AddNativeCalls_RococoEntitiesIParticleSystem(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesParticleSystem, _nceContext, ("GetHandleForIParticleSystem0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemApplySpectrum, nullptr, ("IParticleSystemApplySpectrum (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemClearSpectrum, nullptr, ("IParticleSystemClearSpectrum (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemSetSpectrum, nullptr, ("IParticleSystemSetSpectrum (Pointer hObject)(Sys.Maths.Vec4 colour)(Float32 relativeLifeTime) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemAddDust, nullptr, ("IParticleSystemAddDust (Pointer hObject)(Int32 particles)(Sys.SI.Metres meanParticleSize)(Sys.SI.Metres range)(Sys.SI.Metres minHeight)(Sys.SI.Metres maxHeight)(Int32 colour)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemAddVerticalFlame, nullptr, ("IParticleSystemAddVerticalFlame (Pointer hObject)(Rococo.FlameDef flameDef)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemClear, nullptr, ("IParticleSystemClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIParticleSystemSnuff, nullptr, ("IParticleSystemSnuff (Pointer hObject)(Int64 id) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsICamera(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ICamera* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsCamera, _nceContext, ("GetHandleForICamera0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraClear, nullptr, ("ICameraClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetRHProjection, nullptr, ("ICameraSetRHProjection (Pointer hObject)(Sys.Maths.Degrees fov)(Float32 near)(Float32 far) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetPosition, nullptr, ("ICameraSetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetOrientation, nullptr, ("ICameraSetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraFollowEntity, nullptr, ("ICameraFollowEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraMoveToEntity, nullptr, ("ICameraMoveToEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateWithEntity, nullptr, ("ICameraOrientateWithEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateToEntity, nullptr, ("ICameraOrientateToEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetPosition, nullptr, ("ICameraGetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetOrientation, nullptr, ("ICameraGetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorld, nullptr, ("ICameraGetWorld (Pointer hObject)(Sys.Maths.Matrix4x4 world) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorldAndProj, nullptr, ("ICameraGetWorldAndProj (Pointer hObject)(Sys.Maths.Matrix4x4 worldAndProj) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetProjection, nullptr, ("ICameraGetProjection (Pointer hObject)(Sys.Maths.Matrix4x4 proj) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraAspectRatio, nullptr, ("ICameraAspectRatio (Pointer hObject) -> (Float32 widthOverHeight)"));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsISceneBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::ISceneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSceneBuilder, _nceContext, ("GetHandleForISceneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderAddStatics, nullptr, ("ISceneBuilderAddStatics (Pointer hObject)(Int64 entityId) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderClear, nullptr, ("ISceneBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderClearLights, nullptr, ("ISceneBuilderClearLights (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetClearColour, nullptr, ("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue)(Float32 alpha) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetLight, nullptr, ("ISceneBuilderSetLight (Pointer hObject)(Rococo.LightSpec light)(Int32 index) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetSkyBox, nullptr, ("ISceneBuilderSetSkyBox (Pointer hObject)(Int64 cubeId) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIMessaging(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsMessaging, _nceContext, ("GetHandleForIMessaging0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMessagingLog, nullptr, ("IMessagingLog (Pointer hObject)(Sys.Type.IString message) -> "));
	}
}}
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
	void NativeRococoEntitiesIInstancesLoadMeshList(NativeCallEnvironment& _nce)
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
		_pObject->LoadMeshList(pingPath);
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

namespace Rococo { namespace Entities { 
	void AddNativeCalls_RococoEntitiesIInstances(Rococo::Script::IPublicScriptSystem& ss, Rococo::Entities::IInstances* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesInstances, _nceContext, ("GetHandleForIInstances0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddBody, nullptr, ("IInstancesAddBody (Pointer hObject)(Sys.Type.IString modelName)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddGhost, nullptr, ("IInstancesAddGhost (Pointer hObject)(Sys.Maths.Matrix4x4 model)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesCreateCubeTexture, nullptr, ("IInstancesCreateCubeTexture (Pointer hObject)(Sys.Type.IString folder)(Sys.Type.IString extension) -> (Int64 cubeId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesDelete, nullptr, ("IInstancesDelete (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesLoadMeshList, nullptr, ("IInstancesLoadMeshList (Pointer hObject)(Sys.Type.IString pingPath) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesLoadMaterialArray, nullptr, ("IInstancesLoadMaterialArray (Pointer hObject)(Sys.Type.IString folder)(Int32 txWidth) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesCountMaterialsInCategory, nullptr, ("IInstancesCountMaterialsInCategory (Pointer hObject)(Int32 category) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetMaterialId, nullptr, ("IInstancesGetMaterialId (Pointer hObject)(Int32 category)(Int32 index) -> (Float32 id)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetMaterialDirect, nullptr, ("IInstancesGetMaterialDirect (Pointer hObject)(Sys.Type.IString pingPath) -> (Float32 id)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetRandomMaterialId, nullptr, ("IInstancesGetRandomMaterialId (Pointer hObject)(Int32 category) -> (Float32 id)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetScale, nullptr, ("IInstancesGetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesSetMaterialMacro, nullptr, ("IInstancesSetMaterialMacro (Pointer hObject)(Sys.Type.IString pingPath) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesSetScale, nullptr, ("IInstancesSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesTryGetModelToWorldMatrix, nullptr, ("IInstancesTryGetModelToWorldMatrix (Pointer hObject)(Int64 entityId)(Sys.Maths.Matrix4x4 position) -> (Bool existant)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesClear, nullptr, ("IInstancesClear (Pointer hObject) -> "));
	}
}}
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
	void NativeRococoGraphicsIMeshBuilderSetSpecialShader(NativeCallEnvironment& _nce)
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
		IString* _psSpotlightPingPath;
		ReadInput(_psSpotlightPingPath, _sf, -_offset);
		fstring psSpotlightPingPath { _psSpotlightPingPath->buffer, _psSpotlightPingPath->length };


		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSpecialShader(fqName, psSpotlightPingPath, psAmbientPingPath, alphaBlending);
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIMeshBuilder(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IMeshBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsMeshBuilder, _nceContext, ("GetHandleForIMeshBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddMesh, nullptr, ("IMeshBuilderAddMesh (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Sys.Type.IString sourceName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddTriangleEx, nullptr, ("IMeshBuilderAddTriangleEx (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddTriangle, nullptr, ("IMeshBuilderAddTriangle (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddPhysicsHull, nullptr, ("IMeshBuilderAddPhysicsHull (Pointer hObject)(Sys.Maths.Triangle t) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderBegin, nullptr, ("IMeshBuilderBegin (Pointer hObject)(Sys.Type.IString meshName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderEnd, nullptr, ("IMeshBuilderEnd (Pointer hObject)(Bool preserveCopy)(Bool invisible) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderClear, nullptr, ("IMeshBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderDelete, nullptr, ("IMeshBuilderDelete (Pointer hObject)(Sys.Type.IString fqName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSetShadowCasting, nullptr, ("IMeshBuilderSetShadowCasting (Pointer hObject)(Sys.Type.IString fqName)(Bool isActive) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSetSpecialShader, nullptr, ("IMeshBuilderSetSpecialShader (Pointer hObject)(Sys.Type.IString fqName)(Sys.Type.IString psSpotlightPingPath)(Sys.Type.IString psAmbientPingPath)(Bool alphaBlending) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderSpan, nullptr, ("IMeshBuilderSpan (Pointer hObject)(Sys.Maths.Vec3 span)(Sys.Type.IString fqName) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIRimTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IRimTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRimTesselator, _nceContext, ("GetHandleForIRimTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPoint, nullptr, ("IRimTesselatorAddPoint (Pointer hObject)(Sys.Maths.Vec2 p) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPointXY, nullptr, ("IRimTesselatorAddPointXY (Pointer hObject)(Float32 x)(Float32 y) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorCloseLoop, nullptr, ("IRimTesselatorCloseLoop (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorMakeElipse, nullptr, ("IRimTesselatorMakeElipse (Pointer hObject)(Int32 numberOfSides)(Float32 sx)(Float32 sy) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClear, nullptr, ("IRimTesselatorClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClearFaces, nullptr, ("IRimTesselatorClearFaces (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorScale, nullptr, ("IRimTesselatorScale (Pointer hObject)(Float32 sx)(Float32 sy) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorPerimeterVertices, nullptr, ("IRimTesselatorPerimeterVertices (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimQuad, nullptr, ("IRimTesselatorGetRimQuad (Pointer hObject)(Float32 zHigh)(Float32 zLow)(Int32 index)(Sys.Maths.Quadf quad) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimVertex, nullptr, ("IRimTesselatorGetRimVertex (Pointer hObject)(Int32 index)(Sys.Maths.Vec2 p) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorTesselateUniform, nullptr, ("IRimTesselatorTesselateUniform (Pointer hObject) -> (Int32 nTrianglesPerFace)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetBottomTriangle, nullptr, ("IRimTesselatorGetBottomTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetTopTriangle, nullptr, ("IRimTesselatorGetTopTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorSetTransform, nullptr, ("IRimTesselatorSetTransform (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIFieldTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IFieldTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsFieldTesselator, _nceContext, ("GetHandleForIFieldTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorDestruct, nullptr, ("IFieldTesselatorDestruct (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByFixedCellWidth, nullptr, ("IFieldTesselatorInitByFixedCellWidth (Pointer hObject)(Sys.Maths.Quadf positions)(Float32 maxCellWidth)(Float32 maxCellHeight) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByDivisions, nullptr, ("IFieldTesselatorInitByDivisions (Pointer hObject)(Sys.Maths.Quadf positions)(Int32 xDivs)(Int32 yDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorSetUV, nullptr, ("IFieldTesselatorSetUV (Pointer hObject)(Sys.Maths.Vec2 uvA)(Sys.Maths.Vec2 uvC) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfColumns, nullptr, ("IFieldTesselatorNumberOfColumns (Pointer hObject) -> (Int32 cols)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfRows, nullptr, ("IFieldTesselatorNumberOfRows (Pointer hObject) -> (Int32 rows)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetFlatSubQuad, nullptr, ("IFieldTesselatorGetFlatSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices subQuad) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetPerturbedSubQuad, nullptr, ("IFieldTesselatorGetPerturbedSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStackBondedBrick, nullptr, ("IFieldTesselatorGetStackBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStretchBondedBrick, nullptr, ("IFieldTesselatorGetStretchBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Rococo.QuadVertices top)(Rococo.QuadVertices left)(Rococo.QuadVertices right)(Rococo.QuadVertices bottom)(Sys.SI.Metres cementWidth)(Sys.SI.Metres extrusionBase) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickJoinRight, nullptr, ("IFieldTesselatorGetBrickJoinRight (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickBedTop, nullptr, ("IFieldTesselatorGetBrickBedTop (Pointer hObject)(Int32 row)(Rococo.QuadVertices q)(Sys.SI.Metres cementWidth) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorPerturbField, nullptr, ("IFieldTesselatorPerturbField (Pointer hObject)(Int32 i)(Int32 j)(Float32 dH) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorLevelField, nullptr, ("IFieldTesselatorLevelField (Pointer hObject)(Int32 i0)(Int32 j0)(Int32 i1)(Int32 j1)(Float32 dH) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorRandomizeField, nullptr, ("IFieldTesselatorRandomizeField (Pointer hObject)(Int32 i)(Int32 j)(Float32 minValue)(Float32 maxValue) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBasis, nullptr, ("IFieldTesselatorGetBasis (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIQuadStackTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Graphics::IQuadStackTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsQuadStackTesselator, _nceContext, ("GetHandleForIQuadStackTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorAddCuboid, nullptr, ("IQuadStackTesselatorAddCuboid (Pointer hObject)(Float32 v0)(Float32 v1)(Float32 t0)(Float32 t1)(Float32 thickness)(Float32 uvScale)(Rococo.MaterialVertexData rodMat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorAddCuboidAbs, nullptr, ("IQuadStackTesselatorAddCuboidAbs (Pointer hObject)(Sys.SI.Metres dx0)(Sys.SI.Metres dy0)(Sys.SI.Metres dx1)(Sys.SI.Metres dy1)(Sys.SI.Metres thickness)(Float32 uvScale)(Rococo.MaterialVertexData rodMat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClear, nullptr, ("IQuadStackTesselatorClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClearInput, nullptr, ("IQuadStackTesselatorClearInput (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorClearOutput, nullptr, ("IQuadStackTesselatorClearOutput (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorCopyInputToOutput, nullptr, ("IQuadStackTesselatorCopyInputToOutput (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorDestruct, nullptr, ("IQuadStackTesselatorDestruct (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorIntrude, nullptr, ("IQuadStackTesselatorIntrude (Pointer hObject)(Sys.Maths.Rectf window)(Float32 depth)(Float32 depthUvScale)(Rococo.MaterialVertexData rimMat)(Rococo.MaterialVertexData innerMat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutput, nullptr, ("IQuadStackTesselatorMoveInputToOutput (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInput, nullptr, ("IQuadStackTesselatorMoveOutputToInput (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithMat, nullptr, ("IQuadStackTesselatorMoveOutputToInputWithMat (Pointer hObject)(Rococo.MaterialVertexData mat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveOutputToInputWithNormalDotRange, nullptr, ("IQuadStackTesselatorMoveOutputToInputWithNormalDotRange (Pointer hObject)(Sys.Maths.Vec3 normal)(Float32 minDot)(Float32 maxDot) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorMoveInputToOutputWithNormalDotRange, nullptr, ("IQuadStackTesselatorMoveInputToOutputWithNormalDotRange (Pointer hObject)(Sys.Maths.Vec3 normal)(Float32 minDot)(Float32 maxDot) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorPushQuad, nullptr, ("IQuadStackTesselatorPushQuad (Pointer hObject)(Rococo.QuadVertices quad)(Rococo.MaterialVertexData material) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorPopOutputAsTriangles, nullptr, ("IQuadStackTesselatorPopOutputAsTriangles (Pointer hObject)(Rococo.VertexTriangle topRight)(Rococo.VertexTriangle bottomLeft) -> (Bool wasPopped)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorScaleEdges, nullptr, ("IQuadStackTesselatorScaleEdges (Pointer hObject)(Float32 left)(Float32 right)(Float32 low)(Float32 high)(Bool preserveUVs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetBasis, nullptr, ("IQuadStackTesselatorSetBasis (Pointer hObject)(Sys.Maths.Vec3 tangent)(Sys.Maths.Vec3 normal)(Sys.Maths.Vec3 vertical) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetMaterial, nullptr, ("IQuadStackTesselatorSetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSetTextureRect, nullptr, ("IQuadStackTesselatorSetTextureRect (Pointer hObject)(Sys.Maths.Rectf rect) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorShrink, nullptr, ("IQuadStackTesselatorShrink (Pointer hObject)(Sys.Maths.Rectf rect) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitThreeColumns, nullptr, ("IQuadStackTesselatorSplitThreeColumns (Pointer hObject)(Rococo.MaterialVertexData c1)(Rococo.MaterialVertexData c2)(Rococo.MaterialVertexData c3)(Float32 x0)(Float32 x1) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitThreeRows, nullptr, ("IQuadStackTesselatorSplitThreeRows (Pointer hObject)(Rococo.MaterialVertexData r1)(Rococo.MaterialVertexData r2)(Rococo.MaterialVertexData r3)(Float32 y0)(Float32 y1) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorSplitAcrossTangent, nullptr, ("IQuadStackTesselatorSplitAcrossTangent (Pointer hObject)(Float32 v)(Int32 topColour)(Int32 middleColour)(Int32 lowColour)(Rococo.MaterialVertexData topMat)(Rococo.MaterialVertexData bottomMat) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorTileMosaic, nullptr, ("IQuadStackTesselatorTileMosaic (Pointer hObject)(Rococo.MaterialVertexData a)(Rococo.MaterialVertexData b)(Sys.Maths.Rectf uvRect)(Sys.SI.Metres roughSize) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIQuadStackTesselatorTranslate, nullptr, ("IQuadStackTesselatorTranslate (Pointer hObject)(Sys.Maths.Vec3 v) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIRodTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRodTesselator, _nceContext, ("GetHandleForIRodTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddBox, nullptr, ("IRodTesselatorAddBox (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c)(Sys.Maths.Vec2 d) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddPyramid, nullptr, ("IRodTesselatorAddPyramid (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c)(Sys.Maths.Vec2 d) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddPrism, nullptr, ("IRodTesselatorAddPrism (Pointer hObject)(Sys.SI.Metres length)(Sys.Maths.Vec2 a)(Sys.Maths.Vec2 b)(Sys.Maths.Vec2 c) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddSphere, nullptr, ("IRodTesselatorAddSphere (Pointer hObject)(Sys.SI.Metres radius)(Int32 nRings)(Int32 nDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddTorus, nullptr, ("IRodTesselatorAddTorus (Pointer hObject)(Sys.SI.Metres innerRadius)(Sys.SI.Metres outerRadius)(Int32 nRings)(Int32 nDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddBowl, nullptr, ("IRodTesselatorAddBowl (Pointer hObject)(Sys.SI.Metres radius1)(Sys.SI.Metres radius2)(Int32 nRings)(Int32 startRing)(Int32 endRing)(Int32 nDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddTube, nullptr, ("IRodTesselatorAddTube (Pointer hObject)(Sys.SI.Metres length)(Sys.SI.Metres bottomRadius)(Sys.SI.Metres topRadius)(Int32 nDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAddVertex, nullptr, ("IRodTesselatorAddVertex (Pointer hObject)(Sys.Maths.Vec2 v) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorAdvance, nullptr, ("IRodTesselatorAdvance (Pointer hObject)(Sys.SI.Metres distance) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorClear, nullptr, ("IRodTesselatorClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorClearVertices, nullptr, ("IRodTesselatorClearVertices (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorCloseLoop, nullptr, ("IRodTesselatorCloseLoop (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorCopyToMeshBuilder, nullptr, ("IRodTesselatorCopyToMeshBuilder (Pointer hObject)(Sys.Type.IString meshName)(Bool preserveMesh)(Bool invisible)(Bool castsShadows) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorDestruct, nullptr, ("IRodTesselatorDestruct (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorGetOrigin, nullptr, ("IRodTesselatorGetOrigin (Pointer hObject)(Sys.Maths.Vec3 origin) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorPopNextTriangle, nullptr, ("IRodTesselatorPopNextTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> (Bool wasPopped)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorRaiseBox, nullptr, ("IRodTesselatorRaiseBox (Pointer hObject)(Sys.SI.Metres length) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorRaisePyramid, nullptr, ("IRodTesselatorRaisePyramid (Pointer hObject)(Sys.SI.Metres length) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorScale, nullptr, ("IRodTesselatorScale (Pointer hObject)(Float32 sx)(Float32 sy)(Float32 sz) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialBottom, nullptr, ("IRodTesselatorSetMaterialBottom (Pointer hObject)(Rococo.MaterialVertexData bottom) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialMiddle, nullptr, ("IRodTesselatorSetMaterialMiddle (Pointer hObject)(Rococo.MaterialVertexData middle) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetMaterialTop, nullptr, ("IRodTesselatorSetMaterialTop (Pointer hObject)(Rococo.MaterialVertexData top) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetOrigin, nullptr, ("IRodTesselatorSetOrigin (Pointer hObject)(Sys.Maths.Vec3 origin) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorSetUVScale, nullptr, ("IRodTesselatorSetUVScale (Pointer hObject)(Float32 sUV) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorTransformVertices, nullptr, ("IRodTesselatorTransformVertices (Pointer hObject)(Sys.Maths.Matrix4x4 m) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorUseFaceNormals, nullptr, ("IRodTesselatorUseFaceNormals (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRodTesselatorUseSmoothNormals, nullptr, ("IRodTesselatorUseSmoothNormals (Pointer hObject) -> "));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsITextTesselator(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsTextTesselator, _nceContext, ("GetHandleForITextTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorAddBlankQuad, nullptr, ("ITextTesselatorAddBlankQuad (Pointer hObject)(Sys.Maths.Quadf positions)(Int32 paperColour) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorAddLeftAlignedText, nullptr, ("ITextTesselatorAddLeftAlignedText (Pointer hObject)(Int32 colour)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorClear, nullptr, ("ITextTesselatorClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSaveMesh, nullptr, ("ITextTesselatorSaveMesh (Pointer hObject)(Sys.Type.IString meshName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSetUVScale, nullptr, ("ITextTesselatorSetUVScale (Pointer hObject)(Float32 scaleFactor) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorSetFormatQuad, nullptr, ("ITextTesselatorSetFormatQuad (Pointer hObject)(Sys.Maths.Quadf positions) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorTrySetFontIndex, nullptr, ("ITextTesselatorTrySetFontIndex (Pointer hObject)(Int32 index) -> (Bool isSet)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsITextTesselatorTrySetFont, nullptr, ("ITextTesselatorTrySetFont (Pointer hObject)(Sys.Type.IString name)(Float32 dotSize) -> (Bool isSet)"));
	}
}}
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

namespace Rococo { namespace Graphics { 
	void AddNativeCalls_RococoGraphicsIRendererConfig(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRendererConfig, _nceContext, ("GetHandleForIRendererConfig0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRendererConfigSetSampler, nullptr, ("IRendererConfigSetSampler (Pointer hObject)(Rococo.SampleStateDef ssd)(Int32 index) -> "));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Audio::ILegacySoundControl 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoAudioILegacySoundControlSetMasterVolumne(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float gain;
		_offset += sizeof(gain);
		ReadInput(gain, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMasterVolumne(gain);
	}
	void NativeRococoAudioILegacySoundControlSetChannelVolume(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float rightVolume;
		_offset += sizeof(rightVolume);
		ReadInput(rightVolume, _sf, -_offset);

		float leftVolume;
		_offset += sizeof(leftVolume);
		ReadInput(leftVolume, _sf, -_offset);

		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetChannelVolume(channel, leftVolume, rightVolume);
	}
	void NativeRococoAudioILegacySoundControlSetWave(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float dutyCycle;
		_offset += sizeof(dutyCycle);
		ReadInput(dutyCycle, _sf, -_offset);

		float freqHz;
		_offset += sizeof(freqHz);
		ReadInput(freqHz, _sf, -_offset);

		Rococo::Audio::ELegacySoundShape waveShape;
		_offset += sizeof(waveShape);
		ReadInput(waveShape, _sf, -_offset);

		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetWave(channel, waveShape, freqHz, dutyCycle);
	}
	void NativeRococoAudioILegacySoundControlPlayWave(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PlayWave(channel);
	}
	void NativeRococoAudioILegacySoundControlSustain(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Sustain(channel);
	}
	void NativeRococoAudioILegacySoundControlRelease(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Release(channel);
	}
	void NativeRococoAudioILegacySoundControlSetEnvelope(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float releaseSecs;
		_offset += sizeof(releaseSecs);
		ReadInput(releaseSecs, _sf, -_offset);

		float sustainLevel;
		_offset += sizeof(sustainLevel);
		ReadInput(sustainLevel, _sf, -_offset);

		float decaySecs;
		_offset += sizeof(decaySecs);
		ReadInput(decaySecs, _sf, -_offset);

		float attackSecs;
		_offset += sizeof(attackSecs);
		ReadInput(attackSecs, _sf, -_offset);

		int32 channel;
		_offset += sizeof(channel);
		ReadInput(channel, _sf, -_offset);

		Rococo::Audio::ILegacySoundControl* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetEnvelope(channel, attackSecs, decaySecs, sustainLevel, releaseSecs);
	}

	void NativeGetHandleForRococoAudioLegacySoundControl(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Audio::ILegacySoundControl* FactoryConstructRococoAudioLegacySoundControl(Rococo::Platform* _context);
		Rococo::Audio::ILegacySoundControl* pObject = FactoryConstructRococoAudioLegacySoundControl(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo { namespace Audio { 
	void AddNativeCalls_RococoAudioILegacySoundControl(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Audio.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoAudioLegacySoundControl, _nceContext, ("GetHandleForILegacySoundControl0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlSetMasterVolumne, nullptr, ("ILegacySoundControlSetMasterVolumne (Pointer hObject)(Float32 gain) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlSetChannelVolume, nullptr, ("ILegacySoundControlSetChannelVolume (Pointer hObject)(Int32 channel)(Float32 leftVolume)(Float32 rightVolume) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlSetWave, nullptr, ("ILegacySoundControlSetWave (Pointer hObject)(Int32 channel)(Int32 waveShape)(Float32 freqHz)(Float32 dutyCycle) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlPlayWave, nullptr, ("ILegacySoundControlPlayWave (Pointer hObject)(Int32 channel) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlSustain, nullptr, ("ILegacySoundControlSustain (Pointer hObject)(Int32 channel) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlRelease, nullptr, ("ILegacySoundControlRelease (Pointer hObject)(Int32 channel) -> "));
		ss.AddNativeCall(ns, NativeRococoAudioILegacySoundControlSetEnvelope, nullptr, ("ILegacySoundControlSetEnvelope (Pointer hObject)(Int32 channel)(Float32 attackSecs)(Float32 decaySecs)(Float32 sustainLevel)(Float32 releaseSecs) -> "));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Puppet::IPuppet 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoPuppetIPuppetDestruct(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Destruct();
	}
	void NativeRococoPuppetIPuppetMarkForDeath(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MarkForDeath();
	}
	void NativeRococoPuppetIPuppetExists(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isExistent = _pObject->Exists();
		_offset += sizeof(isExistent);
		WriteOutput(isExistent, _sf, -_offset);
	}
	void NativeRococoPuppetIPuppetSetKey(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetKey(key);
	}
	void NativeRococoPuppetIPuppetAppendKey(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** key;
		ReadInput(key, _sf, -_offset);
		Rococo::Helpers::StringPopulator _keyPopulator(_nce, key);
		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendKey(_keyPopulator);
	}
	void NativeRococoPuppetIPuppetSetAtrribute(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _value;
		ReadInput(_value, _sf, -_offset);
		fstring value { _value->buffer, _value->length };


		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAtrribute(key, value);
	}
	void NativeRococoPuppetIPuppetAppendAttribute(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** valueBuilder;
		ReadInput(valueBuilder, _sf, -_offset);
		Rococo::Helpers::StringPopulator _valueBuilderPopulator(_nce, valueBuilder);
		_offset += sizeof(IString*);
		IString* _key;
		ReadInput(_key, _sf, -_offset);
		fstring key { _key->buffer, _key->length };


		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AppendAttribute(key, _valueBuilderPopulator);
	}
	void NativeRococoPuppetIPuppetId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int64 id = _pObject->Id();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoPuppetIPuppetSetModel(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetModel(*model);
	}
	void NativeRococoPuppetIPuppetGetModel(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetModel(*model);
	}
	void NativeRococoPuppetIPuppetIsActive(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 isActive;
		_offset += sizeof(isActive);
		ReadInput(isActive, _sf, -_offset);

		Rococo::Puppet::IPuppet* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->IsActive(isActive);
	}

}

namespace Rococo { namespace Puppet { 
	void AddNativeCalls_RococoPuppetIPuppet(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Puppet.Native"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetDestruct, nullptr, ("IPuppetDestruct (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetMarkForDeath, nullptr, ("IPuppetMarkForDeath (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetExists, nullptr, ("IPuppetExists (Pointer hObject) -> (Bool isExistent)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetSetKey, nullptr, ("IPuppetSetKey (Pointer hObject)(Sys.Type.IString key) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetAppendKey, nullptr, ("IPuppetAppendKey (Pointer hObject)(Sys.Type.IStringBuilder key) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetSetAtrribute, nullptr, ("IPuppetSetAtrribute (Pointer hObject)(Sys.Type.IString key)(Sys.Type.IString value) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetAppendAttribute, nullptr, ("IPuppetAppendAttribute (Pointer hObject)(Sys.Type.IString key)(Sys.Type.IStringBuilder valueBuilder) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetId, nullptr, ("IPuppetId (Pointer hObject) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetSetModel, nullptr, ("IPuppetSetModel (Pointer hObject)(Sys.Maths.Matrix4x4 model) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetGetModel, nullptr, ("IPuppetGetModel (Pointer hObject)(Sys.Maths.Matrix4x4 model) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetIsActive, nullptr, ("IPuppetIsActive (Pointer hObject)(Bool isActive) -> "));
	}
}}
// BennyHill generated Sexy native functions for Rococo::Puppet::IPuppets 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoPuppetIPuppetsKillAllPuppets(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppets* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->KillAllPuppets();
	}
	void NativeRococoPuppetIPuppetsCreatePuppet(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::NewPuppetDesc* puppetDescription;
		_offset += sizeof(puppetDescription);
		ReadInput(puppetDescription, _sf, -_offset);

		Rococo::Puppet::IPuppets* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Puppet::IPuppet* puppet = _pObject->CreatePuppet(*puppetDescription);
		_offset += sizeof(CReflectedClass*);
		auto& _puppetStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Puppet"),("IPuppet"), ("ProxyIPuppet"), _nce.ss);
		CReflectedClass* _sxypuppet = _nce.ss.Represent(_puppetStruct, puppet);
		WriteOutput(&_sxypuppet->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoPuppetIPuppetsGetFirstPuppetId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Puppet::IPuppets* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_PUPPET id = _pObject->GetFirstPuppetId();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeRococoPuppetIPuppetsFindPuppetById(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_PUPPET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Puppet::IPuppets* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::Puppet::IPuppet* puppet = _pObject->FindPuppetById(id);
		_offset += sizeof(CReflectedClass*);
		auto& _puppetStruct = Rococo::Helpers::GetDefaultProxy(("Rococo.Puppet"),("IPuppet"), ("ProxyIPuppet"), _nce.ss);
		CReflectedClass* _sxypuppet = _nce.ss.Represent(_puppetStruct, puppet);
		WriteOutput(&_sxypuppet->header.pVTables[0], _sf, -_offset);
	}
	void NativeRococoPuppetIPuppetsGetNextPuppetId(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_PUPPET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Rococo::Puppet::IPuppets* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_PUPPET nextId = _pObject->GetNextPuppetId(id);
		_offset += sizeof(nextId);
		WriteOutput(nextId, _sf, -_offset);
	}

	void NativeGetHandleForRococoPuppetPuppets(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Platform* nceContext = reinterpret_cast<Rococo::Platform*>(_nce.context);
		// Uses: Rococo::Puppet::IPuppets* FactoryConstructRococoPuppetPuppets(Rococo::Platform* _context);
		Rococo::Puppet::IPuppets* pObject = FactoryConstructRococoPuppetPuppets(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo { namespace Puppet { 
	void AddNativeCalls_RococoPuppetIPuppets(Rococo::Script::IPublicScriptSystem& ss, Rococo::Platform* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("Rococo.Puppet.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoPuppetPuppets, _nceContext, ("GetHandleForIPuppets0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetsKillAllPuppets, nullptr, ("IPuppetsKillAllPuppets (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetsCreatePuppet, nullptr, ("IPuppetsCreatePuppet (Pointer hObject)(Rococo.Puppet.NewPuppetDesc puppetDescription) -> (Rococo.Puppet.IPuppet puppet)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetsGetFirstPuppetId, nullptr, ("IPuppetsGetFirstPuppetId (Pointer hObject) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetsFindPuppetById, nullptr, ("IPuppetsFindPuppetById (Pointer hObject)(Int64 id) -> (Rococo.Puppet.IPuppet puppet)"));
		ss.AddNativeCall(ns, NativeRococoPuppetIPuppetsGetNextPuppetId, nullptr, ("IPuppetsGetNextPuppetId (Pointer hObject)(Int64 id) -> (Int64 nextId)"));
	}
}}

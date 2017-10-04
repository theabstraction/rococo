// BennyHill generated Sexy native functions for Rococo::ILabelPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
	void NativeRococoILabelPaneBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ILabelPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPane* base = _pObject->Base();
		_offset += sizeof(CReflectedClass*);
		auto& _baseStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPane"), SEXTEXT("ProxyIPane"), _nce.ss);
		CReflectedClass* _sxybase = _nce.ss.Represent(_baseStruct, base);
		WriteOutput(&_sxybase->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { 
	void AddNativeCalls_RococoILabelPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ILabelPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoILabelPaneSetAlignment, nullptr, SEXTEXT("ILabelPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
		ss.AddNativeCall(ns, NativeRococoILabelPaneBase, nullptr, SEXTEXT("ILabelPaneBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}
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

		int32 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignLeftEdges(x, preserveSpan);
	}
	void NativeRococoIPaneAlignRightEdges(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveSpan;
		_offset += sizeof(preserveSpan);
		ReadInput(preserveSpan, _sf, -_offset);

		int32 x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		Rococo::IPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AlignRightEdges(x, preserveSpan);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk1, nullptr, SEXTEXT("IPaneSetColourBk1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourBk2, nullptr, SEXTEXT("IPaneSetColourBk2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge1, nullptr, SEXTEXT("IPaneSetColourEdge1 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourEdge2, nullptr, SEXTEXT("IPaneSetColourEdge2 (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetColourFont, nullptr, SEXTEXT("IPaneSetColourFont (Pointer hObject)(Int32 normal)(Int32 hilight) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneIsVisible, nullptr, SEXTEXT("IPaneIsVisible (Pointer hObject) -> (Bool isVisible)"));
		ss.AddNativeCall(ns, NativeRococoIPaneIsNormalized, nullptr, SEXTEXT("IPaneIsNormalized (Pointer hObject) -> (Bool isNormalized)"));
		ss.AddNativeCall(ns, NativeRococoIPaneSetVisible, nullptr, SEXTEXT("IPaneSetVisible (Pointer hObject)(Bool isVisible) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneGetRect, nullptr, SEXTEXT("IPaneGetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetRect, nullptr, SEXTEXT("IPaneSetRect (Pointer hObject)(Sys.Maths.Recti rect) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignLeftEdges, nullptr, SEXTEXT("IPaneAlignLeftEdges (Pointer hObject)(Int32 x)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneAlignRightEdges, nullptr, SEXTEXT("IPaneAlignRightEdges (Pointer hObject)(Int32 x)(Bool preserveSpan) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneLayoutVertically, nullptr, SEXTEXT("IPaneLayoutVertically (Pointer hObject)(Int32 vertBorder)(Int32 vertSpacing) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetCommand, nullptr, SEXTEXT("IPaneSetCommand (Pointer hObject)(Int32 stateIndex)(Bool deferAction)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeRococoIPaneSetPopulator, nullptr, SEXTEXT("IPaneSetPopulator (Pointer hObject)(Int32 stateIndex)(Sys.Type.IString populatorName) -> "));
	}
}
// BennyHill generated Sexy native functions for Rococo::Graphics::IMeshBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoGraphicsIMeshBuilderBegin(NativeCallEnvironment& _nce)
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
		_pObject->Begin(fqName);
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
	void NativeRococoGraphicsIMeshBuilderEnd(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->End();
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsMeshBuilder, _nceContext, SEXTEXT("GetHandleForIMeshBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderBegin, nullptr, SEXTEXT("IMeshBuilderBegin (Pointer hObject)(Sys.Type.IString fqName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderAddTriangle, nullptr, SEXTEXT("IMeshBuilderAddTriangle (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderEnd, nullptr, SEXTEXT("IMeshBuilderEnd (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderClear, nullptr, SEXTEXT("IMeshBuilderClear (Pointer hObject) -> "));
	}
}}
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
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPaneContainer"), SEXTEXT("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header._vTables[0], _sf, -_offset);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoPaneBuilder, _nceContext, SEXTEXT("GetHandleForIPaneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIPaneBuilderRoot, nullptr, SEXTEXT("IPaneBuilderRoot (Pointer hObject) -> (Rococo.IPaneContainer container)"));
	}
}
// BennyHill generated Sexy native functions for Rococo::IPaneContainer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPaneContainer"), SEXTEXT("ProxyIPaneContainer"), _nce.ss);
		CReflectedClass* _sxycontainer = _nce.ss.Represent(_containerStruct, container);
		WriteOutput(&_sxycontainer->header._vTables[0], _sf, -_offset);
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
		auto& _labelStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("ILabelPane"), SEXTEXT("ProxyILabelPane"), _nce.ss);
		CReflectedClass* _sxylabel = _nce.ss.Represent(_labelStruct, label);
		WriteOutput(&_sxylabel->header._vTables[0], _sf, -_offset);
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
		auto& _sliderStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("ISlider"), SEXTEXT("ProxyISlider"), _nce.ss);
		CReflectedClass* _sxyslider = _nce.ss.Represent(_sliderStruct, slider);
		WriteOutput(&_sxyslider->header._vTables[0], _sf, -_offset);
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
		auto& _textBoxStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("ITextOutputPane"), SEXTEXT("ProxyITextOutputPane"), _nce.ss);
		CReflectedClass* _sxytextBox = _nce.ss.Represent(_textBoxStruct, textBox);
		WriteOutput(&_sxytextBox->header._vTables[0], _sf, -_offset);
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
		auto& _radioStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IRadioButton"), SEXTEXT("ProxyIRadioButton"), _nce.ss);
		CReflectedClass* _sxyradio = _nce.ss.Represent(_radioStruct, radio);
		WriteOutput(&_sxyradio->header._vTables[0], _sf, -_offset);
	}
	void NativeRococoIPaneContainerBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IPaneContainer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPane* base = _pObject->Base();
		_offset += sizeof(CReflectedClass*);
		auto& _baseStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPane"), SEXTEXT("ProxyIPane"), _nce.ss);
		CReflectedClass* _sxybase = _nce.ss.Represent(_baseStruct, base);
		WriteOutput(&_sxybase->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { 
	void AddNativeCalls_RococoIPaneContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::IPaneContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddContainer, nullptr, SEXTEXT("IPaneContainerAddContainer (Pointer hObject)(Sys.Maths.Recti rect) -> (Rococo.IPaneContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddLabel, nullptr, SEXTEXT("IPaneContainerAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddSlider, nullptr, SEXTEXT("IPaneContainerAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTextOutput, nullptr, SEXTEXT("IPaneContainerAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddRadioButton, nullptr, SEXTEXT("IPaneContainerAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerBase, nullptr, SEXTEXT("IPaneContainerBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}
// BennyHill generated Sexy native functions for Rococo::IRadioButton 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
	void NativeRococoIRadioButtonBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IRadioButton* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPane* base = _pObject->Base();
		_offset += sizeof(CReflectedClass*);
		auto& _baseStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPane"), SEXTEXT("ProxyIPane"), _nce.ss);
		CReflectedClass* _sxybase = _nce.ss.Represent(_baseStruct, base);
		WriteOutput(&_sxybase->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { 
	void AddNativeCalls_RococoIRadioButton(Rococo::Script::IPublicScriptSystem& ss, Rococo::IRadioButton* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonSetAlignment, nullptr, SEXTEXT("IRadioButtonSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
		ss.AddNativeCall(ns, NativeRococoIRadioButtonBase, nullptr, SEXTEXT("IRadioButtonBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}
// BennyHill generated Sexy native functions for Rococo::ISlider 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoISliderBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ISlider* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPane* base = _pObject->Base();
		_offset += sizeof(CReflectedClass*);
		auto& _baseStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPane"), SEXTEXT("ProxyIPane"), _nce.ss);
		CReflectedClass* _sxybase = _nce.ss.Represent(_baseStruct, base);
		WriteOutput(&_sxybase->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { 
	void AddNativeCalls_RococoISlider(Rococo::Script::IPublicScriptSystem& ss, Rococo::ISlider* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoISliderBase, nullptr, SEXTEXT("ISliderBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}
// BennyHill generated Sexy native functions for Rococo::ITextOutputPane 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
	void NativeRococoITextOutputPaneBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITextOutputPane* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		Rococo::IPane* base = _pObject->Base();
		_offset += sizeof(CReflectedClass*);
		auto& _baseStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IPane"), SEXTEXT("ProxyIPane"), _nce.ss);
		CReflectedClass* _sxybase = _nce.ss.Represent(_baseStruct, base);
		WriteOutput(&_sxybase->header._vTables[0], _sf, -_offset);
	}

}

namespace Rococo { 
	void AddNativeCalls_RococoITextOutputPane(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITextOutputPane* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneSetAlignment, nullptr, SEXTEXT("ITextOutputPaneSetAlignment (Pointer hObject)(Int32 horz)(Int32 vert)(Int32 paddingX)(Int32 paddingY) -> "));
		ss.AddNativeCall(ns, NativeRococoITextOutputPaneBase, nullptr, SEXTEXT("ITextOutputPaneBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}

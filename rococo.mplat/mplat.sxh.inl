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
}}// Rococo.Graphics.OrientationFlags

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
		IString* _texture;
		ReadInput(_texture, _sf, -_offset);
		fstring texture { _texture->buffer, _texture->length };


		_offset += sizeof(IString*);
		IString* _modelName;
		ReadInput(_modelName, _sf, -_offset);
		fstring modelName { _modelName->buffer, _modelName->length };


		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddBody(modelName, texture, *model, *scale, parentId);
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

		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		Matrix4x4* model;
		_offset += sizeof(model);
		ReadInput(model, _sf, -_offset);

		Rococo::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddGhost(*model, *scale, parentId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesInstances, _nceContext, SEXTEXT("GetHandleForIInstances0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddBody, nullptr, SEXTEXT("IInstancesAddBody (Pointer hObject)(Sys.Type.IString modelName)(Sys.Type.IString texture)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddGhost, nullptr, SEXTEXT("IInstancesAddGhost (Pointer hObject)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesGetScale, nullptr, SEXTEXT("IInstancesGetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesSetScale, nullptr, SEXTEXT("IInstancesSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesTryGetModelToWorldMatrix, nullptr, SEXTEXT("IInstancesTryGetModelToWorldMatrix (Pointer hObject)(Int64 entityId)(Sys.Maths.Matrix4x4 position) -> (Bool existant)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesClear, nullptr, SEXTEXT("IInstancesClear (Pointer hObject) -> "));
	}
}}
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
	void NativeRococoIKeyboardSetKeyName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 scancode;
		_offset += sizeof(scancode);
		ReadInput(scancode, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Rococo::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetKeyName(name, scancode);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoKeyboard, _nceContext, SEXTEXT("GetHandleForIKeyboard0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIKeyboardClearActions, nullptr, SEXTEXT("IKeyboardClearActions (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardSetKeyName, nullptr, SEXTEXT("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 scancode) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardBindAction, nullptr, SEXTEXT("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "));
	}
}
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoEntitiesMobiles, _nceContext, SEXTEXT("GetHandleForIMobiles0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesLink, nullptr, SEXTEXT("IMobilesLink (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesGetAngles, nullptr, SEXTEXT("IMobilesGetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIMobilesSetAngles, nullptr, SEXTEXT("IMobilesSetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
	}
}}
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
	void NativeRococoGraphicsISceneBuilderSetClearColour(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
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
		_pObject->SetClearColour(red, green, blue);
	}
	void NativeRococoGraphicsISceneBuilderSetSunDirection(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* sun;
		_offset += sizeof(sun);
		ReadInput(sun, _sf, -_offset);

		Rococo::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSunDirection(*sun);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSceneBuilder, _nceContext, SEXTEXT("GetHandleForISceneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderAddStatics, nullptr, SEXTEXT("ISceneBuilderAddStatics (Pointer hObject)(Int64 entityId) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderClear, nullptr, SEXTEXT("ISceneBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetClearColour, nullptr, SEXTEXT("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetSunDirection, nullptr, SEXTEXT("ISceneBuilderSetSunDirection (Pointer hObject)(Sys.Maths.Vec3 sun) -> "));
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
	void NativeRococoGraphicsICameraSetProjection(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* proj;
		_offset += sizeof(proj);
		ReadInput(proj, _sf, -_offset);

		Rococo::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetProjection(*proj);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsCamera, _nceContext, SEXTEXT("GetHandleForICamera0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraClear, nullptr, SEXTEXT("ICameraClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetRHProjection, nullptr, SEXTEXT("ICameraSetRHProjection (Pointer hObject)(Sys.Maths.Degrees fov)(Float32 near)(Float32 far) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetProjection, nullptr, SEXTEXT("ICameraSetProjection (Pointer hObject)(Sys.Maths.Matrix4x4 proj) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetPosition, nullptr, SEXTEXT("ICameraSetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraSetOrientation, nullptr, SEXTEXT("ICameraSetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraFollowEntity, nullptr, SEXTEXT("ICameraFollowEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraMoveToEntity, nullptr, SEXTEXT("ICameraMoveToEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateWithEntity, nullptr, SEXTEXT("ICameraOrientateWithEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraOrientateToEntity, nullptr, SEXTEXT("ICameraOrientateToEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetPosition, nullptr, SEXTEXT("ICameraGetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetOrientation, nullptr, SEXTEXT("ICameraGetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorld, nullptr, SEXTEXT("ICameraGetWorld (Pointer hObject)(Sys.Maths.Matrix4x4 world) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraGetWorldAndProj, nullptr, SEXTEXT("ICameraGetWorldAndProj (Pointer hObject)(Sys.Maths.Matrix4x4 worldAndProj) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsICameraAspectRatio, nullptr, SEXTEXT("ICameraAspectRatio (Pointer hObject) -> (Float32 widthOverHeight)"));
	}
}}
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

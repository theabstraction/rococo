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
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddBody, nullptr, SEXTEXT("IInstancesAddBody (Pointer hObject)(Sys.Type.IString modelName)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesAddGhost, nullptr, SEXTEXT("IInstancesAddGhost (Pointer hObject)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesDelete, nullptr, SEXTEXT("IInstancesDelete (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeRococoEntitiesIInstancesLoadMaterialArray, nullptr, SEXTEXT("IInstancesLoadMaterialArray (Pointer hObject)(Sys.Type.IString folder)(Int32 txWidth) -> "));
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoKeyboard, _nceContext, SEXTEXT("GetHandleForIKeyboard0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIKeyboardClearActions, nullptr, SEXTEXT("IKeyboardClearActions (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardSetKeyName, nullptr, SEXTEXT("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 vkeyCode) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardBindAction, nullptr, SEXTEXT("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "));
		ss.AddNativeCall(ns, NativeRococoIKeyboardSaveCppHeader, nullptr, SEXTEXT("IKeyboardSaveCppHeader (Pointer hObject) -> "));
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoConfig, _nceContext, SEXTEXT("GetHandleForIConfig0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoIConfigInt, nullptr, SEXTEXT("IConfigInt (Pointer hObject)(Sys.Type.IString name)(Int32 value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigFloat, nullptr, SEXTEXT("IConfigFloat (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigBool, nullptr, SEXTEXT("IConfigBool (Pointer hObject)(Sys.Type.IString name)(Bool value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigText, nullptr, SEXTEXT("IConfigText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString value) -> "));
		ss.AddNativeCall(ns, NativeRococoIConfigGetInt, nullptr, SEXTEXT("IConfigGetInt (Pointer hObject)(Sys.Type.IString name) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetFloat, nullptr, SEXTEXT("IConfigGetFloat (Pointer hObject)(Sys.Type.IString name) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetBool, nullptr, SEXTEXT("IConfigGetBool (Pointer hObject)(Sys.Type.IString name) -> (Bool value)"));
		ss.AddNativeCall(ns, NativeRococoIConfigGetText, nullptr, SEXTEXT("IConfigGetText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IStringBuilder text) -> "));
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsSprites, _nceContext, SEXTEXT("GetHandleForISprites0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesClear, nullptr, SEXTEXT("ISpritesClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesAddSprite, nullptr, SEXTEXT("ISpritesAddSprite (Pointer hObject)(Sys.Type.IString resourceName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesAddEachSpriteInDirectory, nullptr, SEXTEXT("ISpritesAddEachSpriteInDirectory (Pointer hObject)(Sys.Type.IString directoryName) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISpritesLoadAllSprites, nullptr, SEXTEXT("ISpritesLoadAllSprites (Pointer hObject) -> "));
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
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetClearColour, nullptr, SEXTEXT("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue)(Float32 alpha) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsISceneBuilderSetLight, nullptr, SEXTEXT("ISceneBuilderSetLight (Pointer hObject)(Rococo.LightSpec light)(Int32 index) -> "));
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
		auto& _containerStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("ITabContainer"), SEXTEXT("ProxyITabContainer"), _nce.ss);
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
		auto& _scrollerStruct = Rococo::Helpers::GetDefaultProxy(SEXTEXT("Rococo"),SEXTEXT("IScroller"), SEXTEXT("ProxyIScroller"), _nce.ss);
		CReflectedClass* _sxyscroller = _nce.ss.Represent(_scrollerStruct, scroller);
		WriteOutput(&_sxyscroller->header._vTables[0], _sf, -_offset);
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
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTabContainer, nullptr, SEXTEXT("IPaneContainerAddTabContainer (Pointer hObject)(Int32 tabHeight)(Int32 fontIndex)(Sys.Maths.Recti rect) -> (Rococo.ITabContainer container)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddLabel, nullptr, SEXTEXT("IPaneContainerAddLabel (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect) -> (Rococo.ILabelPane label)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddSlider, nullptr, SEXTEXT("IPaneContainerAddSlider (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Maths.Recti rect)(Float32 minValue)(Float32 maxValue) -> (Rococo.ISlider slider)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddScroller, nullptr, SEXTEXT("IPaneContainerAddScroller (Pointer hObject)(Sys.Type.IString key)(Sys.Maths.Recti rect)(Bool isVertical) -> (Rococo.IScroller scroller)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddTextOutput, nullptr, SEXTEXT("IPaneContainerAddTextOutput (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString key)(Sys.Maths.Recti rect) -> (Rococo.ITextOutputPane textBox)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerAddRadioButton, nullptr, SEXTEXT("IPaneContainerAddRadioButton (Pointer hObject)(Int32 fontIndex)(Sys.Type.IString text)(Sys.Type.IString key)(Sys.Type.IString value)(Sys.Maths.Recti rect) -> (Rococo.IRadioButton radio)"));
		ss.AddNativeCall(ns, NativeRococoIPaneContainerBase, nullptr, SEXTEXT("IPaneContainerBase (Pointer hObject) -> (Rococo.IPane base)"));
	}
}
// BennyHill generated Sexy native functions for Rococo::ITabContainer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

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
	void NativeRococoITabContainerBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::ITabContainer* _pObject;
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
	void AddNativeCalls_RococoITabContainer(Rococo::Script::IPublicScriptSystem& ss, Rococo::ITabContainer* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoITabContainerAddTab, nullptr, SEXTEXT("ITabContainerAddTab (Pointer hObject)(Int32 tabWidth)(Sys.Type.IString caption)(Sys.Type.IString panelText) -> "));
		ss.AddNativeCall(ns, NativeRococoITabContainerBase, nullptr, SEXTEXT("ITabContainerBase (Pointer hObject) -> (Rococo.IPane base)"));
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
// BennyHill generated Sexy native functions for Rococo::IScroller 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeRococoIScrollerBase(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::IScroller* _pObject;
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
	void AddNativeCalls_RococoIScroller(Rococo::Script::IPublicScriptSystem& ss, Rococo::IScroller* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Native"));
		ss.AddNativeCall(ns, NativeRococoIScrollerBase, nullptr, SEXTEXT("IScrollerBase (Pointer hObject) -> (Rococo.IPane base)"));
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
		float cementThicknessRatio;
		_offset += sizeof(cementThicknessRatio);
		ReadInput(cementThicknessRatio, _sf, -_offset);

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
		_pObject->GetStackBondedBrick(i, j, *q, cementThicknessRatio);
	}
	void NativeRococoGraphicsIFieldTesselatorGetStretchBondedBrick(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float cementThicknessRatio;
		_offset += sizeof(cementThicknessRatio);
		ReadInput(cementThicknessRatio, _sf, -_offset);

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
		_pObject->GetStretchBondedBrick(i, j, *q, *top, *left, *right, *bottom, cementThicknessRatio);
	}
	void NativeRococoGraphicsIFieldTesselatorGetBrickJoinRight(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float cementThicknessRatio;
		_offset += sizeof(cementThicknessRatio);
		ReadInput(cementThicknessRatio, _sf, -_offset);

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
		_pObject->GetBrickJoinRight(i, j, *q, cementThicknessRatio);
	}
	void NativeRococoGraphicsIFieldTesselatorGetBrickBedTop(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float cementThicknessRatio;
		_offset += sizeof(cementThicknessRatio);
		ReadInput(cementThicknessRatio, _sf, -_offset);

		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 row;
		_offset += sizeof(row);
		ReadInput(row, _sf, -_offset);

		Rococo::Graphics::IFieldTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetBrickBedTop(row, *q, cementThicknessRatio);
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsFieldTesselator, _nceContext, SEXTEXT("GetHandleForIFieldTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorDestruct, nullptr, SEXTEXT("IFieldTesselatorDestruct (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByFixedCellWidth, nullptr, SEXTEXT("IFieldTesselatorInitByFixedCellWidth (Pointer hObject)(Sys.Maths.Quadf positions)(Float32 maxCellWidth)(Float32 maxCellHeight) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorInitByDivisions, nullptr, SEXTEXT("IFieldTesselatorInitByDivisions (Pointer hObject)(Sys.Maths.Quadf positions)(Int32 xDivs)(Int32 yDivs) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorSetUV, nullptr, SEXTEXT("IFieldTesselatorSetUV (Pointer hObject)(Sys.Maths.Vec2 uvA)(Sys.Maths.Vec2 uvC) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfColumns, nullptr, SEXTEXT("IFieldTesselatorNumberOfColumns (Pointer hObject) -> (Int32 cols)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorNumberOfRows, nullptr, SEXTEXT("IFieldTesselatorNumberOfRows (Pointer hObject) -> (Int32 rows)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetFlatSubQuad, nullptr, SEXTEXT("IFieldTesselatorGetFlatSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices subQuad) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetPerturbedSubQuad, nullptr, SEXTEXT("IFieldTesselatorGetPerturbedSubQuad (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStackBondedBrick, nullptr, SEXTEXT("IFieldTesselatorGetStackBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Float32 cementThicknessRatio) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetStretchBondedBrick, nullptr, SEXTEXT("IFieldTesselatorGetStretchBondedBrick (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Rococo.QuadVertices top)(Rococo.QuadVertices left)(Rococo.QuadVertices right)(Rococo.QuadVertices bottom)(Float32 cementThicknessRatio) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickJoinRight, nullptr, SEXTEXT("IFieldTesselatorGetBrickJoinRight (Pointer hObject)(Int32 i)(Int32 j)(Rococo.QuadVertices q)(Float32 cementThicknessRatio) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBrickBedTop, nullptr, SEXTEXT("IFieldTesselatorGetBrickBedTop (Pointer hObject)(Int32 row)(Rococo.QuadVertices q)(Float32 cementThicknessRatio) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorPerturbField, nullptr, SEXTEXT("IFieldTesselatorPerturbField (Pointer hObject)(Int32 i)(Int32 j)(Float32 dH) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorLevelField, nullptr, SEXTEXT("IFieldTesselatorLevelField (Pointer hObject)(Int32 i0)(Int32 j0)(Int32 i1)(Int32 j1)(Float32 dH) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorRandomizeField, nullptr, SEXTEXT("IFieldTesselatorRandomizeField (Pointer hObject)(Int32 i)(Int32 j)(Float32 minValue)(Float32 maxValue) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIFieldTesselatorGetBasis, nullptr, SEXTEXT("IFieldTesselatorGetBasis (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "));
	}
}}
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
		ss.AddNativeCall(ns, NativeRococoGraphicsIMeshBuilderDelete, nullptr, SEXTEXT("IMeshBuilderDelete (Pointer hObject)(Sys.Type.IString fqName) -> "));
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
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Rococo.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForRococoGraphicsRimTesselator, _nceContext, SEXTEXT("GetHandleForIRimTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPoint, nullptr, SEXTEXT("IRimTesselatorAddPoint (Pointer hObject)(Sys.Maths.Vec2 p) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorAddPointXY, nullptr, SEXTEXT("IRimTesselatorAddPointXY (Pointer hObject)(Float32 x)(Float32 y) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorCloseLoop, nullptr, SEXTEXT("IRimTesselatorCloseLoop (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorMakeElipse, nullptr, SEXTEXT("IRimTesselatorMakeElipse (Pointer hObject)(Int32 numberOfSides)(Float32 sx)(Float32 sy) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClear, nullptr, SEXTEXT("IRimTesselatorClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorClearFaces, nullptr, SEXTEXT("IRimTesselatorClearFaces (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorScale, nullptr, SEXTEXT("IRimTesselatorScale (Pointer hObject)(Float32 sx)(Float32 sy) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorPerimeterVertices, nullptr, SEXTEXT("IRimTesselatorPerimeterVertices (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimQuad, nullptr, SEXTEXT("IRimTesselatorGetRimQuad (Pointer hObject)(Float32 zHigh)(Float32 zLow)(Int32 index)(Sys.Maths.Quadf quad) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetRimVertex, nullptr, SEXTEXT("IRimTesselatorGetRimVertex (Pointer hObject)(Int32 index)(Sys.Maths.Vec2 p) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorTesselateUniform, nullptr, SEXTEXT("IRimTesselatorTesselateUniform (Pointer hObject) -> (Int32 nTrianglesPerFace)"));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetBottomTriangle, nullptr, SEXTEXT("IRimTesselatorGetBottomTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorGetTopTriangle, nullptr, SEXTEXT("IRimTesselatorGetTopTriangle (Pointer hObject)(Int32 index)(Sys.Maths.Triangle pos)(Sys.Maths.Triangle2d uv)(Float32 z) -> "));
		ss.AddNativeCall(ns, NativeRococoGraphicsIRimTesselatorSetTransform, nullptr, SEXTEXT("IRimTesselatorSetTransform (Pointer hObject)(Sys.Maths.Matrix4x4 transform) -> "));
	}
}}
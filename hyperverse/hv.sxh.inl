// BennyHill generated Sexy native functions for HV::ISprites 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISpritesClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVISpritesAddSprite(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };


		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddSprite(resourceName);
	}
	void NativeHVISpritesAddEachSpriteInDirectory(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _directoryName;
		ReadInput(_directoryName, _sf, -_offset);
		fstring directoryName { _directoryName->buffer, _directoryName->length };


		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddEachSpriteInDirectory(directoryName);
	}
	void NativeHVISpritesLoadAllSprites(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadAllSprites();
	}

	void NativeGetHandleForHVSprites(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* nceContext = reinterpret_cast<HV::ISprites*>(_nce.context);
		// Uses: HV::ISprites* FactoryConstructHVSprites(HV::ISprites* _context);
		HV::ISprites* pObject = FactoryConstructHVSprites(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISprites(Rococo::Script::IPublicScriptSystem& ss, HV::ISprites* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSprites, _nceContext, SEXTEXT("GetHandleForISprites0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISpritesClear, nullptr, SEXTEXT("ISpritesClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesAddSprite, nullptr, SEXTEXT("ISpritesAddSprite (Pointer hObject)(Sys.Type.IString resourceName) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesAddEachSpriteInDirectory, nullptr, SEXTEXT("ISpritesAddEachSpriteInDirectory (Pointer hObject)(Sys.Type.IString directoryName) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesLoadAllSprites, nullptr, SEXTEXT("ISpritesLoadAllSprites (Pointer hObject) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::Graphics::ISceneBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVGraphicsISceneBuilderAddStatics(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddStatics(entityId);
	}
	void NativeHVGraphicsISceneBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVGraphicsISceneBuilderSetClearColour(NativeCallEnvironment& _nce)
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

		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetClearColour(red, green, blue);
	}
	void NativeHVGraphicsISceneBuilderSetSunDirection(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* sun;
		_offset += sizeof(sun);
		ReadInput(sun, _sf, -_offset);

		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSunDirection(*sun);
	}

	void NativeGetHandleForHVGraphicsSceneBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ISceneBuilder* nceContext = reinterpret_cast<HV::Graphics::ISceneBuilder*>(_nce.context);
		// Uses: HV::Graphics::ISceneBuilder* FactoryConstructHVGraphicsSceneBuilder(HV::Graphics::ISceneBuilder* _context);
		HV::Graphics::ISceneBuilder* pObject = FactoryConstructHVGraphicsSceneBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Graphics { 
	void AddNativeCalls_HVGraphicsISceneBuilder(Rococo::Script::IPublicScriptSystem& ss, HV::Graphics::ISceneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsSceneBuilder, _nceContext, SEXTEXT("GetHandleForISceneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderAddStatics, nullptr, SEXTEXT("ISceneBuilderAddStatics (Pointer hObject)(Int64 entityId) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderClear, nullptr, SEXTEXT("ISceneBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderSetClearColour, nullptr, SEXTEXT("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderSetSunDirection, nullptr, SEXTEXT("ISceneBuilderSetSunDirection (Pointer hObject)(Sys.Maths.Vec3 sun) -> "));
	}
}}
// BennyHill generated Sexy native functions for HV::IPlayer 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIPlayerSetPlayerEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPlayerEntity(id);
	}
	void NativeHVIPlayerGetPlayerEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->GetPlayerEntity();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}

	void NativeGetHandleForHVGraphicsPlayer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::IPlayerSupervisor* nceContext = reinterpret_cast<HV::IPlayerSupervisor*>(_nce.context);
		// Uses: HV::IPlayer* FactoryConstructHVGraphicsPlayer(HV::IPlayerSupervisor* _context, int32 _index);
		HV::IPlayer* pObject = FactoryConstructHVGraphicsPlayer(nceContext, index);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIPlayer(Rococo::Script::IPublicScriptSystem& ss, HV::IPlayerSupervisor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsPlayer, _nceContext, SEXTEXT("GetHandleForIPlayer0 (Int32 index) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIPlayerSetPlayerEntity, nullptr, SEXTEXT("IPlayerSetPlayerEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerGetPlayerEntity, nullptr, SEXTEXT("IPlayerGetPlayerEntity (Pointer hObject) -> (Int64 id)"));
	}
}
// BennyHill generated Sexy native functions for HV::IKeyboard 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIKeyboardClearActions(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearActions();
	}
	void NativeHVIKeyboardSetKeyName(NativeCallEnvironment& _nce)
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


		HV::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetKeyName(name, scancode);
	}
	void NativeHVIKeyboardBindAction(NativeCallEnvironment& _nce)
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


		HV::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->BindAction(keyName, actionName);
	}

	void NativeGetHandleForHVKeyboard(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IKeyboard* nceContext = reinterpret_cast<HV::IKeyboard*>(_nce.context);
		// Uses: HV::IKeyboard* FactoryConstructHVKeyboard(HV::IKeyboard* _context);
		HV::IKeyboard* pObject = FactoryConstructHVKeyboard(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIKeyboard(Rococo::Script::IPublicScriptSystem& ss, HV::IKeyboard* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVKeyboard, _nceContext, SEXTEXT("GetHandleForIKeyboard0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIKeyboardClearActions, nullptr, SEXTEXT("IKeyboardClearActions (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVIKeyboardSetKeyName, nullptr, SEXTEXT("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 scancode) -> "));
		ss.AddNativeCall(ns, NativeHVIKeyboardBindAction, nullptr, SEXTEXT("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::IConfig 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIConfigInt(NativeCallEnvironment& _nce)
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


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Int(name, value);
	}
	void NativeHVIConfigFloat(NativeCallEnvironment& _nce)
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


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Float(name, value);
	}
	void NativeHVIConfigBool(NativeCallEnvironment& _nce)
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


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Bool(name, value);
	}
	void NativeHVIConfigText(NativeCallEnvironment& _nce)
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


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Text(name, value);
	}
	void NativeHVIConfigGetInt(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 value = _pObject->GetInt(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeHVIConfigGetFloat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->GetFloat(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeHVIConfigGetBool(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 value = _pObject->GetBool(name);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeHVIConfigGetText(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable*);
		VirtualTable* text;
		ReadInput(text, _sf, -_offset);
		Rococo::Helpers::StringPopulator _textPopulator(_nce, text);
		_offset += sizeof(IString*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		HV::IConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetText(name, _textPopulator);
	}

	void NativeGetHandleForHVConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IConfig* nceContext = reinterpret_cast<HV::IConfig*>(_nce.context);
		// Uses: HV::IConfig* FactoryConstructHVConfig(HV::IConfig* _context);
		HV::IConfig* pObject = FactoryConstructHVConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIConfig(Rococo::Script::IPublicScriptSystem& ss, HV::IConfig* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVConfig, _nceContext, SEXTEXT("GetHandleForIConfig0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIConfigInt, nullptr, SEXTEXT("IConfigInt (Pointer hObject)(Sys.Type.IString name)(Int32 value) -> "));
		ss.AddNativeCall(ns, NativeHVIConfigFloat, nullptr, SEXTEXT("IConfigFloat (Pointer hObject)(Sys.Type.IString name)(Float32 value) -> "));
		ss.AddNativeCall(ns, NativeHVIConfigBool, nullptr, SEXTEXT("IConfigBool (Pointer hObject)(Sys.Type.IString name)(Bool value) -> "));
		ss.AddNativeCall(ns, NativeHVIConfigText, nullptr, SEXTEXT("IConfigText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IString value) -> "));
		ss.AddNativeCall(ns, NativeHVIConfigGetInt, nullptr, SEXTEXT("IConfigGetInt (Pointer hObject)(Sys.Type.IString name) -> (Int32 value)"));
		ss.AddNativeCall(ns, NativeHVIConfigGetFloat, nullptr, SEXTEXT("IConfigGetFloat (Pointer hObject)(Sys.Type.IString name) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeHVIConfigGetBool, nullptr, SEXTEXT("IConfigGetBool (Pointer hObject)(Sys.Type.IString name) -> (Bool value)"));
		ss.AddNativeCall(ns, NativeHVIConfigGetText, nullptr, SEXTEXT("IConfigGetText (Pointer hObject)(Sys.Type.IString name)(Sys.Type.IStringBuilder text) -> "));
	}
}

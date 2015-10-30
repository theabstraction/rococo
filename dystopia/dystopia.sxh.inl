// BennyHill generated Sexy native functions for Dystopia::IMeshes 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeDystopiaIMeshesLoad(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		_offset += sizeof(void*);

		IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName { _resourceName->buffer, _resourceName->length };

		Dystopia::IMeshes* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Load(resourceName, editorId);
	}

	void NativeGetHandleForDystopiaMeshesGetMeshes(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		Dystopia::IMeshes* nceContext = reinterpret_cast<Dystopia::IMeshes*>(_nce.context);
		// Uses: Dystopia::IMeshes* FactoryConstructDystopiaMeshesGetMeshes(Dystopia::IMeshes* _context);
		Dystopia::IMeshes* pObject = FactoryConstructDystopiaMeshesGetMeshes(nceContext);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}
namespace Dystopia
{
	void AddNativeCalls_DystopiaIMeshes(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IMeshes* _nceContext)
	{

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Meshes.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaMeshesGetMeshes, _nceContext, SEXTEXT("GetHandleForIMeshes0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaIMeshesLoad, nullptr, SEXTEXT("IMeshesLoad (Pointer hObject)(Sys.Type.IString resourceName)(Int32 editorId) -> "));
	}

}
// BennyHill generated Sexy native functions for Dystopia::ILevelBuilder 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeDystopiaILevelBuilderAddEnemy(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		Matrix4x4* transform;
		_offset += sizeof(transform);

		ReadInput(transform, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddEnemy(*transform, editorId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddAlly(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		Matrix4x4* transform;
		_offset += sizeof(transform);

		ReadInput(transform, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddAlly(*transform, editorId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddRangedWeapon(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float flightTime;
		_offset += sizeof(flightTime);

		ReadInput(flightTime, _sf, -_offset);
		float muzzleVelocity;
		_offset += sizeof(muzzleVelocity);

		ReadInput(muzzleVelocity, _sf, -_offset);
		_offset += sizeof(void*);

		IString* _imageFile;
		ReadInput(_imageFile, _sf, -_offset);
		fstring imageFile { _imageFile->buffer, _imageFile->length };

		_offset += sizeof(void*);

		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };

		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		Matrix4x4* transform;
		_offset += sizeof(transform);

		ReadInput(transform, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddRangedWeapon(*transform, editorId, name, imageFile, muzzleVelocity, flightTime);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddSolid(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		Matrix4x4* transform;
		_offset += sizeof(transform);

		ReadInput(transform, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddSolid(*transform, editorId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeDystopiaILevelBuilderSetPlayerId(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY playerId;
		_offset += sizeof(playerId);

		ReadInput(playerId, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPlayerId(playerId);
	}

	void NativeGetHandleForDystopiaLevelsGetLevel(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		Dystopia::ILevel* nceContext = reinterpret_cast<Dystopia::ILevel*>(_nce.context);
		// Uses: Dystopia::ILevel* FactoryConstructDystopiaLevelsGetLevel(Dystopia::ILevel* _context);
		Dystopia::ILevelBuilder* pObject = FactoryConstructDystopiaLevelsGetLevel(nceContext);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}
namespace Dystopia
{
	void AddNativeCalls_DystopiaILevelBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::ILevel* _nceContext)
	{

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Levels.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaLevelsGetLevel, _nceContext, SEXTEXT("GetHandleForILevel0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddEnemy, nullptr, SEXTEXT("ILevelAddEnemy (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddAlly, nullptr, SEXTEXT("ILevelAddAlly (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddRangedWeapon, nullptr, SEXTEXT("ILevelAddRangedWeapon (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Int32 editorId)(Sys.Type.IString name)(Sys.Type.IString imageFile)(Float32 muzzleVelocity)(Float32 flightTime) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddSolid, nullptr, SEXTEXT("ILevelAddSolid (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderClear, nullptr, SEXTEXT("ILevelClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetPlayerId, nullptr, SEXTEXT("ILevelSetPlayerId (Pointer hObject)(Int64 playerId) -> "));
	}

}
// BennyHill generated Sexy native functions for Dystopia::IGui 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeDystopiaIGuiShowDialogBox(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);

		IString* _buttons;
		ReadInput(_buttons, _sf, -_offset);
		fstring buttons { _buttons->buffer, _buttons->length };

		_offset += sizeof(void*);

		IString* _message;
		ReadInput(_message, _sf, -_offset);
		fstring message { _message->buffer, _message->length };

		_offset += sizeof(void*);

		IString* _title;
		ReadInput(_title, _sf, -_offset);
		fstring title { _title->buffer, _title->length };

		int32 hypzone;
		_offset += sizeof(hypzone);

		ReadInput(hypzone, _sf, -_offset);
		int32 retzone;
		_offset += sizeof(retzone);

		ReadInput(retzone, _sf, -_offset);
		Vec2i* span;
		_offset += sizeof(span);

		ReadInput(span, _sf, -_offset);
		Dystopia::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ShowDialogBox(*span, retzone, hypzone, title, message, buttons);
	}
	void NativeDystopiaIGuiAdd3DHint(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float duration;
		_offset += sizeof(duration);

		ReadInput(duration, _sf, -_offset);
		_offset += sizeof(void*);

		IString* _message;
		ReadInput(_message, _sf, -_offset);
		fstring message { _message->buffer, _message->length };

		Vec3* worldPos;
		_offset += sizeof(worldPos);

		ReadInput(worldPos, _sf, -_offset);
		Dystopia::IGui* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Add3DHint(*worldPos, message, duration);
	}

	void NativeGetHandleForDystopiaGuiGetGui(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		Dystopia::IGui* nceContext = reinterpret_cast<Dystopia::IGui*>(_nce.context);
		// Uses: Dystopia::IGui* FactoryConstructDystopiaGuiGetGui(Dystopia::IGui* _context);
		Dystopia::IGui* pObject = FactoryConstructDystopiaGuiGetGui(nceContext);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}
namespace Dystopia
{
	void AddNativeCalls_DystopiaIGui(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IGui* _nceContext)
	{

		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Gui.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaGuiGetGui, _nceContext, SEXTEXT("GetHandleForIGui0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaIGuiShowDialogBox, nullptr, SEXTEXT("IGuiShowDialogBox (Pointer hObject)(Sys.Maths.Vec2i span)(Int32 retzone)(Int32 hypzone)(Sys.Type.IString title)(Sys.Type.IString message)(Sys.Type.IString buttons) -> "));
		ss.AddNativeCall(ns, NativeDystopiaIGuiAdd3DHint, nullptr, SEXTEXT("IGuiAdd3DHint (Pointer hObject)(Sys.Maths.Vec3 worldPos)(Sys.Type.IString message)(Float32 duration) -> "));
	}

}

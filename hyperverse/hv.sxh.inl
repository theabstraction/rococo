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

	void NativeGetHandleForHVPlayer(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::IPlayerSupervisor* nceContext = reinterpret_cast<HV::IPlayerSupervisor*>(_nce.context);
		// Uses: HV::IPlayer* FactoryConstructHVPlayer(HV::IPlayerSupervisor* _context, int32 _index);
		HV::IPlayer* pObject = FactoryConstructHVPlayer(nceContext, index);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIPlayer(Rococo::Script::IPublicScriptSystem& ss, HV::IPlayerSupervisor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVPlayer, _nceContext, SEXTEXT("GetHandleForIPlayer0 (Int32 index) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIPlayerSetPlayerEntity, nullptr, SEXTEXT("IPlayerSetPlayerEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerGetPlayerEntity, nullptr, SEXTEXT("IPlayerGetPlayerEntity (Pointer hObject) -> (Int64 id)"));
	}
}
// BennyHill generated Sexy native functions for HV::ICorridor 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVICorridorGetSpan(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSpan(*span);
	}
	void NativeHVICorridorIsSloped(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 isSloped = _pObject->IsSloped();
		_offset += sizeof(isSloped);
		WriteOutput(isSloped, _sf, -_offset);
	}
	void NativeHVICorridorCentreComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _textureName;
		ReadInput(_textureName, _sf, -_offset);
		fstring textureName { _textureName->buffer, _textureName->length };


		_offset += sizeof(IString*);
		IString* _meshName;
		ReadInput(_meshName, _sf, -_offset);
		fstring meshName { _meshName->buffer, _meshName->length };


		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CentreComponent(componentName, meshName, textureName);
	}
	void NativeHVICorridorClearComponents(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearComponents(componentName);
	}
	void NativeHVICorridorGetComponentMeshName(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable**);
		VirtualTable** meshName;
		ReadInput(meshName, _sf, -_offset);
		Rococo::Helpers::StringPopulator _meshNamePopulator(_nce, meshName);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ICorridor* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetComponentMeshName(componentName, _meshNamePopulator);
	}

	void NativeGetHandleForHVCorridor(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ICorridor* nceContext = reinterpret_cast<HV::ICorridor*>(_nce.context);
		// Uses: HV::ICorridor* FactoryConstructHVCorridor(HV::ICorridor* _context);
		HV::ICorridor* pObject = FactoryConstructHVCorridor(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVICorridor(Rococo::Script::IPublicScriptSystem& ss, HV::ICorridor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVCorridor, _nceContext, SEXTEXT("GetHandleForICorridor0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVICorridorGetSpan, nullptr, SEXTEXT("ICorridorGetSpan (Pointer hObject)(Sys.Maths.Vec3 span) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorIsSloped, nullptr, SEXTEXT("ICorridorIsSloped (Pointer hObject) -> (Bool isSloped)"));
		ss.AddNativeCall(ns, NativeHVICorridorCentreComponent, nullptr, SEXTEXT("ICorridorCentreComponent (Pointer hObject)(Sys.Type.IString componentName)(Sys.Type.IString meshName)(Sys.Type.IString textureName) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorClearComponents, nullptr, SEXTEXT("ICorridorClearComponents (Pointer hObject)(Sys.Type.IString componentName) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorGetComponentMeshName, nullptr, SEXTEXT("ICorridorGetComponentMeshName (Pointer hObject)(Sys.Type.IString componentName)(Sys.Type.IStringBuilder meshName) -> "));
	}
}

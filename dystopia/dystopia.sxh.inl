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

		#pragma pack(push,1)
		struct _IString { Sexy::Compiler::VirtualTable* vTable; Sexy::int32 length; Sexy::csexstr buffer; };
		#pragma pack(pop)
		_offset += sizeof(void*);

		_IString* _resourceName;
		ReadInput(_resourceName, _sf, -_offset);
		fstring resourceName;
		resourceName.buffer = _resourceName->buffer;
		resourceName.length = _resourceName->length;

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
	void NativeDystopiaILevelBuilderAddEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_MESH editorId;
		_offset += sizeof(editorId);

		ReadInput(editorId, _sf, -_offset);
		Rococo::Matrix4x4* transform;
		_offset += sizeof(transform);

		ReadInput(transform, _sf, -_offset);
		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddEntity(*transform, editorId);
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
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddEntity, nullptr, SEXTEXT("ILevelAddEntity (Pointer hObject)(Sys.Maths.Matrix4x4 transform)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderClear, nullptr, SEXTEXT("ILevelClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetPlayerId, nullptr, SEXTEXT("ILevelSetPlayerId (Pointer hObject)(Int64 playerId) -> "));
	}

}

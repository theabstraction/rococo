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
// BennyHill generated Sexy native functions for HV::ISectorWallTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorWallTesselatorNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfSegments();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorWallTesselatorNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfGaps();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorWallTesselatorGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 ringIndex;
		_offset += sizeof(ringIndex);
		ReadInput(ringIndex, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(ringIndex, *segment);
	}
	void NativeHVISectorWallTesselatorGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 ringIndex;
		_offset += sizeof(ringIndex);
		ReadInput(ringIndex, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(ringIndex, *segment);
	}
	void NativeHVISectorWallTesselatorAddWallTriangle(NativeCallEnvironment& _nce)
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

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddWallTriangle(*a, *b, *c);
	}
	void NativeHVISectorWallTesselatorGetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentClass;
		ReadInput(_componentClass, _sf, -_offset);
		fstring componentClass { _componentClass->buffer, _componentClass->length };


		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}

	void NativeGetHandleForHVSectorWallTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* nceContext = reinterpret_cast<HV::ISectorWallTesselator*>(_nce.context);
		// Uses: HV::ISectorWallTesselator* FactoryConstructHVSectorWallTesselator(HV::ISectorWallTesselator* _context);
		HV::ISectorWallTesselator* pObject = FactoryConstructHVSectorWallTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorWallTesselator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorWallTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorWallTesselator, _nceContext, SEXTEXT("GetHandleForISectorWallTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfSegments, nullptr, SEXTEXT("ISectorWallTesselatorNumberOfSegments (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfGaps, nullptr, SEXTEXT("ISectorWallTesselatorNumberOfGaps (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetSegment, nullptr, SEXTEXT("ISectorWallTesselatorGetSegment (Pointer hObject)(Int32 ringIndex)(HV.WallSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetGap, nullptr, SEXTEXT("ISectorWallTesselatorGetGap (Pointer hObject)(Int32 ringIndex)(HV.GapSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorAddWallTriangle, nullptr, SEXTEXT("ISectorWallTesselatorAddWallTriangle (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetMaterial, nullptr, SEXTEXT("ISectorWallTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "));
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
		_pObject->CentreComponent(componentName, meshName);
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
		ss.AddNativeCall(ns, NativeHVICorridorCentreComponent, nullptr, SEXTEXT("ICorridorCentreComponent (Pointer hObject)(Sys.Type.IString componentName)(Sys.Type.IString meshName) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorClearComponents, nullptr, SEXTEXT("ICorridorClearComponents (Pointer hObject)(Sys.Type.IString componentName) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorGetComponentMeshName, nullptr, SEXTEXT("ICorridorGetComponentMeshName (Pointer hObject)(Sys.Type.IString componentName)(Sys.Type.IStringBuilder meshName) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::ISectorBuilder 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorBuilderAddVertex(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float y;
		_offset += sizeof(y);
		ReadInput(y, _sf, -_offset);

		float x;
		_offset += sizeof(x);
		ReadInput(x, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddVertex(x, y);
	}
	void NativeHVISectorBuilderClear(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVISectorBuilderCreateFromTemplate(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 height;
		_offset += sizeof(height);
		ReadInput(height, _sf, -_offset);

		int32 altitude;
		_offset += sizeof(altitude);
		ReadInput(altitude, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 id = _pObject->CreateFromTemplate(altitude, height);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorBuilderSetTemplateWallScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		boolean32 useScript;
		_offset += sizeof(useScript);
		ReadInput(useScript, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateWallScript(useScript, scriptName);
	}
	void NativeHVISectorBuilderSetTemplateDoorScript(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _scriptName;
		ReadInput(_scriptName, _sf, -_offset);
		fstring scriptName { _scriptName->buffer, _scriptName->length };


		boolean32 hasDoor;
		_offset += sizeof(hasDoor);
		ReadInput(hasDoor, _sf, -_offset);

		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateDoorScript(hasDoor, scriptName);
	}
	void NativeHVISectorBuilderSetTemplateMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _persistentId;
		ReadInput(_persistentId, _sf, -_offset);
		fstring persistentId { _persistentId->buffer, _persistentId->length };


		RGBAb colour;
		_offset += sizeof(colour);
		ReadInput(colour, _sf, -_offset);

		Rococo::Graphics::MaterialCategory cat;
		_offset += sizeof(cat);
		ReadInput(cat, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _bodyClass;
		ReadInput(_bodyClass, _sf, -_offset);
		fstring bodyClass { _bodyClass->buffer, _bodyClass->length };


		HV::ISectorBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetTemplateMaterial(bodyClass, cat, colour, persistentId);
	}

	void NativeGetHandleForHVSectors(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectors* nceContext = reinterpret_cast<HV::ISectors*>(_nce.context);
		// Uses: HV::ISectorBuilder* FactoryConstructHVSectors(HV::ISectors* _context);
		HV::ISectorBuilder* pObject = FactoryConstructHVSectors(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorBuilder(Rococo::Script::IPublicScriptSystem& ss, HV::ISectors* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectors, _nceContext, SEXTEXT("GetHandleForISectors0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorBuilderAddVertex, nullptr, SEXTEXT("ISectorsAddVertex (Pointer hObject)(Float32 x)(Float32 y) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderClear, nullptr, SEXTEXT("ISectorsClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderCreateFromTemplate, nullptr, SEXTEXT("ISectorsCreateFromTemplate (Pointer hObject)(Int32 altitude)(Int32 height) -> (Int32 id)"));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateWallScript, nullptr, SEXTEXT("ISectorsSetTemplateWallScript (Pointer hObject)(Bool useScript)(Sys.Type.IString scriptName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateDoorScript, nullptr, SEXTEXT("ISectorsSetTemplateDoorScript (Pointer hObject)(Bool hasDoor)(Sys.Type.IString scriptName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateMaterial, nullptr, SEXTEXT("ISectorsSetTemplateMaterial (Pointer hObject)(Sys.Type.IString bodyClass)(Int32 cat)(Int32 colour)(Sys.Type.IString persistentId) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::ISectorFloorTesselator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorFloorTesselatorNumberOfSquares(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 count = _pObject->NumberOfSquares();
		_offset += sizeof(count);
		WriteOutput(count, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorGetSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		AABB2d* sq;
		_offset += sizeof(sq);
		ReadInput(sq, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSquare(index, *sq);
	}
	void NativeHVISectorFloorTesselatorCeilingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CeilingQuad(index, *q);
	}
	void NativeHVISectorFloorTesselatorFloorQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FloorQuad(index, *q);
	}
	void NativeHVISectorFloorTesselatorAddCeilingTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddCeilingTriangle(*t);
	}
	void NativeHVISectorFloorTesselatorAddFloorTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddFloorTriangle(*t);
	}
	void NativeHVISectorFloorTesselatorGetMaterial(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentClass;
		ReadInput(_componentClass, _sf, -_offset);
		fstring componentClass { _componentClass->buffer, _componentClass->length };


		MaterialVertexData* mat;
		_offset += sizeof(mat);
		ReadInput(mat, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}
	void NativeHVISectorFloorTesselatorSetUVScale(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetUVScale(scale);
	}

	void NativeGetHandleForHVSectorFloorTesselator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* nceContext = reinterpret_cast<HV::ISectorFloorTesselator*>(_nce.context);
		// Uses: HV::ISectorFloorTesselator* FactoryConstructHVSectorFloorTesselator(HV::ISectorFloorTesselator* _context);
		HV::ISectorFloorTesselator* pObject = FactoryConstructHVSectorFloorTesselator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorFloorTesselator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorFloorTesselator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorFloorTesselator, _nceContext, SEXTEXT("GetHandleForISectorFloorTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfSquares, nullptr, SEXTEXT("ISectorFloorTesselatorNumberOfSquares (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetSquare, nullptr, SEXTEXT("ISectorFloorTesselatorGetSquare (Pointer hObject)(Int32 index)(Rococo.AAB2d sq) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorCeilingQuad, nullptr, SEXTEXT("ISectorFloorTesselatorCeilingQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorFloorQuad, nullptr, SEXTEXT("ISectorFloorTesselatorFloorQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddCeilingTriangle, nullptr, SEXTEXT("ISectorFloorTesselatorAddCeilingTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddFloorTriangle, nullptr, SEXTEXT("ISectorFloorTesselatorAddFloorTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetMaterial, nullptr, SEXTEXT("ISectorFloorTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorSetUVScale, nullptr, SEXTEXT("ISectorFloorTesselatorSetUVScale (Pointer hObject)(Float32 scale) -> "));
	}
}

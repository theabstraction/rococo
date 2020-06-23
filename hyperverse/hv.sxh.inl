namespace HV { 
	bool TryParse(const Rococo::fstring& s, AddItemFlags& value)
	{
		if (s ==  "AddItemFlags_None"_fstring)
		{
			value = AddItemFlags_None;
		}
		else if (s ==  "AddItemFlags_AlignEdge"_fstring)
		{
			value = AddItemFlags_AlignEdge;
		}
		else if (s ==  "AddItemFlags_RandomHeading"_fstring)
		{
			value = AddItemFlags_RandomHeading;
		}
		else if (s ==  "AddItemFlags_RandomPosition"_fstring)
		{
			value = AddItemFlags_RandomPosition;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const Rococo::fstring& s, AddItemFlags& value)
	{
		if (s ==  "None"_fstring)
		{
			value = AddItemFlags_None;
		}
		else if (s ==  "AlignEdge"_fstring)
		{
			value = AddItemFlags_AlignEdge;
		}
		else if (s ==  "RandomHeading"_fstring)
		{
			value = AddItemFlags_RandomHeading;
		}
		else if (s ==  "RandomPosition"_fstring)
		{
			value = AddItemFlags_RandomPosition;
		}
		else
		{
			return false;
		}

		return true;
	}
	fstring ToShortString(AddItemFlags value)
	{
		switch(value)
		{
			case AddItemFlags_None:
				return "None"_fstring;
			case AddItemFlags_AlignEdge:
				return "AlignEdge"_fstring;
			case AddItemFlags_RandomHeading:
				return "RandomHeading"_fstring;
			case AddItemFlags_RandomPosition:
				return "RandomPosition"_fstring;
			default:
				return {"",0};
		}
	}
}// HV.AddItemFlags

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
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVPlayer, _nceContext, ("GetHandleForIPlayer0 (Int32 index) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIPlayerSetPlayerEntity, nullptr, ("IPlayerSetPlayerEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerGetPlayerEntity, nullptr, ("IPlayerGetPlayerEntity (Pointer hObject) -> (Int64 id)"));
	}
}
// BennyHill generated Sexy native functions for HV::IScriptConfig 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVIScriptConfigGetFloat(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float default;
		_offset += sizeof(default);
		ReadInput(default, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _variableName;
		ReadInput(_variableName, _sf, -_offset);
		fstring variableName { _variableName->buffer, _variableName->length };


		HV::IScriptConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float value = _pObject->GetFloat(variableName, default, minValue, maxValue);
		_offset += sizeof(value);
		WriteOutput(value, _sf, -_offset);
	}
	void NativeHVIScriptConfigGetFloatRange(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float maxValue;
		_offset += sizeof(maxValue);
		ReadInput(maxValue, _sf, -_offset);

		float minValue;
		_offset += sizeof(minValue);
		ReadInput(minValue, _sf, -_offset);

		float defaultRight;
		_offset += sizeof(defaultRight);
		ReadInput(defaultRight, _sf, -_offset);

		float defaultLeft;
		_offset += sizeof(defaultLeft);
		ReadInput(defaultLeft, _sf, -_offset);

		Vec2* values;
		_offset += sizeof(values);
		ReadInput(values, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _variableName;
		ReadInput(_variableName, _sf, -_offset);
		fstring variableName { _variableName->buffer, _variableName->length };


		HV::IScriptConfig* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetFloatRange(variableName, *values, defaultLeft, defaultRight, minValue, maxValue);
	}

	void NativeGetHandleForHVScriptConfig(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IScriptConfig* nceContext = reinterpret_cast<HV::IScriptConfig*>(_nce.context);
		// Uses: HV::IScriptConfig* FactoryConstructHVScriptConfig(HV::IScriptConfig* _context);
		HV::IScriptConfig* pObject = FactoryConstructHVScriptConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIScriptConfig(Rococo::Script::IPublicScriptSystem& ss, HV::IScriptConfig* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVScriptConfig, _nceContext, ("GetHandleForIScriptConfig0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIScriptConfigGetFloat, nullptr, ("IScriptConfigGetFloat (Pointer hObject)(Sys.Type.IString variableName)(Float32 default)(Float32 minValue)(Float32 maxValue) -> (Float32 value)"));
		ss.AddNativeCall(ns, NativeHVIScriptConfigGetFloatRange, nullptr, ("IScriptConfigGetFloatRange (Pointer hObject)(Sys.Type.IString variableName)(Sys.Maths.Vec2 values)(Float32 defaultLeft)(Float32 defaultRight)(Float32 minValue)(Float32 maxValue) -> "));
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
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVCorridor, _nceContext, ("GetHandleForICorridor0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVICorridorGetSpan, nullptr, ("ICorridorGetSpan (Pointer hObject)(Sys.Maths.Vec3 span) -> "));
		ss.AddNativeCall(ns, NativeHVICorridorIsSloped, nullptr, ("ICorridorIsSloped (Pointer hObject) -> (Bool isSloped)"));
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

	void NativeGetHandleForHVSectorBuilder(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectors* nceContext = reinterpret_cast<HV::ISectors*>(_nce.context);
		// Uses: HV::ISectorBuilder* FactoryConstructHVSectorBuilder(HV::ISectors* _context);
		HV::ISectorBuilder* pObject = FactoryConstructHVSectorBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorBuilder(Rococo::Script::IPublicScriptSystem& ss, HV::ISectors* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorBuilder, _nceContext, ("GetHandleForISectors0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorBuilderAddVertex, nullptr, ("ISectorsAddVertex (Pointer hObject)(Float32 x)(Float32 y) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderClear, nullptr, ("ISectorsClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderCreateFromTemplate, nullptr, ("ISectorsCreateFromTemplate (Pointer hObject)(Int32 altitude)(Int32 height) -> (Int32 id)"));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateWallScript, nullptr, ("ISectorsSetTemplateWallScript (Pointer hObject)(Bool useScript)(Sys.Type.IString scriptName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateDoorScript, nullptr, ("ISectorsSetTemplateDoorScript (Pointer hObject)(Bool hasDoor)(Sys.Type.IString scriptName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorBuilderSetTemplateMaterial, nullptr, ("ISectorsSetTemplateMaterial (Pointer hObject)(Sys.Type.IString bodyClass)(Int32 cat)(Int32 colour)(Sys.Type.IString persistentId) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::ISectorLayout 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorLayoutExists(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 exists = _pObject->Exists();
		_offset += sizeof(exists);
		WriteOutput(exists, _sf, -_offset);
	}
	void NativeHVISectorLayoutCountSquares(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 sqCount = _pObject->CountSquares();
		_offset += sizeof(sqCount);
		WriteOutput(sqCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutGetSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		AABB2d* sq;
		_offset += sizeof(sq);
		ReadInput(sq, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSquare(sqIndex, *sq);
	}
	void NativeHVISectorLayoutCeilingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CeilingQuad(sqIndex, *q);
	}
	void NativeHVISectorLayoutFloorQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		QuadVertices* q;
		_offset += sizeof(q);
		ReadInput(q, _sf, -_offset);

		int32 sqIndex;
		_offset += sizeof(sqIndex);
		ReadInput(sqIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FloorQuad(sqIndex, *q);
	}
	void NativeHVISectorLayoutAltitude(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2* altitudes;
		_offset += sizeof(altitudes);
		ReadInput(altitudes, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Altitude(*altitudes);
	}
	void NativeHVISectorLayoutNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 segCount = _pObject->NumberOfSegments();
		_offset += sizeof(segCount);
		WriteOutput(segCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 gapCount = _pObject->NumberOfGaps();
		_offset += sizeof(gapCount);
		WriteOutput(gapCount, _sf, -_offset);
	}
	void NativeHVISectorLayoutGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 segIndex;
		_offset += sizeof(segIndex);
		ReadInput(segIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(segIndex, *segment);
	}
	void NativeHVISectorLayoutGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 gapIndex;
		_offset += sizeof(gapIndex);
		ReadInput(gapIndex, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(gapIndex, *segment);
	}
	void NativeHVISectorLayoutAddSceneryAroundObject(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ObjectCreationSpec* ocs;
		_offset += sizeof(ocs);
		ReadInput(ocs, _sf, -_offset);

		HV::InsertItemSpec* iis;
		_offset += sizeof(iis);
		ReadInput(iis, _sf, -_offset);

		ID_ENTITY centrePieceId;
		_offset += sizeof(centrePieceId);
		ReadInput(centrePieceId, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _mesh;
		ReadInput(_mesh, _sf, -_offset);
		fstring mesh { _mesh->buffer, _mesh->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->AddSceneryAroundObject(mesh, centrePieceId, *iis, *ocs);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorLayoutAddItemToLargestSquare(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ObjectCreationSpec* ocs;
		_offset += sizeof(ocs);
		ReadInput(ocs, _sf, -_offset);

		int32 addItemFlags;
		_offset += sizeof(addItemFlags);
		ReadInput(addItemFlags, _sf, -_offset);

		_offset += sizeof(IString*);
		IString* _mesh;
		ReadInput(_mesh, _sf, -_offset);
		fstring mesh { _mesh->buffer, _mesh->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->AddItemToLargestSquare(mesh, addItemFlags, *ocs);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVISectorLayoutPlaceItemOnUpFacingQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 wasMoved = _pObject->PlaceItemOnUpFacingQuad(id);
		_offset += sizeof(wasMoved);
		WriteOutput(wasMoved, _sf, -_offset);
	}
	void NativeHVISectorLayoutDeleteScenery(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeleteScenery();
	}
	void NativeHVISectorLayoutDeleteItemsWithMesh(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _prefix;
		ReadInput(_prefix, _sf, -_offset);
		fstring prefix { _prefix->buffer, _prefix->length };


		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->DeleteItemsWithMesh(prefix);
	}
	void NativeHVISectorLayoutClearManagedEntities(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearManagedEntities();
	}
	void NativeHVISectorLayoutManageEntity(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ManageEntity(id);
	}
	void NativeHVISectorLayoutUseUpFacingQuads(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::ISectorLayout* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->UseUpFacingQuads(id);
	}

}

namespace HV { 
	void AddNativeCalls_HVISectorLayout(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorLayout* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutExists, nullptr, ("ISectorLayoutExists (Pointer hObject) -> (Bool exists)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutCountSquares, nullptr, ("ISectorLayoutCountSquares (Pointer hObject) -> (Int32 sqCount)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetSquare, nullptr, ("ISectorLayoutGetSquare (Pointer hObject)(Int32 sqIndex)(Rococo.AAB2d sq) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutCeilingQuad, nullptr, ("ISectorLayoutCeilingQuad (Pointer hObject)(Int32 sqIndex)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutFloorQuad, nullptr, ("ISectorLayoutFloorQuad (Pointer hObject)(Int32 sqIndex)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutAltitude, nullptr, ("ISectorLayoutAltitude (Pointer hObject)(Sys.Maths.Vec2 altitudes) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutNumberOfSegments, nullptr, ("ISectorLayoutNumberOfSegments (Pointer hObject) -> (Int32 segCount)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutNumberOfGaps, nullptr, ("ISectorLayoutNumberOfGaps (Pointer hObject) -> (Int32 gapCount)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetSegment, nullptr, ("ISectorLayoutGetSegment (Pointer hObject)(Int32 segIndex)(HV.WallSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutGetGap, nullptr, ("ISectorLayoutGetGap (Pointer hObject)(Int32 gapIndex)(HV.GapSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutAddSceneryAroundObject, nullptr, ("ISectorLayoutAddSceneryAroundObject (Pointer hObject)(Sys.Type.IString mesh)(Int64 centrePieceId)(HV.InsertItemSpec iis)(HV.ObjectCreationSpec ocs) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutAddItemToLargestSquare, nullptr, ("ISectorLayoutAddItemToLargestSquare (Pointer hObject)(Sys.Type.IString mesh)(Int32 addItemFlags)(HV.ObjectCreationSpec ocs) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutPlaceItemOnUpFacingQuad, nullptr, ("ISectorLayoutPlaceItemOnUpFacingQuad (Pointer hObject)(Int64 id) -> (Bool wasMoved)"));
		ss.AddNativeCall(ns, NativeHVISectorLayoutDeleteScenery, nullptr, ("ISectorLayoutDeleteScenery (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutDeleteItemsWithMesh, nullptr, ("ISectorLayoutDeleteItemsWithMesh (Pointer hObject)(Sys.Type.IString prefix) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutClearManagedEntities, nullptr, ("ISectorLayoutClearManagedEntities (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutManageEntity, nullptr, ("ISectorLayoutManageEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVISectorLayoutUseUpFacingQuads, nullptr, ("ISectorLayoutUseUpFacingQuads (Pointer hObject)(Int64 id) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::ISectorEnumerator 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorEnumeratorCount(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 nCount = _pObject->Count();
		_offset += sizeof(nCount);
		WriteOutput(nCount, _sf, -_offset);
	}
	void NativeHVISectorEnumeratorGetSector(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 index;
		_offset += sizeof(index);
		ReadInput(index, _sf, -_offset);

		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ISectorLayout* layout = _pObject->GetSector(index);
		_offset += sizeof(CReflectedClass*);
		auto& _layoutStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ISectorLayout"), ("ProxyISectorLayout"), _nce.ss);
		CReflectedClass* _sxylayout = _nce.ss.Represent(_layoutStruct, layout);
		WriteOutput(&_sxylayout->header.pVTables[0], _sf, -_offset);
	}
	void NativeHVISectorEnumeratorGetSelectedSector(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ISectorLayout* layout = _pObject->GetSelectedSector();
		_offset += sizeof(CReflectedClass*);
		auto& _layoutStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ISectorLayout"), ("ProxyISectorLayout"), _nce.ss);
		CReflectedClass* _sxylayout = _nce.ss.Represent(_layoutStruct, layout);
		WriteOutput(&_sxylayout->header.pVTables[0], _sf, -_offset);
	}

	void NativeGetHandleForHVSectorEnumerator(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorEnumerator* nceContext = reinterpret_cast<HV::ISectorEnumerator*>(_nce.context);
		// Uses: HV::ISectorEnumerator* FactoryConstructHVSectorEnumerator(HV::ISectorEnumerator* _context);
		HV::ISectorEnumerator* pObject = FactoryConstructHVSectorEnumerator(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorEnumerator(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorEnumerator* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorEnumerator, _nceContext, ("GetHandleForISectorEnumerator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorCount, nullptr, ("ISectorEnumeratorCount (Pointer hObject) -> (Int32 nCount)"));
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorGetSector, nullptr, ("ISectorEnumeratorGetSector (Pointer hObject)(Int32 index) -> (HV.ISectorLayout layout)"));
		ss.AddNativeCall(ns, NativeHVISectorEnumeratorGetSelectedSector, nullptr, ("ISectorEnumeratorGetSelectedSector (Pointer hObject) -> (HV.ISectorLayout layout)"));
	}
}
// BennyHill generated Sexy native functions for HV::ISectorComponents 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVISectorComponentsAddTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* t;
		_offset += sizeof(t);
		ReadInput(t, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*t);
	}
	void NativeHVISectorComponentsBuildComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->BuildComponent(componentName);
	}
	void NativeHVISectorComponentsClearComponents(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _componentName;
		ReadInput(_componentName, _sf, -_offset);
		fstring componentName { _componentName->buffer, _componentName->length };


		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearComponents(componentName);
	}
	void NativeHVISectorComponentsCompleteComponent(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 preserveMesh;
		_offset += sizeof(preserveMesh);
		ReadInput(preserveMesh, _sf, -_offset);

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CompleteComponent(preserveMesh);
	}
	void NativeHVISectorComponentsGetMaterial(NativeCallEnvironment& _nce)
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

		HV::ISectorComponents* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetMaterial(*mat, componentClass);
	}

	void NativeGetHandleForHVSectorComponents(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorComponents* nceContext = reinterpret_cast<HV::ISectorComponents*>(_nce.context);
		// Uses: HV::ISectorComponents* FactoryConstructHVSectorComponents(HV::ISectorComponents* _context);
		HV::ISectorComponents* pObject = FactoryConstructHVSectorComponents(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISectorComponents(Rococo::Script::IPublicScriptSystem& ss, HV::ISectorComponents* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorComponents, _nceContext, ("GetHandleForISectorComponents0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorComponentsAddTriangle, nullptr, ("ISectorComponentsAddTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeHVISectorComponentsBuildComponent, nullptr, ("ISectorComponentsBuildComponent (Pointer hObject)(Sys.Type.IString componentName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorComponentsClearComponents, nullptr, ("ISectorComponentsClearComponents (Pointer hObject)(Sys.Type.IString componentName) -> "));
		ss.AddNativeCall(ns, NativeHVISectorComponentsCompleteComponent, nullptr, ("ISectorComponentsCompleteComponent (Pointer hObject)(Bool preserveMesh) -> "));
		ss.AddNativeCall(ns, NativeHVISectorComponentsGetMaterial, nullptr, ("ISectorComponentsGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::ITriangleList 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;

	void NativeHVITriangleListAddTriangleByVertices(NativeCallEnvironment& _nce)
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

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangleByVertices(*a, *b, *c);
	}
	void NativeHVITriangleListAddTriangle(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		VertexTriangle* abc;
		_offset += sizeof(abc);
		ReadInput(abc, _sf, -_offset);

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*abc);
	}
	void NativeHVITriangleListAddQuad(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ObjectVertex* d;
		_offset += sizeof(d);
		ReadInput(d, _sf, -_offset);

		ObjectVertex* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		ObjectVertex* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		ObjectVertex* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddQuad(*a, *b, *c, *d);
	}
	void NativeHVITriangleListCountVertices(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 vertices = _pObject->CountVertices();
		_offset += sizeof(vertices);
		WriteOutput(vertices, _sf, -_offset);
	}
	void NativeHVITriangleListCountTriangles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ITriangleList* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 triangles = _pObject->CountTriangles();
		_offset += sizeof(triangles);
		WriteOutput(triangles, _sf, -_offset);
	}

}

namespace HV { 
	void AddNativeCalls_HVITriangleList(Rococo::Script::IPublicScriptSystem& ss, HV::ITriangleList* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeHVITriangleListAddTriangleByVertices, nullptr, ("ITriangleListAddTriangleByVertices (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c) -> "));
		ss.AddNativeCall(ns, NativeHVITriangleListAddTriangle, nullptr, ("ITriangleListAddTriangle (Pointer hObject)(Rococo.VertexTriangle abc) -> "));
		ss.AddNativeCall(ns, NativeHVITriangleListAddQuad, nullptr, ("ITriangleListAddQuad (Pointer hObject)(Rococo.ObjectVertex a)(Rococo.ObjectVertex b)(Rococo.ObjectVertex c)(Rococo.ObjectVertex d) -> "));
		ss.AddNativeCall(ns, NativeHVITriangleListCountVertices, nullptr, ("ITriangleListCountVertices (Pointer hObject) -> (Int32 vertices)"));
		ss.AddNativeCall(ns, NativeHVITriangleListCountTriangles, nullptr, ("ITriangleListCountTriangles (Pointer hObject) -> (Int32 triangles)"));
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
	void NativeHVISectorWallTesselatorWallTriangles(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorWallTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		HV::ITriangleList* triangles = _pObject->WallTriangles();
		_offset += sizeof(CReflectedClass*);
		auto& _trianglesStruct = Rococo::Helpers::GetDefaultProxy(("HV"),("ITriangleList"), ("ProxyITriangleList"), _nce.ss);
		CReflectedClass* _sxytriangles = _nce.ss.Represent(_trianglesStruct, triangles);
		WriteOutput(&_sxytriangles->header.pVTables[0], _sf, -_offset);
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
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorWallTesselator, _nceContext, ("GetHandleForISectorWallTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfSegments, nullptr, ("ISectorWallTesselatorNumberOfSegments (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorNumberOfGaps, nullptr, ("ISectorWallTesselatorNumberOfGaps (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetSegment, nullptr, ("ISectorWallTesselatorGetSegment (Pointer hObject)(Int32 ringIndex)(HV.WallSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetGap, nullptr, ("ISectorWallTesselatorGetGap (Pointer hObject)(Int32 ringIndex)(HV.GapSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorWallTriangles, nullptr, ("ISectorWallTesselatorWallTriangles (Pointer hObject) -> (HV.ITriangleList triangles)"));
		ss.AddNativeCall(ns, NativeHVISectorWallTesselatorGetMaterial, nullptr, ("ISectorWallTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "));
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
	void NativeHVISectorFloorTesselatorFoundationsExist(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 exists = _pObject->FoundationsExist();
		_offset += sizeof(exists);
		WriteOutput(exists, _sf, -_offset);
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
	void NativeHVISectorFloorTesselatorNumberOfSegments(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 segCount = _pObject->NumberOfSegments();
		_offset += sizeof(segCount);
		WriteOutput(segCount, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorNumberOfGaps(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		int32 gapCount = _pObject->NumberOfGaps();
		_offset += sizeof(gapCount);
		WriteOutput(gapCount, _sf, -_offset);
	}
	void NativeHVISectorFloorTesselatorGetSegment(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::WallSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 segIndex;
		_offset += sizeof(segIndex);
		ReadInput(segIndex, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetSegment(segIndex, *segment);
	}
	void NativeHVISectorFloorTesselatorGetGap(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::GapSegment* segment;
		_offset += sizeof(segment);
		ReadInput(segment, _sf, -_offset);

		int32 gapIndex;
		_offset += sizeof(gapIndex);
		ReadInput(gapIndex, _sf, -_offset);

		HV::ISectorFloorTesselator* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetGap(gapIndex, *segment);
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
		const INamespace& ns = ss.AddNativeNamespace(("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSectorFloorTesselator, _nceContext, ("GetHandleForISectorFloorTesselator0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfSquares, nullptr, ("ISectorFloorTesselatorNumberOfSquares (Pointer hObject) -> (Int32 count)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorFoundationsExist, nullptr, ("ISectorFloorTesselatorFoundationsExist (Pointer hObject) -> (Bool exists)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetSquare, nullptr, ("ISectorFloorTesselatorGetSquare (Pointer hObject)(Int32 index)(Rococo.AAB2d sq) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorCeilingQuad, nullptr, ("ISectorFloorTesselatorCeilingQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorFloorQuad, nullptr, ("ISectorFloorTesselatorFloorQuad (Pointer hObject)(Int32 index)(Rococo.QuadVertices q) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddCeilingTriangle, nullptr, ("ISectorFloorTesselatorAddCeilingTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorAddFloorTriangle, nullptr, ("ISectorFloorTesselatorAddFloorTriangle (Pointer hObject)(Rococo.VertexTriangle t) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetMaterial, nullptr, ("ISectorFloorTesselatorGetMaterial (Pointer hObject)(Rococo.MaterialVertexData mat)(Sys.Type.IString componentClass) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorSetUVScale, nullptr, ("ISectorFloorTesselatorSetUVScale (Pointer hObject)(Float32 scale) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfSegments, nullptr, ("ISectorFloorTesselatorNumberOfSegments (Pointer hObject) -> (Int32 segCount)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorNumberOfGaps, nullptr, ("ISectorFloorTesselatorNumberOfGaps (Pointer hObject) -> (Int32 gapCount)"));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetSegment, nullptr, ("ISectorFloorTesselatorGetSegment (Pointer hObject)(Int32 segIndex)(HV.WallSegment segment) -> "));
		ss.AddNativeCall(ns, NativeHVISectorFloorTesselatorGetGap, nullptr, ("ISectorFloorTesselatorGetGap (Pointer hObject)(Int32 gapIndex)(HV.GapSegment segment) -> "));
	}
}

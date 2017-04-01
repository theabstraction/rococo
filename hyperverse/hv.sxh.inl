namespace HV { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s == L"OrientationFlags_None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s == L"OrientationFlags_Heading"_fstring)
		{
			value = OrientationFlags_Heading;
		}
		else if (s == L"OrientationFlags_Elevation"_fstring)
		{
			value = OrientationFlags_Elevation;
		}
		else if (s == L"OrientationFlags_Tilt"_fstring)
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
		if (s == L"None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s == L"Heading"_fstring)
		{
			value = OrientationFlags_Heading;
		}
		else if (s == L"Elevation"_fstring)
		{
			value = OrientationFlags_Elevation;
		}
		else if (s == L"Tilt"_fstring)
		{
			value = OrientationFlags_Tilt;
		}
		else
		{
			return false;
		}

		return true;
	}
}}// HV.Graphics.OrientationFlags

// BennyHill generated Sexy native functions for HV::Entities::IInstances 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVEntitiesIInstancesAddBody(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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


		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddBody(modelName, texture, *model, *scale, parentId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeHVEntitiesIInstancesAddGhost(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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

		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->AddGhost(*model, *scale, parentId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeHVEntitiesIInstancesGetScale(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetScale(entityId, *scale);
	}
	void NativeHVEntitiesIInstancesSetScale(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScale(entityId, *scale);
	}
	void NativeHVEntitiesIInstancesTryGetModelToWorldMatrix(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		boolean32 existant = _pObject->TryGetModelToWorldMatrix(entityId, *position);
		_offset += sizeof(existant);
		WriteOutput(existant, _sf, -_offset);
	}
	void NativeHVEntitiesIInstancesClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Entities::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}

	void NativeGetHandleForHVEntitiesInstances(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Entities::IInstances* nceContext = reinterpret_cast<HV::Entities::IInstances*>(_nce.context);
		// Uses: HV::Entities::IInstances* FactoryConstructHVEntitiesInstances(HV::Entities::IInstances* _context);
		HV::Entities::IInstances* pObject = FactoryConstructHVEntitiesInstances(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Entities { 
	void AddNativeCalls_HVEntitiesIInstances(Sexy::Script::IPublicScriptSystem& ss, HV::Entities::IInstances* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVEntitiesInstances, _nceContext, SEXTEXT("GetHandleForIInstances0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesAddBody, nullptr, SEXTEXT("IInstancesAddBody (Pointer hObject)(Sys.Type.IString modelName)(Sys.Type.IString texture)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesAddGhost, nullptr, SEXTEXT("IInstancesAddGhost (Pointer hObject)(Sys.Maths.Matrix4x4 model)(Sys.Maths.Vec3 scale)(Int64 parentId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesGetScale, nullptr, SEXTEXT("IInstancesGetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesSetScale, nullptr, SEXTEXT("IInstancesSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesTryGetModelToWorldMatrix, nullptr, SEXTEXT("IInstancesTryGetModelToWorldMatrix (Pointer hObject)(Int64 entityId)(Sys.Maths.Matrix4x4 position) -> (Bool existant)"));
		ss.AddNativeCall(ns, NativeHVEntitiesIInstancesClear, nullptr, SEXTEXT("IInstancesClear (Pointer hObject) -> "));
	}
}}
// BennyHill generated Sexy native functions for HV::Graphics::IMeshBuilder 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVGraphicsIMeshBuilderBegin(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		HV::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Begin(fqName);
	}
	void NativeHVGraphicsIMeshBuilderAddTriangle(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::Vertex* c;
		_offset += sizeof(c);
		ReadInput(c, _sf, -_offset);

		HV::Graphics::Vertex* b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		HV::Graphics::Vertex* a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		HV::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddTriangle(*a, *b, *c);
	}
	void NativeHVGraphicsIMeshBuilderEnd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->End();
	}
	void NativeHVGraphicsIMeshBuilderClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IMeshBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}

	void NativeGetHandleForHVGraphicsMeshBuilder(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IMeshBuilder* nceContext = reinterpret_cast<HV::Graphics::IMeshBuilder*>(_nce.context);
		// Uses: HV::Graphics::IMeshBuilder* FactoryConstructHVGraphicsMeshBuilder(HV::Graphics::IMeshBuilder* _context);
		HV::Graphics::IMeshBuilder* pObject = FactoryConstructHVGraphicsMeshBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Graphics { 
	void AddNativeCalls_HVGraphicsIMeshBuilder(Sexy::Script::IPublicScriptSystem& ss, HV::Graphics::IMeshBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsMeshBuilder, _nceContext, SEXTEXT("GetHandleForIMeshBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVGraphicsIMeshBuilderBegin, nullptr, SEXTEXT("IMeshBuilderBegin (Pointer hObject)(Sys.Type.IString fqName) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIMeshBuilderAddTriangle, nullptr, SEXTEXT("IMeshBuilderAddTriangle (Pointer hObject)(HV.Graphics.Vertex a)(HV.Graphics.Vertex b)(HV.Graphics.Vertex c) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIMeshBuilderEnd, nullptr, SEXTEXT("IMeshBuilderEnd (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIMeshBuilderClear, nullptr, SEXTEXT("IMeshBuilderClear (Pointer hObject) -> "));
	}
}}
// BennyHill generated Sexy native functions for HV::Graphics::ISceneBuilder 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVGraphicsISceneBuilderAddStatics(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVGraphicsISceneBuilderSetClearColour(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ISceneBuilder* nceContext = reinterpret_cast<HV::Graphics::ISceneBuilder*>(_nce.context);
		// Uses: HV::Graphics::ISceneBuilder* FactoryConstructHVGraphicsSceneBuilder(HV::Graphics::ISceneBuilder* _context);
		HV::Graphics::ISceneBuilder* pObject = FactoryConstructHVGraphicsSceneBuilder(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Graphics { 
	void AddNativeCalls_HVGraphicsISceneBuilder(Sexy::Script::IPublicScriptSystem& ss, HV::Graphics::ISceneBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsSceneBuilder, _nceContext, SEXTEXT("GetHandleForISceneBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderAddStatics, nullptr, SEXTEXT("ISceneBuilderAddStatics (Pointer hObject)(Int64 entityId) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderClear, nullptr, SEXTEXT("ISceneBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderSetClearColour, nullptr, SEXTEXT("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderSetSunDirection, nullptr, SEXTEXT("ISceneBuilderSetSunDirection (Pointer hObject)(Sys.Maths.Vec3 sun) -> "));
	}
}}
// BennyHill generated Sexy native functions for HV::Entities::IMobiles 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVEntitiesIMobilesLink(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Link(id);
	}
	void NativeHVEntitiesIMobilesGetAngles(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		FPSAngles* angles;
		_offset += sizeof(angles);
		ReadInput(angles, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetAngles(id, *angles);
	}
	void NativeHVEntitiesIMobilesSetAngles(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		FPSAngles* angles;
		_offset += sizeof(angles);
		ReadInput(angles, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Entities::IMobiles* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetAngles(id, *angles);
	}

	void NativeGetHandleForHVEntitiesMobiles(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Entities::IMobiles* nceContext = reinterpret_cast<HV::Entities::IMobiles*>(_nce.context);
		// Uses: HV::Entities::IMobiles* FactoryConstructHVEntitiesMobiles(HV::Entities::IMobiles* _context);
		HV::Entities::IMobiles* pObject = FactoryConstructHVEntitiesMobiles(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Entities { 
	void AddNativeCalls_HVEntitiesIMobiles(Sexy::Script::IPublicScriptSystem& ss, HV::Entities::IMobiles* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Entities.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVEntitiesMobiles, _nceContext, SEXTEXT("GetHandleForIMobiles0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVEntitiesIMobilesLink, nullptr, SEXTEXT("IMobilesLink (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVEntitiesIMobilesGetAngles, nullptr, SEXTEXT("IMobilesGetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
		ss.AddNativeCall(ns, NativeHVEntitiesIMobilesSetAngles, nullptr, SEXTEXT("IMobilesSetAngles (Pointer hObject)(Int64 id)(Sys.Maths.FPSAngles angles) -> "));
	}
}}
// BennyHill generated Sexy native functions for HV::ISprites 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVISpritesClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVISpritesAddSprite(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->LoadAllSprites();
	}

	void NativeGetHandleForHVSprites(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::ISprites* nceContext = reinterpret_cast<HV::ISprites*>(_nce.context);
		// Uses: HV::ISprites* FactoryConstructHVSprites(HV::ISprites* _context);
		HV::ISprites* pObject = FactoryConstructHVSprites(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVISprites(Sexy::Script::IPublicScriptSystem& ss, HV::ISprites* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVSprites, _nceContext, SEXTEXT("GetHandleForISprites0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVISpritesClear, nullptr, SEXTEXT("ISpritesClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesAddSprite, nullptr, SEXTEXT("ISpritesAddSprite (Pointer hObject)(Sys.Type.IString resourceName) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesAddEachSpriteInDirectory, nullptr, SEXTEXT("ISpritesAddEachSpriteInDirectory (Pointer hObject)(Sys.Type.IString directoryName) -> "));
		ss.AddNativeCall(ns, NativeHVISpritesLoadAllSprites, nullptr, SEXTEXT("ISpritesLoadAllSprites (Pointer hObject) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::IConfig 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVIConfigInt(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(VirtualTable*);
		VirtualTable* text;
		ReadInput(text, _sf, -_offset);
		Sexy::Helpers::StringPopulator _textPopulator(_nce, text);
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IConfig* nceContext = reinterpret_cast<HV::IConfig*>(_nce.context);
		// Uses: HV::IConfig* FactoryConstructHVConfig(HV::IConfig* _context);
		HV::IConfig* pObject = FactoryConstructHVConfig(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIConfig(Sexy::Script::IPublicScriptSystem& ss, HV::IConfig* _nceContext)
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
// BennyHill generated Sexy native functions for HV::Graphics::ICamera 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVGraphicsICameraClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}
	void NativeHVGraphicsICameraSetRHProjection(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetRHProjection(fov, near, far);
	}
	void NativeHVGraphicsICameraSetProjection(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* proj;
		_offset += sizeof(proj);
		ReadInput(proj, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetProjection(*proj);
	}
	void NativeHVGraphicsICameraSetPosition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPosition(*position);
	}
	void NativeHVGraphicsICameraSetOrientation(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOrientation(*orientation);
	}
	void NativeHVGraphicsICameraFollowEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FollowEntity(id);
	}
	void NativeHVGraphicsICameraMoveToEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->MoveToEntity(id);
	}
	void NativeHVGraphicsICameraOrientateWithEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->OrientateWithEntity(id, flags);
	}
	void NativeHVGraphicsICameraOrientateToEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		ID_ENTITY id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->OrientateToEntity(id, flags);
	}
	void NativeHVGraphicsICameraGetPosition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetPosition(*position);
	}
	void NativeHVGraphicsICameraGetOrientation(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetOrientation(*orientation);
	}
	void NativeHVGraphicsICameraGetWorld(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* world;
		_offset += sizeof(world);
		ReadInput(world, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetWorld(*world);
	}
	void NativeHVGraphicsICameraGetWorldAndProj(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Matrix4x4* worldAndProj;
		_offset += sizeof(worldAndProj);
		ReadInput(worldAndProj, _sf, -_offset);

		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetWorldAndProj(*worldAndProj);
	}
	void NativeHVGraphicsICameraAspectRatio(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ICamera* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		float widthOverHeight = _pObject->AspectRatio();
		_offset += sizeof(widthOverHeight);
		WriteOutput(widthOverHeight, _sf, -_offset);
	}

	void NativeGetHandleForHVGraphicsCamera(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::ICamera* nceContext = reinterpret_cast<HV::Graphics::ICamera*>(_nce.context);
		// Uses: HV::Graphics::ICamera* FactoryConstructHVGraphicsCamera(HV::Graphics::ICamera* _context);
		HV::Graphics::ICamera* pObject = FactoryConstructHVGraphicsCamera(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Graphics { 
	void AddNativeCalls_HVGraphicsICamera(Sexy::Script::IPublicScriptSystem& ss, HV::Graphics::ICamera* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsCamera, _nceContext, SEXTEXT("GetHandleForICamera0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraClear, nullptr, SEXTEXT("ICameraClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraSetRHProjection, nullptr, SEXTEXT("ICameraSetRHProjection (Pointer hObject)(Sys.Maths.Degrees fov)(Float32 near)(Float32 far) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraSetProjection, nullptr, SEXTEXT("ICameraSetProjection (Pointer hObject)(Sys.Maths.Matrix4x4 proj) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraSetPosition, nullptr, SEXTEXT("ICameraSetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraSetOrientation, nullptr, SEXTEXT("ICameraSetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraFollowEntity, nullptr, SEXTEXT("ICameraFollowEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraMoveToEntity, nullptr, SEXTEXT("ICameraMoveToEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraOrientateWithEntity, nullptr, SEXTEXT("ICameraOrientateWithEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraOrientateToEntity, nullptr, SEXTEXT("ICameraOrientateToEntity (Pointer hObject)(Int64 id)(Int32 flags) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraGetPosition, nullptr, SEXTEXT("ICameraGetPosition (Pointer hObject)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraGetOrientation, nullptr, SEXTEXT("ICameraGetOrientation (Pointer hObject)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraGetWorld, nullptr, SEXTEXT("ICameraGetWorld (Pointer hObject)(Sys.Maths.Matrix4x4 world) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraGetWorldAndProj, nullptr, SEXTEXT("ICameraGetWorldAndProj (Pointer hObject)(Sys.Maths.Matrix4x4 worldAndProj) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsICameraAspectRatio, nullptr, SEXTEXT("ICameraAspectRatio (Pointer hObject) -> (Float32 widthOverHeight)"));
	}
}}
// BennyHill generated Sexy native functions for HV::IPlayer 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVIPlayerSetPlayerEntity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY id = _pObject->GetPlayerEntity();
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeHVIPlayerSetControlFPS(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetControlFPS();
	}
	void NativeHVIPlayerSetControl4WayScroller(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetControl4WayScroller();
	}
	void NativeHVIPlayerSetControlNone(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetControlNone();
	}
	void NativeHVIPlayerSetSpeed(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float straffe;
		_offset += sizeof(straffe);
		ReadInput(straffe, _sf, -_offset);

		float backward;
		_offset += sizeof(backward);
		ReadInput(backward, _sf, -_offset);

		float forward;
		_offset += sizeof(forward);
		ReadInput(forward, _sf, -_offset);

		HV::IPlayer* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetSpeed(forward, backward, straffe);
	}

	void NativeGetHandleForHVGraphicsPlayer(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
	void AddNativeCalls_HVIPlayer(Sexy::Script::IPublicScriptSystem& ss, HV::IPlayerSupervisor* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsPlayer, _nceContext, SEXTEXT("GetHandleForIPlayer0 (Int32 index) -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIPlayerSetPlayerEntity, nullptr, SEXTEXT("IPlayerSetPlayerEntity (Pointer hObject)(Int64 id) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerGetPlayerEntity, nullptr, SEXTEXT("IPlayerGetPlayerEntity (Pointer hObject) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeHVIPlayerSetControlFPS, nullptr, SEXTEXT("IPlayerSetControlFPS (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerSetControl4WayScroller, nullptr, SEXTEXT("IPlayerSetControl4WayScroller (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerSetControlNone, nullptr, SEXTEXT("IPlayerSetControlNone (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVIPlayerSetSpeed, nullptr, SEXTEXT("IPlayerSetSpeed (Pointer hObject)(Float32 forward)(Float32 backward)(Float32 straffe) -> "));
	}
}
// BennyHill generated Sexy native functions for HV::IKeyboard 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVIKeyboardClearActions(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IKeyboard* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ClearActions();
	}
	void NativeHVIKeyboardSetKeyName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
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
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::IKeyboard* nceContext = reinterpret_cast<HV::IKeyboard*>(_nce.context);
		// Uses: HV::IKeyboard* FactoryConstructHVKeyboard(HV::IKeyboard* _context);
		HV::IKeyboard* pObject = FactoryConstructHVKeyboard(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { 
	void AddNativeCalls_HVIKeyboard(Sexy::Script::IPublicScriptSystem& ss, HV::IKeyboard* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVKeyboard, _nceContext, SEXTEXT("GetHandleForIKeyboard0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVIKeyboardClearActions, nullptr, SEXTEXT("IKeyboardClearActions (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVIKeyboardSetKeyName, nullptr, SEXTEXT("IKeyboardSetKeyName (Pointer hObject)(Sys.Type.IString name)(Int32 scancode) -> "));
		ss.AddNativeCall(ns, NativeHVIKeyboardBindAction, nullptr, SEXTEXT("IKeyboardBindAction (Pointer hObject)(Sys.Type.IString keyName)(Sys.Type.IString actionName) -> "));
	}
}

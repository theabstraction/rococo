namespace HV { namespace Graphics { 
	bool TryParse(const Rococo::fstring& s, OrientationFlags& value)
	{
		if (s == L"OrientationFlags_None"_fstring)
		{
			value = OrientationFlags_None;
		}
		else if (s == L"OrientationFlags_OrientationFlagsHeading"_fstring)
		{
			value = OrientationFlags_OrientationFlagsHeading;
		}
		else if (s == L"OrientationFlags_OrientationFlagsElevation"_fstring)
		{
			value = OrientationFlags_OrientationFlagsElevation;
		}
		else if (s == L"OrientationFlags_OrientationFlagsRoll"_fstring)
		{
			value = OrientationFlags_OrientationFlagsRoll;
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
		else if (s == L"OrientationFlagsHeading"_fstring)
		{
			value = OrientationFlags_OrientationFlagsHeading;
		}
		else if (s == L"OrientationFlagsElevation"_fstring)
		{
			value = OrientationFlags_OrientationFlagsElevation;
		}
		else if (s == L"OrientationFlagsRoll"_fstring)
		{
			value = OrientationFlags_OrientationFlagsRoll;
		}
		else
		{
			return false;
		}

		return true;
	}
}}// HV.Graphics.OrientationFlags

// BennyHill generated Sexy native functions for HV::Graphics::IInstances 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;

	void NativeHVGraphicsIInstancesSetOrientation(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetOrientation(entityId, *orientation);
	}
	void NativeHVGraphicsIInstancesSetScale(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScale(entityId, *scale);
	}
	void NativeHVGraphicsIInstancesSetPosition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPosition(entityId, *position);
	}
	void NativeHVGraphicsIInstancesGetScale(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetScale(entityId, *scale);
	}
	void NativeHVGraphicsIInstancesGetPosition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* position;
		_offset += sizeof(position);
		ReadInput(position, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetPosition(entityId, *position);
	}
	void NativeHVGraphicsIInstancesGetOrientation(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Quat* orientation;
		_offset += sizeof(orientation);
		ReadInput(orientation, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GetOrientation(entityId, *orientation);
	}
	void NativeHVGraphicsIInstancesBegin(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _fqName;
		ReadInput(_fqName, _sf, -_offset);
		fstring fqName { _fqName->buffer, _fqName->length };


		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Begin(fqName);
	}
	void NativeHVGraphicsIInstancesSetMeshByName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _texture;
		ReadInput(_texture, _sf, -_offset);
		fstring texture { _texture->buffer, _texture->length };


		_offset += sizeof(IString*);
		IString* _modelName;
		ReadInput(_modelName, _sf, -_offset);
		fstring modelName { _modelName->buffer, _modelName->length };


		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetMeshByName(modelName, texture);
	}
	void NativeHVGraphicsIInstancesSetParent(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetParent(entityId);
	}
	void NativeHVGraphicsIInstancesEnd(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId = _pObject->End();
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeHVGraphicsIInstancesClear(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IInstances* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Clear();
	}

	void NativeGetHandleForHVGraphicsInstances(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		HV::Graphics::IInstances* nceContext = reinterpret_cast<HV::Graphics::IInstances*>(_nce.context);
		// Uses: HV::Graphics::IInstances* FactoryConstructHVGraphicsInstances(HV::Graphics::IInstances* _context);
		HV::Graphics::IInstances* pObject = FactoryConstructHVGraphicsInstances(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace HV { namespace Graphics { 
	void AddNativeCalls_HVGraphicsIInstances(Sexy::Script::IPublicScriptSystem& ss, HV::Graphics::IInstances* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("HV.Graphics.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForHVGraphicsInstances, _nceContext, SEXTEXT("GetHandleForIInstances0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesSetOrientation, nullptr, SEXTEXT("IInstancesSetOrientation (Pointer hObject)(Int64 entityId)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesSetScale, nullptr, SEXTEXT("IInstancesSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesSetPosition, nullptr, SEXTEXT("IInstancesSetPosition (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesGetScale, nullptr, SEXTEXT("IInstancesGetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesGetPosition, nullptr, SEXTEXT("IInstancesGetPosition (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 position) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesGetOrientation, nullptr, SEXTEXT("IInstancesGetOrientation (Pointer hObject)(Int64 entityId)(Sys.Maths.Quat orientation) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesBegin, nullptr, SEXTEXT("IInstancesBegin (Pointer hObject)(Sys.Type.IString fqName) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesSetMeshByName, nullptr, SEXTEXT("IInstancesSetMeshByName (Pointer hObject)(Sys.Type.IString modelName)(Sys.Type.IString texture) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesSetParent, nullptr, SEXTEXT("IInstancesSetParent (Pointer hObject)(Int64 entityId) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesEnd, nullptr, SEXTEXT("IInstancesEnd (Pointer hObject) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeHVGraphicsIInstancesClear, nullptr, SEXTEXT("IInstancesClear (Pointer hObject) -> "));
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
	void NativeHVGraphicsISceneBuilderAddAllStatics(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(IString*);
		IString* _prefix;
		ReadInput(_prefix, _sf, -_offset);
		fstring prefix { _prefix->buffer, _prefix->length };


		HV::Graphics::ISceneBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddAllStatics(prefix);
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
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderAddAllStatics, nullptr, SEXTEXT("ISceneBuilderAddAllStatics (Pointer hObject)(Sys.Type.IString prefix) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderClear, nullptr, SEXTEXT("ISceneBuilderClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeHVGraphicsISceneBuilderSetClearColour, nullptr, SEXTEXT("ISceneBuilderSetClearColour (Pointer hObject)(Float32 red)(Float32 green)(Float32 blue) -> "));
	}
}}
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

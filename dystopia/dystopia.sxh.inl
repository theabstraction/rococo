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

namespace Dystopia { 
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

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddEnemy(*pos, editorId);
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

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddAlly(*pos, editorId);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddAmmunition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 count;
		_offset += sizeof(count);
		ReadInput(count, _sf, -_offset);

		float massPerClip;
		_offset += sizeof(massPerClip);
		ReadInput(massPerClip, _sf, -_offset);

		float massPerBullet;
		_offset += sizeof(massPerBullet);
		ReadInput(massPerBullet, _sf, -_offset);

		int32 ammoType;
		_offset += sizeof(ammoType);
		ReadInput(ammoType, _sf, -_offset);

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

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddAmmunition(*pos, editorId, name, imageFile, ammoType, massPerBullet, massPerClip, count);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddRangedWeapon(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float massKg;
		_offset += sizeof(massKg);
		ReadInput(massKg, _sf, -_offset);

		int32 ammoType;
		_offset += sizeof(ammoType);
		ReadInput(ammoType, _sf, -_offset);

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

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddRangedWeapon(*pos, editorId, name, imageFile, muzzleVelocity, flightTime, ammoType, massKg);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddArmour(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float massKg;
		_offset += sizeof(massKg);
		ReadInput(massKg, _sf, -_offset);

		int32 dollSlot;
		_offset += sizeof(dollSlot);
		ReadInput(dollSlot, _sf, -_offset);

		int32 bulletProt;
		_offset += sizeof(bulletProt);
		ReadInput(bulletProt, _sf, -_offset);

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

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddArmour(*pos, editorId, name, imageFile, bulletProt, dollSlot, massKg);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderAddSolid(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		int32 flags;
		_offset += sizeof(flags);
		ReadInput(flags, _sf, -_offset);

		ID_MESH editorId;
		_offset += sizeof(editorId);
		ReadInput(editorId, _sf, -_offset);

		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_ENTITY entityId;
		entityId = _pObject->AddSolid(*pos, editorId, flags);
		_offset += sizeof(entityId);
		WriteOutput(entityId, _sf, -_offset);
	}
	void NativeDystopiaILevelBuilderName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Name(entityId, name);
	}
	void NativeDystopiaILevelBuilderSetPosition(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* pos;
		_offset += sizeof(pos);
		ReadInput(pos, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetPosition(entityId, *pos);
	}
	void NativeDystopiaILevelBuilderSetVelocity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* velocity;
		_offset += sizeof(velocity);
		ReadInput(velocity, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetVelocity(entityId, *velocity);
	}
	void NativeDystopiaILevelBuilderSetHeading(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Radians theta;
		_offset += sizeof(theta);
		ReadInput(theta, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetHeading(entityId, theta);
	}
	void NativeDystopiaILevelBuilderSetElevation(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Radians phi;
		_offset += sizeof(phi);
		ReadInput(phi, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetElevation(entityId, phi);
	}
	void NativeDystopiaILevelBuilderSetLevel(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _filename;
		ReadInput(_filename, _sf, -_offset);
		fstring filename { _filename->buffer, _filename->length };


		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetLevel(filename);
	}
	void NativeDystopiaILevelBuilderSetScale(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec3* scale;
		_offset += sizeof(scale);
		ReadInput(scale, _sf, -_offset);

		ID_ENTITY entityId;
		_offset += sizeof(entityId);
		ReadInput(entityId, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetScale(entityId, *scale);
	}
	void NativeDystopiaILevelBuilderGenerateCity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Metres radius;
		_offset += sizeof(radius);
		ReadInput(radius, _sf, -_offset);

		_offset += sizeof(void*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->GenerateCity(name, radius);
	}
	void NativeDystopiaILevelBuilderAddStreetName(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _name;
		ReadInput(_name, _sf, -_offset);
		fstring name { _name->buffer, _name->length };


		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddStreetName(name);
	}
	void NativeDystopiaILevelBuilderPopulateCity(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		float populationDensity;
		_offset += sizeof(populationDensity);
		ReadInput(populationDensity, _sf, -_offset);

		Dystopia::ILevelBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->PopulateCity(populationDensity);
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

namespace Dystopia { 
	void AddNativeCalls_DystopiaILevelBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::ILevel* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Levels.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaLevelsGetLevel, _nceContext, SEXTEXT("GetHandleForILevel0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddEnemy, nullptr, SEXTEXT("ILevelAddEnemy (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddAlly, nullptr, SEXTEXT("ILevelAddAlly (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddAmmunition, nullptr, SEXTEXT("ILevelAddAmmunition (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId)(Sys.Type.IString name)(Sys.Type.IString imageFile)(Int32 ammoType)(Float32 massPerBullet)(Float32 massPerClip)(Int32 count) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddRangedWeapon, nullptr, SEXTEXT("ILevelAddRangedWeapon (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId)(Sys.Type.IString name)(Sys.Type.IString imageFile)(Float32 muzzleVelocity)(Float32 flightTime)(Int32 ammoType)(Float32 massKg) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddArmour, nullptr, SEXTEXT("ILevelAddArmour (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId)(Sys.Type.IString name)(Sys.Type.IString imageFile)(Int32 bulletProt)(Int32 dollSlot)(Float32 massKg) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddSolid, nullptr, SEXTEXT("ILevelAddSolid (Pointer hObject)(Sys.Maths.Vec3 pos)(Int32 editorId)(Int32 flags) -> (Int64 entityId)"));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderName, nullptr, SEXTEXT("ILevelName (Pointer hObject)(Int64 entityId)(Sys.Type.IString name) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetPosition, nullptr, SEXTEXT("ILevelSetPosition (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 pos) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetVelocity, nullptr, SEXTEXT("ILevelSetVelocity (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 velocity) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetHeading, nullptr, SEXTEXT("ILevelSetHeading (Pointer hObject)(Int64 entityId)(Float32 theta) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetElevation, nullptr, SEXTEXT("ILevelSetElevation (Pointer hObject)(Int64 entityId)(Float32 phi) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetLevel, nullptr, SEXTEXT("ILevelSetLevel (Pointer hObject)(Sys.Type.IString filename) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetScale, nullptr, SEXTEXT("ILevelSetScale (Pointer hObject)(Int64 entityId)(Sys.Maths.Vec3 scale) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderGenerateCity, nullptr, SEXTEXT("ILevelGenerateCity (Pointer hObject)(Sys.Type.IString name)(Float32 radius) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderAddStreetName, nullptr, SEXTEXT("ILevelAddStreetName (Pointer hObject)(Sys.Type.IString name) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderPopulateCity, nullptr, SEXTEXT("ILevelPopulateCity (Pointer hObject)(Float32 populationDensity) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderClear, nullptr, SEXTEXT("ILevelClear (Pointer hObject) -> "));
		ss.AddNativeCall(ns, NativeDystopiaILevelBuilderSetPlayerId, nullptr, SEXTEXT("ILevelSetPlayerId (Pointer hObject)(Int64 playerId) -> "));
	}
}
namespace Dystopia { 
	bool TryParse(const fstring& s, SolidFlags& value)
	{
		if (s == L"SolidFlags_None"_fstring)
		{
			value = SolidFlags_None;
		}
		else if (s == L"SolidFlags_Obstacle"_fstring)
		{
			value = SolidFlags_Obstacle;
		}
		else if (s == L"SolidFlags_Selectable"_fstring)
		{
			value = SolidFlags_Selectable;
		}
		else if (s == L"SolidFlags_Skeleton"_fstring)
		{
			value = SolidFlags_Skeleton;
		}
		else if (s == L"SolidFlags_IsDirty"_fstring)
		{
			value = SolidFlags_IsDirty;
		}
		else if (s == L"SolidFlags_RoadSection"_fstring)
		{
			value = SolidFlags_RoadSection;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const fstring& s, SolidFlags& value)
	{
		if (s == L"None"_fstring)
		{
			value = SolidFlags_None;
		}
		else if (s == L"Obstacle"_fstring)
		{
			value = SolidFlags_Obstacle;
		}
		else if (s == L"Selectable"_fstring)
		{
			value = SolidFlags_Selectable;
		}
		else if (s == L"Skeleton"_fstring)
		{
			value = SolidFlags_Skeleton;
		}
		else if (s == L"IsDirty"_fstring)
		{
			value = SolidFlags_IsDirty;
		}
		else if (s == L"RoadSection"_fstring)
		{
			value = SolidFlags_RoadSection;
		}
		else
		{
			return false;
		}

		return true;
	}
}// Dystopia.SolidFlags

namespace Dystopia { 
	bool TryParse(const fstring& s, AnimationType& value)
	{
		if (s == L"AnimationType_Standstill"_fstring)
		{
			value = AnimationType_Standstill;
		}
		else if (s == L"AnimationType_Running"_fstring)
		{
			value = AnimationType_Running;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const fstring& s, AnimationType& value)
	{
		if (s == L"Standstill"_fstring)
		{
			value = AnimationType_Standstill;
		}
		else if (s == L"Running"_fstring)
		{
			value = AnimationType_Running;
		}
		else
		{
			return false;
		}

		return true;
	}
}// Dystopia.AnimationType

namespace Dystopia { 
	bool TryParse(const fstring& s, SkeletonType& value)
	{
		if (s == L"SkeletonType_HumanMale"_fstring)
		{
			value = SkeletonType_HumanMale;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const fstring& s, SkeletonType& value)
	{
		if (s == L"HumanMale"_fstring)
		{
			value = SkeletonType_HumanMale;
		}
		else
		{
			return false;
		}

		return true;
	}
}// Dystopia.SkeletonType

namespace Dystopia { 
	bool TryParse(const fstring& s, LimbIndex& value)
	{
		if (s == L"LimbIndex_Head"_fstring)
		{
			value = LimbIndex_Head;
		}
		else if (s == L"LimbIndex_LeftUpperArm"_fstring)
		{
			value = LimbIndex_LeftUpperArm;
		}
		else if (s == L"LimbIndex_RightUpperArm"_fstring)
		{
			value = LimbIndex_RightUpperArm;
		}
		else if (s == L"LimbIndex_Torso"_fstring)
		{
			value = LimbIndex_Torso;
		}
		else if (s == L"LimbIndex_LeftUpperLeg"_fstring)
		{
			value = LimbIndex_LeftUpperLeg;
		}
		else if (s == L"LimbIndex_RightUpperLeg"_fstring)
		{
			value = LimbIndex_RightUpperLeg;
		}
		else if (s == L"LimbIndex_LeftFoot"_fstring)
		{
			value = LimbIndex_LeftFoot;
		}
		else if (s == L"LimbIndex_RightFoot"_fstring)
		{
			value = LimbIndex_RightFoot;
		}
		else if (s == L"LimbIndex_LeftLowerArm"_fstring)
		{
			value = LimbIndex_LeftLowerArm;
		}
		else if (s == L"LimbIndex_RightLowerArm"_fstring)
		{
			value = LimbIndex_RightLowerArm;
		}
		else if (s == L"LimbIndex_LeftLowerLeg"_fstring)
		{
			value = LimbIndex_LeftLowerLeg;
		}
		else if (s == L"LimbIndex_RightLowerLeg"_fstring)
		{
			value = LimbIndex_RightLowerLeg;
		}
		else if (s == L"LimbIndex_LeftHand"_fstring)
		{
			value = LimbIndex_LeftHand;
		}
		else if (s == L"LimbIndex_RightHand"_fstring)
		{
			value = LimbIndex_RightHand;
		}
		else if (s == L"LimbIndex_Count"_fstring)
		{
			value = LimbIndex_Count;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const fstring& s, LimbIndex& value)
	{
		if (s == L"Head"_fstring)
		{
			value = LimbIndex_Head;
		}
		else if (s == L"LeftUpperArm"_fstring)
		{
			value = LimbIndex_LeftUpperArm;
		}
		else if (s == L"RightUpperArm"_fstring)
		{
			value = LimbIndex_RightUpperArm;
		}
		else if (s == L"Torso"_fstring)
		{
			value = LimbIndex_Torso;
		}
		else if (s == L"LeftUpperLeg"_fstring)
		{
			value = LimbIndex_LeftUpperLeg;
		}
		else if (s == L"RightUpperLeg"_fstring)
		{
			value = LimbIndex_RightUpperLeg;
		}
		else if (s == L"LeftFoot"_fstring)
		{
			value = LimbIndex_LeftFoot;
		}
		else if (s == L"RightFoot"_fstring)
		{
			value = LimbIndex_RightFoot;
		}
		else if (s == L"LeftLowerArm"_fstring)
		{
			value = LimbIndex_LeftLowerArm;
		}
		else if (s == L"RightLowerArm"_fstring)
		{
			value = LimbIndex_RightLowerArm;
		}
		else if (s == L"LeftLowerLeg"_fstring)
		{
			value = LimbIndex_LeftLowerLeg;
		}
		else if (s == L"RightLowerLeg"_fstring)
		{
			value = LimbIndex_RightLowerLeg;
		}
		else if (s == L"LeftHand"_fstring)
		{
			value = LimbIndex_LeftHand;
		}
		else if (s == L"RightHand"_fstring)
		{
			value = LimbIndex_RightHand;
		}
		else if (s == L"Count"_fstring)
		{
			value = LimbIndex_Count;
		}
		else
		{
			return false;
		}

		return true;
	}
}// Dystopia.LimbIndex

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

namespace Dystopia { 
	void AddNativeCalls_DystopiaIGui(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IGui* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Gui.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaGuiGetGui, _nceContext, SEXTEXT("GetHandleForIGui0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaIGuiShowDialogBox, nullptr, SEXTEXT("IGuiShowDialogBox (Pointer hObject)(Sys.Maths.Vec2i span)(Int32 retzone)(Int32 hypzone)(Sys.Type.IString title)(Sys.Type.IString message)(Sys.Type.IString buttons) -> "));
		ss.AddNativeCall(ns, NativeDystopiaIGuiAdd3DHint, nullptr, SEXTEXT("IGuiAdd3DHint (Pointer hObject)(Sys.Maths.Vec3 worldPos)(Sys.Type.IString message)(Float32 duration) -> "));
	}
}
// BennyHill generated Sexy native functions for Dystopia::IJournal 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeDystopiaIJournalAddHistory(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _body;
		ReadInput(_body, _sf, -_offset);
		fstring body { _body->buffer, _body->length };


		_offset += sizeof(void*);
		IString* _title;
		ReadInput(_title, _sf, -_offset);
		fstring title { _title->buffer, _title->length };


		Dystopia::IJournal* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddHistory(title, body);
	}
	void NativeDystopiaIJournalAddGoalMeet(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ArchetypeCallback completionFunction;
		_offset += sizeof(completionFunction);
		ReadInput(completionFunction, _sf, -_offset);

		Metres radius;
		_offset += sizeof(radius);
		ReadInput(radius, _sf, -_offset);

		ID_ENTITY b;
		_offset += sizeof(b);
		ReadInput(b, _sf, -_offset);

		ID_ENTITY a;
		_offset += sizeof(a);
		ReadInput(a, _sf, -_offset);

		_offset += sizeof(void*);
		IString* _body;
		ReadInput(_body, _sf, -_offset);
		fstring body { _body->buffer, _body->length };


		_offset += sizeof(void*);
		IString* _title;
		ReadInput(_title, _sf, -_offset);
		fstring title { _title->buffer, _title->length };


		Dystopia::IJournal* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		ID_GOAL id;
		id = _pObject->AddGoalMeet(title, body, a, b, radius, completionFunction);
		_offset += sizeof(id);
		WriteOutput(id, _sf, -_offset);
	}
	void NativeDystopiaIJournalCompleteFirst(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_GOAL precusorGoalId;
		_offset += sizeof(precusorGoalId);
		ReadInput(precusorGoalId, _sf, -_offset);

		ID_GOAL forGoalId;
		_offset += sizeof(forGoalId);
		ReadInput(forGoalId, _sf, -_offset);

		Dystopia::IJournal* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->CompleteFirst(forGoalId, precusorGoalId);
	}
	void NativeDystopiaIJournalFailFirst(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_GOAL precusorGoalId;
		_offset += sizeof(precusorGoalId);
		ReadInput(precusorGoalId, _sf, -_offset);

		ID_GOAL forGoalId;
		_offset += sizeof(forGoalId);
		ReadInput(forGoalId, _sf, -_offset);

		Dystopia::IJournal* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->FailFirst(forGoalId, precusorGoalId);
	}

	void NativeGetHandleForDystopiaJournalGetJournal(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		Dystopia::IJournal* nceContext = reinterpret_cast<Dystopia::IJournal*>(_nce.context);
		// Uses: Dystopia::IJournal* FactoryConstructDystopiaJournalGetJournal(Dystopia::IJournal* _context);
		Dystopia::IJournal* pObject = FactoryConstructDystopiaJournalGetJournal(nceContext);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}

namespace Dystopia { 
	void AddNativeCalls_DystopiaIJournal(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IJournal* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.Journal.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaJournalGetJournal, _nceContext, SEXTEXT("GetHandleForIJournal0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaIJournalAddHistory, nullptr, SEXTEXT("IJournalAddHistory (Pointer hObject)(Sys.Type.IString title)(Sys.Type.IString body) -> "));
		ss.AddNativeCall(ns, NativeDystopiaIJournalAddGoalMeet, nullptr, SEXTEXT("IJournalAddGoalMeet (Pointer hObject)(Sys.Type.IString title)(Sys.Type.IString body)(Int64 a)(Int64 b)(Float32 radius)(Dystopia.Callbacks.TwoInt64InputFunction completionFunction) -> (Int64 id)"));
		ss.AddNativeCall(ns, NativeDystopiaIJournalCompleteFirst, nullptr, SEXTEXT("IJournalCompleteFirst (Pointer hObject)(Int64 forGoalId)(Int64 precusorGoalId) -> "));
		ss.AddNativeCall(ns, NativeDystopiaIJournalFailFirst, nullptr, SEXTEXT("IJournalFailFirst (Pointer hObject)(Int64 forGoalId)(Int64 precusorGoalId) -> "));
	}
}
namespace Dystopia { namespace UI { 
	bool TryParse(const fstring& s, EWidgetState& value)
	{
		if (s == L"EWidgetState_NoFocus"_fstring)
		{
			value = EWidgetState_NoFocus;
		}
		else if (s == L"EWidgetState_HasFocus"_fstring)
		{
			value = EWidgetState_HasFocus;
		}
		else
		{
			return false;
		}

		return true;
	}

	bool TryShortParse(const fstring& s, EWidgetState& value)
	{
		if (s == L"NoFocus"_fstring)
		{
			value = EWidgetState_NoFocus;
		}
		else if (s == L"HasFocus"_fstring)
		{
			value = EWidgetState_HasFocus;
		}
		else
		{
			return false;
		}

		return true;
	}
}}// Dystopia.UI.EWidgetState

// BennyHill generated Sexy native functions for Dystopia::UI::IUIBuilder 
namespace
{
	using namespace Sexy;
	using namespace Sexy::Sex;
	using namespace Sexy::Script;
	using namespace Sexy::Compiler;
	void NativeDystopiaUIIUIBuilderRebuildPanel(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _panelName;
		ReadInput(_panelName, _sf, -_offset);
		fstring panelName { _panelName->buffer, _panelName->length };


		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->RebuildPanel(panelName);
	}
	void NativeDystopiaUIIUIBuilderAddButton(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		_offset += sizeof(void*);
		IString* _text;
		ReadInput(_text, _sf, -_offset);
		fstring text { _text->buffer, _text->length };


		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddButton(id, *span, text);
	}
	void NativeDystopiaUIIUIBuilderAddFrame(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* span;
		_offset += sizeof(span);
		ReadInput(span, _sf, -_offset);

		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddFrame(id, frameId, *span);
	}
	void NativeDystopiaUIIUIBuilderAddWidgetToFrame(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->AddWidgetToFrame(frameId, id);
	}
	void NativeDystopiaUIIUIBuilderHCentreChildren(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->HCentreChildren(frameId);
	}
	void NativeDystopiaUIIUIBuilderVCentreChildren(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->VCentreChildren(frameId);
	}
	void NativeDystopiaUIIUIBuilderShrinkWrap(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ShrinkWrap(frameId);
	}
	void NativeDystopiaUIIUIBuilderExpandToFit(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		ID_WIDGET frameId;
		_offset += sizeof(frameId);
		ReadInput(frameId, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->ExpandToFit(frameId);
	}
	void NativeDystopiaUIIUIBuilderSetBorder(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb c2;
		_offset += sizeof(c2);
		ReadInput(c2, _sf, -_offset);

		RGBAb c1;
		_offset += sizeof(c1);
		ReadInput(c1, _sf, -_offset);

		Vec2i* dxdy;
		_offset += sizeof(dxdy);
		ReadInput(dxdy, _sf, -_offset);

		Dystopia::UI::EWidgetState state;
		_offset += sizeof(state);
		ReadInput(state, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBorder(id, state, *dxdy, c1, c2);
	}
	void NativeDystopiaUIIUIBuilderSetBackcolours(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb b2;
		_offset += sizeof(b2);
		ReadInput(b2, _sf, -_offset);

		RGBAb b1;
		_offset += sizeof(b1);
		ReadInput(b1, _sf, -_offset);

		Dystopia::UI::EWidgetState state;
		_offset += sizeof(state);
		ReadInput(state, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetBackcolours(id, state, b1, b2);
	}
	void NativeDystopiaUIIUIBuilderSetButtonPulse(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		boolean32 fireWhenUp;
		_offset += sizeof(fireWhenUp);
		ReadInput(fireWhenUp, _sf, -_offset);

		boolean32 fireWhenDown;
		_offset += sizeof(fireWhenDown);
		ReadInput(fireWhenDown, _sf, -_offset);

		ID_UI_EVENT_TYPE commandId;
		_offset += sizeof(commandId);
		ReadInput(commandId, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetButtonPulse(id, commandId, fireWhenDown, fireWhenUp);
	}
	void NativeDystopiaUIIUIBuilderSetFont(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		RGBAb fontColour;
		_offset += sizeof(fontColour);
		ReadInput(fontColour, _sf, -_offset);

		int32 fontId;
		_offset += sizeof(fontId);
		ReadInput(fontId, _sf, -_offset);

		Dystopia::UI::EWidgetState state;
		_offset += sizeof(state);
		ReadInput(state, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->SetFont(id, state, fontId, fontColour);
	}
	void NativeDystopiaUIIUIBuilderMove(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Vec2i* positionInContainer;
		_offset += sizeof(positionInContainer);
		ReadInput(positionInContainer, _sf, -_offset);

		ID_WIDGET id;
		_offset += sizeof(id);
		ReadInput(id, _sf, -_offset);

		Dystopia::UI::IUIBuilder* _pObject;
		_offset += sizeof(_pObject);

		ReadInput(_pObject, _sf, -_offset);
		_pObject->Move(id, *positionInContainer);
	}

	void NativeGetHandleForDystopiaUIGetUIBuilder(NativeCallEnvironment& _nce)
	{
		Sexy::uint8* sf = _nce.cpu.SF();
		ptrdiff_t offset = 2 * sizeof(size_t);
		Dystopia::UI::IUIBuilder* nceContext = reinterpret_cast<Dystopia::UI::IUIBuilder*>(_nce.context);
		// Uses: Dystopia::UI::IUIBuilder* FactoryConstructDystopiaUIGetUIBuilder(Dystopia::UI::IUIBuilder* _context);
		Dystopia::UI::IUIBuilder* pObject = FactoryConstructDystopiaUIGetUIBuilder(nceContext);
		offset += sizeof(void*);
		WriteOutput(pObject, sf, -offset);
	}
}

namespace Dystopia { namespace UI { 
	void AddNativeCalls_DystopiaUIIUIBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::UI::IUIBuilder* _nceContext)
	{
		const INamespace& ns = ss.AddNativeNamespace(SEXTEXT("Dystopia.UI.Native"));
		ss.AddNativeCall(ns, NativeGetHandleForDystopiaUIGetUIBuilder, _nceContext, SEXTEXT("GetHandleForIUIBuilder0  -> (Pointer hObject)"));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderRebuildPanel, nullptr, SEXTEXT("IUIBuilderRebuildPanel (Pointer hObject)(Sys.Type.IString panelName) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderAddButton, nullptr, SEXTEXT("IUIBuilderAddButton (Pointer hObject)(Int32 id)(Sys.Maths.Vec2i span)(Sys.Type.IString text) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderAddFrame, nullptr, SEXTEXT("IUIBuilderAddFrame (Pointer hObject)(Int32 id)(Int32 frameId)(Sys.Maths.Vec2i span) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderAddWidgetToFrame, nullptr, SEXTEXT("IUIBuilderAddWidgetToFrame (Pointer hObject)(Int32 frameId)(Int32 id) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderHCentreChildren, nullptr, SEXTEXT("IUIBuilderHCentreChildren (Pointer hObject)(Int32 frameId) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderVCentreChildren, nullptr, SEXTEXT("IUIBuilderVCentreChildren (Pointer hObject)(Int32 frameId) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderShrinkWrap, nullptr, SEXTEXT("IUIBuilderShrinkWrap (Pointer hObject)(Int32 frameId) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderExpandToFit, nullptr, SEXTEXT("IUIBuilderExpandToFit (Pointer hObject)(Int32 frameId) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderSetBorder, nullptr, SEXTEXT("IUIBuilderSetBorder (Pointer hObject)(Int32 id)(Int32 state)(Sys.Maths.Vec2i dxdy)(Int32 c1)(Int32 c2) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderSetBackcolours, nullptr, SEXTEXT("IUIBuilderSetBackcolours (Pointer hObject)(Int32 id)(Int32 state)(Int32 b1)(Int32 b2) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderSetButtonPulse, nullptr, SEXTEXT("IUIBuilderSetButtonPulse (Pointer hObject)(Int32 id)(Int64 commandId)(Bool fireWhenDown)(Bool fireWhenUp) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderSetFont, nullptr, SEXTEXT("IUIBuilderSetFont (Pointer hObject)(Int32 id)(Int32 state)(Int32 fontId)(Int32 fontColour) -> "));
		ss.AddNativeCall(ns, NativeDystopiaUIIUIBuilderMove, nullptr, SEXTEXT("IUIBuilderMove (Pointer hObject)(Int32 id)(Sys.Maths.Vec2i positionInContainer) -> "));
	}
}}

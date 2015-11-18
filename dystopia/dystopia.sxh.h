namespace Dystopia { 
	struct NO_VTABLE IMeshes
	{
		virtual void Load(const fstring& resourceName, ID_MESH editorId) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaIMeshes(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IMeshes* nceContext);
}

namespace Dystopia { 
	struct NO_VTABLE ILevelBuilder
	{
		virtual ID_ENTITY/* entityId */ AddEnemy(const Vec3& pos, ID_MESH editorId) = 0;
		virtual ID_ENTITY/* entityId */ AddAlly(const Vec3& pos, ID_MESH editorId) = 0;
		virtual ID_ENTITY/* entityId */ AddAmmunition(const Vec3& pos, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 ammoType, float massPerBullet, float massPerClip, int32 count) = 0;
		virtual ID_ENTITY/* entityId */ AddRangedWeapon(const Vec3& pos, ID_MESH editorId, const fstring& name, const fstring& imageFile, float muzzleVelocity, float flightTime, int32 ammoType, float massKg) = 0;
		virtual ID_ENTITY/* entityId */ AddArmour(const Vec3& pos, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 bulletProt, int32 dollSlot, float massKg) = 0;
		virtual ID_ENTITY/* entityId */ AddSolid(const Vec3& pos, ID_MESH editorId, int32 flags) = 0;
		virtual void SetPosition(ID_ENTITY entityId, const Vec3& pos) = 0;
		virtual void SetVelocity(ID_ENTITY entityId, const Vec3& velocity) = 0;
		virtual void SetHeading(ID_ENTITY entityId, Radians theta) = 0;
		virtual void SetElevation(ID_ENTITY entityId, Radians phi) = 0;
		virtual void SetScale(ID_ENTITY entityId, const Vec3& scale) = 0;
		virtual void GenerateCity(const fstring& name) = 0;
		virtual void PopulateCity(float populationDensity) = 0;
		virtual void Clear() = 0;
		virtual void SetPlayerId(ID_ENTITY playerId) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaILevelBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::ILevel* nceContext);
}

namespace Dystopia { 
	enum SolidFlags: int32
	{
		SolidFlags_None = 0, 	// 0x0
		SolidFlags_Obstacle = 1, 	// 0x1
		SolidFlags_Selectable = 2, 	// 0x2
		SolidFlags_Skeleton = 4, 	// 0x4
		SolidFlags_IsDirty = 8, 	// 0x8
	};
	bool TryParse(const fstring& s, SolidFlags& value);
	bool TryShortParse(const fstring& s, SolidFlags& value); 
}

namespace Dystopia { 
	enum AnimationType: int32
	{
		AnimationType_Standstill = 0, 	// 0x0
		AnimationType_Running = 1, 	// 0x1
	};
	bool TryParse(const fstring& s, AnimationType& value);
	bool TryShortParse(const fstring& s, AnimationType& value); 
}

namespace Dystopia { 
	enum SkeletonType: int32
	{
		SkeletonType_HumanMale = 0, 	// 0x0
	};
	bool TryParse(const fstring& s, SkeletonType& value);
	bool TryShortParse(const fstring& s, SkeletonType& value); 
}

namespace Dystopia { 
	enum LimbIndex: int32
	{
		LimbIndex_Head = 0, 	// 0x0
		LimbIndex_LeftUpperArm = 1, 	// 0x1
		LimbIndex_RightUpperArm = 2, 	// 0x2
		LimbIndex_Torso = 3, 	// 0x3
		LimbIndex_LeftUpperLeg = 4, 	// 0x4
		LimbIndex_RightUpperLeg = 5, 	// 0x5
		LimbIndex_LeftFoot = 6, 	// 0x6
		LimbIndex_RightFoot = 7, 	// 0x7
		LimbIndex_LeftLowerArm = 8, 	// 0x8
		LimbIndex_RightLowerArm = 9, 	// 0x9
		LimbIndex_LeftLowerLeg = 10, 	// 0xa
		LimbIndex_RightLowerLeg = 11, 	// 0xb
		LimbIndex_LeftHand = 12, 	// 0xc
		LimbIndex_RightHand = 13, 	// 0xd
		LimbIndex_Count = 14, 	// 0xe
	};
	bool TryParse(const fstring& s, LimbIndex& value);
	bool TryShortParse(const fstring& s, LimbIndex& value); 
}

namespace Dystopia { 
	struct NO_VTABLE IGui
	{
		virtual void ShowDialogBox(const Vec2i& span, int32 retzone, int32 hypzone, const fstring& title, const fstring& message, const fstring& buttons) = 0;
		virtual void Add3DHint(const Vec3& worldPos, const fstring& message, float duration) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaIGui(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IGui* nceContext);
}

namespace Dystopia { 
	struct NO_VTABLE IJournal
	{
		virtual void AddHistory(const fstring& title, const fstring& body) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaIJournal(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IJournal* nceContext);
}


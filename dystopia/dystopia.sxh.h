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
		virtual ID_ENTITY/* entityId */ AddEnemy(const Matrix4x4& transform, ID_MESH editorId) = 0;
		virtual ID_ENTITY/* entityId */ AddAlly(const Matrix4x4& transform, ID_MESH editorId) = 0;
		virtual ID_ENTITY/* entityId */ AddAmmunition(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 ammoType, float massPerBullet, float massPerClip, int32 count) = 0;
		virtual ID_ENTITY/* entityId */ AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, float muzzleVelocity, float flightTime, int32 ammoType, float massKg) = 0;
		virtual ID_ENTITY/* entityId */ AddArmour(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 bulletProt, int32 dollSlot, float massKg) = 0;
		virtual ID_ENTITY/* entityId */ AddSolid(const Matrix4x4& transform, ID_MESH editorId, int32 flags) = 0;
		virtual void GenerateCity(const fstring& name) = 0;
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
	};
}namespace Dystopia { 
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


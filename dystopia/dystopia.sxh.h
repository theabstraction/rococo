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
		virtual ID_ENTITY/* entityId */ AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, float muzzleVelocity, float flightTime) = 0;
		virtual ID_ENTITY/* entityId */ AddSolid(const Matrix4x4& transform, ID_MESH editorId) = 0;
		virtual void Clear() = 0;
		virtual void SetPlayerId(ID_ENTITY playerId) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaILevelBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::ILevel* nceContext);
}

namespace Dystopia { 
	struct NO_VTABLE IGui
	{
		virtual void ShowDialogBox(const Vec2& span, const fstring& title, const fstring& message, const fstring& buttons) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaIGui(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IGui* nceContext);
}


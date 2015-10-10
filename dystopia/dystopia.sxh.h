namespace Dystopia { 
	struct IMeshes
	{
		virtual void Load(fstring resourceName, ID_MESH editorId) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaIMeshes(Sexy::Script::IPublicScriptSystem& ss, Dystopia::IMeshes* nceContext);
}

namespace Dystopia { 
	struct ILevelBuilder
	{
		virtual ID_ENTITY/* entityId */ AddEntity(Rococo::Matrix4x4& transform, ID_MESH editorId) = 0;
		virtual void Clear() = 0;
		virtual void SetPlayerId(ID_ENTITY playerId) = 0;
	};
}

namespace Dystopia
{
	void AddNativeCalls_DystopiaILevelBuilder(Sexy::Script::IPublicScriptSystem& ss, Dystopia::ILevel* nceContext);
}


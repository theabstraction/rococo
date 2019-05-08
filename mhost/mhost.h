#pragma once

#include <rococo.api.h>
#include <rococo.renderer.h>

using namespace Rococo;

namespace MHost 
{
	namespace OS
	{
		struct KeyState
		{
			uint8 keys[256];
		};
	}
}

namespace MHost
{
	struct IEngineBase
	{
		virtual void Free() = 0;
	};

	struct IEngine;

	typedef Rococo::Script::ArchetypeCallback GuiPopulator;
}

#include "mhost.sxh.h"

namespace MHost
{
	using namespace Rococo;
	using namespace Rococo::Script;

	// Returns the top left position, using alignment flags to interpret how the pos argument is interpreted
	Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags);
	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc);

	ROCOCOAPI IEngineSupervisor : public IEngine
	{
		virtual void OnCompile(IPublicScriptSystem& ss) = 0;
		virtual void SetRunningScriptContext(IPublicScriptSystem* ss) = 0;
	};

	ROCOCOAPI IScriptDispatcher
	{
		virtual void Free() = 0;
		virtual void OnCompile(IPublicScriptSystem& ss) = 0;
		virtual void RouteGuiToScript(IPublicScriptSystem* ss, IGui* gui, const GuiPopulator& populator) = 0;
	};

	IScriptDispatcher* CreateScriptDispatcher();
}
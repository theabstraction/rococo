#pragma once

#include <rococo.api.h>
#include <rococo.ui.h>
#include <rococo.renderer.h>

using namespace Rococo;

#pragma pack(push,1)

namespace Rococo
{
	struct MHostKeyboardEvent
	{
		int32 scancode;
		int32 vkeyCode;
		int32 asciiCode;
		boolean32 isUp;
	};

	namespace Script
	{
		struct ArchetypeCallback;
		struct ISxyExpression;
	}
}

namespace MHost 
{
	namespace OS
	{
		struct KeyState
		{
			uint8 keys[256];
		};
	} // OS

	namespace Graphics
	{
		struct FontDesc
		{
			float32 ascent;
			float32 height;
		};
	}

	struct WorldOrientation
	{
		Degrees heading;
		Degrees elevation;
		Degrees roll;
	};

	ROCOCO_ID(IdTexture, uint64, 0);
} // MHost

#pragma pack(pop)

namespace MHost
{
	struct IEngineBase
	{
		virtual void Free() = 0;
	};

	struct IEngine;

	typedef Rococo::Script::ArchetypeCallback GuiPopulator;

	struct IDictionaryStream;

	namespace GuiTypes
	{
		enum class GuiEventId : int64;

#pragma pack(push,1)
		struct GuiEvent
		{
			GuiEventId eventId;
			Vec2i buttonPos;
			int64 metaId;
			cstr stringId;
		};
#pragma pack(pop)
	}
}

#include <rococo.io.h>
#include <sexy.compiler.public.h>
#include <..\rococo.mhost\code-gen\mhost.sxh.h>

namespace MHost
{
	using namespace Rococo;
	using namespace Rococo::Graphics;
	using namespace Rococo::Script;

	ROCOCO_INTERFACE IDicionaryStreamSupervisor: public IDictionaryStream
	{
		virtual void Free() = 0;
	};

	IDicionaryStreamSupervisor* CreateDictionaryStream(IO::IInstallation& installation);

	// Returns the top left position, using alignment flags to interpret how the pos argument is interpreted
	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc);

	ROCOCO_INTERFACE IEngineSupervisor : public IEngine
	{
		virtual void OnCompile(IPublicScriptSystem& ss) = 0;
		virtual void SetRunningScriptContext(IPublicScriptSystem* ss) = 0;
	};

	ROCOCO_INTERFACE IGuiOverlaySupervisor : public IGuiOverlay
	{

	};

	ROCOCO_INTERFACE IScriptDispatcher
	{
		virtual void Free() = 0;
		virtual void OnCompile(IPublicScriptSystem& ss) = 0;
		virtual void RouteGuiToScript(IPublicScriptSystem* ss, IGui* gui, const GuiPopulator& populator) = 0;
	};

	IScriptDispatcher* CreateScriptDispatcher();
}
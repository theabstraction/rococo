#pragma once

#include <rococo.types.h>

namespace Rococo
{
	typedef int32 ID_MESH;
}

using namespace Rococo;

namespace Sexy
{
	namespace Script
	{
		struct IPublicScriptSystem;
	}

	namespace Sex
	{
		struct ISParserTree;
	}
}

namespace Dystopia
{
	typedef int64 ID_ENTITY;

	struct ProjectileDef
	{
		ID_ENTITY attacker;
		Vec3 origin;
		Vec3 velocity;
		float lifeTime;
		ID_MESH bulletMesh;
	};

	struct fstring
	{
		const wchar_t* buffer;
		const int32 length;
	};

	using namespace Rococo;

	enum CMD_ID { CMD_ID_RETRY = 101, CMD_ID_IGNORE = 102, CMD_ID_EXIT = 103 };

	CMD_ID ShowContinueBox(Rococo::Windows::IWindow& renderWindow, const wchar_t* message);

	ROCOCOAPI IKeyboard
	{
		virtual uint32 GetScanCode(const wchar_t* keyName) const = 0;
	};

	ROCOCOAPI IKeyboardSupervisor : public IKeyboard
	{
		virtual void Free() = 0;
	};

	IKeyboardSupervisor* CreateKeyboardMap();

	struct IMeshLoader;
	struct ILevelBuilder;

	struct IInventory;
	struct IInventorySupervisor;
	struct IIntent;
	struct IHuman;

	enum HumanType : int32;
	struct IHumanSupervisor;
	struct IHumanFactory;

	enum StatIndex : int32;

	ROCOCOAPI ILevel
	{
		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime) = 0;
		virtual ILevelBuilder& Builder() = 0;
		virtual ID_ENTITY GetPlayerId() const = 0;
		virtual void GetPosition(Vec3& pos, ID_ENTITY id) const = 0;
		virtual void SetPosition(cr_vec3 pos, ID_ENTITY id) = 0;
		virtual void SetTransform(ID_ENTITY id, const Matrix4x4& transform) = 0;

		// Get references to a human's inventory and AI control object. 
		// Null can be supplied to an output if that output is not desired
		// Do not cache these references, they are invalidated when the human deleted from level
		virtual HumanType GetHuman(ID_ENTITY id, IInventory** ppInventory, IHuman** ppHuman) = 0;
		
		virtual IInventory* GetInventory(ID_ENTITY id) = 0;
	};

	ROCOCOAPI ILevelSupervisor: public ILevel
	{
		virtual void Free() = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0;
		virtual void UpdateObjects(float gameTime, float dt) = 0;
	};

	ROCOCOAPI ILevelLoader
	{
		virtual void Free() = 0;
		virtual void Load(const wchar_t* resourceName, bool isReloading) = 0;
		virtual void SyncWithModifiedFiles() = 0;
	};

	struct Environment;

	ROCOCOAPI IDebugControl
	{
		virtual void Continue() = 0;
		virtual void StepOut() = 0;
		virtual void StepOver() = 0;
		virtual void StepNextSymbol() = 0;
		virtual void StepNext() = 0;
	};

	ROCOCOAPI IDebuggerWindow
	{
		virtual void AddDisassembly(bool clearFirst, const wchar_t* text) = 0;
		virtual void Free() = 0;
		virtual void Log(const wchar_t* format, ...) = 0;
		virtual Windows::IWindow& GetDebuggerWindowControl() = 0;
		virtual bool IsVisible() const = 0;
		virtual void ShowWindow(bool show, IDebugControl* debugControl) = 0;
		virtual Visitors::IUITree& StackTree() = 0;
		virtual Visitors::IUIList& RegisterList() = 0;
	};

	ROCOCOAPI ISourceCache
	{
		virtual Sexy::Sex::ISParserTree* GetSource(const wchar_t* resourceName) = 0;
		virtual void Free() = 0;
		virtual void Release(const wchar_t* resourceName) = 0;
	};

	struct Environment;

	IOS& GetOS(Environment& e);

	void Free(Environment* e);

	ILevelLoader* CreateLevelLoader(Environment& e, ILevel& level);
	ILevelSupervisor* CreateLevel(Environment& e, IHumanFactory& humanFactory);
	IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow* parent);

	struct ScriptCompileArgs
	{
		Sexy::Script::IPublicScriptSystem& ss;
	};
	void ExecuteSexyScriptLoop(size_t maxBytes, Environment& e, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile);
	ISourceCache* CreateSourceCache(IInstallation& installation);
}

#include "dystopia.sxh.h"

namespace Rococo
{
	struct IGuiRenderContext;
	struct KeyboardEvent;
	struct MouseEvent;
}

namespace Dystopia
{
	enum GuiEventType
	{
		GuiEventType_CURSOR_BUTTON1_HELD,
		GuiEventType_CURSOR_BUTTON1_RELEASED
	};

	struct GuiEventArgs
	{
		const wchar_t* controlScript;
		GuiEventType type;
	};

	ROCOCOAPI IGuiSupervisor : public IGui
	{
		virtual void AppendKeyboardEvent(const KeyboardEvent& ke) = 0;
		virtual void AppendMouseEvent(const MouseEvent& me) = 0;
		virtual void Free() = 0;
		virtual bool HasFocus() const = 0;
		virtual void Render(IGuiRenderContext& rc) = 0;
		virtual void SetEventHandler(IEventCallback<GuiEventArgs>* guiEventHandler) = 0;
	};

	IGuiSupervisor* CreateGui(IRenderer& renderer);

	struct Environment
	{
		IInstallation& installation;
		IRenderer& renderer;
		IDebuggerWindow& debuggerWindow;
		ISourceCache& sourceCache;
		IMeshLoader& meshes;
		IGui& gui;
	};
}
#pragma once

#include <rococo.types.h>

namespace Rococo
{
	typedef int32 ID_MESH;
	struct IScene;
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
	struct IHumanAI;

	enum HumanType : int32;
	struct IHumanAISupervisor;
	struct IHumanFactory;

	enum StatIndex : int32;

	struct HumanSpec
	{
		HumanType type;
		IInventory* inventory;
		IHumanAI* ai;
	};

	struct EquipmentDesc
	{
		Vec3 worldPosition;
		IInventory* inventory;
	};

	ROCOCOAPI ILevel
	{
		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime) = 0;
		virtual ILevelBuilder& Builder() = 0;
		virtual HumanSpec GetHuman(ID_ENTITY id) = 0;
		virtual IInventory* GetInventory(ID_ENTITY id) = 0;
		virtual ID_ENTITY GetPlayerId() const = 0;
		virtual void GetPosition(ID_ENTITY id, Vec3& pos) const = 0;
		virtual void SetPosition(ID_ENTITY id, cr_vec3 pos) = 0;
		virtual void SetTransform(ID_ENTITY id, const Matrix4x4& transform) = 0;
		virtual void SetGroundCursorPosition(cr_vec3 groundZero) = 0;
		virtual ID_ENTITY SelectedId() = 0;

		virtual void DeleteEquipment(ID_ENTITY id) = 0;
		virtual bool TryGetEquipment(ID_ENTITY id, EquipmentDesc& desc) const = 0;
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
	struct IUIStack;

	enum GuiEventType
	{
		GuiEventType_CURSOR_BUTTON1_HELD,
		GuiEventType_CURSOR_BUTTON1_RELEASED,
		GuiEventType_CONTEXT_POLL
	};

	struct GuiEventArgs
	{
		const wchar_t* controlScript;
		GuiEventType type;
	};

	struct ContextMenuItem
	{
		const wchar_t* buttonName;
		int32 commandId;
		int64 context;
		bool isActive;
	};

	ROCOCOAPI IGuiSupervisor : public IGui
	{
		virtual void Free() = 0;
		virtual void SetEventHandler(IEventCallback<GuiEventArgs>* guiEventHandler) = 0;
	};

	struct TimestepEvent
	{
		const ticks appStartTicks;
		const ticks now;
		const ticks deltaTicks;
		const ticks hz;
	};

	struct AdvanceTimestepEvent
	{
		TimestepEvent cpuTime;
		float dt;
		float gameTime;
	};

	IGuiSupervisor* CreateGui(Environment& e, IUIStack& stack);

	enum PaneModality
	{
		PaneModality_Modal,
		PaneModality_Modeless,
	};

	ROCOCOAPI IUIPane
	{
		virtual PaneModality OnTimestep(const TimestepEvent& timestep) = 0;
		virtual PaneModality OnKeyboardEvent(const KeyboardEvent& ke) = 0;
		virtual PaneModality OnMouseEvent(const MouseEvent& me) = 0;
		virtual void RenderGui(IGuiRenderContext& grc) = 0;
		virtual void RenderObjects(IRenderContext& rc) = 0;
	};

	ROCOCOAPI IUIPaneSupervisor: public IUIPane
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IUIControlPane: IUIPaneSupervisor
	{
		virtual IIntent* PlayerIntent() = 0;
	};

	IUIControlPane* CreatePaneIsometric(Environment& e, ILevelSupervisor& level);
	IUIPaneSupervisor* CreateGuiWorldInterface(Environment& e, ILevelSupervisor& level);

	IUIPaneSupervisor* CreateDialogBox(Environment& e, IEventCallback<GuiEventArgs>& _handler,
		const wchar_t* _title,
		const wchar_t* _message,
		const wchar_t* _buttons,
		Vec2i _span,
		int32 _retzone,
		int32 _hypzone);

	IUIPaneSupervisor* CreateContextMenu(Environment& e, Vec2i topLeft, ContextMenuItem newMenu[], IEventCallback<ContextMenuItem>& onClick);
	
	enum ID_PANE;

	ROCOCOAPI IUIPaneFactory
	{
		virtual void FreeInstance(ID_PANE id, IUIPaneSupervisor* pane) = 0;
		virtual IUIPaneSupervisor* GetOrCreatePane(ID_PANE id) = 0; // Either construct or retrieve pane
	};

	struct PaneBind
	{
		IUIPaneSupervisor& pane;
		ID_PANE id;
	};

	ROCOCOAPI IUIStack
	{
		virtual PaneBind PopTop() = 0;
		virtual PaneBind PushTop(ID_PANE id) = 0;
		virtual PaneBind PushTop(IUIPaneSupervisor* pane, ID_PANE id) = 0;
		virtual PaneBind Top() = 0;
	};

	ROCOCOAPI IUIStackSupervisor : public IUIStack
	{
		virtual void Free() = 0;
		virtual void OnCreated() = 0;
		virtual void SetFactory(IUIPaneFactory& factory) = 0;
		virtual IScene& Scene() = 0;
	};

	IUIStackSupervisor* CreateUIStack(Post::IPostbox& postbox);

	struct Environment
	{
		IInstallation& installation;
		IRenderer& renderer;
		IDebuggerWindow& debuggerWindow;
		ISourceCache& sourceCache;
		IMeshLoader& meshes;
		IGui& gui;
		IUIStack& uiStack;
		Post::IPostbox& postbox;
	};

	enum ID_PANE
	{
		ID_PANE_ISOMETRIC_GAME_VIEW,
		ID_PANE_GUI_WORLD_INTERFACE,
		ID_PANE_GENERIC_DIALOG_BOX,
		ID_PANE_GENERIC_CONTEXT_MENU
	};
}

namespace Rococo
{
	namespace Post
	{
		using namespace Dystopia;

		enum POST_TYPE : int64
		{
			POST_TYPE_INVALID = 0,
			POST_TYPE_MOUSE_EVENT,
			POST_TYPE_KEYBOARD_EVENT,
			POST_TYPE_TIMESTEP,
			POST_TYPE_ADVANCE_TIMESTEP // sent when the game/simulation advances by dt
		};

		inline POST_TYPE GetPostType(const MouseEvent& t) { return POST_TYPE_MOUSE_EVENT; }
		inline POST_TYPE GetPostType(const KeyboardEvent& t) { return POST_TYPE_KEYBOARD_EVENT; }
		inline POST_TYPE GetPostType(const TimestepEvent& t) { return POST_TYPE_TIMESTEP; }
		inline POST_TYPE GetPostType(const AdvanceTimestepEvent& t) { return POST_TYPE_ADVANCE_TIMESTEP; }
	}
}
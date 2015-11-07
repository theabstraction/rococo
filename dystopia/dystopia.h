#pragma once

#include <rococo.types.h>

namespace Rococo
{
	struct IScene;
	struct IGuiRenderContext;
	struct KeyboardEvent;
	struct MouseEvent;
}

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
	using namespace Rococo;

	struct IMeshLoader;
	struct ILevelBuilder;

	struct IInventory;
	struct IInventorySupervisor;
	struct IIntent;
	struct IHumanAI;

	enum HumanType : int32;
	struct IHumanAISupervisor;
	struct IHumanFactory;
	struct ISourceCache;
	struct Environment;

	enum StatIndex : int32;

	class ID_ENTITY
	{
	private:
		uint64 value;
	public:
		ID_ENTITY() : value(0) {}
		explicit ID_ENTITY(uint64 _value) : value(_value) {}
		operator uint64() const { return value; }
		operator bool() const { return value != 0; }
		size_t operator()(const ID_ENTITY& k) const { return k; }
		static ID_ENTITY Invalid() { return ID_ENTITY(); }
	};

	inline bool operator == (ID_ENTITY a, ID_ENTITY b) { return (uint64) a == (uint64) b; }
	inline bool operator != (ID_ENTITY a, ID_ENTITY b) { return !(a == b); }

	struct ProjectileDef
	{
		ID_ENTITY attacker;
		Vec3 origin;
		Vec3 velocity;
		float lifeTime;
		ID_SYS_MESH bulletMesh;
	};

	enum CMD_ID { CMD_ID_RETRY = 101, CMD_ID_IGNORE = 102, CMD_ID_EXIT = 103 };

	CMD_ID ShowContinueBox(Rococo::Windows::IWindow& renderWindow, const wchar_t* message);

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

	ROCOCOAPI IKeyboard
	{
		virtual uint32 GetScanCode(const wchar_t* keyName) const = 0;
	};

	ROCOCOAPI IKeyboardSupervisor : public IKeyboard
	{
		virtual void Free() = 0;
	};

	IKeyboardSupervisor* CreateKeyboardMap(IInstallation& installation, ISourceCache& sourceCache);

	struct IItem;

	ROCOCOAPI ILevel
	{
		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime) = 0;
		virtual ILevelBuilder& Builder() = 0;
		virtual ID_ENTITY CreateStash(IItem* item, cr_vec3 location) = 0;
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

		virtual void RenderObjects(IRenderContext& rc) = 0;
		virtual void UpdateObjects(float gameTime, float dt) = 0;
	};

	ROCOCOAPI ILevelSupervisor: public ILevel
	{
		virtual void Free() = 0;
		virtual void OnCreated() = 0;
	};

	ROCOCOAPI ILevelLoader
	{
		virtual void Free() = 0;
		virtual void Load(const wchar_t* resourceName, bool isReloading) = 0;
		virtual void SyncWithModifiedFiles() = 0;
	};

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
		virtual int Log(const wchar_t* format, ...) = 0;
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

	IOS& GetOS(Environment& e);

	void Free(Environment* e);

	ILevelLoader* CreateLevelLoader(Environment& e);
	ILevelSupervisor* CreateLevel(Environment& e, IHumanFactory& humanFactory);
	IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow* parent);

	struct ScriptCompileArgs
	{
		Sexy::Script::IPublicScriptSystem& ss;
	};
	void ExecuteSexyScriptLoop(size_t maxBytes, Environment& e, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile);
	ISourceCache* CreateSourceCache(IInstallation& installation);

	enum ActionMapType : int32;

	struct ActionMap
	{
		ActionMapType type;

		union
		{
			Vec2i vector;
			bool isActive;
		};
	};

	struct IControls
	{
		virtual void LoadMapping(const wchar_t* resourceName) = 0;
		virtual void MapKeyboardEvent(const KeyboardEvent& ke, IEventCallback<ActionMap>& onAction) = 0;
		virtual void MapMouseEvent(const MouseEvent& me, IEventCallback<ActionMap>& onAction) = 0;
	};

	struct IControlsSupervisor : public IControls
	{
		virtual void AddAction(const wchar_t* name, ActionMapType type, bool isVector) = 0;
		virtual void Free() = 0;
	};

	IControlsSupervisor* CreateControlMapper(IInstallation& installation, ISourceCache& sourceCache);
}

#include "dystopia.sxh.h"

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

	ROCOCOAPI IGuiSupervisor : public IGui /* gui is script interface to allow scripts to bring up gui elements */
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
		const float dt;
		const float gameTime;
	};

	IGuiSupervisor* CreateGui(Environment& e, IUIStack& stack);

	enum Relay
	{
		Relay_None,
		Relay_Next,
	};

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
		IControls& controls;
		IBitmapCache& bitmapCache;
		ILevel& level;
	};

	enum ActionMapType
	{
		ActionMapTypeWait = 0,
		ActionMapTypeForward,
		ActionMapTypeBackward,
		ActionMapTypeLeft,
		ActionMapTypeRight,
		ActionMapTypeSelect,
		ActionMapTypeInventory,
		ActionMapTypeFire,
		ActionMapTypeRotate,
		ActionMapTypeScale,
		ActionMapType_TypeCount
	};

	void InitControlMap(IControlsSupervisor& controls);
	void BuildRandomCity(const fstring& name, uint32 seedDelta, Environment& e);
	ID_MESH GenerateRandomHouse(Environment& e);
}

#pragma once

#include <rococo.api.h>

namespace Rococo
{
	struct IScene;
	struct IGuiRenderContext;
	struct KeyboardEvent;
	struct MouseEvent;
	struct ObjectInstance;
	struct ISourceCache;
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
	struct Environment;
	struct ISkeleton;
	struct IBoneLibrary;

	enum StatIndex : int32;

	ROCOCO_ID(ID_GOAL, size_t, -1)

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
		virtual Vec3 GetForwardDirection(ID_ENTITY id) = 0;
		virtual cr_vec3 GetPosition(ID_ENTITY id) const = 0;
		virtual const Vec3* TryGetPosition(ID_ENTITY id) const = 0;
		virtual cr_vec3 GetVelocity(ID_ENTITY id) const = 0;
		virtual bool TryGetTransform(ID_ENTITY id, Matrix4x4& transform);
		virtual void SetPosition(ID_ENTITY id, cr_vec3 pos) = 0;
		virtual void SetVelocity(ID_ENTITY id, cr_vec3 v) = 0;
		virtual void SetHeading(ID_ENTITY id, Radians theta) = 0;
		virtual void SetElevation(ID_ENTITY id, Radians phi) = 0;
		virtual void SetScale(ID_ENTITY, cr_vec3 scale) = 0;
		virtual void SetGroundCursorPosition(cr_vec3 groundZero) = 0;
		virtual void SetNextAIUpdate(ID_ENTITY id, float nextUpdateTime) = 0;
		virtual ID_ENTITY SelectedId() = 0;
		virtual ID_ENTITY NearestRoadId() const = 0;
		virtual const wchar_t* TryGetName(ID_ENTITY id) = 0;
		virtual void Delete(ID_ENTITY id) = 0;
		virtual bool TryGetEquipment(ID_ENTITY id, EquipmentDesc& desc) const = 0;

		virtual void RenderObjects(IRenderContext& rc) = 0;
		virtual void UpdateGameTime(float dt) = 0;
	};

	ROCOCOAPI ILevelSupervisor: public ILevel
	{
		virtual void Free() = 0;
		virtual void OnCreated() = 0;
	};

	ROCOCOAPI ILevelLoader
	{
		virtual void ExecuteLevelFunction(ArchetypeCallback bytecodeId, IArgEnumerator& args) = 0;
		virtual void Free() = 0;
		virtual void Load(const wchar_t* resourceName, bool isReloading) = 0;
		virtual void SyncWithModifiedFiles() = 0;
	};

	IOS& GetOS(Environment& e);

	void Free(Environment* e);

	ILevelLoader* CreateLevelLoader(Environment& e);
	ILevelSupervisor* CreateLevel(Environment& e, IHumanFactory& humanFactory);
	IDebuggerWindow* CreateDebuggerWindow(Windows::IWindow* parent);

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

	ROCOCOAPI IControls
	{
		virtual void LoadMapping(const wchar_t* resourceName) = 0;
		virtual void MapKeyboardEvent(const KeyboardEvent& ke, IEventCallback<ActionMap>& onAction) = 0;
		virtual void MapMouseEvent(const MouseEvent& me, IEventCallback<ActionMap>& onAction) = 0;
	};

	ROCOCOAPI IControlsSupervisor : public IControls
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
	enum ID_PANE;

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
		const float dt;
		const float gameTime;
	};

	IGuiSupervisor* CreateGui(Environment& e, IUIStack& stack);

	enum Relay
	{
		Relay_None,
		Relay_Next,
	};

	enum HistoricEventType
	{
		HistoricEventType_Narrative,
		HistoricEventType_CompletedGoal,
		HistoricEventType_FailedGoal
	};

	struct HistoricEventLayout
	{
		int32 yTitlePixelRow;
		int32 yBodyPixelRow;
		int32 yEndOfBodyPixelRow;
	};

	ROCOCOAPI IHistoricEvent
	{
		virtual const wchar_t* Title() const = 0;
		virtual const wchar_t* Body() const = 0;
		virtual HistoricEventType Type() const = 0;
		virtual HistoricEventLayout& Layout() = 0;
	};

	enum GoalState
	{
		GoalState_Pending,
		GoalState_Ongoing,
		GoalState_Complete,
		GoalState_Failed
	};

	struct IGoal
	{
		virtual const wchar_t* Title() const = 0;
		virtual const wchar_t* Body() const = 0;
		virtual GoalState State() const = 0;
	};

	template<class T> ROCOCOAPI IMutableVectorEnumerator
	{
		virtual T& operator[](size_t index) = 0;
		virtual size_t Count() const = 0;
		virtual void Enumerate(IMutableEnumerator<T>& cb) = 0;
	};

	struct IJournalSupervisor : public IJournal
	{
		virtual void EnumerateGoals(IEnumerator<IGoal>& cb) = 0;
		virtual IMutableVectorEnumerator<IHistoricEvent>& History() = 0;
		virtual void Free() = 0;
		virtual bool IsReadyForRender() const = 0;
		virtual void PostConstruct() = 0;
		virtual void UpdateGoals() = 0;
	};

	struct IGoalSupervisor : public IGoal
	{
		virtual void Free() = 0;
		virtual void NotifyPrecursorInvalid() = 0;
		virtual void Start() = 0;
	};

	IGoalSupervisor* CreateGoal_MeetObject(Environment& e, Metres radius, const wchar_t* title, const wchar_t* body, ID_ENTITY a, ID_ENTITY b);

	IJournalSupervisor* CreateJournal(Environment& e);

	// Dystopia object environment - not necessarily well defined when passed to a constructor. Check the app constructor if in doubt.
	struct Environment
	{
		IInstallation& installation;
		IRenderer& renderer;
		IDebuggerWindow& debuggerWindow;
		ISourceCache& sourceCache;
		IMeshLoader& meshes;
		IBoneLibrary& boneLibrary;
		IGui& gui;
		IUIStack& uiStack;
		Post::IPostbox& postbox;
		IControls& controls;
		IBitmapCache& bitmapCache;
		ILevel& level;
		IJournalSupervisor& journal;
		ILevelLoader& levelLoader;
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
		ActionMapTypeStats,
		ActionMapTypeJournal,
		ActionMapType_TypeCount
	};

	void InitControlMap(IControlsSupervisor& controls);
	void BuildRandomCity(const fstring& name, uint32 seedDelta, Environment& e, IEnumerable<const wchar_t*>& names);
	ID_MESH GenerateRandomHouse(Environment& e, uint32 seed);

	ROCOCOAPI IPersistentScript
	{
		virtual void ExecuteFunction(ArchetypeCallback bytecodeId, IArgEnumerator& args) = 0;
		virtual void ExecuteFunction(const wchar_t* name, IArgEnumerator& args) = 0;
		virtual void Free() = 0;
	};

	IPersistentScript* CreatePersistentScript(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, const wchar_t* resourcePath, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile);

	void ExecuteSexyScriptLoop(size_t maxBytes, ISourceCache& sources, IDebuggerWindow& debugger, const wchar_t* resourcePath, int32 param, int32 maxScriptSizeBytes, IEventCallback<ScriptCompileArgs>& onCompile);
}

namespace Dystopia
{
	struct RangedWeapon;

	namespace AI
	{
		namespace Low
		{
			void RunForward(ILevel& level, ID_ENTITY actorId, MetresPerSecond speed);
			void RunInRandomDirection(ILevel& level, ID_ENTITY actorId, Seconds gameTime, Seconds runPeriod, MetresPerSecond runSpeed);
			void Stop(ILevel& level, ID_ENTITY actorId);

			// Rotate the actor towards the target point using the given angular velocity and timestep
			// Returns -2.0f if case is degenerate, otherwise dot product of previous heading and direction unit vectors 
			// The final parameter gives the alpha value, above which the target snaps exactly onto the heading direction
			float TurnTowardsTarget(ILevel& level, ID_ENTITY actorId, cr_vec3 targetPoint, Radians turnAnglePerSec, Seconds dt, float snapAlpha = 0.950f);

			void FireRangedWeapon(ILevel& level, Post::IPostbox* hintBox, ID_ENTITY creditToActorId, IItem* weapon, const ProjectileDef& def, Seconds now);

			// Get ranged weapon, doll's left hand takes priority over right.
			// If one hand is free, then weapon is used with both hands, and may get accuracy bonus etc
			RangedWeapon* GetRangedWeapon(ILevel& level, ID_ENTITY actorId, bool& isUsedTwoHanded, IItem** ppItem = nullptr);
		}

		namespace Middle
		{

		}

		namespace High
		{

		}
	}
}
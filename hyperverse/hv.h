#ifndef ROCOCO_HV
#define ROCOCO_HV

#include <rococo.mplat.h>

using namespace Rococo;
using namespace Rococo::Events;

namespace Rococo
{
	namespace Random
	{
		struct IRandom;
	}
}

namespace HV
{
	struct IPlayerSupervisor;
	struct ISectors;

	ROCOCOAPI IPlayerBase
	{
	   virtual float& JumpSpeed() = 0;
	   virtual float& DuckFactor() = 0;
	   virtual float Height() const = 0;
	};

	struct Cosmos;
	struct ISectorAIBuilder;
	struct ISectors;
	struct ISector;

	HV::ISectorAIBuilder* FactoryConstructHVSectorAIBuilder(Cosmos* c, int32 sectorId);
	ISector* GetFirstSectorContainingPoint(Vec2 a, ISectors& sectors);
}

#include "hv.script.types.h"

#define NO_VTABLE __declspec(novtable)

#include "hv.sxh.h"

namespace HV
{
	ROCOCO_ID(ID_OBJECT, uint64, 0);

	HV::ISectorLayout* GetSector(int32 index, ISectors& sectors);
	HV::ISectorLayout* GetSectorById(int32 index, ISectors& sectors);
	ISector* GetFirstSectorCrossingLine(Vec2 a, Vec2 b, ISectors& sectors);
	
	ROCOCOAPI ISectorBuildAPI
	{
		virtual void Attach(ISector * s) = 0;
		virtual void ClearSectors() = 0;
		virtual ISector* CreateSector() = 0;
		virtual MaterialId GetMaterialId(cstr name) = 0;
		virtual MaterialId GetRandomMaterialId(Rococo::Graphics::MaterialCategory cat) = 0;

		// platform.utilities.ShowBusy(true, "Loading level", "Created sector %u", s->Id());
		virtual void UpdateProgress(int id) = 0;

		virtual void GenerateMeshes() = 0;
	};

	struct VariableCallbackData;

	ROCOCOAPI ISectorBuilderSupervisor : public ISectorBuilder
	{
		virtual void EnumerateDoorVars(IEventCallback<VariableCallbackData> & cb) = 0;
		virtual void EnumerateWallVars(IEventCallback<VariableCallbackData>& cb) = 0;
		virtual void EnumerateFloorVars(IEventCallback<VariableCallbackData>& cb) = 0;
		virtual cstr GetTemplateFloorScript(bool& usesScript) const = 0;
		virtual cstr GetTemplateDoorScript(bool& hasDoor) const = 0;
		virtual cstr GetTemplateWallScript(bool& usesScript) const = 0;
		virtual void Free() = 0;
		virtual bool IsMeshGenerationEnabled() const = 0;
	};

	ISectorBuilderSupervisor* CreateSectorBuilder(ISectorBuildAPI& api);

	struct ConfigText
	{
		const fstring key;
		const fstring value;

		operator const fstring() { return key; }
	};

	struct ConfigInt
	{
		const fstring key;
		int value;

		operator const fstring() { return key; }
	};

	struct ConfigFloat
	{
		const fstring key;
		float value;

		operator const fstring() { return key; }
	};

	struct ConfigBool
	{
		const fstring key;
		const boolean32 value;
		operator const fstring() { return key; }
	};

	struct ISector;

	struct TagContext
	{
		ISector& sector;
		cstr tag;
		const int32 index;
	};

	ROCOCOAPI ITagCallback
	{
		virtual void OnTag(TagContext & context) = 0;
	};

	ROCOCOAPI ITags
	{
		 virtual void Invalidate() = 0;
		 virtual void ForEachSectorWithTag(cstr tag, ITagCallback& cb) = 0;
	};

	ROCOCOAPI ITagsSupervisor : ITags
	{
		virtual void Free() = 0;
	};

	namespace Events
	{
		namespace Player
		{
			struct PlayerActionEvent;
			struct PlayerViewChangeEvent;
		}
	}

	struct VariableCallbackData
	{
		cstr name;
		float value;
	};

	ROCOCOAPI IScriptConfigSupervisor : public IScriptConfig
	{
		 virtual void BindProperties(IBloodyPropertySetEditor & editor) = 0;
		 virtual void Enumerate(IEventCallback<VariableCallbackData>& cb) = 0;
		 virtual void SetVariable(cstr name, float value) = 0;
		 virtual void Free() = 0;
	};

	ROCOCOAPI IScriptConfigSet
	{
		virtual void Free() = 0;
		virtual void SetCurrentScript(cstr scriptName) = 0;
		virtual IScriptConfigSupervisor& Current() = 0;
		virtual void SetVariable(cstr name, float value) = 0;
	};

	IScriptConfigSet* CreateScriptConfigSet();
	IScriptConfigSupervisor* CreateScriptConfig();

	ROCOCOAPI IPlayerSupervisor
	{
	   virtual void Free() = 0;
	   virtual IPlayer* GetPlayer(int32 index) = 0;
	};

	IPlayerSupervisor* CreatePlayerSupervisor(Platform& platform);

	bool QueryYesNo(Windows::IWindow& ownerWindow, cstr message);

	struct IPropertyTarget;

	ROCOCOAPI IEditor
	{
	   virtual bool IsScrollLocked() const = 0;
	   virtual void Free() = 0;
	};

	struct ISectors;
	struct ISector;
	struct IEditorState;

	ROCOCOAPI IEditMode : public IUIElement
	{
	   virtual const ISector * GetHilight() const = 0;
	};

	ROCOCOAPI ISectorEditor
	{
		 virtual void Free() = 0;
		 virtual IEditMode& Mode() = 0;
		 virtual void CancelHilight() = 0;
		 virtual void SetEditor(IEditorState* editor) = 0;
	};

	ROCOCOAPI ISectorBuilderEditor
	{
		 virtual void Free() = 0;
		 virtual void SetTexture(int32 index, cstr name) = 0;
		 virtual cstr GetTexture(int32 index) const = 0;
		 virtual cstr GetTexture(int32 state) = 0;
		 virtual IEditMode& Mode() = 0;
	};

	ROCOCOAPI IWorldMap
	{
		 virtual ISectors & Sectors() = 0;
		 virtual void ZoomIn(int32 degrees) = 0;
		 virtual void ZoomOut(int32 degrees) = 0;
		 virtual void GrabAtCursor() = 0;
		 virtual void ReleaseGrab() = 0;
		 virtual Vec2 GetWorldPosition(Vec2i screenPosition) = 0;
		 virtual Vec2i GetScreenPosition(Vec2 worldPosition) = 0;
		 virtual Vec2 SnapToGrid(Vec2 worldPosition) = 0;
		 virtual void RenderTopGui(IGuiRenderContext& grc, ID_ENTITY cameraId) = 0;
		 virtual void Render(IGuiRenderContext& grc, const ISector* litSector, bool isTransparent) = 0;
	};

	ROCOCOAPI IFieldEditorEventHandler
	{
		virtual void OnActiveIndexChanged(int32 index, const char* stringRepresentation) = 0;
	};

	struct FieldEditorContext
	{
		IPublisher& publisher;
		IGUIStack& gui;
		IKeyboardSupervisor& keyboard;
		IFieldEditorEventHandler& onActiveChange;
		ID_FONT idFont;
	};

	ROCOCOAPI IFieldEditor
	{
		 virtual void AddInt32FieldUnbounded(cstr name, int32 value, bool preferHex) = 0;
		 virtual void AddInt32FieldBounded(cstr name, int32 value, int32 minValue, int32 maxValue) = 0;
		 virtual void AddFloat32FieldBounded(cstr name, float value, float minValue, float maxValue) = 0;
		 virtual void GetActiveValueAsString(char* buffer, size_t capacity) = 0;
		 virtual void AddStringField(cstr name, cstr value, size_t capacity, bool isVariableName) = 0;
		 virtual void Clear() = 0;
		 virtual void Deactivate() = 0;
		 virtual IUIElement& UIElement() = 0;
		 virtual void Free() = 0;
	};

	IFieldEditor* CreateFieldEditor(FieldEditorContext& context);

	ROCOCOAPI ITextureList
	{
		 virtual void Free() = 0;
		 virtual void ScrollTo(cstr filename) = 0;
	};

	ITextureList* CreateTextureList(Platform& _platform);

	ROCOCOAPI IWorldMapSupervisor : public IWorldMap
	{
		virtual void Free() = 0;
	};

	IWorldMapSupervisor* CreateWorldMap(Platform& platform, ISectors& sectors);
	ISectorEditor* CreateSectorEditor(Platform& _platform, IWorldMap& _map, Windows::IWindow& _parent);
	ISectorBuilderEditor* CreateSectorBuilder(IPublisher& publisher, IWorldMap& map);

	ROCOCOAPI IGameMode
	{
		 virtual void Activate() = 0;
		 virtual void Deactivate() = 0;
		 virtual void UpdateAI(const IUltraClock& clock) = 0;
	};

	ROCOCOAPI IFPSGameMode : public IGameMode
	{
		virtual IPropertyTarget * GetPropertyTarget() = 0;
	};

	ROCOCOAPI IFPSGameModeSupervisor : public IFPSGameMode
	{
		virtual void ClearCache() = 0;
		virtual void Free() = 0;
	};

	IEditor* CreateEditor(Platform& platform, IPlayerSupervisor& players, ISectors& sectors, IFPSGameMode& fpsGameMode);

	struct ObjectVertexBuffer
	{
		const ObjectVertex* v;
		const size_t VertexCount;
	};

	struct Segment
	{
		int32 perimeterIndexStart;
		int32 perimeterIndexEnd;
	};

	struct Gap
	{
		Vec2 a;
		Vec2 b;
		float z0;
		float z1;
		ISector* other;
		Sphere bounds;
		mutable int64 iterationFrame;
	};

	struct SectorAndSegment
	{
		ISector* sector;
		Segment segment;
	};

	SectorAndSegment GetFirstSectorWithVertex(Vec2 a, ISectors& sectors);

	struct IPropertyHost;

	ROCOCOAPI IPropertyTarget
	{
		virtual void Assign(IPropertyHost * host) = 0; // N.B a property target must never by Next to itself
		virtual void GetProperties(cstr category, IBloodyPropertySetEditor& editor) = 0;
		virtual void NotifyChanged(BloodyNotifyArgs& args) = 0;
	};

	ROCOCOAPI IPropertyHost
	{
		virtual void SetPropertyTarget(IPropertyTarget * target) = 0;
		virtual void SetPropertyTargetToSuccessor() = 0;
	};

	ROCOCOAPI IEditorState : public IPropertyHost
	{
	   virtual void BindSectorPropertiesToPropertyEditor(IPropertyTarget * target) = 0;
	   virtual cstr TextureName(int index) const = 0;
	};

	struct Barrier
	{
		Vec2 p; // barrier LHS
		Vec2 q; // barrier RHS 
		float z0; // Bottom level above ground
		float z1; // Top level above ground. z1 > z0
	};

	struct VisibleSector
	{
		ISector& sector;
	};

	ROCOCOAPI ISectorVisibilityBuilder
	{
		virtual size_t ForEverySectorVisibleBy(ISectors & sectors, const Matrix4x4 & cameraMatrix, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector> & cb) = 0;
		virtual void Free() = 0;
	};

	struct Material
	{
		MaterialVertexData mvd;
		char persistentName[IO::MAX_PATHLEN];
		Rococo::Graphics::MaterialCategory category;
	};

	struct MaterialArgs
	{
		Material* mat;
		BodyComponentMatClass bcmc;
	};

	ROCOCOAPI MatEnumerator
	{
		virtual void Enumerate(IEventCallback<MaterialArgs> & cb) = 0;
	};

	struct ISectorLayout;

	enum ADVANCE_STATE
	{
		// state has advanced, but not completed and will advance next timestep. 
		// The action trigger may restart the action sequence
		ADVANCE_STATE_YIELD,

		// state has yielded and trigger be ignored until delays have expired
		ADVANCE_STATE_YIELD_UNINTERRUPTIBLE,

		// state had advanced and completed, move on to next action in sequence
		ADVANCE_STATE_COMPLETED,

		// state has advanced and orderd the action sequence to terminate
		ADVANCE_STATE_TERMINATE
	};

	struct AdvanceInfo
	{
		IPublisher& publisher;
		const Seconds dt;
	};

	enum PARAMETER_TYPE
	{
		PARAMETER_TYPE_SECTOR_STRING,
		PARAMETER_TYPE_EVENT_NAME,
		PARAMETER_TYPE_GLOBALVAR_NAME,
		PARAMETER_TYPE_FLOAT,
		PARAMETER_TYPE_INT,
		PARAMETER_TYPE_INT_HEX,
		PARAMETER_TYPE_INT_UNBOUNDED
	};

	struct ParamDesc
	{
		cstr name;
		PARAMETER_TYPE type;
		float minValue;
		float maxValue;
	};

	struct ParameterBuffer
	{
		enum { CAPACITY = 256 };
		char data[CAPACITY];
		operator cstr() const { return data; }
	};

	struct ComponentRef
	{
		cstr name;
		ID_ENTITY id;
	};

	ROCOCOAPI ISectorContents
	{
		virtual void AddComponent(cr_m4x4 model, cstr componentName, cstr meshName) = 0;
		virtual ID_ENTITY AddItemToLargestSquare(const fstring& meshName, int addItemFlags, const HV::ObjectCreationSpec & obs) = 0;
		virtual ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) = 0;
		virtual void ClickButton() = 0;
		virtual void ClickLever() = 0;
		virtual void ClearAllComponents() = 0;
		virtual void ClearComponents(const fstring& componentName) = 0;
		virtual void ClearManagedEntities() = 0;
		virtual void DeleteItemsWithMesh(const fstring & prefix) = 0;
		virtual void DeleteScenery() = 0;
		virtual bool DoesSceneryCollide(const AABB& aabb) const = 0;
		virtual void ForEachComponent(IEventCallback<const ComponentRef>& cb) = 0;
		virtual void ForEveryObjectInContent(IEventCallback<const ID_ENTITY>& cb) = 0;
		virtual void Free() = 0;
		virtual void ManageEntity(ID_ENTITY id) = 0;
		virtual void NotifySectorPlayerIsInSector(const IUltraClock& clock) = 0;
		virtual boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) = 0;
		virtual bool TryClickButton(ID_ENTITY idButton, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach) = 0;
		virtual bool TryGetScenery(ID_ENTITY id, AABB& worldBounds) const = 0;
		virtual bool TryPlaceItemOnQuad(const Quad& qModel, ID_ENTITY quadsEntityId, ID_ENTITY itemId) = 0;
		virtual void UseUpFacingQuadsOnScenery(ID_ENTITY id) = 0;
		virtual void OnTick(const IUltraClock& clock) = 0;
		virtual void LowerScenery() = 0;
		virtual void RaiseScenery() = 0;
		virtual void ToggleElevation() = 0;
		virtual bool TraversalBlocked() const = 0;
	};

	struct IActionFactory;

	ROCOCOAPI IAction
	{
		 virtual IActionFactory & Factory() = 0;
		 virtual ADVANCE_STATE Advance(AdvanceInfo& info) = 0;
		 virtual int32 ParameterCount() const = 0;
		 virtual void GetParameter(int32 index, ParameterBuffer& buf) const = 0;
		 virtual void SetParameter(int32 index, cstr value) = 0;
		 virtual ParamDesc GetParameterName(int32 index) const = 0;
		 virtual void Format(char* buffer, size_t capacity) = 0;
		 virtual void Free() = 0;
	};

	Random::IRandom& GetRandomizer();

	ROCOCOAPI IGlobalVariables
	{
		enum { MAX_VARIABLE_NAME_LENGTH = 32 };
	// Get or else create a variable of the given name, returning true if the variable already exists
	// If the variable is created the variable is initialized to the default value passed into the function
	virtual bool GetValue(cstr name, int32& outputValue) const = 0;

	// Sets the value of the variable with the given name, returns the previous value.
	// If the variable was not defined, it is created, initialized, and the output is zero.
	virtual int32 SetValue(cstr name, int32 value) = 0;

	// Test the name for conformity to naming rules. If it is violated an IException is thrown
	// with a descriptive message that includes the original string.
	virtual void ValidateName(cstr name) const = 0;
	};

	ROCOCOAPI IIActionFactoryCreateContext
	{
		virtual IGlobalVariables & GetGlobals() = 0;
	};

	ROCOCOAPI IActionFactory
	{
		 virtual IAction * Create(IIActionFactoryCreateContext & context) = 0;
		 virtual cstr Name() const = 0;
	};

	IActionFactory& GetDefaultActionFactory();

	ROCOCOAPI IActionArray
	{
		 virtual IAction & operator[](int32 index) = 0;
		 virtual int32 Count() const = 0;
		 virtual void AddAction(IActionFactory& factory, IIActionFactoryCreateContext& context) = 0;
		 virtual void RemoveAction(int32 index) = 0;
		 virtual void SetAction(int32 index, IActionFactory& factory, IIActionFactoryCreateContext& context) = 0;
		 virtual void Swap(int32 i, int32 j) = 0;
	};

	ROCOCOAPI ITrigger
	{
		 virtual TriggerType Type() const = 0;
		 virtual void SetType(TriggerType type) = 0;
		 virtual IActionArray& Actions() = 0;
		 virtual IStringVector& GetStringVector() = 0;
	};

	struct ITriggerSupervisor : public ITrigger
	{
		virtual void QueueActionSequence() = 0;
		virtual void Advance(AdvanceInfo& info) = 0;
		virtual void Free() = 0;
	};

	ITriggerSupervisor* CreateTrigger();

	ROCOCOAPI IAIBrain
	{
		virtual void Free() = 0;
	};

	IAIBrain* CreateAIBrain(IPublisher& publisher, ISectors& sectors);

	IActionFactory& GetDefaultActionFactory();
	size_t ActionFactoryCount();
	IActionFactory& GetActionFactory(size_t index);
	IActionFactory& GetActionFactory(cstr name);

	ROCOCOAPI ITriggersAndActions
	{
	   virtual void AddTrigger(int32 pos) = 0;
	   virtual void RemoveTrigger(int32 pos) = 0;
	   virtual int32 TriggerCount() const = 0;
	   virtual ITrigger& operator[](int32 i) = 0;
	   virtual void AddAction(int32 triggerIndex) = 0;
	   virtual void RemoveAction(int32 triggerIndex, int32 actionIndex) = 0;
	};

	cstr GET_UNDEFINDED_TAG();

	ROCOCOAPI ITagContainer
	{
		// AddTag returns true if a tag was added, otherwise it returns false, indicating the tag already exists
		virtual bool AddTag(int32 pos, cstr text) = 0;
		virtual void RemoveTag(int32 pos) = 0;
		virtual void SetTag(int32 pos, cstr text) = 0;
		virtual void LowerTag(int32 pos) = 0;
		virtual void RaiseTag(int32 pos) = 0;
		[[nodiscard]] virtual int32 TagCount() const = 0;
		[[nodiscard]] virtual IStringVector& EnumTags() = 0;
	};

	struct SectorSquares
	{
		const AABB2d* first;
		const AABB2d* end;
	};

	// Getting to be a god class
	ROCOCOAPI ISector : public IPropertyTarget
	{
		 virtual ISectorContents& Contents() = 0;
		 virtual SectorSquares Squares() const = 0;
		 virtual ISectorAIBuilder& GetSectorAIBuilder() = 0;
		 virtual IIActionFactoryCreateContext& AFCC() = 0;
		 virtual ITagContainer& Tags() = 0;
		 virtual const AABB2d& GetAABB() const = 0;
		 virtual uint32 Id() const = 0;

		 virtual bool IsDirty() const = 0;

		 virtual bool UseAnythingAt(cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach) = 0;

		 // Iteration frames are used by some iteration functions to mark sectors as having been enumrerated
		 // Generally the frame count is incremented each function call
		 // 0x81000000000 to 0x82000000000 are from calls to ForEverySectorVisibleAt
		 virtual int64 IterationFrame() const = 0;
		 virtual void SetIterationFrame(int64 value) = 0;

		 virtual const Gap* GetGapAtSegment(const Vec2& a, const Vec2& b) const = 0;

		 virtual const Barrier* Barriers(size_t& barrierCount) const = 0;
		 virtual void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) = 0;
		 virtual bool DoesLineCrossSector(Vec2 a, Vec2 b) = 0;
		 virtual ObjectVertexBuffer FloorVertices() const = 0;
		 virtual const Gap* Gaps(size_t& count) const = 0;

		 virtual void Decouple() = 0; // Called to take the sector out of the world, prior to deletion
		 virtual void Free() = 0;
		 virtual float Z0() const = 0;
		 virtual float Z1() const = 0;
		 virtual Segment GetSegment(Vec2 p, Vec2 q) = 0;
		 virtual int32 GetFloorTriangleIndexContainingPoint(Vec2 p) = 0;
		 virtual RGBAb GetGuiColour(float intensity) const = 0;
		 virtual int32 GetPerimeterIndex(Vec2 a) const = 0;
		 virtual void InvokeSectorRebuild(bool force) = 0;
		 virtual const Vec2* WallVertices(size_t& nVertices) const = 0;
		 virtual void Rebuild() = 0;
		 virtual bool Is4PointRectangular() const = 0; // The sector has four points and its perimeter in 2D space is a rectangle or square
		 virtual bool IsCorridor() const = 0; // The sector Is4PointRectangular & two opposing edges are portals to other sectors and neither is itself a 4PtRect
		 virtual const Segment* GetWallSegments(size_t& count) const = 0;
		 virtual void OnSectorScriptChanged(const FileModifiedArgs& args) = 0;

		 virtual void ForEveryObjectInSector(IEventCallback<const ID_ENTITY>& cb) = 0;

		 virtual void SaveTemplate(StringBuilder& sb) = 0;
		 virtual void SetTemplate(MatEnumerator& enumerator) = 0;

		 virtual const LightSpec* Lights(size_t& numberOfLights) const = 0;

		 virtual void SyncEnvironmentMapToSector() = 0;

		 virtual ISectorLayout* Layout() = 0;

		 virtual void OnTick(const IUltraClock& clock) = 0;

		 virtual ITriggersAndActions& TriggersAndActions() = 0;
	};

	float GetHeightAtPointInSector(cr_vec3 p, ISector& sector);

	ISector* CreateSector(Platform& platform, ISectors& co_sectors);

	void RebaseSectors();

	ROCOCOAPI ISectors
	{
	   virtual bool IsMeshGenerationEnabled() const = 0;
	   virtual ISectorBuilderSupervisor* Builder() = 0;
	   virtual void Free() = 0;

	   virtual ITags& Tags() = 0;

	   virtual void AddSector(const Vec2* perimeter, size_t nVertices) = 0;
	   virtual void Delete(ISector* sector) = 0;

	   virtual ISector** begin() = 0;
	   virtual ISector** end() = 0;
	   virtual bool empty() const = 0;
	   virtual size_t size() const = 0;
	   virtual ISector& operator[](int index) = 0;
	   virtual const ISector& operator[](int index) const = 0;

	   virtual void OnSectorScriptChanged(const FileModifiedArgs& args) = 0;
	   virtual size_t ForEverySectorVisibleBy(cr_m4x4 worldToScreen, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector>& cb) = 0;

	   virtual void BindProperties(IBloodyPropertySetEditor& editor) = 0;
	   virtual void NotifyChanged() = 0;

	   virtual size_t GetSelectedSectorId() const = 0;
	   virtual void SelectSector(size_t id) = 0;

	   virtual IIActionFactoryCreateContext& AFCC() = 0;
	   virtual ISectorEnumerator& Enumerator() = 0;
	};

	void AdvanceInTime(ISectors& sectors, const IUltraClock& clock);

	ISectors* CreateSectors(Platform& platform);

	struct Cosmos
	{
		Platform& platform;
		IPlayerSupervisor& players;
		IEditor& editor;
		ISectors& sectors;
		IFPSGameModeSupervisor& fpsMode;
		IObjectPrototypeBuilder& object_prototypes;
	};

	struct IObjectManager;

	IFPSGameModeSupervisor* CreateFPSGameLogic(Platform& platform, IPlayerSupervisor& players, ISectors& sectors, IObjectManager& objects);

	IApp* CreateHVApp(Cosmos& e);
	void RunEnvironmentScript(Cosmos& e, cstr name, bool releaseSource = false, bool trace = false);

	namespace GraphicsEx
	{
		void DrawPointer(IGuiRenderContext& grc, Vec2i pos, Degrees heading, RGBAb shadowColour, RGBAb bodyColour);

		extern BodyComponentMatClass BodyComponentMatClass_Physics_Hull;
		extern BodyComponentMatClass BodyComponentMatClass_Brickwork;
		extern BodyComponentMatClass BodyComponentMatClass_Cement;
		extern BodyComponentMatClass BodyComponentMatClass_Floor;
		extern BodyComponentMatClass BodyComponentMatClass_Ceiling;
		extern BodyComponentMatClass BodyComponentMatClass_Door_Mullions;
		extern BodyComponentMatClass BodyComponentMatClass_Door_Panels;
		extern BodyComponentMatClass BodyComponentMatClass_Door_Casing;
		extern BodyComponentMatClass BodyComponentMatClass_Door_Rails;
	}

	void AddMathsEx(Rococo::Script::IPublicScriptSystem& ss);

	ROCOCOAPI ISectorAIBuilderSupervisor : ISectorAIBuilder
	{
		virtual void AdvanceInTime(IPublisher & publisher, const IUltraClock & clock) = 0;
		virtual void SaveAsScript(StringBuilder& sb) = 0;
		virtual ITagContainer& Tags() = 0;
		virtual ITriggersAndActions & TriggersAndActions() = 0;
		virtual void Trigger(TriggerType type) = 0;
		virtual void Free() = 0;
	};

	ISectorAIBuilderSupervisor* CreateSectorAI(IIActionFactoryCreateContext& afcc);

	namespace Roll
	{
		uint32 d(uint32 maxValue);
		uint32 x(uint32 oneAboveMaxValue);
		boolean32 FiftyFifty();
		float AnyOf(float minValue, float maxValue);
	}

	namespace HVMaths
	{
		bool IsQuadRectangular(const Quad& q);
		bool IsTriangleFacingUp(cr_m4x4 model, const VertexTriangle& t);
		void Expand(AABB2d& rect, Metres ds);
		bool GetRandomOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, int32 guesses);
		bool TryGetOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, Degrees theta);
		bool TryGetRotationToFit(Matrix4x4& Rz, bool randomizeHeading, const AABB& bounds, cr_vec2 containerSpan);
		bool TryGetRandomTransformation(Matrix4x4& model, AABB& worldBounds, bool randomHeading, bool randomizePosition, const AABB& bounds, const AABB2d& container, float z0, float z1);
		bool TryClickGraphicsMesh(ID_ENTITY idObject, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach, Platform& platform);
	}

	ROCOCOAPI IObjectPrototype : IObjectPrototypeBase
	{
		virtual const ObjectDynamics& Dynamics() const = 0;
		virtual const MaterialData& Mats() const = 0;
		virtual const fstring ShortName() const = 0;
		virtual const InventoryData& InvData() const = 0;
		virtual const ArmourData& Armour() const = 0;
		virtual const MeleeData& Melee() const = 0;
		virtual const Textures::BitmapLocation & Bitmap() const = 0;
		virtual bool CanFitSlot(int64 flags) const = 0;
		virtual const fstring Description() const = 0;
	};

	struct ObjectRef
	{
		IObjectPrototype* prototype;
		const int32 stackSize;
	};

	ROCOCOAPI IObjectPrototypeSupervisor : IObjectPrototype
	{
		virtual void Free() = 0;
	};

	ROCOCOAPI IObjectManager : IObjectPrototypeBuilder
	{
		virtual ID_OBJECT CreateObject(cstr name, int32 stackSize = 1) = 0;
		virtual ObjectRef GetObject(ID_OBJECT id) = 0;
		virtual void Free() = 0;
	};

	IObjectManager* CreateObjectManager(IRenderer& renderer);

	void FormatEquipmentInfo(char* buffer, size_t capacity, IObjectPrototype& p);
}


#endif
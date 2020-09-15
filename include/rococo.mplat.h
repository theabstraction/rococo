#pragma once

#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.visitors.h>
#include <rococo.app.h>
#include <rococo.os.h>
#include <rococo.events.h>

namespace Rococo
{
   ROCOCO_ID(ID_ENTITY, int64, 0);
   ROCOCO_ID(ID_PUPPET, uint64, 0);

   struct IEnumVector
   {
	   virtual int32 GetActiveIndex() const = 0;
	   virtual void SetActiveIndex(int32 index) = 0;
	   virtual void SetValue(int32 index, int32 value) = 0;
	   virtual int32 operator[] (int32 index) const = 0;
	   virtual int32 Count() const = 0;
   };

   ROCOCOAPI IStringVector
   {
	   virtual int32 Count() const = 0;
	   virtual void GetItem(int32 item, char* text, size_t capacity) const = 0;
   };

   struct FileUpdatedEvent : public Rococo::Events::EventArgs
   {
	   cstr pingPath;
   };

   struct TextOutputClickedEvent : public Rococo::Events::EventArgs
   {
	   cstr key;
	   cstr value;
   };

	struct LightSpec
	{
		Vec3 position;
		Vec3 direction;
		Degrees fov;
		RGBA diffuse;
		RGBA ambience;	// Only the nearest light's ambience is used for ambient lighting, and it is modulated by fog
		Degrees cutoffAngle;
		float cutoffPower;
		float attenuation;
		Metres nearPlane;
		Metres farPlane;
		float fogConstant; // fog = e^(range * fogExponent); // Take natural log of both sides to compute exponent, which should be negative
	};

	struct FlameDef
	{
		Metres minStartParticleSize;
		Metres maxStartParticleSize;
		Metres minEndParticleSize;
		Metres maxEndParticleSize;
		int32 particleCount;
		Seconds minLifeSpan;
		Seconds maxLifeSpan;
		float initialVelocityRange;
		float initialSpawnPosRange;
		float jetSpeed;
		Metres attractorHeight;
		Metres attractorMaxRange;
		Metres attractorMinRange;
		Metres attractorSpawnPosRange;
		Seconds attractorAIduration;
		float attractorResetProbability;
		float attractorDriftFactor;
		float attractorPerturbFactor;
		float attractorForce;
	};

	namespace Graphics
	{
		struct SampleStateDef;

		struct FontMetrics
		{
			int32 ascent;
			int32 descent; 
			int32 height; 
			int32 internalLeading; 
			int32 italic; 
			int32 weight;
			int32 imgWidth;
			int32 imgHeight;
		};
	}

	namespace Puppet
	{
		struct NewPuppetDesc;
	}

	struct QuadColours
	{
		RGBAb a;
		RGBAb b;
		RGBAb c;
		RGBAb d;
	};

	struct QuadVertices
	{
		Quad positions;
		GuiRectf uv;
		Quad normals;
		QuadColours colours;
	};
}

#include <../rococo.mplat/mplat.sxh.h>

namespace Rococo
{
	namespace Graphics
	{
		struct SampleStateDef
		{
			SampleMethod method;
			SampleFilter u;
			SampleFilter v;
			SampleFilter w;
			RGBA borderColour;
		};
	}

	namespace Puppet
	{
		ROCOCOAPI IPuppetsSupervisor : public Rococo::Puppet::IPuppets
		{
			virtual void Free() = 0;
		};

		IPuppetsSupervisor* CreatePuppets(size_t maxPuppets, size_t maxActivePuppets);
	}

	ROCOCOAPI IConfigSupervisor : public IConfig
	{
		virtual cstr GetText(cstr name) const = 0;
		virtual void Free() = 0;
	};

	IConfigSupervisor* CreateConfig();

	ROCOCOAPI IMathsVisitorSupervisor : public IMathsVisitor
	{
		 virtual bool AppendKeyboardEvent(const KeyboardEvent & key) = 0;
		 virtual void AppendMouseEvent(const MouseEvent& ev) = 0;

		 virtual void CancelSelect() = 0;
		 virtual void SelectAtPos(Vec2i pos) = 0;

		 virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, int padding) = 0;

		 virtual void Free() = 0;
	};

	struct IUtilitiies;
	struct IKeyboardSupervisor;
	struct ScriptCompileArgs;

	IMathsVisitorSupervisor* CreateMathsVisitor(IUtilitiies& utilities, Events::IPublisher& publisher);

	void Run_MPLat_EnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform);

	namespace Entities
	{
		ROCOCOAPI IEntity
		{
		   virtual Vec3 Position() const = 0;
		   virtual Matrix4x4& Model() = 0;
		   virtual ID_ENTITY ParentId() const = 0;
		   virtual const ID_ENTITY* begin() const = 0;
		   virtual const ID_ENTITY* end() const = 0;
		   virtual ID_SYS_MESH MeshId() const = 0;
		   virtual void SetMesh(ID_SYS_MESH id) = 0;
		};

		ROCOCOAPI IEntityCallback
		{
		   virtual void OnEntity(int64 index, IEntity & entity, ID_ENTITY id) = 0;
		};

		ROCOCOAPI IInstancesSupervisor : public IInstances
		{
		   virtual void ForAll(IEntityCallback & cb) = 0;
		   virtual void Free() = 0;
		   virtual IEntity* GetEntity(ID_ENTITY id) = 0;
		   virtual void ConcatenateModelMatrices(ID_ENTITY id, Matrix4x4& result) = 0;
		   virtual void ConcatenatePositionVectors(ID_ENTITY id, Vec3& position) = 0;
		   virtual Rococo::Graphics::IMeshBuilder& MeshBuilder() = 0;
		};
	}

	ROCOCOAPI IRigLoader
	{
		virtual void LoadFromFile(const fstring & pingName) = 0;
	};

	struct Key
	{
		cstr KeyName;
		bool isPressed;
	};

	ROCOCOAPI IKeyboardSupervisor : public IKeyboard
	{
	   virtual cstr GetAction(cstr keyName) = 0;
	   virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
	   virtual void AppendKeyboardInputToEditBuffer(int& caretPos, char* buffer, size_t capacity, const KeyboardEvent& key) = 0;
	   virtual void Free() = 0;
	};

	IKeyboardSupervisor* CreateKeyboardSupervisor();

	namespace Graphics
	{
		ROCOCOAPI IMeshBuilderSupervisor : public IMeshBuilder
		{
		   virtual void Free() = 0;
		   virtual void SaveCSV(cstr name, IExpandingBuffer& buffer) = 0;
		   virtual bool TryGetByName(cstr name, ID_SYS_MESH& id, AABB& bounds) = 0;
		   virtual IMathsVenue* Venue() = 0;
		   virtual const fstring GetName(ID_SYS_MESH id) const = 0;
		   virtual const VertexTriangle* GetTriangles(ID_SYS_MESH id, size_t& nTriangles) const = 0;
		   virtual const Triangle* GetPhysicsHull(ID_SYS_MESH id, size_t& nTriangles) const = 0;
		   virtual AABB Bounds(ID_SYS_MESH id) const = 0;
		};

		ROCOCOAPI ICameraSupervisor : public ICamera
		{
		   virtual void ElevateView(ID_ENTITY entityId, Degrees delta, cr_vec3 relativePos) = 0;
		   virtual void Free() = 0;
		   virtual void Update(const IUltraClock& clock) = 0;
		   virtual IMathsVenue& Venue() = 0;
		};

		ROCOCOAPI ITextTesselatorSupervisor : ITextTesselator
		{
			virtual void Free() = 0;
		};

		ITextTesselatorSupervisor* CreateTextTesselator(Platform& platform);

		ICameraSupervisor* CreateCamera(Entities::IInstancesSupervisor& instances, Entities::IMobiles& mobiles, IRenderer& renderer);

		ROCOCOAPI IScenePopulator
		{
			// Called to trigger scene population prior to shadow generation
			// DO NOT SET LIGHTS here, as it was the light array that caused this function to be called
			virtual void PopulateShadowCasters(ISceneBuilder & sb, const DepthRenderData & drd) = 0;

		// Called to trigger scene population prior to rendering
		// DO NOT SET LIGHTS here, as it was the light array that caused this function to be called
		virtual void PopulateScene(ISceneBuilder& sb) = 0;
		};

		ROCOCOAPI ISceneBuilderSupervisor : public ISceneBuilder
		{
		   virtual void Free() = 0;
		};

		ROCOCOAPI ISceneSupervisor : public IScene
		{
		   virtual void Free() = 0;
		   virtual ISceneBuilderSupervisor& Builder() = 0;
		   virtual void SetPopulator(IScenePopulator* populator) = 0;
		};

		ISceneSupervisor* CreateScene(Rococo::Entities::IInstancesSupervisor& instances, ICameraSupervisor& camera);

		ROCOCOAPI ISpriteSupervisor : public ISprites
		{
		   virtual void Free() = 0;
		};

		ISpriteSupervisor* CreateSpriteSupervisor(IRenderer& renderer);

		IQuadStackTesselator* CreateQuadStackTesselator();

		ROCOCOAPI IRendererConfigSupervisor : public IRendererConfig
		{
			virtual void Free() = 0;
		};

		IRendererConfigSupervisor* CreateRendererConfig(IRenderer& renderer);
	}

	namespace Entities
	{
		struct MoveMobileArgs
		{
			ID_ENTITY entityId;
			float fowardDelta;
			float straffeDelta;
			FPSAngles delta;
			FPSAngles angles;
		};

		ROCOCOAPI IMobilesSupervisor : public Entities::IMobiles
		{
		   virtual bool TryMoveMobile(const MoveMobileArgs & tmm) = 0;
		   virtual void Free() = 0;
		};

		IMobilesSupervisor* CreateMobilesSupervisor(Entities::IInstancesSupervisor& instances);

		IInstancesSupervisor* CreateInstanceBuilder(IRigLoader& meshLoader, Graphics::IMeshBuilderSupervisor& meshes, IRenderer& renderer, Events::IPublisher& publisher);

		ROCOCOAPI IParticleSystemSupervisor : IParticleSystem
		{
			virtual void Free() = 0;
			virtual void GetParticles(ID_ENTITY id, IRenderer& renderer) = 0;
		};

		IParticleSystemSupervisor* CreateParticleSystem(IRenderer& renderer, IInstances& instances);

		ROCOCOAPI IRigBuilderSupervisor : IRigBuilder
		{
			virtual void Free() = 0;
		};

		struct RigBuilderContext
		{

		};

		IRigBuilderSupervisor* CreateRigBuilder(RigBuilderContext& rbc);
	}

	struct Platform;

	struct ColourScheme
	{
		RGBAb topLeft;
		RGBAb bottomRight;
		RGBAb topLeftEdge;
		RGBAb bottomRightEdge;
		RGBAb fontColour;

		RGBAb hi_topLeft;
		RGBAb hi_bottomRight;
		RGBAb hi_topLeftEdge;
		RGBAb hi_bottomRightEdge;
		RGBAb hi_fontColour;
	};

	struct Modality
	{
		bool isTop;
		bool isModal;
		bool isUnderModal;
	};

	namespace Events
	{
		extern EventIdRef evUIInvoke;
		extern EventIdRef evUIPopulate;
		extern EventIdRef evBusy;
		extern EventIdRef evScreenResize;
	}

	struct AsciiEventArgs : public Events::EventArgs
	{
		fstring asciiText;
	};

	struct UIInvoke : public Events::EventArgs
	{
		char command[232];
	};

	struct PingPathArgs : public Events::EventArgs
	{
		cstr pingPath;
	};

	struct IUIElement
	{
		// Route a keyboard event to the element. Returns false to redirect event to the parent element
		virtual bool OnKeyboardEvent(const KeyboardEvent& key) = 0;
		virtual void OnRawMouseEvent(const MouseEvent& ev) = 0;
		virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) = 0;
		virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown) = 0;
		virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown) = 0;
		virtual void Render(IGuiRenderContext& rc, const GuiRect& absRect) = 0;
	};

	ROCOCOAPI IBloodyPropertySetEditor : public IUIElement
	{
		virtual void AddBool(cstr name, bool* value) = 0;
		virtual void AddSpacer() = 0;
		virtual void AddFloat(cstr name, float* value, float minValue, float maxValue) = 0;
		virtual void AddFloatRange(cstr name, float* leftValue, float* rightValue, float minValue, float maxValue) = 0;
		virtual void AddInt(cstr name, bool addHexView, int* value) = 0;
		virtual void AddMaterialCategory(cstr name, cstr notifyId, Graphics::MaterialCategory* cat) = 0;
		virtual void AddMessage(cstr message) = 0;
		virtual void AddColour(cstr name, RGBAb* colour) = 0;
		virtual void AddMaterialString(cstr name, MaterialId& id, cstr notifyId, char* value, size_t valueLen) = 0;
		virtual void AddPingPath(cstr name, char* value, size_t valueLen, cstr defaultSubDir, int32 width) = 0;
		virtual void AddButton(cstr name, cstr eventName) = 0;
		virtual void Clear();
	};

	ROCOCOAPI IBloodyPropertySetEditorSupervisor : public IBloodyPropertySetEditor
	{
		virtual void Free() = 0;
	};

	struct BloodyNotifyArgs
	{
		IBloodyPropertySetEditorSupervisor& bs;
		cstr sourceName;
		cstr notifyId;
	};

	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(Platform& _platform, IEventCallback<BloodyNotifyArgs>& _onDirty);

	struct UIPopulate : public Events::EventArgs
	{
		IUIElement* renderElement;
		cstr name;
	};

	struct IPaneSupervisor : virtual IPane
	{
		virtual bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) = 0;
		virtual void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) = 0;

		virtual const GuiRect& ClientRect() const = 0;
		virtual void SetScheme(const ColourScheme& scheme) = 0;
		virtual const ColourScheme& Scheme() const = 0;

		virtual IPaneSupervisor* operator[](int index) = 0;
		virtual int Children() const = 0;

		virtual void AddChild(IPaneSupervisor* child) = 0;
		virtual void RemoveChild(IPaneSupervisor* child) = 0;
		virtual void FreeAllChildren() = 0;

		virtual void Free() = 0;
		virtual void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
		virtual void SetParent(IPaneSupervisor* parent) = 0;
	};

	struct IPaneBuilderSupervisor : public IPaneBuilder
	{
		virtual void Free() = 0;
		virtual void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
		virtual IPaneSupervisor* Supervisor() = 0;
	};

	struct PopulateTabsEvent : Events::EventArgs
	{
		struct TabRef
		{
			cstr name;
			cstr populator;
			int32 width;
		};

		TabRef* tabArray;
		size_t numberOfTabs;
		cstr populatorName;
	};

	struct ICommandHandler
	{

	};

	typedef void(*FN_OnCommand)(ICommandHandler* context, cstr command);

	struct IKeyboardSink;

	struct IGUIStack
	{
		virtual void AppendEvent(const MouseEvent& me) = 0;
		virtual bool AppendEvent(const KeyboardEvent& ke) = 0; // Returns true if some UI control consumed the keyboard event
		virtual void AttachKeyboardSink(IKeyboardSink* ks) = 0;
		virtual void DetachKeyboardSink(IKeyboardSink* ks) = 0;
		virtual bool IsOverwriting() const = 0;
		virtual void LogMessage(const char* format, ...) = 0;
		virtual void ToggleOverwriteMode() = 0;
		virtual IKeyboardSink* CurrentKeyboardSink() = 0;
		virtual IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName) = 0;
		virtual IPaneBuilderSupervisor* CreateDebuggingOverlay() = 0;
		virtual void Render(IGuiRenderContext& grc) = 0;
		virtual void PushTop(IPaneSupervisor* panel, bool isModal) = 0;
		virtual IPaneSupervisor* Pop() = 0;
		virtual IPaneSupervisor* Top() = 0;
		virtual void RegisterEventHandler(ICommandHandler* context, FN_OnCommand method, cstr cmd, cstr helpString = nullptr) = 0;
		virtual void UnregisterEventHandler(ICommandHandler* handler) = 0;
		virtual void RegisterPopulator(cstr name, IUIElement* renderElement) = 0;
		virtual void UnregisterPopulator(IUIElement* renderElement) = 0;
		virtual int32 Count() const = 0; // Number of panes in the stack

		template<class T> inline void UnregisterEventHandler(T* handler)
		{
			UnregisterEventHandler(reinterpret_cast<ICommandHandler*>(handler));
		}
	};

	ROCOCOAPI IGuiStackSupervisor : public IGUIStack
	{
		 virtual void Free() = 0;
		 virtual void PostConstruct(Platform* platform) = 0;
	};

	struct IVariableEditor;
	struct IVariableEditorEventHandler;

	struct SaveDesc
	{
		cstr caption;
		cstr ext;
		cstr extDesc;
		wchar_t path[IO::MAX_PATHLEN];
		const wchar_t* shortName;
	};

	struct LoadDesc
	{
		cstr caption;
		cstr ext;
		cstr extDesc;
		wchar_t path[IO::MAX_PATHLEN];
		const wchar_t* shortName;
	};

	namespace Events
	{
		struct ScrollEvent;

		struct BusyEvent : public EventArgs
		{
			boolean32 isNowBusy;
			cstr message;
			U8FilePath pingPath;
		};

		struct DirectMouseEvent : public EventArgs
		{
			const MouseEvent& me;
			bool consumed = false;
			DirectMouseEvent(const MouseEvent& _me) : me(_me) {}
		};

		extern EventIdRef evUIMouseEvent;
	}

	struct IMPlatFileBrowser;

	ROCOCOAPI IScrollbar
	{
		virtual void GetScrollState(Events::ScrollEvent & s) = 0;
		virtual void SetScrollState(const Events::ScrollEvent& s) = 0;

		virtual bool AppendEvent(const KeyboardEvent& k, Events::ScrollEvent& updateStatus) = 0;
		virtual bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, Events::ScrollEvent& updateStatus) = 0;
		virtual void Free() = 0;
		virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hi_sliderCol, RGBAb sliderCol, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<Events::ScrollEvent>& populator, const Events::EventIdRef& populationEventId) = 0;
	};

	ROCOCOAPI IBrowserRules
	{
		virtual void GetRoot(U32FilePath& path) const = 0;
		virtual cstr GetLastError() const = 0;
		virtual void GetInitialFilename(U32FilePath& path) const = 0;
		virtual void GetCaption(char* caption, size_t capacity) = 0;
		virtual bool Select(const U32FilePath & browserSelection) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IBrowserRulesFactory
	{
		virtual IBrowserRules * CreateRules() = 0;
		virtual cstr GetPanePingPath() const = 0;
	};

	ROCOCOAPI IContextMenuSupervisor : public IContextMenu
	{
		virtual void AppendEvent(const KeyboardEvent& me) = 0;
		virtual void AppendEvent(const MouseEvent & me) = 0;
		virtual void Render(IGuiRenderContext & grc) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IContextMenuEvents
	{
		virtual void OnClickOutsideControls(IContextMenuSupervisor& menu) = 0;
		virtual void OnItemSelected(IContextMenuSupervisor & menu) = 0;
	};

	namespace MPlatImpl
	{
		IContextMenuSupervisor* CreateContextMenu(Events::IPublisher& publisher, IContextMenuEvents& eventHandler);
	}

	// If this class grows too long, consider adding sub-interfaces to better index the functionality
	ROCOCOAPI IUtilitiies
	{
		virtual void AddSubtitle(cstr subtitle) = 0;
		virtual IScrollbar* CreateScrollbar(bool _isVertical) = 0;
		virtual void EnumerateFiles(IEventCallback<const wchar_t*>& cb, cstr pingPathDirectory) = 0;
		virtual Graphics::ITextTesselator& GetTextTesselator() = 0;
		virtual bool GetSaveLocation(Windows::IWindow& parent, SaveDesc& sd) = 0;
		virtual bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& sd) = 0;
		virtual bool QueryYesNo(Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
		virtual void RefreshResource(cstr pingPath) = 0;
		virtual void RunEnvironmentScript(IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false) = 0;
		virtual void RunEnvironmentScript(IEventCallback<ScriptCompileArgs>& _onScriptEvent, int32 id, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false) = 0;
		virtual void SaveBinary(const wchar_t* pathname, const void* buffer, size_t nChars) = 0;
		virtual void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) = 0;
		virtual IVariableEditor* CreateVariableEditor(Windows::IWindow& parent, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const Vec2i* topLeft = nullptr) = 0;
		virtual IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(IEventCallback<BloodyNotifyArgs>& _onDirty) = 0;
		virtual fstring ToShortString(Graphics::MaterialCategory value) const = 0;
		virtual IMathsVenue* Venue() = 0;
		virtual void BrowseFiles(IBrowserRulesFactory& factory) = 0;
		virtual void ShowBusy(bool enable, cstr title, cstr messageFormat, ...) = 0;
		virtual IContextMenuSupervisor& GetContextMenu() = 0;
		virtual IContextMenu& PopupContextMenu() = 0;
		virtual Rococo::Graphics::IHQFonts& GetHQFonts() = 0;
	};

	ROCOCOAPI IUtilitiesSupervisor : public IUtilitiies
	{
		virtual void Free() = 0;
		virtual void SetPlatform(Platform& platform) = 0;
	};

	namespace MPlatImpl
	{
		IUtilitiesSupervisor* CreateUtilities(IInstallation& installation, IRenderer& renderer);
	}

	namespace Audio
	{
		ROCOCOAPI ILegacySoundControlSupervisor : public ILegacySoundControl
		{
			 virtual void EnumerateDeviceDesc(IEventCallback<StringKeyValuePairArg> & cb) = 0;
			 virtual void Free() = 0;
		};

		ILegacySoundControlSupervisor* CreateLegacySoundControl();
	}

	namespace Graphics
	{
		struct IMeshBuilder;
		ROCOCOAPI IRimTesselatorSupervisor : public IRimTesselator
		{
			virtual void Free() = 0;
		};

		IRimTesselatorSupervisor* CreateRimTesselator();

		ROCOCOAPI IRodTesselatorSupervisor : public IRodTesselator
		{
			virtual void Free() = 0;
		};

		IRodTesselatorSupervisor* CreateRodTesselator(IMeshBuilder& meshes);

		ROCOCOAPI IMessagingSupervisor : public IMessaging
		{
			virtual void PostCreate(Platform & platform) = 0;
			virtual void Free() = 0;
		};

		IMessagingSupervisor* CreateMessaging();

		ROCOCOAPI IRigsSupervisor : public Rococo::Graphics::IRigs
		{
			virtual void Free() = 0;
		};

		ROCOCOAPI IRigSupervisor : public Rococo::Graphics::IRig
		{
			virtual void Free() = 0;
		};

		IRigsSupervisor* CreateRigs();
	}

	struct Tesselators
	{
		Graphics::IRimTesselatorSupervisor& rim;
		Graphics::IRodTesselator& rod;
	};

	namespace Script
	{
		struct IScriptSystemFactory;
	}

	namespace Graphics
	{
		ROCOCOAPI IHQFontsSupervisor : IHQFonts
		{
			virtual void Free() = 0;
		};

		IHQFontsSupervisor* CreateHQFonts(IRenderer& renderer);
	}

	struct ISourceCache;

	struct Platform
	{
		// Operating system functions
		IOS& os;

		// Content directory and raw binary file streaming
		IInstallation& installation;

		// Execution control
		OS::IAppControl& appControl;

		// Renderer
		IRenderer& renderer;

		// Render config used to set up sampler states et al
		Graphics::IRendererConfig& rendererConfig;

		// Messaging used to communicate to the player in text
		Graphics::IMessaging& messaging;

		// Script source cache
		ISourceCache& sourceCache;

		// Script debugger window
		IDebuggerWindow& debuggerWindow;

		// Event publisher
		Events::IPublisher& publisher;

		// Platform utilities
		IUtilitiies& utilities;

		// GUI stack
		IGUIStack& gui;

		IKeyboardSupervisor& keyboard;

		IConfigSupervisor& config;

		// Mesh builder object
		Graphics::IMeshBuilderSupervisor& meshes;

		Graphics::IRigsSupervisor& rigs;

		// Entity instances
		Entities::IInstancesSupervisor& instances;

		Entities::IMobilesSupervisor& mobiles;

		Entities::IParticleSystemSupervisor& particles;

		Entities::IRigBuilderSupervisor& rigBuilder;

		Graphics::ISpriteSupervisor& sprites;

		Graphics::ICameraSupervisor& camera;

		Graphics::ISceneSupervisor& scene;

		Tesselators& tesselators;

		IMathsVisitorSupervisor& mathsVisitor;

		Audio::ILegacySoundControl& legacySoundControl;

		Rococo::Script::IScriptSystemFactory& ssFactory;

		Rococo::Puppet::IPuppets& puppets;

		// Application title
		const char* const title;
	};

	namespace Events
	{
		struct ScrollEvent : public Events::EventArgs
		{
			int32 logicalMinValue;
			int32 logicalMaxValue;
			int32 logicalValue;
			int32 logicalPageSize;
			int32 rowSize; // Value delta for right clicks and mouse wheel events
			boolean32 fromScrollbar;
		};
	}

	ROCOCOAPI IMPlatFileBrowser
	{
		// Select returns false if the path is an invalid selection, otherwise triggers selection events
		virtual bool Select() = 0;
		virtual void Engage(IBrowserRulesFactory& factory) = 0;
		virtual void Free() = 0;
	};

	namespace Browser
	{
		struct IBrowserFileChangeNotification;
	}

	IMPlatFileBrowser* CreateMPlatFileBrowser(Events::IPublisher& publisher, IInstallation& installation, IGUIStack& gui, IKeyboardSupervisor& keyboard, Browser::IBrowserFileChangeNotification& onChange);
}

#define REGISTER_UI_EVENT_HANDLER(guistack, instance, classname, methodname, cmd, helpString)  \
{                                                                                          \
   using namespace Rococo;                                                                 \
   IGUIStack& g = guistack;                                                                \
   ICommandHandler* handler = reinterpret_cast<ICommandHandler*>(instance);                \
   struct ANON                                                                             \
   {                                                                                       \
      static void OnCommand(ICommandHandler *obj, cstr commandText)                        \
      {                                                                                    \
         auto* pInstance = reinterpret_cast<classname*>(obj);                              \
         pInstance->methodname(commandText);                                               \
      }                                                                                    \
   };                                                                                      \
   g.RegisterEventHandler(handler, ANON::OnCommand, cmd, helpString);                      \
}
            
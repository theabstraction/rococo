#pragma once

#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.visitors.h>
#include <rococo.app.h>
#include <rococo.os.h>
#include <rococo.events.h>
#include <rococo.animation.types.h>

#include <rococo.component.entities.h>
#include <mplat.components.decl.h>

using namespace Rococo::Strings;

namespace Rococo
{
	using ID_ENTITY = Rococo::Components::ROID;

	struct IScriptEnumerator;
	struct ISourceCache;

	template<>
	struct Hash<ID_ENTITY>
	{
		size_t operator()(ID_ENTITY id) const
		{
			return id.index;
		}
	};

   ROCOCO_ID(ID_SPRITE, uint64, 0);

   namespace Entities
   {
	   struct IRigs;
   }

   namespace Audio
   {
	   struct IAudio;
	   struct IAudioSupervisor;
	   struct IAudioSampleDatabase;
   }

   namespace MPEditor
   {
	   struct IMPEditor;
   }

   struct IEnumVector
   {
	   virtual int32 GetActiveIndex() const = 0;
	   virtual void SetActiveIndex(int32 index) = 0;
	   virtual void SetValue(int32 index, int32 value) = 0;
	   virtual int32 operator[] (int32 index) const = 0;
	   virtual int32 Count() const = 0;
   };

   ROCOCO_INTERFACE IStringVector
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

   struct TriangleScan;
   struct InventoryLayoutRules;

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

namespace Rococo::Joysticks
{
	union XBox360ControllerButtons
	{
		struct Button
		{
			// buttons take 1 bit each, but the type of ButtonBool determines the size of 
			// word in which the bit is stored. This works for Windows at least, which
			// is all that matters here, because this code is for Windows & XBOX360 controllers only.
			typedef uint16 ButtonBool;
			ButtonBool up : 1;
			ButtonBool down : 1;
			ButtonBool left : 1;
			ButtonBool right : 1;
			ButtonBool start : 1;
			ButtonBool back : 1;
			ButtonBool left_thumb : 1;
			ButtonBool right_thumb : 1;
			ButtonBool left_shoulder : 1;
			ButtonBool right_shoulder : 1;
			ButtonBool unused_1 : 1;
			ButtonBool unused_2 : 1;
			ButtonBool A : 1;
			ButtonBool B : 1;
			ButtonBool X : 1;
			ButtonBool Y : 1;

		} button;
		uint16 allButtons;
	};

	/* field for field copy of XINPUT_STATE so we can avoid including the Xinput header
	* which amounts to thousands of lines of code to define the one struct!
	*/
	struct Joystick_XBOX360
	{
		uint32 packetNumber;
		XBox360ControllerButtons buttons;
		uint8  leftTrigger;
		uint8  rightTrigger;
		int16 thumbLX;
		int16 thumbLY;
		int16 thumbRX;
		int16 thumbRY;
	};

	ROCOCO_INTERFACE IJoystick_XBOX360
	{
		/* describe the gamepad state as a number of lines as text - used for debugging gamepad issues */
		virtual void EnumerateStateAsText(const Joystick_XBOX360& x, IEventCallback<cstr> & cb) = 0;
		[[nodiscard]] virtual boolean32 TryGet(uint32 index, Joystick_XBOX360 & state) = 0;
		/* Vibrate the controller, strength ranges from 0 to 1 */
		virtual void Vibrate(uint32 index, float leftStrength, float rightStrength) = 0;
	};

	ROCOCO_INTERFACE IJoystick_XBOX360_Supervisor : IJoystick_XBOX360
	{
		virtual void Free() = 0;
	};

	IJoystick_XBOX360_Supervisor* CreateJoystick_XBox360Proxy();
}

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

	ROCOCO_INTERFACE IConfigSupervisor : public IConfig
	{
		virtual cstr GetText(cstr name) const = 0;
		virtual bool TryGetInt(cstr name, int& value, int defaultValue) const = 0;
		virtual void Free() = 0;
	};

	IConfigSupervisor* CreateConfig();

	using namespace Rococo::Graphics;

	ROCOCO_INTERFACE IMathsVisitorSupervisor : public IMathsVisitor
	{
		 virtual bool AppendKeyboardEvent(const KeyboardEvent & key) = 0;
		 virtual void AppendMouseEvent(const MouseEvent& ev) = 0;

		 virtual void CancelSelect() = 0;
		 virtual void SelectAtPos(Vec2i pos) = 0;

		 virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, int padding) = 0;

		 virtual void Free() = 0;
	};

	struct IUtilities;
	struct IKeyboardSupervisor;
	struct ScriptCompileArgs;

	IMathsVisitorSupervisor* CreateMathsVisitor(IUtilities& utilities, Events::IPublisher& publisher);

	void Run_MPLat_EnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform);

	namespace Entities
	{
		struct IBone;
		struct IAnimation;
		struct ISkeleton;
		struct ISkeletons;

		ROCOCO_INTERFACE IEntityCallback
		{
		   virtual void OnEntity(int64 index, Rococo::Components::IBodyComponent& body, ID_ENTITY id) = 0;
		};

		ROCOCO_INTERFACE IInstancesSupervisor : public IInstances
		{
		   virtual Rococo::Components::IRCObjectTable & ECS() = 0;
		   virtual void ForAll(IEntityCallback & cb) = 0;
		   virtual void Free() = 0;
		   virtual void ConcatenateModelMatrices(ID_ENTITY id, Matrix4x4& result) = 0;
		   virtual void ConcatenatePositionVectors(ID_ENTITY id, Vec3& position) = 0;
		   virtual Rococo::Graphics::IMeshBuilder& MeshBuilder() = 0;
		};
	}

	struct Key
	{
		cstr KeyName;
		bool isPressed;
	};

	ROCOCO_INTERFACE IKeyboardSupervisor : public IKeyboard
	{
	   virtual cstr GetAction(cstr keyName) = 0;
	   virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
	   virtual void AppendKeyboardInputToEditBuffer(int& caretPos, char* buffer, size_t capacity, const KeyboardEvent& key) = 0;
	   virtual void Free() = 0;
	};

	IKeyboardSupervisor* CreateKeyboardSupervisor();

	namespace Graphics
	{
		ROCOCO_INTERFACE IMeshBuilderSupervisor : public IMeshBuilder
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

		ROCOCO_INTERFACE ICameraSupervisor : public ICamera
		{
		   virtual void ElevateView(ID_ENTITY entityId, Degrees delta, cr_vec3 relativePos) = 0;
		   virtual void Free() = 0;
		   virtual void Update(const IUltraClock& clock) = 0;
		   virtual IMathsVenue& Venue() = 0;
		};

		ROCOCO_INTERFACE ITextTesselatorSupervisor : ITextTesselator
		{
			virtual void Free() = 0;
		};

		ITextTesselatorSupervisor* CreateTextTesselator(Platform& platform);

		ICameraSupervisor* CreateCamera(Entities::IInstancesSupervisor& instances, Entities::IMobiles& mobiles, IRenderer& renderer);

		ROCOCO_INTERFACE IScenePopulator
		{
			// Called to trigger scene population prior to shadow generation
			// DO NOT SET LIGHTS here, as it was the light array that caused this function to be called
			virtual void PopulateShadowCasters(ISceneBuilder & sb, const DepthRenderData & drd) = 0;

		// Called to trigger scene population prior to rendering
		// DO NOT SET LIGHTS here, as it was the light array that caused this function to be called
		virtual void PopulateScene(ISceneBuilder& sb) = 0;
		};

		ROCOCO_INTERFACE ISceneBuilderSupervisor : public ISceneBuilder
		{
		   virtual void Free() = 0;
		};

		ROCOCO_INTERFACE ISceneSupervisor : public IScene
		{
		   virtual void AdvanceAnimations(Seconds dt) = 0;
		   virtual void Free() = 0;
		   virtual ISceneBuilderSupervisor& Builder() = 0;
		   virtual void SetPopulator(IScenePopulator* populator) = 0;
		};

		ISceneSupervisor* CreateScene(Rococo::Entities::IInstancesSupervisor& instances, ICameraSupervisor& camera, Entities::IRigs& rigs);

		ROCOCO_INTERFACE ISpriteBuilderSupervisor : public ISpriteBuilder
		{
		   virtual void Free() = 0;
		};

		ISpriteBuilderSupervisor* CreateSpriteBuilderSupervisor(IRenderer& renderer);

		IQuadStackTesselator* CreateQuadStackTesselator();

		ROCOCO_INTERFACE IRendererConfigSupervisor : public IRendererConfig
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

		ROCOCO_INTERFACE IMobilesSupervisor : public Entities::IMobiles
		{
		   virtual bool TryMoveMobile(const MoveMobileArgs & tmm) = 0;
		   virtual void Free() = 0;
		};

		IMobilesSupervisor* CreateMobilesSupervisor(Entities::IInstancesSupervisor& instances);

		ROCOCO_INTERFACE IRigs
		{
			virtual IRigBuilder & Builder() = 0;
			virtual ISkeletons & Skeles() = 0;
			virtual ISkeletons& Poses() = 0;
			virtual void Free() = 0;
		};

		IInstancesSupervisor* CreateInstanceBuilder(Graphics::IMeshBuilderSupervisor& meshes, IRenderer& renderer, Events::IPublisher& publisher, Components::IRCObjectTable& ecs, size_t maxEntities);

		ROCOCO_INTERFACE IParticleSystemSupervisor : IParticleSystem
		{
			virtual void Free() = 0;
			virtual void GetParticles(ID_ENTITY id, IRenderer& renderer) = 0;
		};

		IParticleSystemSupervisor* CreateParticleSystem(IRenderer& renderer, IInstances& instances);

		IRigs* CreateRigBuilder();
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

	ROCOCO_INTERFACE IBloodyPropertySetEditor : public IUIElement
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

	ROCOCO_INTERFACE IBloodyPropertySetEditorSupervisor : public IBloodyPropertySetEditor
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

		/*
		*	N.B if the onCompile parameter is supplied then some of the mplat.sxh interfaces
		*   are added that may be useful in panel generation. A application can also add native
		*   interfaces and functions using the onCompile implementation
		*/
		virtual IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName, IEventCallback<ScriptCompileArgs>* onCompile = nullptr) = 0;
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

	ROCOCO_INTERFACE IGuiStackSupervisor : public IGUIStack
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

	ROCOCO_INTERFACE IScrollbar
	{
		virtual void GetScrollState(Events::ScrollEvent & s) = 0;
		virtual void SetScrollState(const Events::ScrollEvent& s) = 0;

		virtual bool AppendEvent(const KeyboardEvent& k, Events::ScrollEvent& updateStatus) = 0;
		virtual bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, Events::ScrollEvent& updateStatus) = 0;
		virtual void Free() = 0;
		virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hi_sliderCol, RGBAb sliderCol, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<Events::ScrollEvent>& populator, const Events::EventIdRef& populationEventId) = 0;
	};

	ROCOCO_INTERFACE IBrowserRules
	{
		virtual void GetRoot(U32FilePath& path) const = 0;
		virtual cstr GetLastError() const = 0;
		virtual void GetInitialFilename(U32FilePath& path) const = 0;
		virtual void GetCaption(char* caption, size_t capacity) = 0;
		virtual bool Select(const U32FilePath & browserSelection) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IBrowserRulesFactory
	{
		virtual IBrowserRules * CreateRules() = 0;
		virtual cstr GetPanePingPath() const = 0;
	};

	ROCOCO_INTERFACE IContextMenuSupervisor : public IContextMenu
	{
		virtual void AppendEvent(const KeyboardEvent& me) = 0;
		virtual void AppendEvent(const MouseEvent & me) = 0;
		virtual void Render(IGuiRenderContext & grc) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IContextMenuEvents
	{
		virtual void OnClickOutsideControls(IContextMenuSupervisor& menu) = 0;
		virtual void OnItemSelected(IContextMenuSupervisor & menu) = 0;
	};

	namespace MPlatImpl
	{
		IContextMenuSupervisor* CreateContextMenu(Events::IPublisher& publisher, IContextMenuEvents& eventHandler);
	}

	ROCOCO_INTERFACE IInventoryArraySupervisor : IInventoryArray
	{
		virtual void Free() = 0;
	};

	// If this class grows too long, consider adding sub-interfaces to better index the functionality
	ROCOCO_INTERFACE IUtilities
	{
		virtual void AddSubtitle(cstr subtitle) = 0;
		virtual IScrollbar* CreateScrollbar(bool _isVertical) = 0;
		virtual void EnumerateFiles(IEventCallback<const wchar_t*>& cb, cstr pingPathDirectory) = 0;
		virtual Graphics::ITextTesselator& GetTextTesselator() = 0;
		virtual bool GetSaveLocation(Windows::IWindow& parent, SaveDesc& sd) = 0;
		virtual bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& sd) = 0;
		virtual bool QueryYesNo(Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
		virtual void RefreshResource(cstr pingPath) = 0;
		virtual void RunEnvironmentScript(IScriptEnumerator& implicitIncludes, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false, IEventCallback<cstr>* onScriptCrash = nullptr, Strings::StringBuilder* declarationBuilder = nullptr) = 0;
		virtual void RunEnvironmentScript(IScriptEnumerator& implicitIncludes, IEventCallback<ScriptCompileArgs>& _onScriptEvent, int32 id, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false, IEventCallback<cstr>* onScriptCrash = nullptr, Strings::StringBuilder* declarationBuilder = nullptr) = 0;
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
		virtual IInventoryArraySupervisor* CreateInventoryArray(int32 capacity) = 0;
	};

	ROCOCO_INTERFACE IUtilitiesSupervisor : public IUtilities
	{
		virtual void Free() = 0;
		virtual void SetPlatform(Platform& platform) = 0;
	};

	namespace MPlatImpl
	{
		IUtilitiesSupervisor* CreateUtilities(IO::IInstallation& installation, IRenderer& renderer);
	}

	namespace Graphics
	{
		struct IMeshBuilder;
		ROCOCO_INTERFACE IRimTesselatorSupervisor : public IRimTesselator
		{
			virtual void Free() = 0;
		};

		IRimTesselatorSupervisor* CreateRimTesselator();

		ROCOCO_INTERFACE IRodTesselatorSupervisor : public IRodTesselator
		{
			virtual VertexTriangle* begin() = 0;
			virtual VertexTriangle* end() = 0;
			virtual void Free() = 0;
		};

		IRodTesselatorSupervisor* CreateRodTesselator(IMeshBuilder& meshes);

		/// <summary>
		/// Creates a rod tesselator isloated from the platform mesh builder. 
		/// Useful for standalone geometry generation
		/// </summary>
		/// <returns>A reference to the rod tesselator. Be sure to call Free() when done</returns>
		IRodTesselatorSupervisor* CreateIsolatedRodTesselator();

		ROCOCO_INTERFACE IMessagingSupervisor : public IMessaging
		{
			virtual void PostCreate(Platform & platform) = 0;
			virtual void Free() = 0;
		};

		IMessagingSupervisor* CreateMessaging();
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
		ROCOCO_INTERFACE IHQFontsSupervisor : IHQFonts
		{
			virtual void Free() = 0;
		};

		IHQFontsSupervisor* CreateHQFonts(IHQFontResource& hq);
	}

	ROCOCO_INTERFACE IInstallationManagerSupervisor :  IInstallationManager
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IArchiveSupervisor : IArchive
	{
		virtual void Free() = 0;
	};

	IArchiveSupervisor* CreateArchive();

	ROCOCO_INTERFACE IWorldSupervisor : IWorldBuilder
	{
		virtual void Free() = 0;
	};

	IWorldSupervisor* CreateWorld(Graphics::IMeshBuilderSupervisor& meshes, Entities::IInstancesSupervisor& instances);

	namespace Graphics
	{
		ROCOCO_INTERFACE ISpritesSupervisor : ISprites
		{
			virtual void Free() = 0;
		};

		ISpritesSupervisor* CreateSpriteTable(IRenderer& renderer);
	}

	namespace Components
	{
		struct IRCObjectTable;
	}

	namespace Gui
	{
		struct IGRSystem;
		struct IGRCustodian;

		struct IMPlatGuiCustodianSupervisor
		{
			virtual IGRCustodian& Custodian() = 0;
			virtual void Render(IGuiRenderContext& rc, IGRSystem& gr) = 0;
			virtual void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) = 0;
			virtual void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) = 0;
			virtual void Free() = 0;
		};
	}

	struct PlatformGraphics
	{
		// Render config used to set up sampler states et al
		Graphics::IRendererConfig& rendererConfig;	
		
		IRenderer& renderer;

		Graphics::ISprites& sprites;

		// GUI stack
		IGUIStack& gui;	
		
		// Mesh builder object
		Graphics::IMeshBuilderSupervisor& meshes;

		// Entity instances
		Entities::IInstancesSupervisor& instances;

		Graphics::ISpriteBuilderSupervisor& spriteBuilder;

		Graphics::ICameraSupervisor& camera;

		Graphics::ISceneSupervisor& scene;

		Rococo::Gui::IGRSystem& GR;

		Rococo::Gui::IMPlatGuiCustodianSupervisor& GR_Custodian;
	};

	struct PlatformOS
	{
		// Operating system functions
		IO::IOS& ios;

		// Content directory and raw binary file streaming
		IO::IInstallation& installation;

		// Scriptable directory management
		Rococo::IInstallationManager& installationManager;

		// Execution control
		OS::IAppControl& appControl;

		Windows::IWindow& mainWindow;

		// Application title
		const char* const title;
	};

	struct PlatformScripts
	{
		// Script source cache
		ISourceCache& sourceCache;

		// Script debugger window
		IDebuggerWindow& debuggerWindow;

		Rococo::Script::IScriptSystemFactory& ssFactory;
	};

	struct PlatformHardware
	{
		IKeyboardSupervisor& keyboard;
		Audio::IAudio& audio;
		Rococo::Joysticks::IJoystick_XBOX360& xbox360joystick;
	};

	struct PlatformWorld
	{
		Entities::IMobilesSupervisor& mobiles;

		Entities::IParticleSystemSupervisor& particles;

		Entities::IRigs& rigs;	
		
		Rococo::IWorldBuilder& worldBuilder;

		// (E)ntity(C)omponent(S)ystem
		Rococo::Components::IRCObjectTable& ECS;
	};

	struct PlatformData
	{
		IConfigSupervisor& config;
		IArchive& archive;
	};

	struct PlatformPlumbing
	{
		// Messaging used to communicate to the player in text
		Graphics::IMessaging& messaging;

		// Event publisher
		Events::IPublisher& publisher;

		// Platform utilities
		IUtilities& utilities;
	};

	struct PlatformCreator
	{
		Rococo::MPEditor::IMPEditor& editor;
	};

	struct PlatformMisc
	{	
		IMathsVisitorSupervisor& mathsVisitor;
	};

	struct Platform
	{
		PlatformGraphics graphics;
		PlatformOS os;
		PlatformScripts scripts;
		PlatformHardware hardware;
		PlatformWorld world;
		PlatformData data;
		PlatformPlumbing plumbing;
		PlatformCreator creator;
		PlatformMisc misc;
		Tesselators& tesselators;		
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

	ROCOCO_INTERFACE IMPlatFileBrowser
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

	IMPlatFileBrowser* CreateMPlatFileBrowser(Events::IPublisher& publisher, IO::IInstallation& installation, IGUIStack& gui, IKeyboardSupervisor& keyboard, Browser::IBrowserFileChangeNotification& onChange);

	IInventoryArraySupervisor* CreateInventoryArray(int32 capacity);
}

namespace Rococo
{
	struct ScriptPerformanceStats;

	namespace Windows::IDE
	{
		enum class EScriptExceptionFlow;
	}

	namespace MPlatImpl
	{
		Rococo::IInstallationManagerSupervisor* CreateIMS(IO::IInstallation& installation);

		void RunBareScript(
			ScriptPerformanceStats& stats,
			IEventCallback<ScriptCompileArgs>& _onScriptEvent,
			Windows::IDE::EScriptExceptionFlow flow,
			const char* name,
			int id,
			Script::IScriptSystemFactory& ssf,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			IScriptEnumerator& implicitIncludes,
			OS::IAppControl& appControl,
			Strings::StringBuilder* declarationBuilder
		);

		void RunMPlatConfigScript(OUT IConfig& config,
			cstr scriptName,
			Script::IScriptSystemFactory& ssf,
			Windows::IDE::EScriptExceptionFlow flow,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			OS::IAppControl& appControl,
			Strings::StringBuilder* declarationBuilder
		);
	}
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
        

namespace Rococo::Entities
{
	/// <summary>
	/// Dynamically adds geometry to the render context to graphically visualize a skeleton
	/// </summary>
	/// <param name="e">- a reference to an entity with a skeleton</param>
	/// <param name="rc">- the 3D render context to render to </param>
	/// <param name="rod">- the rod tesselator object used to generate geometry</param>
	/// <param name="rigs">- the set of poses used by the entity object</param>
	void AddDebugBones(ID_ENTITY id, Rococo::Components::IRCObjectTable& ecs, IRenderContext& rc, Rococo::Graphics::IRodTesselatorSupervisor& rod);
}
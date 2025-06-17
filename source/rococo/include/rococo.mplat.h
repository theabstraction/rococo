#pragma once

#include <rococo.api.h>
#include <components/rococo.ecs.roid.h>
#include <rococo.meshes.h>
#include <rococo.maths.h>
#include <../rococo.mplat/code-gen/rococo.script.types.h>
#include <rococo.graphics.types.h>
#include <rococo.renderer.h>
#include <rococo.visitors.h>
#include <rococo.app.h>
#include <rococo.os.h>
#include <rococo.events.h>
#include <rococo.animation.types.h>

#include <components/rococo.component.entities.h>
#include <components/rococo.components.body.h>
#include <components/rococo.components.skeleton.h>
#include <components/rococo.components.animation.h>
#include <mplat.components.decl.h>
#include <rococo.mplat.types.h>
#include <rococo.mplat.editors.h>
#include <rococo.subsystems.h>

#include <3D/rococo.mesh-builder.h>
#include <rococo.ui.joystick.h>
#include <sexy.script.exports.h>

namespace Rococo
{
	DECLARE_ROCOCO_INTERFACE IScriptEnumerator;
	DECLARE_ROCOCO_INTERFACE ISourceCache;

	template<>
	struct Hash<ID_ENTITY>
	{
		size_t operator()(ID_ENTITY id) const
		{
			return id.index;
		}
	};

	namespace IO
	{
		struct IShaderMonitorEventsProxy;
	}

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
		float shadowFudge;
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
}

namespace Rococo
{
	void SaveAsSexML(cstr userDocName, Reflection::IReflectionTarget& target);
}

#include <../rococo.mplat/code-gen/rococo.sxh.h>

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

	ROCOCO_INTERFACE IConfigSupervisor : public Rococo::Configuration::IConfig
	{
		virtual cstr GetText(cstr name) const = 0;
		virtual bool TryGetInt(cstr name, int& value, int defaultValue) const = 0;
		virtual void Free() = 0;
	};

	IConfigSupervisor* CreateConfig();

	ROCOCO_INTERFACE IMathsVisitorSupervisor : public IMathsVisitor
	{
		 virtual bool AppendKeyboardEvent(const KeyboardEvent & key) = 0;
		 virtual void AppendMouseEvent(const MouseEvent& ev) = 0;

		 virtual void CancelSelect() = 0;
		 virtual void SelectAtPos(Vec2i pos) = 0;

		 virtual void Render(Graphics::IGuiRenderContext& grc, const GuiRect& absRect, int padding) = 0;

		 virtual void Free() = 0;
	};

	struct IUtilities;
	struct IKeyboardSupervisor;
	struct ScriptCompileArgs;

	IMathsVisitorSupervisor* CreateMathsVisitor(IUtilities& utilities, Events::IPublisher& publisher);

	void Run_MPLat_EnvironmentScript(Platform& platform, IScriptCompilationEventHandler& _onScriptEvent, const char* name, bool addPlatform);

	namespace Entities
	{
		struct IBone;
		struct IAnimation;
		struct ISkeleton;
		struct ISkeletons;

		ROCOCO_INTERFACE IEntityCallback
		{
		   virtual void OnEntity(int64 index, Rococo::Components::IBodyComponent & body, ID_ENTITY id) = 0;
		};
	}

	struct Key
	{
		cstr KeyName;
		bool isPressed;
	};

	struct KeyboardEventEx;

	ROCOCO_INTERFACE IKeyboardSupervisor : public IKeyboard
	{
	   virtual cstr GetAction(cstr keyName) = 0;
	   virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
	   virtual void AppendKeyboardInputToEditBuffer(int& caretPos, char* buffer, size_t capacity, const KeyboardEventEx& key) = 0;
	   virtual void Free() = 0;
	};

	IKeyboardSupervisor* CreateKeyboardSupervisor(Windows::IWindow& window);

	namespace Graphics
	{
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

		ICameraSupervisor* CreateCamera(Entities::IMobiles& mobiles, IRenderer& renderer);

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

		ISceneSupervisor* CreateScene(Rococo::IECS& ecs, ICameraSupervisor& camera, Entities::IRigs& rigs);

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

		IMobilesSupervisor* CreateMobilesSupervisor(Rococo::IECS& ecs);

		ROCOCO_INTERFACE IRigs
		{
			virtual IRigBuilder & Builder() = 0;
			virtual ISkeletons & Skeles() = 0;
			virtual ISkeletons& Poses() = 0;
			virtual void Free() = 0;
		};

		ROCOCO_INTERFACE IParticleSystemSupervisor : IParticleSystem
		{
			virtual void Free() = 0;
			virtual void AdvanceParticleSimulation(ID_ENTITY id, Graphics::IRenderer& renderer) = 0;
		};

		IParticleSystemSupervisor* CreateParticleSystem(Graphics::IRenderer& renderer);

		IRigs* CreateRigBuilder();
	}

	struct Platform;

	struct MPlatColourScheme
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

	struct KeyboardEventEx;

	struct IPaneSupervisor : virtual GUI::IPane
	{
		virtual bool AppendEvent(const KeyboardEventEx& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) = 0;
		virtual void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) = 0;

		virtual const GuiRect& ClientRect() const = 0;
		virtual void SetScheme(const MPlatColourScheme& scheme) = 0;
		virtual const MPlatColourScheme& Scheme() const = 0;

		virtual IPaneSupervisor* operator[](int index) = 0;
		virtual int Children() const = 0;

		virtual void AddChild(IPaneSupervisor* child) = 0;
		virtual void RemoveChild(IPaneSupervisor* child) = 0;
		virtual void FreeAllChildren() = 0;

		virtual void Free() = 0;
		virtual void Render(Graphics::IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
		virtual void SetParent(IPaneSupervisor* parent) = 0;
	};

	struct IPaneBuilderSupervisor : public GUI::IPaneBuilder
	{
		virtual void Free() = 0;
		virtual void Render(Graphics::IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
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
		// Returns true if some UI control consumed the keyboard event
		virtual bool AppendEvent(const KeyboardEventEx& ke) = 0;
		virtual void AttachKeyboardSink(IKeyboardSink* ks) = 0;
		virtual void DetachKeyboardSink(IKeyboardSink* ks) = 0;
		virtual bool IsOverwriting() const = 0;
		virtual void LogMessage(const char* format, ...) = 0;
		virtual void ClearFileError(cstr file) = 0;
		virtual void ShowFileError(cstr file, cstr message) = 0;
		virtual void ToggleOverwriteMode() = 0;
		// Returns true if the gui believes a high frame rate improves the user experience
		virtual bool HighFrameRateImproves() const = 0;
		virtual IKeyboardSink* CurrentKeyboardSink() = 0;

		/*
		*	N.B if the onCompile parameter is supplied then some of the mplat.sxh interfaces
		*   are added that may be useful in panel generation. A application can also add native
		*   interfaces and functions using the onCompile implementation
		*/
		virtual IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName, IScriptCompilationEventHandler* onCompile, IScriptEnumerator* implicitIncludes) = 0;
		virtual void Render(Graphics::IGuiRenderContext& grc) = 0;
		virtual void PushTop(IPaneSupervisor* panel, bool isModal) = 0;
		virtual IPaneSupervisor* Pop() = 0;
		virtual IPaneSupervisor* Top() = 0;
		virtual void RegisterEventHandler(ICommandHandler* context, FN_OnCommand method, cstr cmd, cstr helpString = nullptr) = 0;
		virtual void UnregisterEventHandler(ICommandHandler* handler) = 0;
		virtual void RegisterPopulator(cstr name, IUIElement* renderElement) = 0;
		virtual void UnregisterPopulator(IUIElement* renderElement) = 0;
		// Number of panes in the stack
		virtual int32 Count() const = 0;

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
		WideFilePath wPath;
		crwstr shortName;
	};

	struct LoadDesc
	{
		cstr caption;
		cstr ext;
		cstr extDesc;
		WideFilePath wPath;
		crwstr shortName;
	};

	namespace Events
	{
		struct ScrollEvent;
	}

	struct IMPlatFileBrowser;

	namespace GUI
	{
		ROCOCO_INTERFACE IScrollbar
		{
			virtual void GetScrollState(Events::ScrollEvent & s) = 0;
			virtual void SetScrollState(const Events::ScrollEvent& s) = 0;

			virtual bool AppendEvent(const KeyboardEvent& k, Events::ScrollEvent& updateStatus) = 0;
			virtual bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, Events::ScrollEvent& updateStatus) = 0;
			virtual void Free() = 0;
			virtual void Render(Graphics::IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hi_sliderCol, RGBAb sliderCol, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<Events::ScrollEvent>& populator, const Events::EventIdRef& populationEventId) = 0;
		};
	}

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
		virtual void Render(Graphics::IGuiRenderContext & grc) = 0;
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
		void ValidateSafePathToWrite(IO::IInstallation& installation, Rococo::Script::IPublicScriptSystem& ss, cstr pathname);
		void ValidateSafePathToRead(IO::IInstallation& installation, Rococo::Script::IPublicScriptSystem& ss, cstr pathname);
	}

	// If this class grows too long, consider adding sub-interfaces to better index the functionality
	ROCOCO_INTERFACE IUtilities
	{
		virtual void AddSubtitle(cstr subtitle) = 0;
		virtual GUI::IScrollbar* CreateScrollbar(bool _isVertical) = 0;
		virtual void EnumerateFiles(IEventCallback<crwstr>& cb, cstr pingPathDirectory) = 0;
		virtual Graphics::ITextTesselator& GetTextTesselator() = 0;
		virtual bool GetSaveLocation(Windows::IWindow& parent, SaveDesc& sd) = 0;
		virtual bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& sd) = 0;
		virtual bool QueryYesNo(Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
		virtual void RefreshResource(cstr pingPath) = 0;

		// Note, if implicitIncludes is null, mplat defaults are used, which may conflict with your security settings.
		virtual void RunEnvironmentScript(IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false, Strings::IStringPopulator* onScriptCrash = nullptr, Strings::StringBuilder* declarationBuilder = nullptr) = 0;

		// Note, if implicitIncludes is null, mplat defaults are used, which may conflict with your security settings.
		virtual void RunEnvironmentScriptWithId(IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, int32 id, const char* name, bool addPlatform, bool shutdownOnFail = true, bool trace = false, Strings::IStringPopulator* onScriptCrash = nullptr, Strings::StringBuilder* declarationBuilder = nullptr) = 0;

		virtual void SaveBinary(crwstr pathname, const void* buffer, size_t nChars) = 0;
		virtual void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) = 0;
		virtual IVariableEditor* CreateVariableEditor(Windows::IWindow& parent, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const Vec2i* topLeft = nullptr) = 0;
		virtual IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(IEventCallback<BloodyNotifyArgs>& onDirty, IScriptCompilationEventHandler& onCompileUIPanel) = 0;
		virtual fstring ToShortString(Graphics::MaterialCategory value) const = 0;
		virtual IMathsVenue* Venue() = 0;
		virtual void BrowseFiles(IBrowserRulesFactory& factory, IScriptCompilationEventHandler* onCompile) = 0;
		virtual void ShowBusy(bool enable, cstr title, cstr messageFormat, ...) = 0;
		virtual IContextMenuSupervisor& GetContextMenu() = 0;
		virtual IContextMenu& PopupContextMenu(IScriptCompilationEventHandler& onCompile) = 0;
		virtual Rococo::Graphics::IHQFonts& GetHQFonts() = 0;
	};

	ROCOCO_INTERFACE IUtilitiesSupervisor : public IUtilities
	{
		virtual void Free() = 0;
		virtual void SetPlatform(Platform& platform) = 0;
	};

	namespace MPlatImpl
	{
		IUtilitiesSupervisor* CreateUtilities(IO::IInstallation& installation, Graphics::IRenderer& renderer);
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

	IWorldSupervisor* CreateWorld(Graphics::IMeshBuilderSupervisor& meshes, IECS& ecs);

	namespace Graphics
	{
		ROCOCO_INTERFACE ISpritesSupervisor : ISprites
		{
			virtual void Free() = 0;
		};

		ISpritesSupervisor* CreateSpriteTable(IRenderer& renderer);

		struct IMaterialBuilderSupervisor;
		struct IShaderOptionsConfig;
	}

	namespace Components
	{
		struct IRCObjectTable;
	}

	namespace Gui
	{
		struct IGRSystem;
		struct IGRCustodian;
		struct GRKeyContextFlags;

		struct IMPlatGuiCustodianSupervisor
		{
			virtual IGRCustodian& Custodian() = 0;
			virtual void Render(Graphics::IGuiRenderContext& rc, IGRSystem& gr) = 0;
			virtual void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) = 0;
			virtual void RouteMouseEvent(const MouseEvent& me, const GRKeyContextFlags& context, IGRSystem& gr) = 0;
			virtual void Free() = 0;
		};
	}

	void RegisterSubsystems(ISubsystemsSupervisor& subsystems, Platform& platform, ID_SUBSYSTEM platformId);

	struct PlatformGraphics
	{
		// Render config used to set up sampler states et al
		Graphics::IRendererConfig& rendererConfig;	
		
		Graphics::IRenderer& renderer;

		Graphics::ISprites& sprites;

		// GUI stack
		IGUIStack& gui;	
		
		// Mesh builder object
		Graphics::IMeshBuilderSupervisor& meshes;

		Graphics::IMaterialBuilderSupervisor& materials;

		Graphics::ISoftBoxBuilder& softBoxBuilder;

		Graphics::ISpriteBuilderSupervisor& spriteBuilder;

		Graphics::ICameraSupervisor& camera;

		Graphics::ISceneSupervisor& scene;

		Rococo::Gui::IGRSystem& GR;

		Rococo::Gui::IMPlatGuiCustodianSupervisor& GR_Custodian;

		Rococo::Graphics::IShaderOptionsConfig& shaderConfig;

		Rococo::IO::IShaderMonitorEventsProxy& shaderMonitorEventsProxy;
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
		IDesignator<IScriptCompilationEventHandler>& panelCompilationDesignator;

		IScriptCompilationEventHandler& panelCompilationHandler;

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
		Rococo::IECS& ECS;
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
		ISubsystems& subSystems;
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
			IScriptCompilationEventHandler& _onScriptEvent,
			Windows::IDE::EScriptExceptionFlow flow,
			const char* name,
			int id,
			Script::IScriptSystemFactory& ssf,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			IScriptEnumerator& implicitIncludes,
			OS::IAppControl& appControl,
			Strings::StringBuilder* declarationBuilder,
			IO::IInstallation& installation
		);

		void RunMPlatConfigScript(Graphics::IShaderOptionsConfig& shaderOptions, Configuration::IConfig& config,
			cstr scriptName,
			Script::IScriptSystemFactory& ssf,
			Windows::IDE::EScriptExceptionFlow flow,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			OS::IAppControl& appControl,
			Strings::StringBuilder* declarationBuilder,
			IO::IInstallation& installation
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
	void AddDebugBones(ID_ENTITY id, Graphics::IRenderContext& rc, Rococo::Graphics::IRodTesselatorSupervisor& rod);
}

namespace Rococo::Components::Generated::Interop
{
	// Defined in include/components/interop.inl
	void AddComponentNatives(Rococo::Script::IPublicScriptSystem& ss, Rococo::IECS* ecs);
}
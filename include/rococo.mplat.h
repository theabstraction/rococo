#pragma once

#include <rococo.renderer.h>
#include <rococo.visitors.h>

namespace Rococo
{
   ROCOCO_ID(ID_ENTITY, int64, 0);
}

namespace Rococo
{
	struct LightSpec
	{
		Vec3 position;
		Vec3 direction;
		Degrees fov;
		RGBA diffuse;
		RGBA ambience;
		Degrees cutoffAngle;
		float cutoffPower;
		float attenuation;
	};
}

#include <../rococo.mplat/mplat.sxh.h>

namespace Rococo
{
   ROCOCOAPI IConfigSupervisor : public IConfig
   {
      virtual cstr GetText(cstr name) const = 0;
      virtual void Free() = 0;
   };

   IConfigSupervisor* CreateConfig();

   ROCOCOAPI IMathsVisitorSupervisor : public IMathsVisitor
   {
		virtual bool AppendKeyboardEvent(const KeyboardEvent& key) = 0;
		virtual void AppendMouseEvent(const MouseEvent& ev) = 0;

		virtual void CancelSelect() = 0;

		virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, int padding) = 0;

		virtual cstr SelectedKey() const = 0;
		virtual cstr SelectedValue() const = 0;

		virtual void Free() = 0;
   };

   struct IUtilitiies;
   struct IKeyboardSupervisor;

   IMathsVisitorSupervisor* CreateMathsVisitor(IUtilitiies& utilities, IKeyboardSupervisor& keyboard);

   void Run_MPLat_EnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform);

   namespace Entities
   {
      ROCOCOAPI IEntity
      {
         virtual const Vec3 Position() const = 0;
         virtual Matrix4x4& Model() = 0;
         virtual ID_ENTITY ParentId() const = 0;
         virtual const ID_ENTITY* begin() const = 0;
         virtual const ID_ENTITY* end() const = 0;
         virtual ID_SYS_MESH MeshId() const = 0;
         virtual ID_TEXTURE TextureId() const = 0;
		 virtual void SetTexture(ID_TEXTURE id) = 0;
		 virtual void SetMesh(ID_SYS_MESH id) = 0;
      };

      ROCOCOAPI IEntityCallback
      {
         virtual void OnEntity(int64 index, IEntity& entity, ID_ENTITY id) = 0;
      };

      ROCOCOAPI IInstancesSupervisor : public IInstances
      {
         virtual void ForAll(IEntityCallback& cb) = 0;
         virtual void Free() = 0;
         virtual IEntity* GetEntity(ID_ENTITY id) = 0;
         virtual void ConcatenateModelMatrices(ID_ENTITY id, Matrix4x4& result) = 0;
         virtual void ConcatenatePositionVectors(ID_ENTITY id, Vec3& position) = 0;
         virtual Rococo::Graphics::IMeshBuilder& MeshBuilder() = 0;
         virtual ID_TEXTURE ReadyTexture(cstr pingName) = 0;
      };
   }

   struct Key
   {
      cstr KeyName;
      bool isPressed;
   };

   ROCOCOAPI IKeyboardSupervisor : public IKeyboard
   {
      virtual cstr GetAction(cstr keyName) = 0;
      virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
      virtual void Free() = 0;
   };

   IKeyboardSupervisor* CreateKeyboardSupervisor();

   namespace Graphics
   {
	   ROCOCOAPI IMeshBuilderSupervisor : public IMeshBuilder
	   {
		  virtual void Free() = 0;
		  virtual bool TryGetByName(cstr name, ID_SYS_MESH& id) = 0;
		  virtual IMathsVenue* Venue() = 0;
	   };

	   ROCOCOAPI ICameraSupervisor : public ICamera
	   {
		  virtual void ElevateView(ID_ENTITY entityId, Degrees delta, cr_vec3 relativePos) = 0;
		  virtual void Free() = 0;
		  virtual void Update(const IUltraClock& clock) = 0;
		  virtual IMathsVenue& Venue() = 0;
	   };

	   ICameraSupervisor* CreateCamera(Entities::IInstancesSupervisor& instances, Entities::IMobiles& mobiles, IRenderer& renderer);

	   ROCOCOAPI IScenePopulator
	   {
			// Called to trigger scene population prior to shadow generation
			// DO NOT SET LIGHTS here, as it was the light array that caused this function to be called
			virtual void PopulateShadowCasters(ISceneBuilder& sb, const DepthRenderData& drd) = 0;

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
		  virtual ISceneBuilderSupervisor&  Builder() = 0;
		  virtual void SetPopulator(IScenePopulator* populator) = 0;
	   };

	   ISceneSupervisor* CreateScene(Rococo::Entities::IInstancesSupervisor& instances, ICameraSupervisor& camera);

	   ROCOCOAPI ISpriteSupervisor : public ISprites
	   {
		  virtual void Free() = 0;
	   };

	   ISpriteSupervisor* CreateSpriteSupervisor(IRenderer & renderer);
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
         virtual bool TryMoveMobile(const MoveMobileArgs& tmm) = 0;
         virtual void Free() = 0;
      };

      IMobilesSupervisor* CreateMobilesSupervisor(Entities::IInstancesSupervisor& instances);

      IInstancesSupervisor* CreateInstanceBuilder(Graphics::IMeshBuilderSupervisor& meshes, IRenderer& renderer);
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

   struct UIInvoke : public Events::Event
   {
      UIInvoke();
      static Events::EventId EvId();
      char command[232];
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

   struct UIPopulate : public Events::Event
   {
      UIPopulate();
      static Events::EventId EvId();
      IUIElement* renderElement;
      cstr name;
   };

   struct IPanelSupervisor: IPane
   {
      virtual bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) = 0;
      virtual void AppendEvent(const MouseEvent& me,  const Vec2i& absTopLeft) = 0;

      virtual const GuiRect& ClientRect() const = 0;
      virtual void SetScheme(const ColourScheme& scheme) = 0;
      virtual const ColourScheme& Scheme() const = 0;

      virtual IPanelSupervisor* operator[](int index) = 0;
      virtual int Children() const = 0;

      virtual void AddChild(IPanelSupervisor* child) = 0;
      virtual void RemoveChild(IPanelSupervisor* child) = 0;
      virtual void FreeAllChildren() = 0;

      virtual void Free() = 0;
      virtual void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
   };

   struct IPaneBuilderSupervisor : public IPaneBuilder
   {
      virtual void Free() = 0;
      virtual void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) = 0;
      virtual IPanelSupervisor* Supervisor() = 0;
   };

   struct ICommandHandler
   {

   };

   typedef void(*FN_OnCommand)(ICommandHandler* context, cstr command);

   struct IGUIStack
   {
      virtual void AppendEvent(const MouseEvent& me) = 0;
      virtual bool AppendEvent(const KeyboardEvent& ke) = 0; // Returns true if some UI control consumed the keyboard event
      virtual IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName) = 0;
	  virtual IPaneBuilderSupervisor* CreateOverlay() = 0;
      virtual void Render(IGuiRenderContext& grc) = 0;
      virtual void PushTop(IPanelSupervisor* panel, bool isModal) = 0;
      virtual IPanelSupervisor* Pop() = 0;
      virtual IPanelSupervisor* Top() = 0;
      virtual void RegisterEventHandler(ICommandHandler* context, FN_OnCommand method, cstr cmd, cstr helpString = nullptr) = 0;
      virtual void UnregisterEventHandler(ICommandHandler* handler) = 0;
      virtual void RegisterPopulator(cstr name, IUIElement* renderElement) = 0;
      virtual void UnregisterPopulator(IUIElement* renderElement) = 0;

      template<class T> inline void UnregisterEventHandler(T* handler)
      {
         UnregisterEventHandler(reinterpret_cast<ICommandHandler*>(handler));
      }
   };

   struct IVariableEditor;
   struct IVariableEditorEventHandler;

   struct SaveDesc
   {
	   cstr caption;
	   cstr ext;
	   cstr extDesc;
	   char path[IO::MAX_PATHLEN];
	   cstr shortName;
   };

   struct LoadDesc
   {
	   cstr caption;
	   cstr ext;
	   cstr extDesc;
	   char path[IO::MAX_PATHLEN];
	   cstr shortName;
   };

   namespace Events
   {
	   struct ScrollEvent;
   }

   struct IScrollbar
   {
	   virtual void GetScroller(Events::ScrollEvent& s) = 0;
	   virtual void SetScroller(const Events::ScrollEvent& s) = 0;

	   virtual bool AppendEvent(const KeyboardEvent& k, Events::ScrollEvent& updateStatus) = 0;
	   virtual bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, Events::ScrollEvent& updateStatus) = 0;
	   virtual void Free() = 0;
	   virtual void Render(IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<Events::ScrollEvent>& populator, Events::EventId populationEventId) = 0;
   };

   struct IUtilitiies
   {
	   virtual void AddSubtitle(Platform& platform, cstr subtitle) = 0;
	   virtual IScrollbar* CreateScrollbar(IKeyboardSupervisor& keyboard, bool _isVertical) = 0;
	   virtual void EnumerateFiles(IEventCallback<cstr>& cb, cstr pingPathDirectory) = 0;
	   virtual bool GetSaveLocation(Windows::IWindow& parent, SaveDesc& sd) = 0;
	   virtual bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& sd) = 0;
	   virtual bool QueryYesNo(Platform& platform, Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
	   virtual void RefreshResource(Platform& platform, cstr pingPath) = 0;
	   virtual void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform) = 0;
	   virtual void SaveBinary(cstr pathname, const void* buffer, size_t nChars) = 0;
	   virtual void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) = 0;
	   virtual IVariableEditor* CreateVariableEditor(Windows::IWindow& parent, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler = nullptr, const Vec2i* topLeft = nullptr) = 0;
   };

   namespace Graphics
   {
      struct IMeshBuilder;
	  ROCOCOAPI IRimTesselatorSupervisor: public IRimTesselator
	  {
		  virtual void Free() = 0;
	  };

	  IRimTesselatorSupervisor* CreateRimTesselator();
   }

   struct Tesselators
   {
	   Graphics::IRimTesselatorSupervisor& rim;
   };

   struct Platform
   {
      // Operating system functions
      IOS& os;

      // Content directory and raw binary file streaming
      IInstallation& installation;

      // Renderer
      IRenderer& renderer;

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

      // Entity instances
      Entities::IInstancesSupervisor& instances;

      Entities::IMobilesSupervisor& mobiles;

      Graphics::ISpriteSupervisor& sprites;

      Graphics::ICameraSupervisor& camera;

      Graphics::ISceneSupervisor& scene;

	  Tesselators& tesselators;

      IMathsVisitorSupervisor& mathsVisitor;

      // Application title
      const char* const title;
   };

   struct IApp;

   ROCOCOAPI IAppFactory
   {
      virtual IApp* CreateApp(Platform& platform) = 0;
   };

   namespace Events
   {
      struct ScrollEvent : public Events::Event
      {
         ScrollEvent(EventId id) : Event(id) {}
         int32 logicalMinValue;
         int32 logicalMaxValue;
         int32 logicalValue;
         int32 logicalPageSize;
         int32 rowSize; // Value delta for right clicks and mouse wheel events
         boolean32 fromScrollbar;
      };
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
            
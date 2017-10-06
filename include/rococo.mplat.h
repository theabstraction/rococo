#pragma once

#include <rococo.renderer.h>

namespace Rococo
{
   ROCOCO_ID(ID_ENTITY, int64, 0);
}

#include <../rococo.mplat/mplat.sxh.h>

namespace Rococo
{
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
      };
   }

   namespace Graphics
   {
      ROCOCOAPI IMeshBuilderSupervisor : public IMeshBuilder
      {
         virtual void Free() = 0;
         virtual bool TryGetByName(cstr name, ID_SYS_MESH& id) = 0;
      };
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
      virtual void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) = 0;

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
      virtual IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName) = 0;
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

   struct IUtilitiies
   {
      virtual bool QueryYesNo(Platform& platform, Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
      virtual void RefreshResource(Platform& platform, cstr pingPath) = 0;
      virtual void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform) = 0;
   };

   namespace Graphics
   {
      struct IMeshBuilder;
   }

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

      // Mesh builder object
      Graphics::IMeshBuilderSupervisor& meshes;

      // Entity instances
      Entities::IInstancesSupervisor& instances;

      Entities::IMobilesSupervisor& mobiles;

      // Application title
      const char* const title;
   };

   struct IApp;

   ROCOCOAPI IAppFactory
   {
      virtual IApp* CreateApp(Platform& platform) = 0;
   };
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
            
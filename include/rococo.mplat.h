#pragma once

#include <rococo.api.h>

#include <../rococo.mplat/mplat.sxh.h>

namespace Rococo
{
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

   struct ICommandHandler
   {

   };

   typedef void(*FN_OnCommand)(ICommandHandler* context, cstr command);

   struct IGUIStack
   {
      virtual void AppendEvent(const MouseEvent& me) = 0;
      virtual IPanelSupervisor* BindPanelToScript(cstr scriptName) = 0;
      virtual void Render(IGuiRenderContext& grc) = 0;
      virtual void PushTop(IPanelSupervisor* panel, bool isModal) = 0;
      virtual IPanelSupervisor* Pop() = 0;
      virtual IPanelSupervisor* Top() = 0;
      virtual void RegisterEventHandler(ICommandHandler* context, FN_OnCommand method, cstr cmd, cstr helpString = nullptr) = 0;
      virtual void UnregisterEventHandler(ICommandHandler* handler) = 0;
      template<class T> inline void UnregisterEventHandler(T* handler)
      {
         UnregisterEventHandler(reinterpret_cast<ICommandHandler*>(handler));
      }
   };

   struct IUtilitiies
   {
      virtual bool QueryYesNo(Platform& platform, Windows::IWindow& parent, cstr question, cstr caption = nullptr) = 0;
      virtual void RefreshResource(Platform& platform, cstr pingPath) = 0;
      virtual void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name) = 0;
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
                     
#include "hv.h"
#include <rococo.strings.h>

namespace
{
   using namespace Rococo;
   using namespace HV;

   class HVApp : public IApp, public IEventCallback<FileModifiedArgs>
   {
      Cosmos& e;

   public:
      HVApp(Cosmos& _e) : e(_e)
      {
         
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnCreated()
      {

      }

      virtual void OnEvent(FileModifiedArgs& args)
      {
         HV::Events::OnFileChangedEvent ev;
         ev.args = &args;
         Publish(e.publisher, ev);

         if (Eq(Rococo::GetFileExtension(args.resourceName), L".sxy"))
         {
            wchar_t pingname[1024];
            args.GetPingPath(pingname, 1024);
            e.sources.Release(pingname);
         }

         if (args.Matches(L"!scripts/hv/main.sxy"))
         {
            HV::RunEnvironmentScript(e, L"!scripts/hv/main.sxy");
         }
      }

      virtual uint32 OnFrameUpdated(const IUltraClock& clock)
      {
         e.installation.OS().EnumerateModifiedFiles(*this);
         e.players.Update(clock);
         e.camera.Update(clock);
         e.renderer.Render(e.scene);
         return 5;
      }

      virtual void OnKeyboardEvent(const KeyboardEvent& keyboardEvent)
      {
         Key key = e.keyboard.GetKeyFromEvent(keyboardEvent);
         auto* action = e.keyboard.GetAction(key.KeyName);
         if (action)
         {
            HV::Events::OnPlayerActionEvent pae;
            pae.Name = action;
            pae.start = key.isPressed;
            Rococo::Events::Publish(e.publisher, pae);
         }
      }

      virtual void OnMouseEvent(const MouseEvent& me)
      {

      }

      virtual const wchar_t* Title() const
      {
         return L"Hyperverse, by Mark Anthony Taylor";
      }
   };
}

namespace HV
{
   IApp* CreateHVApp(Cosmos& e)
   {
      return new HVApp(e);
   }
}

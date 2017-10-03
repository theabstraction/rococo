#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include "hv.events.h"
#include "hv.defaults.h"

namespace
{
   using namespace HV;
   using namespace HV::Graphics;
   using namespace HV::Entities;

   class HVApp : public IApp, public IEventCallback<FileModifiedArgs>
   {
      Platform& platform;
      bool editorActive{ false };

      AutoFree<IMeshBuilderSupervisor> meshes;;
      AutoFree<IInstancesSupervisor> instances;
      AutoFree<IMobilesSupervisor> mobiles;
      AutoFree<ICameraSupervisor> camera;
      AutoFree<ISceneSupervisor> scene;
      AutoFree<IPlayerSupervisor> players;
      AutoFree<IKeyboardSupervisor> keyboardSupervisor;
      AutoFree<IMouse> mouse;
      AutoFree<IMathsVisitorSupervisor> mathsVisitor;
      AutoFree<ISpriteSupervisor> sprites;
      AutoFree<IEditor> editor;
      AutoFree<IConfigSupervisor> config;

      Cosmos e; // Put this as the last member, since other members need to be constructed first
   public:
      HVApp(Platform& _platform) : 
         platform(_platform),
         config(CreateConfig()),
         meshes(CreateMeshBuilder(platform.renderer)),
         instances(CreateInstanceBuilder(*meshes, platform.renderer, platform.publisher)),
         mobiles(CreateMobilesSupervisor(*instances, platform.publisher)),
         camera(CreateCamera(*instances, *mobiles, platform.renderer, platform.publisher)),
         scene(CreateScene(*instances, *camera, platform.publisher)),
         players(CreatePlayerSupervisor(platform.publisher)),
         keyboardSupervisor(CreateKeyboardSupervisor()),
         mouse(CreateMouse(platform.publisher)),
         mathsVisitor(CreateMathsVisitor()),
         sprites(CreateSpriteSupervisor(platform.renderer)),
         editor(CreateEditor(platform, *instances)),
         e { _platform, *config, *scene, *meshes, *instances, *mobiles, *camera, *sprites, *players, *keyboardSupervisor, *mouse, *mathsVisitor, *editor }
      {
         Defaults::SetDefaults(*config);

         RunEnvironmentScript(e, "!scripts/hv/config.sxy");

         RunEnvironmentScript(e, "!scripts/hv/keys.sxy");
         RunEnvironmentScript(e, "!scripts/hv/controls.sxy");
         RunEnvironmentScript(e, "!scripts/hv/main.sxy");

         e.platform.renderer.AddOverlay(1000, &mathsVisitor->Overlay());
         e.platform.renderer.AddOverlay(1001, &e.editor.Overlay());
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
         HV::Events::OS::OnFileChangedEvent ev;
         ev.args = &args;
         e.platform.publisher.Publish(ev);

         if (Eq(Rococo::GetFileExtension(args.resourceName), ".sxy"))
         {
            rchar pingname[1024];
            args.GetPingPath(pingname, 1024);
            e.platform.utilities.RefreshResource(e.platform, pingname);
         }

         if (args.Matches("!scripts/hv/main.sxy"))
         {
            HV::RunEnvironmentScript(e, "!scripts/hv/main.sxy");
         }
      }

      virtual uint32 OnFrameUpdated(const IUltraClock& clock)
      {
         e.platform.installation.OS().EnumerateModifiedFiles(*this);
         e.players.Update(clock);
         e.camera.Update(clock);
   //    e.camera.Venue().ShowVenue(e.mathsDebugger);
         e.platform.renderer.Render(e.scene);
         return 5;
      }

      virtual void OnKeyboardEvent(const KeyboardEvent& keyboardEvent)
      {
         Key key = e.keyboard.GetKeyFromEvent(keyboardEvent);
         auto* action = e.keyboard.GetAction(key.KeyName);
         if (action)
         {  
            if (Eq(action, "gui.editor.toggle") && key.isPressed)
            {
               editorActive = !editorActive;
               e.editor.Activate(editorActive);
            }
            else
            {
               HV::Events::Player::OnPlayerActionEvent pae;
               pae.Name = action;
               pae.start = key.isPressed;
               e.platform.publisher.Publish(pae);
            }
         }
      }

      virtual void OnMouseEvent(const MouseEvent& me)
      {
         e.mouse.TranslateMouseEvent(me);
      }
   };
}

namespace HV
{
   IApp* CreateApp(Platform& p)
   {
      return new HVApp(p);
   }
}

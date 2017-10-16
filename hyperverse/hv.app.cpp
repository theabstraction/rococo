#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include "hv.events.h"
#include "hv.defaults.h"

namespace ANON
{
   using namespace HV;
   using namespace HV::Graphics;
   using namespace Rococo::Entities;

   class HVApp : public IApp, public IEventCallback<FileModifiedArgs>, public IScene
   {
	   Platform& platform;
	   bool editorActive{ false };

	   AutoFree<ISectors> sectors;
	   AutoFree<IPlayerSupervisor> players;
	   AutoFree<IEditor> editor;
	   AutoFree<IPaneBuilderSupervisor> editorPanel;
	   AutoFree<IPaneBuilderSupervisor> fpsPanel;

	   Cosmos e; // Put this as the last member, since other members need to be constructed first

	   IGameMode* mode;
	   AutoFree<IGameModeSupervisor> fpsLogic;

   public:
	   HVApp(Platform& _platform) :
		   platform(_platform),
		   sectors(CreateSectors(_platform)),
		   players(CreatePlayerSupervisor(platform)),
		   editor(CreateEditor(platform, *players, *sectors)),
		   e{ _platform, *players, *editor, *sectors },
		   fpsLogic(CreateFPSGameLogic(e))
	   {
		   mode = fpsLogic;

		   Defaults::SetDefaults(e.platform.config);

		   RunEnvironmentScript(e, "!scripts/hv/config.sxy");
		   RunEnvironmentScript(e, "!scripts/hv/keys.sxy");
		   RunEnvironmentScript(e, "!scripts/hv/controls.sxy");
		   RunEnvironmentScript(e, "!scripts/hv/main.sxy");

		   editorPanel = e.platform.gui.BindPanelToScript("!scripts/panel.editor.sxy");
		   fpsPanel = e.platform.gui.BindPanelToScript("!scripts/panel.fps.sxy");

		   e.platform.gui.PushTop(fpsPanel->Supervisor(), true);

		   editorActive = false;
	   }

	   virtual RGBA GetClearColour() const
	   {
		   return e.platform.scene.GetClearColour();
	   }

	   virtual void RenderGui(IGuiRenderContext& grc)
	   {
		   GuiMetrics metrics;
		   grc.Renderer().GetGuiMetrics(metrics);
		   GuiRect fullScreen = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
		   fpsPanel->Supervisor()->SetRect(fullScreen);
		   platform.gui.Render(grc);
	   }

	   virtual void RenderObjects(IRenderContext& rc)
	   {
		   e.platform.scene.RenderObjects(rc);
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

		   char pingPath[IO::MAX_PATHLEN];
		   args.GetPingPath(pingPath, IO::MAX_PATHLEN);

		   if (StartsWith(pingPath, "!scripts/hv/sector/"))
		   {
			   sectors->OnSectorScriptChanged(args);
		   }
	   }

	   virtual uint32 OnFrameUpdated(const IUltraClock& clock)
	   {
		   GuiMetrics metrics;
		   e.platform.renderer.GetGuiMetrics(metrics);

		   e.platform.installation.OS().EnumerateModifiedFiles(*this);
		   e.platform.publisher.Deliver();

		   mode->UpdateAI(clock);

		   e.platform.camera.Update(clock);
		   //    e.camera.Venue().ShowVenue(e.mathsDebugger);

		   GuiRect fullRect{ 0,0,metrics.screenSpan.x, metrics.screenSpan.y };
		   editorPanel->Root()->Base()->SetRect(fullRect);
		   e.platform.renderer.Render(*this);
		   return 5;
	   }

	   virtual void OnKeyboardEvent(const KeyboardEvent& keyboardEvent)
	   {
		   if (!e.platform.gui.AppendEvent(keyboardEvent))
		   {
			   Key key = e.platform.keyboard.GetKeyFromEvent(keyboardEvent);
			   auto* action = e.platform.keyboard.GetAction(key.KeyName);
			   if (action)
			   {
				   if (Eq(action, "gui.editor.toggle") && key.isPressed)
				   {
					   editorActive = !editorActive;

					   if (editorActive)
					   {
						   if (e.platform.gui.Top() != editorPanel->Supervisor())
						   {
							   e.platform.gui.PushTop(editorPanel->Supervisor(), true);
						   }

						   mode->Deactivate();
					   }
					   else
					   {
						   if (e.platform.gui.Top() == editorPanel->Supervisor())
						   {
							   e.platform.gui.Pop();
						   }

						   mode->Activate();
					   }
				   }
			   }
		   }
	   }

	   virtual void OnMouseEvent(const MouseEvent& me)
	   {
		   e.platform.gui.AppendEvent(me);
	   }
   };
}

namespace HV
{
   IApp* CreateApp(Platform& p)
   {
      return new ANON::HVApp(p);
   }
}

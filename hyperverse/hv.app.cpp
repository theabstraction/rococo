#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include "hv.events.h"
#include "hv.defaults.h"

#include <string>

namespace HV
{
   using namespace HV::Graphics;
   using namespace Rococo::Entities;

   class App : public IApp, public IEventCallback<FileModifiedArgs>, public IScene, public IObserver
   {
	   Platform& platform;
	   bool editorActive{ false };
	   bool overlayActive{ false };

	   AutoFree<ISectors> sectors;
	   AutoFree<IPlayerSupervisor> players;
	   AutoFree<IEditor> editor;
	   AutoFree<IPaneBuilderSupervisor> editorPanel;
	   AutoFree<IPaneBuilderSupervisor> fpsPanel;
	   AutoFree<IPaneBuilderSupervisor> overlayPanel;

	   Cosmos e; // Put this as the last member, since other members need to be constructed first

	   IGameMode* mode;
	   AutoFree<IGameModeSupervisor> fpsLogic;

	   std::string nextLevelName;

	   virtual void OnEvent(Event& ev)
	   {
		   if (ev.id == HV::Events::setNextLevelEventId)
		   {
			   auto& nl = As<HV::Events::SetNextLevelEvent>(ev);
			   nextLevelName = nl.name;
		   }
	   }
   public:
	   App(Platform& _platform) :
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
		   overlayPanel = e.platform.gui.CreateOverlay();

		   e.platform.gui.PushTop(fpsPanel->Supervisor(), true);

		   editorActive = false;

		   e.platform.publisher.Attach(this, HV::Events::setNextLevelEventId);
	   }

	   ~App()
	   {
		   e.platform.publisher.Detach(this);
	   }

	   RGBA GetClearColour() const override
	   {
		   if (e.sectors.begin() == e.sectors.end())
		   {
			   return RGBA(0, 0.25f, 0);
		   }
		   else
		   {
			   return e.platform.scene.GetClearColour();
		   }
	   }

	   const Light* GetLights(size_t& nLights) const
	   {
		   return e.platform.scene.GetLights(nLights);
	   }

	   void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc)
	   {
		   return e.platform.scene.RenderShadowPass(drd, rc);
	   }

	   void RenderGui(IGuiRenderContext& grc) override
	   {
		   GuiMetrics metrics;
		   grc.Renderer().GetGuiMetrics(metrics);
		   GuiRect fullScreen = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
		   fpsPanel->Supervisor()->SetRect(fullScreen);

		   platform.gui.Render(grc);
	   }

	   void RenderObjects(IRenderContext& rc)  override
	   {
		   e.platform.scene.RenderObjects(rc);
	   }

	   void Free() override
	   {
		   delete this;
	   }

	   void OnEvent(FileModifiedArgs& args) override
	   {
		   HV::Events::OS::OnFileChangedEvent ev;
		   ev.args = &args;
		   e.platform.publisher.Publish(ev);

		   rchar pingname[1024];
		   args.GetPingPath(pingname, 1024);

		   auto ext = Rococo::GetFileExtension(args.resourceName);
		   if (!ext)
		   {

		   }
		   else if (Eq(ext, ".sxy"))
		   {
			   e.platform.utilities.RefreshResource(e.platform, pingname);

			   if (args.Matches("!scripts/hv/main.sxy"))
			   {
				   HV::RunEnvironmentScript(e, "!scripts/hv/main.sxy");
			   }

			   if (StartsWith(pingname, "!scripts/hv/sector/"))
			   {
				   sectors->OnSectorScriptChanged(args);
			   }
		   }
		   else if (Eq(ext, ".ps"))
		   {
			   e.platform.renderer.UpdatePixelShader(pingname);
		   }
		   else if (Eq(ext, ".vs"))
		   {
			   e.platform.renderer.UpdateVertexShader(pingname);
		   }
	   }

	   uint32 OnFrameUpdated(const IUltraClock& clock) override
	   {
		   GuiMetrics metrics;
		   e.platform.renderer.GetGuiMetrics(metrics);

		   e.platform.installation.OS().EnumerateModifiedFiles(*this);
		   e.platform.publisher.Deliver();

		   if (!nextLevelName.empty())
		   {
			   RunEnvironmentScript(e, nextLevelName.c_str());
			   nextLevelName.clear();
			   e.platform.sourceCache.Release(nextLevelName.c_str());
		   }

		   mode->UpdateAI(clock);

		   e.platform.camera.Update(clock);
		   //    e.camera.Venue().ShowVenue(e.mathsDebugger);

		   GuiRect fullRect{ 0,0,metrics.screenSpan.x, metrics.screenSpan.y };
		   editorPanel->Root()->Base()->SetRect(fullRect);
		   e.platform.renderer.Render(*this);

		   if (editorActive)
		   {
			   if (editor->IsLoading())
			   {
				   return 5;
			   }
			   else
			   {
				   return 100;
			   }
		   }
		   else if (e.sectors.begin() == e.sectors.end())
		   {
			   return 100;
		   }
		   else
		   {
			   return 5;
		   }
	   }

	   void ActivateEditor()
	   {
		   if (!editorActive && !overlayActive)
		   {
			   if (e.platform.gui.Top() != editorPanel->Supervisor())
			   {
				   e.platform.gui.PushTop(editorPanel->Supervisor(), true);
				   mode->Deactivate();

				   editorActive = true;
			   }
		   }
	   }

	   void DeactivateEditor()
	   {
		   if (editorActive)
		   {
			   if (e.platform.gui.Top() == editorPanel->Supervisor())
			   {
				   e.platform.gui.Pop();
				   if (!overlayActive) mode->Activate();
				   editorActive = false;
			   }
		   }
	   }

	   void ActivateOverlay()
	   {
		   if (!overlayActive)
		   {
			   if (e.platform.gui.Top() != overlayPanel->Supervisor())
			   {
				   e.platform.gui.PushTop(overlayPanel->Supervisor(), true);
				   mode->Deactivate();
				   overlayActive = true;
			   }
		   }
	   }

	   void DeactivateOverlay()
	   {
		   if (overlayActive)
		   {
			   if (e.platform.gui.Top() == overlayPanel->Supervisor())
			   {
				   e.platform.gui.Pop();
				   if (!editorActive) mode->Activate();
				   overlayActive = false;
			   }
		   }
	   }

	   void OnKeyboardEvent(const KeyboardEvent& keyboardEvent) override
	   {
		   if (!e.platform.gui.AppendEvent(keyboardEvent))
		   {
			   Key key = e.platform.keyboard.GetKeyFromEvent(keyboardEvent);
			   auto* action = e.platform.keyboard.GetAction(key.KeyName);
			   if (action)
			   {
				   if (Eq(action, "gui.editor.toggle") && key.isPressed)
				   {
					   if (!editorActive)
					   {
						   ActivateEditor();
					   }
					   else
					   {
						   DeactivateEditor();
					   }
				   }
				   else if (Eq(action, "gui.overlay.toggle") && key.isPressed)
				   {
					   if (!overlayActive)
					   {
						   ActivateOverlay();
					   }
					   else
					   {
						   DeactivateOverlay();
					   }
				   }
			   }
		   }
	   }

	   void OnMouseEvent(const MouseEvent& me) override
	   {
		   e.platform.gui.AppendEvent(me);
	   }
   };
}

namespace HV
{
   IApp* CreateApp(Platform& p)
   {
      return new HV::App(p);
   }
}

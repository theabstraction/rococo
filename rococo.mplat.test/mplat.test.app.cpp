#include <rococo.renderer.h>
#include <rococo.mplat.h>
#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

#include <sexy.script.h>

#include <vector>
#include <algorithm>
#include <string>

using namespace Rococo;
using namespace Rococo::Events;

struct TestAttributes: IObserver
{
   struct Attribute
   {
      EventIdRef id;
      int value;
   };

   std::vector<Attribute> textOutEvents =
   {
      { "strength.update.text"_event, 16 },
      { "linguistics.update.text"_event, 14 },
      { "puissance.update.text"_event, 12 },
      { "agility.update.text"_event, 17 },
      { "resilience.update.text"_event, 3 },
      { "beauty.update.text"_event, 14 }
   };

   IPublisher& publisher;

   TestAttributes(IPublisher& _publisher): 
      publisher(_publisher)
   {
      for (auto& i : textOutEvents)
      {
         publisher.Subscribe(this, i.id);
      }
   }

   void OnEvent(Event& ev) override
   {
      for (auto& i : textOutEvents)
      {
         if (ev == i.id)
         {
            auto& toe = As<TextOutputEvent>(ev);
            if (toe.isGetting)
            {
               SafeFormat(toe.text, sizeof(toe.text), "%d", i.value);
            }
         }
      }
   }

   ~TestAttributes()
   {
      publisher.Unsubscribe(this);
   }
};

struct TestApp : IApp, private IScene, public IEventCallback<FileModifiedArgs>
{
   Platform& platform;
   AutoFree<IPaneBuilderSupervisor> testPanel;
   AutoFree<IPaneBuilderSupervisor> introPanel;
   AutoFree<IPaneBuilderSupervisor> soundPanel;
   AutoFree<IPaneBuilderSupervisor> creditsPanel;
   AutoFree<IPaneBuilderSupervisor> graphicsPanel;
   AutoFree<IPaneBuilderSupervisor> quitPanel;

   TestAttributes attributes;

   TestApp(Platform& _platform): platform(_platform), attributes(platform.publisher)
   {
      introPanel = platform.gui.BindPanelToScript("!scripts/panel.intro.sxy");
      testPanel = platform.gui.BindPanelToScript("!scripts/panel.test.sxy");
      soundPanel = platform.gui.BindPanelToScript("!scripts/panel.sound.sxy");
      creditsPanel = platform.gui.BindPanelToScript("!scripts/panel.credits.sxy");
      graphicsPanel = platform.gui.BindPanelToScript("!scripts/panel.graphics.sxy");
      quitPanel = platform.gui.BindPanelToScript("!scripts/panel.quit.sxy");

      platform.gui.PushTop(testPanel->Supervisor(), true);
      platform.gui.PushTop(introPanel->Supervisor(), true);

      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandPushSound,  "gui.push.sound", nullptr);
      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandPushCredits, "gui.push.credits", nullptr);
      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandPushGraphics, "gui.push.graphics", nullptr);
      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandPushQuit, "gui.push.quit", nullptr);
      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandQuit, "app.quit", nullptr);
      REGISTER_UI_EVENT_HANDLER(platform.gui, this, TestApp, OnCommandPop, "gui.pop", nullptr);
   }

   ~TestApp()
   {
      platform.gui.UnregisterEventHandler(this);
   }

   void Free() override
   {
      delete this;
   }

   RGBA GetClearColour() const override
   {
      return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
   }

   const Light* GetLights(size_t& nLights) const override
   {
	   nLights = 0;
	   return nullptr;
   }

   void RenderGui(IGuiRenderContext& grc) override
   {
      using namespace Graphics;

      GuiMetrics metrics;
      grc.Renderer().GetGuiMetrics(metrics);
         
      auto ss = metrics.screenSpan;

      GuiRect testRect{ ss.x >> 1, 0, ss.x, ss.y };
      testPanel->Supervisor()->SetRect(testRect);

      GuiRect fullRect{ 0, 0, ss.x, ss.y };
      introPanel->Supervisor()->SetRect(fullRect);
      soundPanel->Supervisor()->SetRect(fullRect);
      creditsPanel->Supervisor()->SetRect(fullRect);
      graphicsPanel->Supervisor()->SetRect(fullRect);
      quitPanel->Supervisor()->SetRect(fullRect);

      platform.gui.Render(grc);
   }

   void GetCamera(Matrix4x4& camera, Matrix4x4& world, Vec4& eye, Vec4& viewDir)
   {
	   camera = world = Matrix4x4::Identity();
	   eye = { 0,0,0 };
	   viewDir = { 0,0,1 };
   }

   void RenderObjects(IRenderContext& rc) override
   {
         
   }

   void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) override
   {

   }

   void OnCommandQuit(cstr uiCommand)
   {
      OS::ShutdownApp();
   }

   void OnCommandPushGraphics(cstr uiCommand)
   {
      platform.gui.PushTop(graphicsPanel->Supervisor(), true);
   }

   void OnCommandPushQuit(cstr uiCommand)
   {
      platform.gui.PushTop(quitPanel->Supervisor(), true);
   }

   void OnCommandPushSound(cstr uiCommand)
   {
      platform.gui.PushTop(soundPanel->Supervisor(), true);
   }

   void OnCommandPushCredits(cstr uiCommand)
   {
      platform.gui.PushTop(creditsPanel->Supervisor(), true);
   }

   void OnCommandPop(cstr uiCommand)
   {
      platform.gui.Pop();
   }

   void OnEvent(FileModifiedArgs& args) override
   {
      char pingPath[IO::MAX_PATHLEN];
      args.GetPingPath(pingPath, IO::MAX_PATHLEN);
      platform.utilities.RefreshResource(pingPath);
   }

   uint32 OnFrameUpdated(const IUltraClock& clock) override
   {
      platform.os.EnumerateModifiedFiles(*this);
      platform.publisher.Deliver();

	  Graphics::RenderPhaseConfig config;
	  config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
      platform.renderer.Render(config, *this);
      
      return 2; // 2 millisecond sleep period
   }

   void OnKeyboardEvent(const KeyboardEvent& k) override
   {

   }

   void OnMouseEvent(const MouseEvent& me) override
   {
      platform.gui.AppendEvent(me);
   }

   void OnCreate() override
   {

   }
};

namespace Test
{
   IApp* CreateApp(Platform& platform)
   {
      return new TestApp(platform);
   }
}
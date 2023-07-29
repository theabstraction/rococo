#include <rococo.renderer.h>
#include <rococo.mplat.h>
#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

#include <sexy.script.h>

#include <vector>
#include <algorithm>
#include <string>

#include <rococo.asset.generator.h>
#include <rococo.csv.h>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Assets;
using namespace Rococo::Events;
using namespace Rococo::Sexy;

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

   AutoFree<IAssetGenerator> assetGenerator;

   TestApp(Platform& _platform): platform(_platform), attributes(platform.publisher)
   {
      assetGenerator = Rococo::Assets::CreateAssetGenerator_SexyContentFile(platform.installation);
     
      introPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.intro.sxy");
      testPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.test.sxy");
      soundPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.sound.sxy");
      creditsPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.credits.sxy");
      graphicsPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.graphics.sxy");
      quitPanel = platform.gui.BindPanelToScript("!scripts/panels/panel.quit.sxy");

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

   const Light* GetLights(uint32& nLights) const override
   {
	   nLights = 0;
	   return nullptr;
   }

   void OnGuiResize(Vec2i span) override
   {
	   GuiRect testRect{ span.x >> 1, 0, span.x, span.y };
	   testPanel->Supervisor()->SetRect(testRect);

	   GuiRect fullRect{ 0, 0, span.x, span.y };
	   introPanel->Supervisor()->SetRect(fullRect);
	   soundPanel->Supervisor()->SetRect(fullRect);
	   creditsPanel->Supervisor()->SetRect(fullRect);
	   graphicsPanel->Supervisor()->SetRect(fullRect);
	   quitPanel->Supervisor()->SetRect(fullRect);
   }

   void RenderGui(IGuiRenderContext& grc) override
   {
      platform.gui.Render(grc);
   }

   void GetCamera(Matrix4x4& camera, Matrix4x4& world, Matrix4x4& proj, Vec4& eye, Vec4& viewDir)
   {
	   proj = camera = world = Matrix4x4::Identity();
	   eye = { 0,0,0 };
	   viewDir = { 0,0,1 };
   }

   void RenderObjects(IRenderContext& rc, bool skinned) override
   {
         
   }

   void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned) override
   {

   }

   void OnCommandQuit(cstr uiCommand)
   {
      platform.appControl.ShutdownApp();
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
      U8FilePath pingPath;
	  platform.installation.ConvertSysPathToPingPath(args.sysPath, pingPath);
      platform.utilities.RefreshResource(pingPath);
   }

   uint32 OnFrameUpdated(const IUltraClock& clock) override
   {
      platform.os.EnumerateModifiedFiles(*this);
      platform.publisher.Deliver();
      platform.renderer.Render(Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, *this);
      
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
	   struct LinkAssetGeneratorCallback : IEventCallback<ScriptCompileArgs>
	   {
		   LinkAssetGeneratorCallback(IAssetGenerator& _generator, IInstallation& _installation) :
			   generator(_generator), installation(_installation)
		   {

		   }

           IInstallation& installation;
		   IAssetGenerator& generator;

		   void OnEvent(ScriptCompileArgs& args)
		   {
			   LinkAssetGenerator(generator, args.ss);
               LinkAssetLoader(installation, args.ss);
		   }
       } addArchiver(*assetGenerator, platform.installation);
       platform.utilities.RunEnvironmentScript(addArchiver, "!scripts/test.app.created.sxy", true);
   }

   ID_CUBE_TEXTURE GetSkyboxCubeId() const
   {
	   return ID_CUBE_TEXTURE::Invalid();
   }
};

namespace Test
{
   IApp* CreateApp(Platform& platform)
   {
      return new TestApp(platform);
   }
}
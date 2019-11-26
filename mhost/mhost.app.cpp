#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>

#include <rococo.textures.h>

#include <rococo.ui.h>

namespace MHost
{
	using namespace Rococo::Entities;
	using namespace Rococo::Events;
	using namespace Rococo::Textures;
	using namespace Rococo::Script;
	using namespace MHost::OS;

	auto evPopulateBusyCategoryId = "busy.category"_event;
	auto evPopulateBusyResourceId = "busy.resource"_event;

	void RunEnvironmentScript(Platform& platform, IEngineSupervisor* engine, cstr name, bool releaseAfterUse, bool trace);

	namespace UI
	{
		void CaptureMouse(Rococo::Windows::IWindow& window);
		void ReleaseMouse();
	}

	struct AppSceneManager : public IScene
	{
		Platform& platform;
		GuiPopulator populator;
		IPublicScriptSystem* ss;
		AutoFree<IScriptDispatcher> dispatcher;
		char guiBuffer[64];

		ISceneBuilderSupervisor& scene3DBuilder;

		AppSceneManager(Platform& _platform, ISceneBuilderSupervisor& _scene3DBuilder):
			platform(_platform),
			scene3DBuilder(_scene3DBuilder)
		{
			dispatcher = CreateScriptDispatcher();
			platform.camera.SetRHProjection(45_degrees, 0.1_metres, 1000_metres);
		}

		void GetCamera(Matrix4x4& camera, Matrix4x4& world, Vec4& eye, Vec4& viewDir) override
		{
			return platform.scene.GetCamera(camera, world, eye, viewDir);
		}

		RGBA GetClearColour() const override
		{
			return RGBA{ 0.5f, 0.0f, 0.0f, 1.0f };
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			return platform.scene.OnGuiResize(screenSpan);
		}

		void OnCompile(IPublicScriptSystem& ss)
		{
			dispatcher->OnCompile(ss);
		}

		void RenderGui(IGuiRenderContext& grc)  override
		{
			IGui* gui = CreateGuiOnStack(guiBuffer, grc);
			dispatcher->RouteGuiToScript(ss, gui, populator);
			platform.scene.RenderGui(grc);
		}

		ID_TEXTURE GetSkyboxCubeId() const override
		{
			return scene3DBuilder.GetSkyBoxCubeId();
		}

		void RenderObjects(IRenderContext& rc)  override
		{
			return platform.scene.RenderObjects(rc);
		}

		const Light* GetLights(size_t& nCount) const override
		{
			return platform.scene.GetLights(nCount);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc)  override
		{
			return platform.scene.RenderShadowPass(drd, rc);
		}
	};

	class App : 
		public IDirectApp, 
		public IEventCallback<FileModifiedArgs>,
		public IObserver, 
		public IEngineSupervisor
	{
		Platform& platform;
		IDirectAppControl& control;

		AutoFree<IPaneBuilderSupervisor> overlayPanel;
		AutoFree<IPaneBuilderSupervisor> busyPanel;
		AutoFree<ISceneBuilderSupervisor> scene3DBuilder;

		AppSceneManager sceneManager;

		HString mainScript = "!scripts/mhost/mhost.init.sxy";
		bool queuedForExecute = false;

		Vec2 cursorPosition;

		// Busy event handler responds to resource loading and renders progress panel
		void OnBusy(const Rococo::Events::BusyEvent& be)
		{
			struct BusyEventCapture : public IObserver
			{
				IPublisher& publisher;
				const Rococo::Events::BusyEvent& be;
				BusyEventCapture(IPublisher& _publisher, const Rococo::Events::BusyEvent& _be) : publisher(_publisher), be(_be)
				{
					publisher.Subscribe(this, evPopulateBusyCategoryId);
					publisher.Subscribe(this, evPopulateBusyResourceId);
				}

				~BusyEventCapture()
				{
					publisher.Unsubscribe(this);
				}

				virtual void OnEvent(Event& ev)
				{
					if (ev == evPopulateBusyCategoryId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, sizeof(te.text), "%s", be.message);
						}
					}
					else if (ev == evPopulateBusyResourceId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, sizeof(te.text), "%s", be.resourceName);
						}
					}
				}
			} ec(platform.publisher, be);

			platform.gui.PushTop(busyPanel->Supervisor(), true);

			Rococo::Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, platform.scene);
			platform.gui.Pop();
		}

		void SetRunningScriptContext(IPublicScriptSystem* ss)
		{
			sceneManager.ss = ss;
		}

		virtual void OnEvent(Event& ev)
		{
			if (ev == Rococo::Events::evBusy)
			{
				auto& be = As <Rococo::Events::BusyEvent>(ev);
				if (be.isNowBusy)
				{
					if (platform.gui.Top() != busyPanel->Supervisor())
					{
						OnBusy(be);
					}
				}
			}
			else if (ev == evPopulateBusyCategoryId)
			{
			}

			else if (ev == evPopulateBusyResourceId)
			{
			}
		}

		void OnCompile(IPublicScriptSystem& ss) override
		{
			sceneManager.OnCompile(ss);
		}

		boolean32 TryGetSpriteSpec(const fstring& resourceName, BitmapLocation& loc) override
		{
			return platform.renderer.SpriteBuilder().TryGetBitmapLocation(resourceName, loc);
		}

		void GetSpriteSpec(const fstring& resourceName, Rococo::Textures::BitmapLocation& loc) override
		{
			if (!TryGetSpriteSpec(resourceName, loc))
			{
				Throw(0, "Could not load bitmap: %s", (cstr)resourceName);
			}
		}
	public:
		App(Platform& _platform, IDirectAppControl& _control) :
			platform(_platform), control(_control), scene3DBuilder(CreateSceneBuilder(_platform)), sceneManager(_platform, *scene3DBuilder)
		{
			busyPanel = platform.gui.BindPanelToScript("!scripts/panel.opening.sxy");
			overlayPanel = platform.gui.CreateOverlay();

			platform.publisher.Subscribe(this, Rococo::Events::evBusy);
		}

		~App()
		{
			platform.publisher.Unsubscribe(this);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			char pingname[1024];
			SafeFormat(pingname, 1024, "!%S", args.resourceName);
			Rococo::OS::ToUnixPath(pingname);
			
			platform.gui.LogMessage("File modified: %s", pingname);

			auto ext = Rococo::GetFileExtension(args.resourceName);
			if (!ext)
			{

			}
			else if (Eq(ext, L".sxy"))
			{
				platform.utilities.RefreshResource(pingname);
				queuedForExecute = true;
			}
			else if (Eq(ext, L".ps"))
			{
				platform.gui.LogMessage("Updating pixel shader");
				platform.renderer.UpdatePixelShader(pingname);
			}
			else if (Eq(ext, L".vs"))
			{
				platform.gui.LogMessage("Updating vertex shader");
				platform.renderer.UpdateVertexShader(pingname);
			}
		}

		bool isScriptRunning = true; // Will be set to false in YieldForSystemMessages if script rerun required

		bool isShutdown = false;

		// used by a script in Run() via IEngine.Run to determine if it should terminate gracefully
		// termination should occur of the user interface has collapsed, or script queued for re-run
		boolean32 IsRunning() override 
		{
			return platform.appControl.IsRunning() && isScriptRunning && !isShutdown;
		}

		void CleanupResources()
		{
			ReleaseMouse();
		}

		void Run() override
		{
			RunEnvironmentScript(platform, this, "!scripts/mhost/keys.sxy", true, false);

			while (platform.appControl.IsRunning() && !isShutdown)
			{
				isScriptRunning = true;
				RunEnvironmentScript(platform, this, mainScript, true, false);
				CleanupResources();
			}
		}

		void YieldForSystemMessages(int32 sleepMS) override
		{
			platform.installation.OS().EnumerateModifiedFiles(*this);
			platform.publisher.Deliver();

			if (!control.TryRouteSysMessages(sleepMS))
			{
				platform.appControl.ShutdownApp();
			}
			else
			{
				// Okay message queue is fine, no WM_QUIT yet, but script may have changed, and script must be rerun
				if (queuedForExecute)
				{
					isScriptRunning = false; // Script should detect [IEngine.IsRunning] and terminate to allow next script to run
					queuedForExecute = false; 
				}
			}
		}

		void Render(MHost::GuiPopulator populator) override
		{
			Rococo::Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;

			sceneManager.populator = populator;

			platform.renderer.Render(config, sceneManager);

			sceneManager.populator = GuiPopulator{ 0,nullptr };
		}

		void PollKeyState(KeyState& keyState) override
		{
			Rococo::OS::PollKeys(keyState.keys);
		}

		void SetNextScript(const fstring& scriptName) override
		{
			mainScript = scriptName;
		}

		void Shutdown() override
		{
			isShutdown = true;
		}

		boolean32 GetNextMouseEvent(Rococo::MouseEvent& me) override
		{
			cursorPosition = { (float)me.cursorPos.x, (float)me.cursorPos.y };
			return (boolean32)control.TryGetNextMouseEvent(me);
		}

		boolean32 GetNextKeyboardEvent(MHostKeyboardEvent& k) override
		{
			KeyboardEvent key;
			if (control.TryGetNextKeyboardEvent(key))
			{
				k.asciiCode = (key.unicode > 0 && key.unicode < 128) ? key.unicode : 0;
				k.isUp = key.IsUp();
				k.scancode = key.scanCode;
				k.vkeyCode = key.VKey;
				return 1;
			}
			else
			{
				k = { 0 };
				return 0;
			}
		}

		void CaptureMouse() override
		{
			MHost::UI::CaptureMouse(this->platform.renderer.Window());
		}

		void ReleaseMouse() override
		{
			MHost::UI::ReleaseMouse();
		}

		void CursorPosition(Vec2& cursorPosition)
		{
			cursorPosition = this->cursorPosition;
		}

		ISceneBuilder* Scene()
		{
			return scene3DBuilder;
		}
	};
}

namespace MHost
{
	IDirectApp* CreateApp(Platform& p, IDirectAppControl& control)
	{
		p.installation.Macro("#bitmaps", "!scripts/mhost/bitmaps/");
		p.installation.Macro("#chaps", "!scripts/mhost/rpg/chaps/");
		return new MHost::App(p, control);
	}
}

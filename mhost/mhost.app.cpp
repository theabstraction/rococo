#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>
#include <string>

#include <rococo.textures.h>

#include <rococo.ui.h>

struct AppArgs
{
	cstr mainScript;
};

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

		AppSceneManager(Platform& _platform): platform(_platform)
		{
			dispatcher = CreateScriptDispatcher();
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return platform.scene.GetSkyboxCubeId();
		}

		void GetCamera(Matrix4x4& worldToScreen, Matrix4x4& worldToCamera, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{
			return platform.scene.GetCamera(worldToScreen, worldToCamera, proj, eye, viewDir);
		}

		RGBA GetClearColour() const override
		{
			return RGBA{ 0.0f, 0.0f, 0.0f, 1.0f };
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			platform.scene.OnGuiResize(screenSpan);
		}

		void OnCompile(IPublicScriptSystem& ss)
		{
			dispatcher->OnCompile(ss);
		}

		void RenderGui(IGuiRenderContext& grc)  override
		{
			IGui* gui = CreateGuiOnStack(guiBuffer, grc);
			dispatcher->RouteGuiToScript(ss, gui, populator);
			platform.gui.Render(grc);
		}

		void RenderObjects(IRenderContext& rc)  override
		{
			platform.scene.RenderObjects(rc);
		}

		const Light* GetLights(size_t& nCount) const override
		{
			return platform.scene.GetLights(nCount);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc)  override
		{
			platform.scene.RenderShadowPass(drd, rc);
		}
	};

	struct EmptyScene : public IScene
	{
		EmptyScene()
		{
		}

		void GetCamera(Matrix4x4& worldToScreen, Matrix4x4& worldToCamera, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{
			proj = worldToScreen = worldToCamera = Matrix4x4::Identity();
			eye = { 0,0,0 };
			viewDir = { 1,0,0 };
		}

		RGBA GetClearColour() const override
		{
			return RGBA{ 0.5f, 0.0f, 0.0f, 1.0f };
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
		}

		void RenderGui(IGuiRenderContext& grc)  override
		{
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		void RenderObjects(IRenderContext& rc)  override
		{
		}

		const Light* GetLights(size_t& nCount) const override
		{
			return nullptr;
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc)  override
		{

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

		AppSceneManager sceneManager;

		HString mainScript;
		bool queuedForExecute = false;

		Vec2 cursorPosition;

		int32 overlayToggleKey = 0;

		// Busy event handler responds to resource loading and renders progress panel
		void OnBusy(const Rococo::Events::BusyEvent& be)
		{
			struct BusyEventCapture : public IObserver
			{
				IPublisher& publisher;
				const Rococo::Events::BusyEvent& be;
				BusyEventCapture(IPublisher& _publisher, const Rococo::Events::BusyEvent& _be) : 
					publisher(_publisher), be(_be)
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

			EmptyScene emptyScene;
			platform.renderer.Render(config, emptyScene);
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
		App(Platform& _platform, IDirectAppControl& _control, const AppArgs& args) :
			platform(_platform), control(_control), sceneManager(_platform)
		{
			busyPanel = platform.gui.BindPanelToScript("!scripts/panel.opening.sxy");
			overlayPanel = platform.gui.CreateDebuggingOverlay();

			platform.publisher.Subscribe(this, Rococo::Events::evBusy);

			this->mainScript = args.mainScript;
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

				// mainScript variable can be changed by a script, so not safe to pass references to the
				// script and expect it to be unchanged throughout. So duplicate on the stack

				char currentScript[Rococo::IO::MAX_PATHLEN];
				SecureFormat(currentScript, Rococo::IO::MAX_PATHLEN, "%s", mainScript.c_str());

				RunEnvironmentScript(platform, this, currentScript, true, false);
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
			if (!populator.byteCodeId)
			{
				Throw(0, "GuiPopulator undefined");
			}

			Rococo::Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			sceneManager.populator = populator;
			platform.renderer.Render(config, sceneManager);
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
			if (IsOverlayActive())
			{
				while (IsOverlayActive() && control.TryGetNextMouseEvent(me))
				{
					platform.gui.AppendEvent(me);
				}

				return false;
			}

			boolean32 found = control.TryGetNextMouseEvent(me);
			if (found)
			{
				cursorPosition = { (float)me.cursorPos.x, (float)me.cursorPos.y };
				return true;
			}
			else
			{
				return false;
			}
		}

		void GetNextMouseDelta(Vec2& delta)
		{
			control.GetNextMouseDelta(delta);
		}

		bool IsOverlayActive()
		{
			return platform.gui.Top() == overlayPanel->Supervisor();
		}

		void ToggleOverlay()
		{
			if (!IsOverlayActive())
			{
				platform.gui.PushTop(overlayPanel->Supervisor(), true);
			}
			else
			{
				platform.gui.Pop();
			}
		}

		void SetOverlayToggleKey(int32 vkeyCode) override
		{
			overlayToggleKey = vkeyCode;
		}

		boolean32 GetNextKeyboardEvent(MHostKeyboardEvent& k) override
		{
			KeyboardEvent key;
			if (control.TryGetNextKeyboardEvent(key))
			{
				if (IsOverlayActive())
				{
					platform.gui.AppendEvent(key);

					if (key.VKey == (uint16)overlayToggleKey)
					{
						if (!key.IsUp())
						{
							ToggleOverlay();
						}
					}

					k = { 0 };
					return 0;
				}
				else if (key.VKey == (uint16) overlayToggleKey)
				{
					if (!key.IsUp())
					{
						ToggleOverlay();
					}
					k = { 0 };
					return 0;
				}
				else
				{
					k.asciiCode = (key.unicode > 0 && key.unicode < 128) ? key.unicode : 0;
					k.isUp = key.IsUp();
					k.scancode = key.scanCode;
					k.vkeyCode = key.VKey;
				}
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
	};
}

namespace MHost
{
	IDirectApp* CreateApp(Platform& p, IDirectAppControl& control, cstr cmdLine)
	{
		p.installation.Macro("#bitmaps", "!scripts/mhost/bitmaps/");
		p.installation.Macro("#chaps", "!scripts/mhost/rpg/chaps/");

		struct arglist: IEventCallback<cstr>
		{
			std::vector<std::string> items;
			void OnEvent(cstr token) override
			{
				items.push_back(token);
			}
		} args;

		Rococo::SplitString(cmdLine, 0, " \t", args);

		AppArgs appArgs;
		appArgs.mainScript = "!scripts/mhost/mhost.default.sxy";

		for (size_t i = 1; i < args.items.size(); ++i)
		{
			cstr arg = args.items[i].c_str();
			if (arg[0] == '-')
			{
				// Opt
			}
			else
			{
				// filename
				appArgs.mainScript = arg;
			}
		}

		return new MHost::App(p, control, appArgs);
	}
}

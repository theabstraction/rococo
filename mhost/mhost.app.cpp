#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>
#include <string>

#include <rococo.textures.h>

#include <rococo.ui.h>
#include <rococo.sexy.api.h>

#include <rococo.package.h>

using namespace Rococo;
using namespace Rococo::Strings;

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

	void RunMHostEnvironmentScript(Platform& platform, IEngineSupervisor* engine, cstr name, bool releaseAfterUse, bool trace, IPackage& package, IEventCallback<cstr>* onScriptCrash, StringBuilder* declarationBuilder);

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

		void RenderObjects(IRenderContext& rc, bool skinned)  override
		{
			platform.scene.RenderObjects(rc, skinned);
		}

		const Light* GetLights(uint32& nCount) const override
		{
			return platform.scene.GetLights(nCount);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned)  override
		{
			platform.scene.RenderShadowPass(drd, rc, skinned);
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

		void RenderObjects(IRenderContext& rc, bool skinned)  override
		{
		}

		const Light* GetLights(uint32& nCount) const override
		{
			return nullptr;
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned)  override
		{

		}
	};

	class App : 
		public IDirectApp, 
		public IEventCallback<FileModifiedArgs>,
		public IObserver, 
		public IEngineSupervisor,
		public IEventCallback<cstr>
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

		AutoFree<IPackageSupervisor> packageMHost;

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

				void OnEvent(Event& ev) override
				{
					if (ev == evPopulateBusyCategoryId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, "%s", be.message);
						}
					}
					else if (ev == evPopulateBusyResourceId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, "%s", be.pingPath.buf);
						}
					}
				}
			} ec(platform.publisher, be);

			platform.gui.PushTop(busyPanel->Supervisor(), true);

			EmptyScene emptyScene;
			platform.renderer.Render(Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, emptyScene);
			platform.gui.Pop();
		}

		void SetRunningScriptContext(IPublicScriptSystem* ss)
		{
			sceneManager.ss = ss;
		}

		void OnEvent(Event& ev) override
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
			return platform.renderer.Gui().SpriteBuilder().TryGetBitmapLocation(resourceName, loc);
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

			WideFilePath sysPathMHost;
			platform.installation.ConvertPingPathToSysPath("!packages/mhost_1000.sxyz", sysPathMHost);
			this->packageMHost = OpenZipPackage(sysPathMHost, "mhost");

			platform.sourceCache.AddPackage(packageMHost);
		}

		~App()
		{
			platform.publisher.Unsubscribe(this);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(cstr sourceOfCrash) override
		{
			platform.os.EnumerateModifiedFiles(*this);
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			U8FilePath pingPath;
			platform.installation.ConvertSysPathToPingPath(args.sysPath, pingPath);	
			platform.gui.LogMessage("File modified: %s", pingPath);

			auto ext = Rococo::Strings::GetFileExtension(pingPath);
			if (!ext)
			{

			}
			else if (Eq(pingPath, "!scripts/mhost/declarations.sxy"))
			{
				// The declarations file is for sexy-sense, and is not meant to be compiled at any time.
			}
			else if (Eq(ext, ".sxy"))
			{
				platform.gui.LogMessage("Updating script file");
				platform.utilities.RefreshResource(pingPath);
				queuedForExecute = true;
			}
			else if (Eq(ext, ".ps"))
			{
				platform.gui.LogMessage("Updating pixel shader");
				platform.renderer.Shaders().UpdatePixelShader(pingPath);
			}
			else if (Eq(ext, ".vs"))
			{
				platform.gui.LogMessage("Updating vertex shader");
				platform.renderer.Shaders().UpdateVertexShader(pingPath);
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

		struct NoEventArgs : IEventCallback<ScriptCompileArgs>
		{
			void OnEvent(ScriptCompileArgs& args) override
			{

			}
		};

		void RunMPlatScript(const fstring& scriptName) override
		{
			struct CLOSURE : IEventCallback<ScriptCompileArgs>
			{
				void OnEvent(ScriptCompileArgs& args) override
				{

				}
			} onCompile;
			platform.utilities.RunEnvironmentScript(onCompile, 0, scriptName, true, false);
		}

		void CreateMHostDeclarations()
		{
			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			RunMHostEnvironmentScript(platform, this, "!scripts/MHost/_Init/create_declarations.sxy", true, false, *packageMHost, this, &sb->Builder());

			WideFilePath wPath;
			platform.installation.ConvertPingPathToSysPath("!scripts/MHost/declarations.sxy", wPath);

			try
			{
				Rococo::OS::SaveAsciiTextFile(Rococo::OS::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void CreateMPlatPlatformDeclarations()
		{
			NoEventArgs noEventArgs;

			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			platform.utilities.RunEnvironmentScript(noEventArgs, "!scripts/mplat/create_platform_declarations.sxy", true, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.installation.ConvertPingPathToSysPath("!scripts/mplat/platform_declarations.sxy", wPath);

			try
			{
				Rococo::OS::SaveAsciiTextFile(Rococo::OS::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void CreateSysDeclarations()
		{
			NoEventArgs noEventArgs;

			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			platform.utilities.RunEnvironmentScript(noEventArgs, "!scripts/native/create_declarations.sxy", false, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.installation.ConvertPingPathToSysPath("!scripts/native/declarations.sxy", wPath);

			try
			{
				Rococo::OS::SaveAsciiTextFile(Rococo::OS::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void Run() override
		{
			CreateMHostDeclarations();
			CreateMPlatPlatformDeclarations();
			CreateSysDeclarations();

			RunMHostEnvironmentScript(platform, this, "!scripts/MHost/_Init/keys.sxy", true, false, *packageMHost, this, nullptr);

			while (platform.appControl.IsRunning() && !isShutdown)
			{
				isScriptRunning = true;

				// mainScript variable can be changed by a script, so not safe to pass references to the
				// script and expect it to be unchanged throughout. So duplicate on the stack

				U8FilePath currentScript;
				Format(currentScript, "%s", mainScript.c_str());

				RunMHostEnvironmentScript(platform, this, currentScript, true, false, *packageMHost, this, nullptr);
				CleanupResources();
			}
		}

		Rococo::OS::ticks lastTick = 0;

		Seconds YieldForSystemMessages(int32 sleepMS) override
		{
			auto now = Rococo::OS::CpuTicks();
			auto DT = now - lastTick;

			float dt = (float)DT / (float)Rococo::OS::CpuHz();

			dt = clamp(dt, 0.0f, 0.1f);

			lastTick = now;

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

			platform.scene.AdvanceAnimations(Seconds{ dt });

			return Seconds{ dt };
		}

		void Render(MHost::GuiPopulator populator) override
		{
			if (!populator.byteCodeId)
			{
				Throw(0, "GuiPopulator undefined");
			}
			sceneManager.populator = populator;
			platform.renderer.Render(Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, sceneManager);
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
			MHost::UI::CaptureMouse(platform.mainWindow);
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

		struct arglist: IEventCallback<cstr>
		{
			std::vector<std::string> items;
			void OnEvent(cstr token) override
			{
				items.push_back(token);
			}
		} args;

		Rococo::Strings::SplitString(cmdLine, 0, " \t", args);

		AppArgs appArgs;
		appArgs.mainScript = "!scripts/mhost/mhost_startup.sxy";

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

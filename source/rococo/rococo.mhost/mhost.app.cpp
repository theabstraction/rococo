#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <rococo.textures.h>
#include <rococo.ui.h>
#include <rococo.sexy.api.h>
#include <rococo.package.h>
#include <rococo.gui.retained.h>

#include "..\rococo.mplat\mplat.editor.h"

#include <rococo.task.queue.h>
#include <rococo.time.h>
#include <mplat/mplat.events.h>

#include <vector>
#include <string>

using namespace Rococo;

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

	void AddMHostNativeCallSecurity(Rococo::ScriptCompileArgs& args);
	IScriptCompilationEventHandler& GetBaseCompileOptions();
	Rococo::Reflection::IReflectionTarget& GetTestTarget();
	void RegisterMHostPackage(ScriptCompileArgs& args, IPackage* package);

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
			return platform.graphics.scene.GetSkyboxCubeId();
		}

		void GetCamera(Matrix4x4& worldToScreen, Matrix4x4& worldToCamera, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{
			return platform.graphics.scene.GetCamera(worldToScreen, worldToCamera, proj, eye, viewDir);
		}

		RGBA GetClearColour() const override
		{
			return RGBA{ 0.0f, 0.0f, 0.0f, 1.0f };
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			platform.graphics.scene.OnGuiResize(screenSpan);
		}

		void OnCompile(IPublicScriptSystem& ss)
		{
			dispatcher->OnCompile(ss);
		}

		void RenderGui(IGuiRenderContext& grc)  override
		{
			IGui* gui = CreateGuiOnStack(guiBuffer, grc);
			dispatcher->RouteGuiToScript(ss, gui, populator);
			if (platform.graphics.GR.IsVisible())
			{
				platform.graphics.GR_Custodian.Render(grc, platform.graphics.GR);
			}
		}

		void RenderObjects(IRenderContext& rc, bool skinned)  override
		{
			platform.graphics.scene.RenderObjects(rc, skinned);
		}

		const Light* GetLights(uint32& nCount) const override
		{
			return platform.graphics.scene.GetLights(nCount);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned)  override
		{
			platform.graphics.scene.RenderShadowPass(drd, rc, skinned);
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

		void OnGuiResize(Vec2i) override
		{
		}

		void RenderGui(IGuiRenderContext&)  override
		{
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		void RenderObjects(IRenderContext&, bool)  override
		{
		}

		const Light* GetLights(uint32&) const override
		{
			return nullptr;
		}

		void RenderShadowPass(const DepthRenderData&, IRenderContext&, bool)  override
		{

		}
	};

	struct PanelCompileOptions : Rococo::IScriptCompilationEventHandler
	{
		IPackage* package = nullptr;

		void OnCompile(ScriptCompileArgs& args) override
		{
			RegisterMHostPackage(args, package);
			AddMHostNativeCallSecurity(args);
		}

		IScriptEnumerator* ImplicitIncludes() override
		{
			return Rococo::NoImplicitIncludes();
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
		PanelCompileOptions panelCompileOptions;

		AutoFree<IPaneBuilderSupervisor> overlayPanel;
		AutoFree<IPaneBuilderSupervisor> busyPanel;

		AppSceneManager sceneManager;

		HString mainScript;
		bool queuedForExecute = false;

		Vec2 cursorPosition;

		int32 overlayToggleKey = 0;
		int32 guiToggleKey = 0;

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
			} ec(platform.plumbing.publisher, be);

			platform.graphics.gui.PushTop(busyPanel->Supervisor(), true);

			EmptyScene emptyScene;
			platform.graphics.renderer.Render(Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, emptyScene);
			platform.graphics.gui.Pop();
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
					if (platform.graphics.gui.Top() != busyPanel->Supervisor())
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
			return platform.graphics.renderer.Gui().SpriteBuilder().TryGetBitmapLocation(resourceName, loc);
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
			busyPanel = platform.graphics.gui.BindPanelToScript("!scripts/panel.opening.sxy", nullptr, Rococo::NoImplicitIncludes());
			overlayPanel = platform.graphics.gui.CreateDebuggingOverlay();

			platform.plumbing.publisher.Subscribe(this, Rococo::Events::evBusy);

			this->mainScript = args.mainScript;

			WideFilePath sysPathMHost;
			platform.os.installation.ConvertPingPathToSysPath("!packages/mhost_1000.sxyz", sysPathMHost);
			this->packageMHost = OpenZipPackage(sysPathMHost, "mhost");
			platform.scripts.sourceCache.AddPackage(packageMHost);
			panelCompileOptions.package = packageMHost;

			platform.scripts.panelCompilationDesignator.Designate(&panelCompileOptions);
		}

		~App()
		{
			platform.plumbing.publisher.Unsubscribe(this);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(cstr) override
		{
			platform.os.ios.EnumerateModifiedFiles(*this);
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			U8FilePath pingPath;
			platform.os.installation.ConvertSysPathToPingPath(args.sysPath, pingPath);
			platform.graphics.gui.LogMessage("File modified: %s", pingPath);

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
				platform.graphics.gui.LogMessage("Updating script file");
				platform.plumbing.utilities.RefreshResource(pingPath);
				queuedForExecute = true;
			}
			else if (Eq(ext, ".ps"))
			{
				platform.graphics.gui.LogMessage("Updating pixel shader");
				platform.graphics.renderer.Shaders().UpdatePixelShader(pingPath);
			}
			else if (Eq(ext, ".vs"))
			{
				platform.graphics.gui.LogMessage("Updating vertex shader");
				platform.graphics.renderer.Shaders().UpdateVertexShader(pingPath);
			}
		}

		bool isScriptRunning = true; // Will be set to false in YieldForSystemMessages if script rerun required

		bool isShutdown = false;

		// used by a script in Run() via IEngine.Run to determine if it should terminate gracefully
		// termination should occur of the user interface has collapsed, or script queued for re-run
		boolean32 IsRunning() override 
		{
			return platform.os.appControl.IsRunning() && isScriptRunning && !isShutdown;
		}

		void CleanupResources()
		{
			ReleaseMouse();
		}

		struct NoEventArgs : IScriptCompilationEventHandler
		{
			void OnCompile(ScriptCompileArgs&) override
			{

			}
		};

		void RunMPlatScript(const fstring& scriptName) override
		{
			MHost::GetBaseCompileOptions();
			platform.plumbing.utilities.RunEnvironmentScriptWithId(NoImplicitIncludes(), GetBaseCompileOptions(), 0, scriptName, true, false);
		}

		void CreateMHostDeclarations()
		{
			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			RunMHostEnvironmentScript(platform, this, "!scripts/MHost/_Init/create_declarations.sxy", true, false, *packageMHost, this, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/MHost/declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFile(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void CreateMPlatPlatformDeclarations()
		{
			struct : IScriptCompilationEventHandler
			{
				void OnCompile(ScriptCompileArgs& args) override
				{
					AddMHostNativeCallSecurity(args);
				}

				IScriptEnumerator* ImplicitIncludes() override
				{
					return nullptr;
				}
			} onCompile;

			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			platform.plumbing.utilities.RunEnvironmentScript(NoImplicitIncludes(), onCompile, "!scripts/mplat/create_platform_declarations.sxy", true, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/mplat/platform_declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFile(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void CreateSysDeclarations()
		{
			struct : IScriptCompilationEventHandler
			{
				void OnCompile(ScriptCompileArgs& args) override
				{
					Rococo::Script::AddNativeCallSecurity_ToSysNatives(args.ss);
				}
				
				IScriptEnumerator* ImplicitIncludes() override
				{
					return nullptr;
				}
			} onSysScript;

			AutoFree<IDynamicStringBuilder> sb = CreateDynamicStringBuilder(4096);
			platform.plumbing.utilities.RunEnvironmentScript(Rococo::NoImplicitIncludes(), onSysScript, "!scripts/native/create_declarations.sxy", false, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/native/declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFile(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
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

			while (platform.os.appControl.IsRunning() && !isShutdown)
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

		Rococo::Time::ticks lastTick = 0;
		int64 frameIndex = 0;;

		Seconds YieldForSystemMessages(int32 sleepMS) override
		{
			auto now = Rococo::Time::TickCount();
			auto DT = now - lastTick;

			float dt = (float)DT / (float)Rococo::Time::TickHz();

			dt = clamp(dt, 0.0f, 0.1f);

			lastTick = now;

			platform.os.installation.OS().EnumerateModifiedFiles(*this);
			platform.plumbing.publisher.Deliver();

			if (!control.TryRouteSysMessages(sleepMS))
			{
				platform.os.appControl.ShutdownApp();
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

			platform.graphics.scene.AdvanceAnimations(Seconds{ dt });

			while (platform.os.appControl.MainThreadQueue().ExecuteNext());

			frameIndex++;

			enum { GC_FREQUENCY = 2 };

			if (0 == (frameIndex % GC_FREQUENCY))
			{
				platform.world.ECS.CollectGarbage();
			}

			return Seconds{ dt };
		}

		void Render(MHost::GuiPopulator populator) override
		{
			if (!populator.byteCodeId)
			{
				Throw(0, "GuiPopulator undefined");
			}
			sceneManager.populator = populator;
			platform.graphics.renderer.Render(Rococo::Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, sceneManager);
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
					platform.graphics.gui.AppendEvent(me);
				}

				return false;
			}

			if (platform.graphics.GR.IsVisible())
			{
				while (platform.graphics.GR.IsVisible() && control.TryGetNextMouseEvent(me))
				{
					platform.graphics.GR_Custodian.RouteMouseEvent(me, platform.graphics.GR);
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
			return platform.graphics.gui.Top() == overlayPanel->Supervisor();
		}

		void ToggleOverlay()
		{
			if (!IsOverlayActive())
			{
				platform.graphics.gui.PushTop(overlayPanel->Supervisor(), true);
			}
			else
			{
				platform.graphics.gui.Pop();
			}
		}

		void SetEditorVisibility(boolean32 isVisible) override
		{
			using namespace Rococo::Gui;

			platform.creator.editor.SetVisibility(isVisible);

			if (isVisible)
			{
				platform.creator.editor.Preview(platform.graphics.GR, GetTestTarget());
			}
		}

		void SetGUIToggleKey(int32 vkeyCode) override
		{
			guiToggleKey = vkeyCode;
		}

		void SetOverlayToggleKey(int32 vkeyCode) override
		{
			overlayToggleKey = vkeyCode;
		}

		boolean32 GetNextKeyboardEvent(MHostKeyboardEvent& k) override
		{
			if (platform.graphics.GR.IsVisible())
			{
				KeyboardEvent keyEv;
				while (platform.graphics.GR.IsVisible() && control.TryGetNextKeyboardEvent(keyEv))
				{
					if (guiToggleKey == keyEv.VKey && !keyEv.IsUp())
					{
						SetEditorVisibility(false);
						break;
					}
					else
					{
						platform.graphics.GR_Custodian.RouteKeyboardEvent(keyEv, platform.graphics.GR);
					}
				}
				return false;
			}

			KeyboardEvent key;
			if (control.TryGetNextKeyboardEvent(key))
			{
				if (IsOverlayActive())
				{
					platform.graphics.gui.AppendEvent(key);

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
				else if (overlayToggleKey && key.VKey == (uint16) overlayToggleKey)
				{
					if (!key.IsUp())
					{
						ToggleOverlay();
					}
					k = { 0 };
					return 0;
				}
				else if (guiToggleKey && key.VKey == (uint16)guiToggleKey)
				{
					if (!key.IsUp())
					{
						SetEditorVisibility(!platform.graphics.GR.IsVisible());
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
			MHost::UI::CaptureMouse(platform.os.mainWindow);
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
		p.os.installation.Macro("#bitmaps", "!scripts/mhost/bitmaps/");

		struct arglist: IEventCallback<cstr>
		{
			std::vector<std::string> items;
			void OnEvent(cstr token) override
			{
				items.push_back(token);
			}
		} args;

		Rococo::Strings::SplitString(cmdLine, 0, args);

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
				break;
			}
		}

		return new MHost::App(p, control, appArgs);
	}
}

#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <rococo.textures.h>
#include <rococo.ui.h>
#include <rococo.sexy.api.h>
#include <rococo.package.h>
#include <rococo.gui.retained.ex.h>
#include <rococo.os.h>
#include <rococo.great.sex.h>
#include <rococo.reflector.h>

#include "..\rococo.mplat\mplat.editor.h"

#include <rococo.task.queue.h>
#include <rococo.time.h>
#include <mplat/mplat.events.h>

#include <vector>
#include <string>
#include <list>

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Events;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Textures;
using namespace Rococo::Script;
using namespace MHost::OS;
using namespace Rococo::Sex;
using namespace Rococo::GreatSex;

struct AppArgs
{
	cstr mainScript;
};


Gui::GRIdWidget ID_EDITOR_FRAME = { "MPlat-MainFrame" };

using namespace Rococo::Strings;

namespace MHost
{
	void AddMHostNativeCallSecurity(Rococo::ScriptCompileArgs& args);
	IScriptCompilationEventHandler& GetBaseCompileOptions();
	Rococo::Reflection::IReflectionTarget& GetTestTarget();
	void RegisterMHostPackage(ScriptCompileArgs& args, IPackage* package);

	auto evPopulateBusyCategoryId = "busy.category"_event;
	auto evPopulateBusyResourceId = "busy.resource"_event;

	void RunMHostEnvironmentScript(Platform& platform, IEngineSupervisor* engine, IGuiOverlaySupervisor* guiOverlay, cstr name, bool releaseAfterUse, bool trace, IPackage& package, Strings::IStringPopulator* onScriptCrash, StringBuilder* declarationBuilder);

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

		ID_CUBE_TEXTURE GetEnvironmentMap() const override
		{
			return platform.graphics.scene.GetEnvironmentMap();
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

			platform.graphics.gui.Render(grc);
		}

		void RenderObjects(IRenderContext& rc, EShadowCasterFilter filter)  override
		{
			platform.graphics.scene.RenderObjects(rc, filter);
		}

		Lights GetLights() const override
		{
			return platform.graphics.scene.GetLights();
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, EShadowCasterFilter filter)  override
		{
			platform.graphics.scene.RenderShadowPass(drd, rc, filter);
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

		ID_CUBE_TEXTURE GetEnvironmentMap() const override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return ID_CUBE_TEXTURE::Invalid();
		}

		void RenderObjects(IRenderContext&, EShadowCasterFilter)  override
		{
		}

		Lights GetLights() const override
		{
			return Lights{ nullptr, 0 };
		}

		void RenderShadowPass(const DepthRenderData&, IRenderContext&, EShadowCasterFilter)  override
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
		public IGuiOverlaySupervisor,
		public Strings::IStringPopulator,
		public Rococo::MPEditor::IMPEditorEventHandler,
		IO::IShaderMonitorEventHook,
		public IGreatSexResourceLoader
	{
		Platform& platform;
		IDirectAppControl& control;
		PanelCompileOptions panelCompileOptions;

		AutoFree<IPaneBuilderSupervisor> busyPanel;

		AppSceneManager sceneManager;

		HString mainScript;
		bool queuedForExecute = false;

		Vec2 cursorPosition{ 0,0 };

		int32 guiToggleKey = 0;

		AutoFree<IPackageSupervisor> packageMHost;

		// IO::IShaderMonitorEventHook
		void OnLog(IO::IShaderMonitor& monitor, IO::EShaderLogPriority priority, cstr file, cstr message) override
		{
			UNUSED(monitor);

			switch (priority)
			{
			case IO::EShaderLogPriority::Compiled:
				platform.graphics.gui.ClearFileError(file);
				{
					char mhostInfo[1024];
					if (*file) SafeFormat(mhostInfo, "%s: %s", file, message);
					else SafeFormat(mhostInfo, "%s", file, message);
					LogMessageToMHostScript(mhostInfo);
				}
				break;
			case IO::EShaderLogPriority::Info:
				break;
			case IO::EShaderLogPriority::ErrorCode:
			case IO::EShaderLogPriority::Error:
				platform.graphics.gui.ShowFileError(file, message);
				break;
			case IO::EShaderLogPriority::Cosmetic:
				return;
			default:
				if (*file && *message) platform.graphics.gui.LogMessage("%s: %s", file, message);
				else if (*message) platform.graphics.gui.LogMessage("%s", message);
				break;
			}
		}

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
							CopyString(te.text, sizeof(te.text), be.message);
						}
					}
					else if (ev == evPopulateBusyResourceId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							CopyString(te.text, sizeof(te.text), be.pingPath.buf);
						}
					}
				}
			} ec(platform.plumbing.publisher, be);

			platform.graphics.gui.PushTop(busyPanel->Supervisor(), true);

			EmptyScene emptyScene;
			platform.graphics.renderer.RenderToBackBufferAndPresent(emptyScene);
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

		boolean32 TryGetSpriteSpec(const fstring& resourceName, OUT BitmapLocation& loc) override
		{
			return platform.graphics.renderer.GuiResources().SpriteBuilder().TryGetBitmapLocation(resourceName, OUT loc);
		}

		void GetSpriteSpec(const fstring& resourceName, OUT BitmapLocation& loc) override
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

			platform.plumbing.publisher.Subscribe(this, Rococo::Events::evBusy);

			this->mainScript = args.mainScript;

			WideFilePath sysPathMHost;
			platform.os.installation.ConvertPingPathToSysPath("!packages/mhost_1000.sxyz", sysPathMHost);
			this->packageMHost = OpenZipPackage(sysPathMHost, "mhost");
			platform.scripts.sourceCache.AddPackage(packageMHost);
			panelCompileOptions.package = packageMHost;

			platform.scripts.panelCompilationDesignator.Designate(&panelCompileOptions);;
		}

		~App()
		{
			platform.plumbing.publisher.Unsubscribe(this);
			platform.graphics.shaderMonitorEventsProxy.RemoveHook(this);
		}

		void Free() override
		{
			delete this;
		}

		void LoadGreatSexResource(cstr resourcePath, Rococo::IO::ILoadEventsCallback& onLoad) override
		{
			platform.os.installation.LoadResource(resourcePath, onLoad);
		}

		void Populate(cstr) override
		{
			platform.os.ios.EnumerateModifiedFiles(*this);
		}

		void LoadFrame(cstr sexmlPath, Gui::IGRWidgetMainFrame& frame, IEventCallback<IGreatSexGenerator>& onConstruct)
		{
			struct ErrorHandler : IEventCallback<LoadFrameException>
			{
				void OnEvent(LoadFrameException& ex) override
				{
					Windows::ShowErrorBox(p->os.mainWindow, ex.errorCode, ex.message, ex.filename);
				}

				Platform* p = nullptr;
			} onError;

			onError.p = &platform;

			GreatSex::LoadFrame(platform.os.installation, Memory::CheckedAllocator(), sexmlPath, frame, onConstruct, onError);
		}

		void SetGreatSexOverlay(const fstring& pingPath) override
		{
			SetEditorVisibility(true);
			auto* frame = platform.graphics.GR.Root().GR().FindFrame(ID_EDITOR_FRAME);
			if (frame)
			{
				frame->MenuBar().Panel().Parent()->SetCollapsed(true);

				auto& scheme = frame->Widget().Panel().Root().Scheme();
				Gui::SetSchemeColours_ThemeGrey(scheme);
				Gui::SetPropertyEditorColours_PastelScheme(frame->Panel());

				struct : IEventCallback<GreatSex::IGreatSexGenerator>
				{
					void OnEvent(GreatSex::IGreatSexGenerator& generator) override
					{
						GreatSex::AddTestOptions(generator);
					}
				} onConstruct;

				LoadFrame(pingPath, *frame, onConstruct);
			}
		}

		std::vector<HString> messages;

		void LogMessageToMHostScript(cstr message)
		{
			messages.push_back(message);

			GuiTypes::GuiEvent ev;
			ev.stringId = messages.back();
			ev.eventId = GuiTypes::GuiEventId::LogMessage;
			guiEventList.push_back(ev);
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			U8FilePath pingPath;
			platform.os.installation.ConvertSysPathToPingPath(args.sysPath, pingPath);

			char message[256];
			SafeFormat(message, "File modified: %s", pingPath);

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
				LogMessageToMHostScript(message);
				platform.plumbing.utilities.RefreshResource(pingPath);

				if (!EndsWith(pingPath, "declarations.sxy"))
				{
					queuedForExecute = true;
				}
			}
			else if (Eq(ext, ".ps"))
			{
				LogMessageToMHostScript(message);

				try
				{
					platform.graphics.renderer.Shaders().UpdatePixelShader(pingPath);
				}
				catch (IException& ex)
				{
					LogMessageToMHostScript(ex.Message());
				}
			}
			else if (Eq(ext, ".vs"))
			{
				LogMessageToMHostScript(message);

				try
				{
					platform.graphics.renderer.Shaders().UpdateVertexShader(pingPath);
				}
				catch (IException& ex)
				{
					LogMessageToMHostScript(ex.Message());
				}
			}
		}

		bool isScriptRunning = true; // Will be set to false in YieldForSystemMessages if script rerun required

		bool isShutdown = false;

		// used by a script in Run() via IEngine.Run to determine if it should terminate gracefully
		// termination should occur of the user interface has collapsed, or script queued for re-run
		boolean32 IsRunning() const override 
		{
			bool isRunning = platform.os.appControl.IsRunning() && isScriptRunning && !isShutdown;
			return isRunning;
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
			sb->Builder().AppendFormat("(SexyDeclarations) // Generated by %s\n", __ROCOCO_FUNCTION__);
			RunMHostEnvironmentScript(platform, this, this, "!scripts/MHost/_Init/create_declarations.sxy", true, false, *packageMHost, this, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/declarations/MHost/declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFileIfDifferent(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
			}
			catch (...)
			{

			}
		}

		void AddMenu(const fstring& id, int64 metaId, const fstring& menuPath) override
		{
			auto idCurrentMenu = Rococo::Gui::GRMenuItemId::Root();
			auto* frame = platform.graphics.GR.Root().GR().FindFrame(ID_EDITOR_FRAME);
			if (frame)
			{
				Substring s = Substring::ToSubstring(menuPath);

				for (;;)
				{
					cstr nextDot = FindChar(s.start, '.');
					if (nextDot)
					{
						Substring subspace = { s.start, nextDot };
						char subspaceBuffer[16];
						if (!subspace.TryCopyWithoutTruncate(subspaceBuffer, sizeof(subspaceBuffer)))
						{
							Throw(0, "%s: Failed to convert subpsace to string for %s", __ROCOCO_FUNCTION__, (cstr)menuPath);
						}

						idCurrentMenu = frame->MenuBar().AddSubMenu(idCurrentMenu, Gui::GRMenuSubMenu(subspaceBuffer));
						s.start = nextDot + 1;
					}
					else
					{
						Gui::GRMenuButtonItem button;
						button.isEnabled = 1;
						button.metaData.intData = metaId;
						button.metaData.stringData = id;
						button.text = s.start;
						frame->MenuBar().AddButton(idCurrentMenu, button);
						break;
					}
				}
			}
		}

		enum class SUBSYSTEM_BITS: uint64
		{
			HIGHBITS = 0x8400'0000'0000'0000UL
		};

		ID_SUBSYSTEM MenuIdToSubsystemId(int64 id)
		{
			if ((id & (int64)SUBSYSTEM_BITS::HIGHBITS) == (int64)SUBSYSTEM_BITS::HIGHBITS)
			{
				return ID_SUBSYSTEM(id & 0xFFFFFFFF);
			}

			return ID_SUBSYSTEM::Invalid();
		}

		void AppendToMenuRecursive(Rococo::Gui::IGRWidgetMainFrame& frame, Rococo::Gui::GRMenuItemId parentMenu, ISubsystem& subsystem, ID_SUBSYSTEM id)
		{
			int childCount = 0;
			platform.misc.subSystems.ForEachChild(subsystem, [&childCount](ISubsystem&, ID_SUBSYSTEM)
				{
					childCount++;
				}
			);

			if (childCount)
			{
				platform.misc.subSystems.ForEachChild(subsystem, [this, &frame, parentMenu](ISubsystem& subsystem, ID_SUBSYSTEM id)
					{
						AppendToMenuRecursive(frame, parentMenu, subsystem, id);
					}
				);
			}
			else
			{
				char text[256];
				SafeFormat(text, "Profile %s...", subsystem.SubsystemName());

				Gui::GRMenuButtonItem button;
				button.isEnabled = 1;
				button.isImplementedInCPP = 1;
				button.metaData.intData = (int64)(SUBSYSTEM_BITS::HIGHBITS) | (int64)id.value;
				button.metaData.stringData = subsystem.SubsystemName();
				button.text = text;

				frame.MenuBar().AddButton(parentMenu, button);
			}
		}

		void AddProfileMenu()
		{
			auto idRootMenu = Rococo::Gui::GRMenuItemId::Root();
			auto* frame = platform.graphics.GR.Root().GR().FindFrame(ID_EDITOR_FRAME);
			if (frame)
			{
				auto idProfileMenu = frame->MenuBar().AddSubMenu(idRootMenu, Gui::GRMenuSubMenu("Profile"));

				platform.misc.subSystems.ForEachRoot([this, frame, idProfileMenu](ISubsystem& subsystem, ID_SUBSYSTEM id)
					{
						AppendToMenuRecursive(*frame, idProfileMenu, subsystem, id);
					}
				);

				LoadTestPage(*frame);
			}
		}

		void LoadTestPage(Gui::IGRWidgetMainFrame& frame)
		{
			struct : IEventCallback<IGreatSexGenerator>
			{
				void OnEvent(IGreatSexGenerator& generator)
				{
					GreatSex::AddTestOptions(generator);
				}
			} onConstruct;
			LoadFrame("!tests/greatsex.test.sexml", frame, onConstruct);
		}

		void ClearMenus() override
		{
			auto* frame = platform.graphics.GR.Root().GR().FindFrame(ID_EDITOR_FRAME);
			if (frame)
			{
				frame->MenuBar().ClearMenus();
			}

			platform.graphics.GR.GarbageCollect();
		}

		void AppendEventString(IStringPopulator& sb, OUT MHost::GuiTypes::GuiEvent& ev) override
		{
			if (ev.stringId) sb.Populate(ev.stringId);
		}

		std::list<GuiTypes::GuiEvent> guiEventList;

		boolean32 GetNextGuiEvent(OUT MHost::GuiTypes::GuiEvent& emittedEvent) override
		{
			if (!guiEventList.empty())
			{
				emittedEvent = guiEventList.front();
				guiEventList.pop_front();
				return true;
			}
			else
			{
				// The script has consumed the string pointers, so safe to delete the backing
				messages.clear();
			}

			return false;
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
			sb->Builder().AppendFormat("(SexyDeclarations) // Generated by %s\n", __ROCOCO_FUNCTION__);
			platform.plumbing.utilities.RunEnvironmentScript(NoImplicitIncludes(), onCompile, "!scripts/declarations/rococo/create_platform_declarations.sxy", true, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/declarations/rococo/platform_declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFileIfDifferent(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
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
			sb->Builder().AppendFormat("(SexyDeclarations) // Generated by %s\n", __ROCOCO_FUNCTION__);
			platform.plumbing.utilities.RunEnvironmentScript(Rococo::NoImplicitIncludes(), onSysScript, "!scripts/declarations/sys/create_declarations.sxy", false, false, false, nullptr, &sb->Builder());

			WideFilePath wPath;
			platform.os.installation.ConvertPingPathToSysPath("!scripts/declarations/sys/declarations.sxy", wPath);

			try
			{
				Rococo::IO::SaveAsciiTextFileIfDifferent(Rococo::IO::TargetDirectory_Root, wPath, *sb->Builder());
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

			RunMHostEnvironmentScript(platform, this, this, "!scripts/MHost/_Init/keys.sxy", true, false, *packageMHost, this, nullptr);

			while (platform.os.appControl.IsRunning() && !isShutdown)
			{
				isScriptRunning = true;

				// mainScript variable can be changed by a script, so not safe to pass references to the
				// script and expect it to be unchanged throughout. So duplicate on the stack
				// note that a change in an sxy file can result in premature script termination and a rerun of the existing script.
				// see: void OnEvent(FileModifiedArgs & args)

				U8FilePath currentScript;
				Format(currentScript, "%s", mainScript.c_str());

				RunMHostEnvironmentScript(platform, this, this, currentScript, true, false, *packageMHost, this, nullptr);
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

			platform.graphics.GR.DispatchMessages();

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

			Rococo::OS::ThrowOnThreadError();

			return Seconds{ dt };
		}

		void Render(MHost::GuiPopulator populator) override
		{
			if (!populator.byteCodeId)
			{
				Throw(0, "GuiPopulator undefined");
			}
			sceneManager.populator = populator;
			platform.graphics.renderer.RenderToBackBufferAndPresent(sceneManager);
		}

		void PollKeyState(OUT KeyState& keyState) override
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
			if (platform.graphics.GR.IsVisible())
			{
				while (platform.graphics.GR.IsVisible() && control.TryGetNextMouseEvent(me))
				{
					static_assert(sizeof(Gui::GRKeyContextFlags) == sizeof(MouseContext));
					auto& context = reinterpret_cast<Gui::GRKeyContextFlags&>(me.contextFlags);
					platform.graphics.GR_Custodian.RouteMouseEvent(me, context, platform.graphics.GR);
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

		void OnMPEditor_ButtonClick(Gui::GRWidgetEvent& buttonEvent) override
		{
			if (buttonEvent.isCppOnly)
			{
				ID_SUBSYSTEM id = MenuIdToSubsystemId(buttonEvent.iMetaData);
				ISubsystem* subsystem = platform.misc.subSystems.Find(id);
				if (subsystem)
				{
					auto* target = subsystem->ReflectionTarget();
					if (target)
					{
						auto* frame = platform.graphics.GR.Root().GR().FindFrame(ID_EDITOR_FRAME);
						Gui::SetUniformColourForAllRenderStates(frame->Widget().Panel(), Gui::EGRSchemeColourSurface::BACKGROUND, RGBAb(0, 0, 0, 192));
						Gui::SetUniformColourForAllRenderStates(frame->ClientArea().Panel(), Gui::EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 0));

						platform.creator.editor.Preview(platform.graphics.GR, target->Visitation());
					}
				}
			}
			else
			{
				GuiTypes::GuiEvent ev;
				ev.eventId = GuiTypes::GuiEventId::OverlayButtonClick;
				ev.buttonPos = buttonEvent.clickPosition;
				ev.metaId = buttonEvent.panelId;
				ev.stringId = buttonEvent.sMetaData;

				guiEventList.push_back(ev);
			}
		}

		void SetEditorVisibility(boolean32 isVisible) override
		{
			platform.creator.editor.SetVisibility(isVisible);

			if (isVisible)
			{
				platform.creator.editor.AddHook(this);

				GuiTypes::GuiEvent ev;
				ev.stringId = "";
				ev.eventId = GuiTypes::GuiEventId::GRisMadeVisible;
				guiEventList.push_back(ev);
			}
			else
			{
				platform.creator.editor.RemoveHook(this);
				GuiTypes::GuiEvent ev;
				ev.stringId = "";
				ev.eventId = GuiTypes::GuiEventId::GRisHidden;
				guiEventList.push_back(ev);
			}
		}

		void SetGUIToggleKey(int32 vkeyCode) override
		{
			guiToggleKey = vkeyCode;
		}

		boolean32 IsAppModal() const override
		{
			return platform.graphics.GR.IsVisible();
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
				if (guiToggleKey && key.VKey == (uint16)guiToggleKey)
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

		void PostCreate()
		{
			platform.graphics.shaderMonitorEventsProxy.AddHook(this);
			platform.graphics.GR.SetFocusOverlayRenderer(&Gui::GetDefaultFocusRenderer());
		}

		void AddSexmlSheet(Rococo::Script::ISxyExpressionRef sexml) override
		{
			cr_sex s = Rococo::Compiler::GetExpression(sexml);

			SetEditorVisibility(true);

			auto* frame = platform.graphics.GR.FindFrame(ID_EDITOR_FRAME);
			if (!frame)
			{
				Throw(0, "Could not find %s inside the platform.graphics.GR object", ID_EDITOR_FRAME);
			}

			struct Cleaner : IEventCallback<Rococo::Gui::IGRPanel>
			{
				void OnEvent(Gui::IGRPanel& panel) override
				{
					panel.MarkForDelete();
				}
			} eraseChildren;

			frame->ClientArea().Panel().EnumerateChildren(&eraseChildren);

			platform.graphics.GR.GarbageCollect();

			AutoFree<IGreatSexGeneratorSupervisor> greatSex = CreateGreatSexGenerator(Memory::CheckedAllocator(), *this);
			greatSex->AppendWidgetTreeFromSexML(s, frame->ClientArea().Widget());
		}
	};
}

namespace MHost
{
	IDirectApp* CreateApp(Platform& p, IDirectAppControl& control, cstr cmdLine)
	{
		p.os.installation.Macro("#bitmaps", "!scripts/mhost/bitmaps/");

		struct arglist: Strings::IStringPopulator
		{
			std::vector<std::string> items;
			void Populate(cstr token) override
			{
				items.push_back(token);
			}
		} args;
		
		Rococo::Strings::SplitString(cmdLine, 0, args, " ");

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

		for (size_t i = 1; i < args.items.size(); ++i)
		{
			cstr arg = args.items[i].c_str();
			if (arg[0] == '-')
			{
				// Opt
				if (EqI("-debug", arg))
				{
					Windows::ShowMessageBox(p.graphics.renderer.CurrentWindow(), "-debug was specified on the command line. Attach your debugger now", "rococo.mhost.exe Startup Prompt", (uint32)Windows::SHOW_WINDOW_TYPE::INFO);
					Rococo::OS::TripDebugger();
				}
			}
		}

		auto* app = new MHost::App(p, control, appArgs);
		app->PostCreate();
		return app;
	}
}

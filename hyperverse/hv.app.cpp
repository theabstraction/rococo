#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <rococo.file.browser.h>
#include <rococo.io.h>
#include <rococo.sexy.api.h>
#include <rococo.random.h>
#include "hv.events.h"
#include "hv.defaults.h"
#include <vector>

namespace HV
{
	using namespace Rococo::IO;
	using namespace Rococo::Entities;
	using namespace Rococo::Strings;

	namespace Events
	{
		const EventIdRef  evPopulateBusyCategoryId = "busy.category"_event;
		const EventIdRef  evPopulateBusyResourceId = "busy.resource"_event;
		const EventIdRef evEnableEditor = "editor.enable"_event;
	}

	struct IGuiResizeEvent
	{
		virtual void OnGuiResize(Vec2i span) = 0;
	};

	struct NoExtraNativeLibs : IEventCallback<ScriptCompileArgs>
	{
		void OnEvent(ScriptCompileArgs& args)
		{

		}
	};

	struct FPSExtraNativeLibs : IEventCallback<ScriptCompileArgs>
	{
		Cosmos& cosmos;
		FPSExtraNativeLibs(Cosmos& refCosmos) : cosmos(refCosmos) {}

		void OnEvent(ScriptCompileArgs& args)
		{
			AddNativeCalls_HVIPlayer(args.ss, &cosmos.players);
		}
	};

	// Route scene methods to the platform.graphics.scene object
	// We intercept the gui events to resize application panels
	class AppScene: public IScene
	{
		Cosmos e;

		RGBA GetClearColour() const override
		{
			if (e.sectors.begin() == e.sectors.end())
			{
				return RGBA(0, 0.25f, 0);
			}
			else
			{
				return e.platform.graphics.scene.GetClearColour();
			}
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return e.platform.graphics.scene.GetSkyboxCubeId();
		}

		const Light* GetLights(uint32& nLights) const override
		{
			return e.platform.graphics.scene.GetLights(nLights);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned) override
		{
			return e.platform.graphics.scene.RenderShadowPass(drd, rc, skinned);
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			resizeCallback->OnGuiResize(screenSpan);
		}

		void RenderGui(IGuiRenderContext& g) override
		{
			GuiMetrics metrics;
			g.Renderer().GetGuiMetrics(metrics);
			if (metrics.screenSpan.y >= 256 && metrics.screenSpan.x >= 256)
			{
				e.platform.graphics.gui.Render(g);
			}
		}

		void GetCamera(Matrix4x4& wordlToScreen, Matrix4x4& world, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{
			e.platform.graphics.scene.GetCamera(wordlToScreen, world, proj, eye, viewDir);
		}

		void RenderObjects(IRenderContext& rc, bool skinned)  override
		{
			e.platform.graphics.scene.RenderObjects(rc, skinned);
		}
	public:
		IGuiResizeEvent* resizeCallback = nullptr;

		AppScene(Cosmos& _e) :
			e(_e)
		{}
	};

	struct : IScriptEnumerator
	{
		size_t Count() const override
		{
			return 0;
		}

		cstr ResourceName(size_t index) const override
		{
			return nullptr;
		}
	} useMplatDefaultImplictIncludes;

	class App : public IApp, public IEventCallback<FileModifiedArgs>,  public IObserver, public IGuiResizeEvent
	{
		// N.B make sure fields are in the correct order of construction

		Platform& platform;
		AutoFree<IObjectManager> object_manager;
		AutoFree<ISectors> sectors;
		AutoFree<IPlayerSupervisor> players;	
		AutoFree<IFPSGameModeSupervisor> fpsLogic;
		AutoFree<IEditor> editor;
		AutoFree<IPaneBuilderSupervisor> editorPanel;
		AutoFree<IPaneBuilderSupervisor> fpsPanel;
		AutoFree<IPaneBuilderSupervisor> overlayPanel;
		AutoFree<IPaneBuilderSupervisor> busyPanel;
		AutoFree<IPaneBuilderSupervisor> colourPanel;

		AutoFree<IAIBrain> brain;

		Cosmos e; // Put this as the last member, since other members need to be constructed first

		IGameMode* mode;
		HString nextLevelName;

		AppScene scene;

		FPSExtraNativeLibs fpsExtraLibs;

		// Busy event handler responds to resource loading and renders progress panel
		void OnBusy(const Rococo::Events::BusyEvent& be)
		{
			struct BusyEventCapture : public IObserver
			{
				IPublisher& publisher;
				const Rococo::Events::BusyEvent& be;
				BusyEventCapture(IPublisher& _publisher, const Rococo::Events::BusyEvent& _be) : publisher(_publisher), be(_be)
				{
					publisher.Subscribe(this, Events::evPopulateBusyCategoryId);
					publisher.Subscribe(this, Events::evPopulateBusyResourceId);
				}

				~BusyEventCapture()
				{
					publisher.Unsubscribe(this);
				}

				virtual void OnEvent(Event& ev)
				{
					if (ev == Events::evPopulateBusyCategoryId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, sizeof(te.text), "%s", be.message);
						}
					}
					else if (ev == Events::evPopulateBusyResourceId)
					{
						auto& te = As<TextOutputEvent>(ev);
						if (te.isGetting)
						{
							SafeFormat(te.text, sizeof(te.text), "%s", be.pingPath.buf);
						}
					}
				}
			} ec(e.platform.plumbing.publisher, be);

			e.platform.graphics.gui.PushTop(busyPanel->Supervisor(), true);

			platform.graphics.renderer.Render(Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, (IScene&) scene);
			e.platform.graphics.gui.Pop();
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
		}

		void OnEvent(Event& ev) override
		{
			if (ev == HV::Events::evSetNextLevel)
			{
				auto& nl = As<HV::Events::SetNextLevelEvent>(ev);
				nextLevelName = nl.name;
			}
			else if (ev == Rococo::Events::evBusy)
			{
				auto& be = As <Rococo::Events::BusyEvent>(ev);
				if (be.isNowBusy)
				{
					if (e.platform.graphics.gui.Top() != busyPanel->Supervisor())
					{
						OnBusy(be);
					}
				}
			}
			else if (ev == Events::evEnableEditor)
			{
				auto isEnabled = As<TEventArgs<bool>>(ev);
				if (isEnabled)
				{
					if (!IsEditorActive())
					{
						e.platform.graphics.gui.PushTop(editorPanel->Supervisor(), true);
						mode->Deactivate();
					}
				}
				else
				{
					if (IsEditorActive())
					{
						e.platform.graphics.gui.Pop();
						mode->Activate();
					}
				}
			}
			else if (ev == Events::evPopulateBusyCategoryId)
			{
			}

			else if (ev == Events::evPopulateBusyResourceId)
			{
			}
		}
	public:
		App(Platform& _platform) :
			platform(_platform),
			sectors(CreateSectors(_platform)),
			players(CreatePlayerSupervisor(platform)),
			object_manager(CreateObjectManager(platform.graphics.renderer)),
			fpsLogic(CreateFPSGameLogic(platform, *players, *sectors, *object_manager)),
			editor(CreateEditor(platform, *players, *sectors, *fpsLogic)),
			e{ _platform, *players, *editor, *sectors, *fpsLogic, *object_manager },
			scene(e),
			fpsExtraLibs(e)
		{
			mode = fpsLogic;

			Defaults::SetDefaults(e.platform.data.config);

			RunEnvironmentScript(e, "!scripts/hv/config.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/keys.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/controls.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/equipment.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/main.sxy", true);

			NoExtraNativeLibs noExtraLibs;
			e.platform.plumbing.utilities.RunEnvironmentScript(useMplatDefaultImplictIncludes, noExtraLibs, "!scripts/samplers.sxy", true);

			editorPanel = e.platform.graphics.gui.BindPanelToScript("!scripts/panel.editor.sxy");
			fpsPanel = e.platform.graphics.gui.BindPanelToScript("!scripts/panel.fps.sxy", &fpsExtraLibs);
			busyPanel = e.platform.graphics.gui.BindPanelToScript("!scripts/panel.opening.sxy");
			colourPanel = e.platform.graphics.gui.BindPanelToScript("!scripts/panel.colour.sxy");
			overlayPanel = e.platform.graphics.gui.CreateDebuggingOverlay();

			e.platform.graphics.gui.PushTop(fpsPanel->Supervisor(), true);

			e.platform.plumbing.publisher.Subscribe(this, HV::Events::evSetNextLevel);
			e.platform.plumbing.publisher.Subscribe(this, Rococo::Events::evBusy);
			e.platform.plumbing.publisher.Subscribe(this, Events::evEnableEditor);

			scene.resizeCallback = this;

			brain = CreateAIBrain(platform.plumbing.publisher, e.sectors);

			int i = 0;

			auto id = object_manager->CreateObject("weapon.sword.falchion");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64) id.value);

			id = object_manager->CreateObject("weapon.sword.rapier");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.long.1");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.long.2");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.broad.1");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.broad.2");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.axe.battle");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.hammer.mace");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.katana");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);	

			id = object_manager->CreateObject("weapon.xbow.hand");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.xbow.modern");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

		//	id = object_manager->CreateObject("weapon.sword.delta");
		//	e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.short");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.sabre.1");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.sabre.2");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.sword.bastard");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);

			id = object_manager->CreateObject("weapon.hammer.claw");
			e.players.GetPlayer(0)->GetInventory()->SetId(i++, (uint64)id.value);
		}

		~App()
		{
			e.platform.plumbing.publisher.Unsubscribe(this);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			HV::Events::OS::FileChangedEvent ev;
			ev.args = &args;
			e.platform.plumbing.publisher.Publish(ev, HV::Events::OS::evFileChanged);

			U8FilePath pingname;
			e.platform.os.installation.ConvertSysPathToPingPath(args.sysPath, pingname);

			platform.graphics.gui.LogMessage("File modified: %s", pingname.buf);

			auto ext = Rococo::Strings::GetFileExtension(pingname);
			if (!ext)
			{

			}
			else if (Eq(ext, ".sxy"))
			{
				e.platform.plumbing.utilities.RefreshResource(pingname);

				if (args.Matches("!scripts/hv/main.sxy"))
				{
					platform.graphics.gui.LogMessage("Running !scripts/hv/main.sxy");
					HV::RunEnvironmentScript(e, "!scripts/hv/main.sxy", true);
				}

				if (StartsWith(pingname, "!scripts/hv/sector/"))
				{
					platform.graphics.gui.LogMessage("Running sector script");
					sectors->OnSectorScriptChanged(args);
				}
			}
			else if (Eq(ext, ".ps"))
			{
				platform.graphics.gui.LogMessage("Updating pixel shader");
				e.platform.graphics.renderer.Shaders().UpdatePixelShader(pingname);
			}
			else if (Eq(ext, ".vs"))
			{
				platform.graphics.gui.LogMessage("Updating vertex shader");
				e.platform.graphics.renderer.Shaders().UpdateVertexShader(pingname);
			}
		}

		uint32 OnFrameUpdated(const IUltraClock& clock) override
		{
			GuiMetrics metrics;
			e.platform.graphics.renderer.GetGuiMetrics(metrics);

			e.platform.os.installation.OS().EnumerateModifiedFiles(*this);
			e.platform.plumbing.publisher.Deliver();

			if (nextLevelName.length() > 0)
			{
				e.fpsMode.ClearCache();
				RebaseSectors();
				RunEnvironmentScript(e, nextLevelName.c_str());
				e.platform.scripts.sourceCache.Release(nextLevelName.c_str());
				nextLevelName = "";
			}

			mode->UpdateAI(clock);

			e.platform.graphics.camera.Update(clock);

			e.platform.graphics.renderer.Render(Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE, (IScene&) scene);

			e.platform.world.ECS.CollectGarbage();

			if (IsEditorActive() || IsOverlayActive())
			{
				return 100;
			}
			else if (e.sectors.begin() == e.sectors.end())
			{
				return 100;
			}
			else
			{
				AdvanceInTime(*sectors, clock);
				return 5;
			}
		}

		bool IsOverlayActive()
		{
			return e.platform.graphics.gui.Top() == overlayPanel->Supervisor();
		}

		bool IsEditorActive()
		{
			return e.platform.graphics.gui.Top() == editorPanel->Supervisor();
		}

		void OnKeyboardEvent(const KeyboardEvent& keyboardEvent) override
		{
			if (!e.platform.graphics.gui.AppendEvent(keyboardEvent))
			{
				Key key = e.platform.hardware.keyboard.GetKeyFromEvent(keyboardEvent);
				auto* action = e.platform.hardware.keyboard.GetAction(key.KeyName);

				if (IsOverlayActive())
				{
					if (action && Eq(action, "gui.overlay.toggle") && key.isPressed)
					{
						e.platform.graphics.gui.Pop();
						if (!IsEditorActive())  mode->Activate();
					}
				}
				else if (action)
				{
					if (Eq(action, "gui.overlay.toggle") && key.isPressed)
					{
						e.platform.graphics.gui.PushTop(overlayPanel->Supervisor(), true);
						mode->Deactivate();
					}
				}
			}
		}

		void OnMouseEvent(const MouseEvent& me) override
		{
			e.platform.graphics.gui.AppendEvent(me);
		}

		void OnCreate() override
		{
			NoExtraNativeLibs noExtras;
			e.platform.plumbing.utilities.RunEnvironmentScript(useMplatDefaultImplictIncludes, noExtras, "!scripts/hv/app.created.sxy", true);
		//	e.platform.graphics.gui.PushTop(colourPanel->Supervisor(), true);
		}
	};
}

namespace HV
{
	IApp* CreateApp(Platform& p)
	{
		Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
		p.os.installation.Macro("#walls", "!scripts/hv/sector/walls/");
		p.os.installation.Macro("#floors", "!scripts/hv/sector/floors/");
		p.os.installation.Macro("#corridor", "!scripts/hv/sector/corridor/");
		p.os.installation.Macro("#objects", "!scripts/hv/sector/objects/");
		p.os.installation.Macro("#icons", "!textures/hv/icons/");
		return new HV::App(p);
	}

	Rococo::Random::IRandom& GetRandomizer()
	{
		static Rococo::Random::RandomMT mt;
		return mt;
	}
}

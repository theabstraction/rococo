#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <rococo.file.browser.h>
#include <rococo.io.h>

#include "hv.events.h"
#include "hv.defaults.h"

#include <vector>

namespace HV
{
	using namespace Rococo::IO;
	using namespace Rococo::Entities;

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

	// Route scene methods to the platform.scene object
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
				return e.platform.scene.GetClearColour();
			}
		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{
			return e.platform.scene.GetSkyboxCubeId();
		}

		const Light* GetLights(uint32& nLights) const override
		{
			return e.platform.scene.GetLights(nLights);
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) override
		{
			return e.platform.scene.RenderShadowPass(drd, rc);
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			resizeCallback->OnGuiResize(screenSpan);
		}

		void RenderHelloWorld(IGuiRenderContext& g)
		{
			/* enable font loading in app.created.sxy to make this work */

			ID_FONT idFont = e.platform.utilities.GetHQFonts().GetSysFont(Graphics::HQFont_EditorFont);
			if (idFont)
			{
				Graphics::RenderHQText_LeftAligned_VCentre(g, idFont, GuiRect{ 100,2,300,300 }, "Hello World", RGBAb(0, 0, 0, 255));
				Graphics::RenderHQText_LeftAligned_VCentre(g, idFont, GuiRect{ 98,0,298,298 }, "Hello World", RGBAb(255, 255, 255, 255));
			}
		}

		void RenderGui(IGuiRenderContext& g) override
		{
			GuiMetrics metrics;
			g.Renderer().GetGuiMetrics(metrics);
			if (metrics.screenSpan.y >= 256 && metrics.screenSpan.x >= 256)
			{
				e.platform.gui.Render(g);
			//	RenderHelloWorld(g);
			}
		}

		void GetCamera(Matrix4x4& wordlToScreen, Matrix4x4& world, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{
			e.platform.scene.GetCamera(wordlToScreen, world, proj, eye, viewDir);
		}

		void RenderObjects(IRenderContext& rc)  override
		{
			e.platform.scene.RenderObjects(rc);
		}
	public:
		IGuiResizeEvent* resizeCallback = nullptr;

		AppScene(Cosmos& _e) :
			e(_e)
		{}
	};

	class App : public IApp, public IEventCallback<FileModifiedArgs>,  public IObserver, public IGuiResizeEvent
	{
		Platform& platform;

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
							SafeFormat(te.text, sizeof(te.text), "%s", be.resourceName);
						}
					}
				}
			} ec(e.platform.publisher, be);

			e.platform.gui.PushTop(busyPanel->Supervisor(), true);

			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, (IScene&) scene);
			e.platform.gui.Pop();
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
					if (e.platform.gui.Top() != busyPanel->Supervisor())
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
						e.platform.gui.PushTop(editorPanel->Supervisor(), true);
						mode->Deactivate();
					}
				}
				else
				{
					if (IsEditorActive())
					{
						e.platform.gui.Pop();
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
			fpsLogic(CreateFPSGameLogic(platform, *players, *sectors)),
			editor(CreateEditor(platform, *players, *sectors, *fpsLogic)),
			e{ _platform, *players, *editor, *sectors, *fpsLogic },
			scene(e)
		{
			mode = fpsLogic;

			Defaults::SetDefaults(e.platform.config);

			RunEnvironmentScript(e, "!scripts/hv/config.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/keys.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/controls.sxy", true);
			RunEnvironmentScript(e, "!scripts/hv/main.sxy", true);

			NoExtraNativeLibs noExtraLibs;
			e.platform.utilities.RunEnvironmentScript(noExtraLibs, "!scripts/samplers.sxy", true);

			editorPanel = e.platform.gui.BindPanelToScript("!scripts/panel.editor.sxy");
			fpsPanel = e.platform.gui.BindPanelToScript("!scripts/panel.fps.sxy");
			busyPanel = e.platform.gui.BindPanelToScript("!scripts/panel.opening.sxy");
			colourPanel = e.platform.gui.BindPanelToScript("!scripts/panel.colour.sxy");
			overlayPanel = e.platform.gui.CreateDebuggingOverlay();

			e.platform.gui.PushTop(fpsPanel->Supervisor(), true);

			e.platform.publisher.Subscribe(this, HV::Events::evSetNextLevel);
			e.platform.publisher.Subscribe(this, Rococo::Events::evBusy);
			e.platform.publisher.Subscribe(this, Events::evEnableEditor);

			scene.resizeCallback = this;

			brain = CreateAIBrain(platform.publisher, e.sectors);
		}

		~App()
		{
			e.platform.publisher.Unsubscribe(this);
		}

		void Free() override
		{
			delete this;
		}

		void OnEvent(FileModifiedArgs& args) override
		{
			HV::Events::OS::FileChangedEvent ev;
			ev.args = &args;
			e.platform.publisher.Publish(ev, HV::Events::OS::evFileChanged);

			U8FilePath pingname;
			e.platform.installation.ConvertSysPathToPingPath(args.sysPath, pingname);

			platform.gui.LogMessage("File modified: %s", pingname.buf);

			auto ext = Rococo::GetFileExtension(pingname);
			if (!ext)
			{

			}
			else if (Eq(ext, ".sxy"))
			{
				e.platform.utilities.RefreshResource(pingname);

				if (args.Matches("!scripts/hv/main.sxy"))
				{
					platform.gui.LogMessage("Running !scripts/hv/main.sxy");
					HV::RunEnvironmentScript(e, "!scripts/hv/main.sxy", true);
				}

				if (StartsWith(pingname, "!scripts/hv/sector/"))
				{
					platform.gui.LogMessage("Running sector script");
					sectors->OnSectorScriptChanged(args);
				}
			}
			else if (Eq(ext, ".ps"))
			{
				platform.gui.LogMessage("Updating pixel shader");
				e.platform.renderer.UpdatePixelShader(pingname);
			}
			else if (Eq(ext, ".vs"))
			{
				platform.gui.LogMessage("Updating vertex shader");
				e.platform.renderer.UpdateVertexShader(pingname);
			}
		}

		uint32 OnFrameUpdated(const IUltraClock& clock) override
		{
			GuiMetrics metrics;
			e.platform.renderer.GetGuiMetrics(metrics);

			e.platform.installation.OS().EnumerateModifiedFiles(*this);
			e.platform.publisher.Deliver();

			if (nextLevelName.length() > 0)
			{
				e.fpsMode.ClearCache();
				RebaseSectors();
				RunEnvironmentScript(e, nextLevelName.c_str());
				e.platform.sourceCache.Release(nextLevelName.c_str());
				nextLevelName = "";
			}

			mode->UpdateAI(clock);

			e.platform.camera.Update(clock);

			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			e.platform.renderer.Render(config, (IScene&) scene);

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
				sectors->OnTick(clock);
				return 5;
			}
		}

		bool IsOverlayActive()
		{
			return e.platform.gui.Top() == overlayPanel->Supervisor();
		}

		bool IsEditorActive()
		{
			return e.platform.gui.Top() == editorPanel->Supervisor();
		}

		void OnKeyboardEvent(const KeyboardEvent& keyboardEvent) override
		{
			if (!e.platform.gui.AppendEvent(keyboardEvent))
			{
				Key key = e.platform.keyboard.GetKeyFromEvent(keyboardEvent);
				auto* action = e.platform.keyboard.GetAction(key.KeyName);

				if (IsOverlayActive())
				{
					if (action && Eq(action, "gui.overlay.toggle") && key.isPressed)
					{
						e.platform.gui.Pop();
						if (!IsEditorActive())  mode->Activate();
					}
				}
				else if (action)
				{
					if (Eq(action, "gui.overlay.toggle") && key.isPressed)
					{
						e.platform.gui.PushTop(overlayPanel->Supervisor(), true);
						mode->Deactivate();
					}
				}
			}
		}

		void OnMouseEvent(const MouseEvent& me) override
		{
			e.platform.gui.AppendEvent(me);
		}

		void OnCreate() override
		{
			NoExtraNativeLibs noExtras;
			e.platform.utilities.RunEnvironmentScript(noExtras, "!scripts/hv/app.created.sxy", true);
		//	e.platform.gui.PushTop(colourPanel->Supervisor(), true);
		}
	};
}

namespace HV
{
	IApp* CreateApp(Platform& p)
	{
		p.installation.Macro("#walls", "!scripts/hv/sector/walls/");
		p.installation.Macro("#floors", "!scripts/hv/sector/floors/");
		p.installation.Macro("#corridor", "!scripts/hv/sector/corridor/");
		p.installation.Macro("#objects", "!scripts/hv/sector/objects/");
		return new HV::App(p);
	}

	IRandom& GetRandomizer()
	{
		static Random::RandomMT mt;
		return mt;
	}
}

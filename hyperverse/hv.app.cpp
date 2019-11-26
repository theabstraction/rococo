#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include "hv.events.h"
#include "hv.defaults.h"

#include <vector>

namespace HV
{
	using namespace Rococo::Entities;

	auto evPopulateBusyCategoryId = "busy.category"_event;
	auto evPopulateBusyResourceId = "busy.resource"_event;

	struct IGuiResizeEvent
	{
		virtual void OnGuiResize(Vec2i span) = 0;
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

		const Light* GetLights(size_t& nLights) const override
		{
			return e.platform.scene.GetLights(nLights);
		}

		ID_TEXTURE GetSkyboxCubeId() const override
		{
			return ID_TEXTURE::Invalid();
		}

		void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc) override
		{
			return e.platform.scene.RenderShadowPass(drd, rc);
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			resizeCallback->OnGuiResize(screenSpan);
		}

		void RenderGui(IGuiRenderContext& grc) override
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);
			if (metrics.screenSpan.y >= 256 && metrics.screenSpan.x >= 256)
			{
				e.platform.gui.Render(grc);
			}
		}

		void GetCamera(Matrix4x4& proj, Matrix4x4& world, Vec4& eye, Vec4& viewDir) override
		{
			e.platform.scene.GetCamera(proj, world, eye, viewDir);
		}

		void RenderObjects(IRenderContext& rc)  override
		{
			e.platform.scene.RenderObjects(rc);
		}
	public:
		IGuiResizeEvent* resizeCallback;

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
			} ec(e.platform.publisher, be);

			e.platform.gui.PushTop(busyPanel->Supervisor(), true);

			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, (IScene&) scene);
			e.platform.gui.Pop();
		}

		virtual void OnGuiResize(Vec2i screenSpan) override
		{
			GuiRect fullScreen = { 0, 0, screenSpan.x, screenSpan.y };
			fpsPanel->Supervisor()->SetRect(fullScreen);
			busyPanel->Supervisor()->SetRect(fullScreen);
			editorPanel->Root()->SetRect(fullScreen);
		}

		virtual void OnEvent(Event& ev)
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
			else if (ev == evPopulateBusyCategoryId)
			{
			}

			else if (ev == evPopulateBusyResourceId)
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
			RunEnvironmentScript(e, "!scripts/samplers.sxy", true);

			editorPanel = e.platform.gui.BindPanelToScript("!scripts/panel.editor.sxy");
			fpsPanel = e.platform.gui.BindPanelToScript("!scripts/panel.fps.sxy");
			busyPanel = e.platform.gui.BindPanelToScript("!scripts/panel.opening.sxy");
			overlayPanel = e.platform.gui.CreateOverlay();

			e.platform.gui.PushTop(fpsPanel->Supervisor(), true);

			e.platform.publisher.Subscribe(this, HV::Events::evSetNextLevel);
			e.platform.publisher.Subscribe(this, Rococo::Events::evBusy);

			scene.resizeCallback = this;
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

			char pingname[1024];
			e.platform.installation.ConvertSysPathToPingPath(args.resourceName, pingname, 1024);

			platform.gui.LogMessage("File modified: %s", pingname);

			auto ext = Rococo::GetFileExtension(args.resourceName);
			if (!ext)
			{

			}
			else if (Eq(ext, L".sxy"))
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
			else if (Eq(ext, L".ps"))
			{
				platform.gui.LogMessage("Updating pixel shader");
				e.platform.renderer.UpdatePixelShader(pingname);
			}
			else if (Eq(ext, L".vs"))
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
					if (Eq(action, "gui.overlay.toggle") && key.isPressed)
					{
						e.platform.gui.Pop();
						if (!IsEditorActive())  mode->Activate();
					}
				}
				else if (action)
				{
					if (Eq(action, "gui.editor.toggle") && key.isPressed)
					{
						if (IsEditorActive())
						{
							e.platform.gui.Pop();
							mode->Activate();
						}
						else
						{
							e.platform.gui.PushTop(editorPanel->Supervisor(), true);
							mode->Deactivate();
						}
					}
					else if (Eq(action, "gui.overlay.toggle") && key.isPressed)
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
			RunEnvironmentScript(e, "!scripts/hv/app.created.sxy", true);
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
}

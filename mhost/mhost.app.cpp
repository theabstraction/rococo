#include <rococo.api.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>

using namespace Rococo;

#include "mhost.script.types.h"
#include "mhost.sxh.h"

namespace MHost
{
	using namespace Rococo::Entities;
	using namespace Rococo::Events;

	auto evPopulateBusyCategoryId = "busy.category"_event;
	auto evPopulateBusyResourceId = "busy.resource"_event;

	void RunEnvironmentScript(Platform& platform, IEngine* engine, cstr name, bool releaseAfterUse);

	class App : 
		public IDirectApp, 
		public IEventCallback<FileModifiedArgs>,
		public IObserver, 
		public IEngine,
		public IScreenBuilder
	{
		Platform& platform;
		IDirectAppControl& control;

		AutoFree<IPaneBuilderSupervisor> overlayPanel;
		AutoFree<IPaneBuilderSupervisor> busyPanel;

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

			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, platform.scene);
			platform.gui.Pop();
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
	public:
		App(Platform& _platform, IDirectAppControl& _control) :
			platform(_platform), control(_control)
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
			args.GetPingPath(pingname, 1024);

			platform.gui.LogMessage("File modified: %s", pingname);

			auto ext = Rococo::GetFileExtension(args.resourceName);
			if (!ext)
			{

			}
			else if (Eq(ext, ".sxy"))
			{
				platform.utilities.RefreshResource(pingname);

				if (args.Matches("!scripts/hv/main.sxy"))
				{
					platform.gui.LogMessage("Running !scripts/hv/main.sxy");
					MHost::RunEnvironmentScript(platform, this, "!scripts/hv/main.sxy", true);
				}
			}
			else if (Eq(ext, ".ps"))
			{
				platform.gui.LogMessage("Updating pixel shader");
				platform.renderer.UpdatePixelShader(pingname);
			}
			else if (Eq(ext, ".vs"))
			{
				platform.gui.LogMessage("Updating vertex shader");
				platform.renderer.UpdateVertexShader(pingname);
			}
		}

		bool isRunning = true;

		IScreenBuilder* GetScreenBuilder()
		{
			return this;
		}

		boolean32 IsRunning()
		{
			return isRunning;
		}

		void DrawBitmap(const BitmapSpec& spec, Vec2i& topLeft)
		{

		}

		boolean32 TryGetBitmapSpec(const fstring& resourceName, BitmapSpec& spec)
		{
			return false;
		}

		void Run() override
		{
			RunEnvironmentScript(platform, this, "mhost.main.sxy", true);
		}

		void SetPhase(int32 phaseIndex) override
		{

		}

		void DrawAll() override
		{
			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, platform.scene);
		}

		void YieldForSystemMessages(int32 sleepMS) override
		{
			platform.installation.OS().EnumerateModifiedFiles(*this);
			platform.publisher.Deliver();

			if (!control.TryRouteSysMessages(sleepMS))
			{
				isRunning = false;
			}
		}
	};
}

namespace MHost
{
	IDirectApp* CreateApp(Platform& p, IDirectAppControl& control)
	{
		p.installation.Macro("#bitmaps", "!scripts/mhost/bitmaps/");
		return new MHost::App(p, control);
	}
}

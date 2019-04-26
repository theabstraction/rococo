#include "mhost.h"
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>

#include <vector>

#include <rococo.textures.h>

namespace MHost
{
	using namespace Rococo::Entities;
	using namespace Rococo::Events;
	using namespace Rococo::Textures;

	auto evPopulateBusyCategoryId = "busy.category"_event;
	auto evPopulateBusyResourceId = "busy.resource"_event;

	void RunEnvironmentScript(Platform& platform, IEngine* engine, cstr name, bool releaseAfterUse);

	struct AppSceneBuilder : public IScene, public IScreenBuilder
	{
		Platform& platform;

		std::vector<GuiVertex> triangles;

		AppSceneBuilder(Platform& _platform): platform(_platform)
		{
			platform.camera.SetRHProjection(45_degrees, 0.1_metres, 1000_metres);
		}

		void AppendTriangles(const GuiVertex* v, size_t nVertices)
		{
			enum { MAX_TRIANGLES = 1000000 };
			if (triangles.size() > MAX_TRIANGLES)
				Throw(0, "Maximum GUI triangles reached (%d)", MAX_TRIANGLES);

			auto end = v + nVertices;
			while (v < end)
			{
				triangles.push_back(*v++);
			}
		}

		void SetGuiQuadForBitmap(GuiTriangle t[2], const GuiRectf& txUV, int textureIndex)
		{
			t[0].a.colour = RGBAb(0xFFFFFFFF);
			t[0].a.sd.lerpBitmapToColour = 0;
			t[0].a.sd.matIndex = 0;
			t[0].a.sd.textureIndex = (float) textureIndex;
			t[0].a.sd.lerpBitmapToColour = 0;
			t[0].a.sd.textureToMatLerpFactor = 0;
			t[0].a.vd.fontBlend = 0;

			t[0].b = t[0].c = t[0].a;
			t[1] = t[0];

			t[0].a.vd.uv = { txUV.left, txUV.top };
			t[0].b.vd.uv = { txUV.right, txUV.top };
			t[0].c.vd.uv = { txUV.left, txUV.bottom };

			t[1].a.vd.uv = { txUV.right, txUV.bottom };
			t[1].b.vd.uv = t[0].c.vd.uv;
			t[1].c.vd.uv = t[0].b.vd.uv;
		}

		void DrawSprite(const Vec2i& pixelPos, int32 alignmentFlags, const Rococo::Textures::BitmapLocation& loc) override
		{
			//   a --- b
			//   |   /
			//   |  /  = t0
			//   | /
			//   |/
			//   c

			//         c
			//       / |
			//      /  | = t1
			//     /   |
			//    /    |
			//   b-----a

			GuiRectf txUV = { (float) loc.txUV.left,  (float) loc.txUV.top, (float) loc.txUV.right,  (float) loc.txUV.bottom };

			Vec2 span = Span(txUV);

			Vec2 pxPos = { (float)pixelPos.x, (float)pixelPos.y };
			Vec2 topLeftPos = GetTopLeftPos(pxPos, span, alignmentFlags);

			GuiTriangle t[2];
			SetGuiQuadForBitmap(t, txUV, loc.textureIndex);

			t[0].a.pos = topLeftPos;
			t[0].b.pos = { topLeftPos.x + span.x, topLeftPos.y };
			t[0].c.pos = { topLeftPos.x, topLeftPos.y + span.y };

			t[1].a.pos = topLeftPos + span;
			t[1].b.pos = t[0].c.pos;
			t[1].c.pos = t[0].b.pos;

			AppendTriangles(&t->a, 6);
		}

		void StretchSprite(const GuiRect& quad, const Rococo::Textures::BitmapLocation& loc) override
		{
			GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

			GuiTriangle t[2];
			SetGuiQuadForBitmap(t, txUV, loc.textureIndex);

			t[0].a.pos = { (float) quad.left, (float) quad.top };
			t[0].b.pos = { (float) quad.right, (float) quad.top };
			t[0].c.pos = { (float) quad.left, (float) quad.bottom };

			t[1].a.pos = { (float) quad.right, (float) quad.bottom };
			t[1].b.pos = t[0].c.pos;
			t[1].c.pos = t[0].b.pos;

			AppendTriangles(&t->a, 6);
		}

		void GetCamera(Matrix4x4& camera, Matrix4x4& world, Vec4& eye, Vec4& viewDir) override
		{
			return platform.scene.GetCamera(camera, world, eye, viewDir);
		}

		RGBA GetClearColour() const override
		{
			return platform.scene.GetClearColour();
		}

		void OnGuiResize(Vec2i screenSpan) override
		{
			return platform.scene.OnGuiResize(screenSpan);
		}

		void RenderGui(IGuiRenderContext& grc)  override
		{
			for (auto& t : triangles)
			{
				grc.AddTriangle(&t);
			}
			triangles.clear();
			return platform.scene.RenderGui(grc);
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

		void PushTriangle(const Rococo::GuiTriangle& t) override
		{
			AppendTriangles(&t.a, 3);
		}

		void Render() override
		{
			Graphics::RenderPhaseConfig config;
			config.EnvironmentalMap = Graphics::ENVIRONMENTAL_MAP_FIXED_CUBE;
			platform.renderer.Render(config, *this);
		}
	};

	class App : 
		public IDirectApp, 
		public IEventCallback<FileModifiedArgs>,
		public IObserver, 
		public IEngine
	{
		Platform& platform;
		IDirectAppControl& control;

		AutoFree<IPaneBuilderSupervisor> overlayPanel;
		AutoFree<IPaneBuilderSupervisor> busyPanel;
		AppSceneBuilder sceneBuilder;

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
			platform(_platform), control(_control), sceneBuilder(_platform)
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

		IScreenBuilder* ScreenBuilder()
		{
			return &sceneBuilder;
		}

		boolean32 IsRunning()
		{
			return isRunning;
		}

		void Run() override
		{
			RunEnvironmentScript(platform, this, "!scripts/mhost/galaxians.sxy", true);
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

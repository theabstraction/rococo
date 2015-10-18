#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "dystopia.text.h"
#include "rococo.geometry.inl"
#include "meshes.h"
#include "human.types.h"
#include "rococo.ui.h"
#include "controls.inl"

using namespace Dystopia;
using namespace Rococo;
using namespace Rococo::Fonts;

namespace
{
	float GetAspectRatio(const IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);
		float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
		return aspectRatio;
	}

	void DrawTestTriangles(IGuiRenderContext& rc)
	{
		GuiVertex q0[6] =
		{
			{ 0.0f,600.0f, 1.0f, 0.0f,{ 255,0,0,255 }, 0.0f, 0.0f, 0 }, // bottom left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,0,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 0,0,255,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 255,255,0,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,255,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 800.0f,  0.0f, 1.0f, 0.0f,{ 255,0,255,255 }, 0.0f, 0.0f, 0 }  // top right
		};

		rc.AddTriangle(q0);
		rc.AddTriangle(q0 + 3);
	}

	void DrawGuiMetrics(IGuiRenderContext& rc)
	{
		GuiMetrics metrics;
		rc.Renderer().GetGuiMetrics(metrics);

		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"Mouse: (%d,%d). Screen(%d,%d)", metrics.cursorPosition.x, metrics.cursorPosition.y, metrics.screenSpan.x, metrics.screenSpan.y);

		RenderHorizontalCentredText(rc, info, RGBAb{ 255, 255, 255, 255 }, 1, Vec2i{ 25, 25 });
	}

	void DrawVector(IGuiRenderContext& grc, const Vec4& v, const Vec2i& pos)
	{
		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"(%f) (%f) (%f) (%f)\n", v.x, v.y, v.z, v.w);

		RenderHorizontalCentredText(grc, info, RGBAb{ 255, 255, 255, 255 }, 3, pos);
	}

	void DrawVector(IGuiRenderContext& grc, const Vec3& v, const Vec2i& pos)
	{
		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"(%f) (%f) (%f) ", v.x, v.y, v.z);

		RenderHorizontalCentredText(grc, info, RGBAb{ 255, 255, 255, 255 }, 3, pos);
	}

	struct HumanFactory: public IHumanFactory
	{
		ILevel* level;
		IIntent* controls;

		virtual IHumanSupervisor* CreateHuman(ID_ENTITY id, IInventory& inventory, HumanType typeId)
		{
			switch (typeId)
			{
			case HumanType_Bobby:
				return CreateBobby(id, inventory, *level);
			case HumanType_Vigilante:
				return CreateVigilante(id, *controls, *level);
			}
			return nullptr;
		}
	};

	void DrawHealthBar(IGuiRenderContext& gr, IHuman& human)
	{
		Stat health = human.GetStat(StatIndex_Health);

		if (health.cap <= 0 || health.current <= 0) return;

		GuiMetrics metrics;
		gr.Renderer().GetGuiMetrics(metrics);

		float left = 0.985f * (float)metrics.screenSpan.x;
		float bottom = 0.125f * (float)metrics.screenSpan.y;

		float healthRatio = (float)health.current / (float)health.cap;

		bool overLoad = false;
		if (healthRatio > 1.0f)
		{
			overLoad = true;
			healthRatio = 1.0f;
		}

		float height = 0.1f * (float)metrics.screenSpan.y * healthRatio;

		float right = left + 0.005f * (float)metrics.screenSpan.x;
		float top = bottom - height;

		int redness = 63 + (int)(192.0f * healthRatio);

		uint8 yellow = overLoad ? 0xFF : 0x00;

		RGBAb healthColour{ (uint8)redness, yellow, 0, 0xFF };

		GuiVertex q0[6] =
		{
			{ left,   bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom left
			{ right,  bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom right
			{ left,      top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // top left
			{ left,      top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // top left
			{ right,  bottom, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }, // bottom right
			{ right,     top, 1.0f, 0.0f, healthColour, 0.0f, 0.0f, 0 }  // top right
		};

		gr.AddTriangle(q0);
		gr.AddTriangle(q0 + 3);
	}

	Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);

		return{  (2.0f * (float)metrics.cursorPosition.x - metrics.screenSpan.x) / metrics.screenSpan.x,
				-(2.0f * (float)metrics.cursorPosition.y - metrics.screenSpan.y) / metrics.screenSpan.y };
	}

	class DystopiaApp : public IApp, private IScene, public IEventCallback<GuiEventArgs>
	{
	private:
		AutoFree<IGuiSupervisor> gui;
		AutoFree<IDebuggerWindow> debuggerWindow;
		AutoFree<ISourceCache> sourceCache;
		AutoFree<IMeshLoader> meshes;
		Environment e;
		HumanFactory humanFactory;
		AutoFree<ILevelSupervisor> level;
		AutoFree<ILevelLoader> levelLoader;

		GameControls controls;
	
		float gameTime;
		float lastFireTime;

		Matrix4x4 worldMatrix;
	public:
		DystopiaApp(IRenderer& _renderer, IInstallation& _installation) : 
			gui(CreateGui(_renderer)),
			debuggerWindow(CreateDebuggerWindow(&_renderer.Window())),
			sourceCache(CreateSourceCache(_installation)),
			meshes(CreateMeshLoader(_installation, _renderer, *sourceCache)),
			e{ _installation, _renderer, *debuggerWindow, *sourceCache, *meshes, *gui },
			level(CreateLevel(e, humanFactory)),
			levelLoader(CreateLevelLoader(e, *level)),
			gameTime(0.0f),
			lastFireTime(0.0f)
		{
			humanFactory.level = level;
			humanFactory.controls = &controls;

			gui->SetEventHandler(this);
		}
		
		~DystopiaApp()
		{
		}

		virtual void OnEvent(GuiEventArgs& args)
		{

		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnCreated()
		{
			levelLoader->Load(L"!levels/level1.sxy", false);
		}

		Vec3 xCursor;
		Vec3 selectionPoint;
		Vec4 cursorPos;

		virtual uint32 OnFrameUpdated(const IUltraClock& clock)
		{
			levelLoader->SyncWithModifiedFiles();

			if (!gui->HasFocus())
			{
				float dt = clock.FrameDelta() / (float)clock.Hz();
				if (dt < 0.0f) dt = 0.0f;
				if (dt > 0.1f) dt = 0.1f;

				gameTime += dt;

				level->UpdateObjects(gameTime, dt);
			}

			auto id = level->GetPlayerId();

			Vec3 playerPosition;
			level->GetPosition(playerPosition, id);

			float g = 0.04f * powf(1.25f, controls.GlobalScale());

			Degrees phi{ -45.0f };
			Degrees theta{ controls.ViewTheta() };
			GetIsometricWorldMatrix(worldMatrix, g, GetAspectRatio(e.renderer), playerPosition, phi, theta);

			GuiMetrics metrics;
			e.renderer.GetGuiMetrics(metrics);

			Matrix4x4 inverseWorldMatrix;
			InverseMatrix(worldMatrix, inverseWorldMatrix);

			Vec2 C = PixelSpaceToScreenSpace(metrics.cursorPosition, e.renderer);

			Vec4 xCursor = Vec4 { C.x, C.y, 0.0f, 1.0f };

			cursorPos = Transform(inverseWorldMatrix, xCursor);
			Vec3 cursorPos3{ cursorPos.x, cursorPos.y, cursorPos.z };

			Vec4 k = { 0, 0, 1.0f, 0.0f };

			Vec4 worldDir = Transform(inverseWorldMatrix, k);
			Vec3 worldDir3{ worldDir.x, worldDir.y, worldDir.z };

			// We now have all the information to specify cursor probe. Point on probe are P(t) = cursorPos + t.worldDir.
			// We want intersection with earth z = 0
			// t0.worldDir.z + cursorPos.z = 0
			// t0 = -cursorPos.z / worldDir.z, giving P(t0), intersection with cursor and earth

			float t0 = -cursorPos.z / worldDir.z;
			selectionPoint = cursorPos3 + t0 * worldDir3;

			e.renderer.Render(*this);
			return 5;
		}

		virtual RGBA GetClearColour() const
		{
			return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
		}

		virtual void GetWorldMatrix(Matrix4x4& worldMatrix) const
		{
			worldMatrix = this->worldMatrix;
		}

		virtual void RenderGui(IGuiRenderContext& gr)
		{
			// DrawTestTriangles(gr);

			if (gui->HasFocus())
			{
				gui->Render(gr);
				return;
			}

			auto id = level->GetPlayerId();

			IInventory* inventory;
			IHuman* human;
			if (level->GetHuman(id, &inventory, &human))
			{
				DrawHealthBar(gr, *human);
			}

			DrawVector(gr, cursorPos, Vec2i{ 25, 25 });
			DrawVector(gr, selectionPoint, Vec2i{ 25, 75 });

			// DrawGuiMetrics(gr);
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			level->RenderObjects(rc);
		}

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			if (gui->HasFocus())
			{
				gui->AppendMouseEvent(me);
			}
			else
			{
				controls.AppendMouseEvent(me);
			}
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& k)
		{
			if (gui->HasFocus())
			{
				gui->AppendKeyboardEvent(k);
			}
			else
			{
				controls.AppendKeyboardEvent(k);
			}
		}
	};
}

namespace Dystopia
{
	IApp* CreateDystopiaApp(IRenderer& renderer, IInstallation& installatiion)
	{
		return new DystopiaApp(renderer, installatiion);
	}
}
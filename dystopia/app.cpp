#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "dystopia.text.h"
#include "rococo.geometry.inl"
#include <DirectXMath.h>
#include "meshes.h"
#include "human.types.h"
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

	void GetIsometricWorldMatrix(Matrix4x4& worldMatrix, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta)
	{
		using namespace DirectX;

		XMMATRIX aspectCorrect = XMMatrixScaling(scale * aspectRatio, scale, scale);
		XMMATRIX rotZ = XMMatrixRotationZ(ToRadians(viewTheta));
		XMMATRIX rotX = XMMatrixRotationX(ToRadians(phi));
		XMMATRIX origin = XMMatrixTranslation(-centre.x, -centre.y, 0.0f);
		XMMATRIX totalRot = XMMatrixMultiply(rotZ, rotX);

		XMMATRIX t = XMMatrixMultiply(XMMatrixMultiply(origin, totalRot), aspectCorrect);

		XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &worldMatrix, t);
	}

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

		virtual uint32 OnTick(const IUltraClock& clock)
		{
			levelLoader->SyncWithModifiedFiles();

			if (!gui->HasFocus())
			{
				float dt = clock.TickDelta() / (float)clock.Hz();
				if (dt < 0.0f) dt = 0.0f;
				if (dt > 0.1f) dt = 0.1f;

				gameTime += dt;

				level->UpdateObjects(gameTime, dt);
			}

			e.renderer.Render(*this);
			return 5;
		}

		virtual RGBA GetClearColour() const
		{
			return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
		}

		virtual void GetWorldMatrix(Matrix4x4& worldMatrix) const
		{
			auto id = level->GetPlayerId();

			Vec3 playerPosition;
			level->GetPosition(playerPosition, id);

			float g = 0.04f * powf(1.25f, controls.GlobalScale());
			GetIsometricWorldMatrix(worldMatrix, g, GetAspectRatio(e.renderer), playerPosition, Degrees{ -45.0f }, controls.ViewTheta());
		}

		virtual void RenderGui(IGuiRenderContext& gr)
		{
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

			// DrawGuiMetrics(gr);
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			level->RenderObjects(rc);
		}

		MouseEvent lastEvent;

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			lastEvent = me;

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
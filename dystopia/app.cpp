#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "dystopia.text.h"
#include "rococo.geometry.inl"
#include <DirectXMath.h>
#include "meshes.h"
#include "dystopia.h"
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

		RenderHorizontalCentredText(rc, info, RGBAb{ 255, 255, 255, 255 }, 1, Vec2i(25, 25));
	}

	class DystopiaApp : public IApp, private IScene
	{
	private:
		AutoFree<IDebuggerWindow> debuggerWindow;
		Environment* e;	
		AutoFree<ILevelSupervisor> level;
		AutoFree<ILevelLoader> levelLoader;

		GameControls controls;
	
		
	public:
		DystopiaApp(IRenderer& _renderer, IInstallation& _installation) : 
			debuggerWindow(CreateDebuggerWindow(nullptr)),
			e(ConstructEnvironment(_installation, _renderer, *debuggerWindow)),
			level(CreateLevel(*e)),
			levelLoader(CreateLevelLoader(*e, *level))
		{
		}
		
		~DystopiaApp()
		{
			Dystopia::Free(e);
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnCreated()
		{
			levelLoader->Load(L"!levels/level1.sxy", false);
		}

		void AdvanceGame(float dt)
		{
			auto id = level->GetPlayerId();

			Vec3 pos;
			level->GetPosition(pos, id);

			float speed = 4.0f;
			Vec3 newPos = pos + dt * speed * Vec3(controls.GetImpulse().x, controls.GetImpulse().y, 0);
			
			level->SetPosition(newPos, id);
		}

		virtual uint32 OnTick(const IUltraClock& clock)
		{
			levelLoader->SyncWithModifiedFiles();

			float dt = clock.TickDelta() / (float) clock.Hz();
			if (dt < 0.0f) dt = 0.0f;
			if (dt > 0.1f) dt = 0.1f;

			AdvanceGame(dt);

			e->renderer.Render(*this);
			return 5;
		}

		virtual RGBA GetClearColour() const
		{
			return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
		}

		virtual void GetWorldMatrix(Matrix4x4& worldMatrix) const
		{
			using namespace DirectX;

			auto id = level->GetPlayerId();

			Vec3 playerPosition;
			level->GetPosition(playerPosition, id);

			Degrees phi{ -45.0f };

			float g = 0.04f * powf(1.25f, controls.GlobalScale());

			XMMATRIX aspectCorrect = XMMatrixScaling(g * GetAspectRatio(e->renderer), g, g);
			XMMATRIX rotZ = XMMatrixRotationZ(ToRadians(controls.ViewTheta()));
			XMMATRIX rotX = XMMatrixRotationX(ToRadians(phi));
			XMMATRIX origin = XMMatrixTranslation(-playerPosition.x, -playerPosition.y, 0.0f);
			XMMATRIX totalRot = XMMatrixMultiply(rotZ, rotX);

			XMMATRIX t = XMMatrixMultiply(XMMatrixMultiply(origin, totalRot), aspectCorrect);

			XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &worldMatrix, t);
		}


		virtual void RenderGui(IGuiRenderContext& gr)
		{
			// DrawTestTriangles(gr);
			DrawGuiMetrics(gr);
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			level->RenderObjects(rc);
		}

		MouseEvent lastEvent;

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			lastEvent = me;
			controls.AppendMouseEvent(me);
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& k)
		{
			controls.AppendKeyboardEvent(k);
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
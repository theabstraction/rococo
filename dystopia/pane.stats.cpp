#include "dystopia.h"
#include "rococo.renderer.h"
#include "human.types.h"

#include "rococo.ui.h"
#include "dystopia.ui.h"

using namespace Rococo;
using namespace Dystopia;

namespace
{
	float GetAspectRatio(const IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);
		float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
		return aspectRatio;
	}

	Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer)
	{
		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);

		return{ (2.0f * (float)metrics.cursorPosition.x - metrics.screenSpan.x) / metrics.screenSpan.x,
			-(2.0f * (float)metrics.cursorPosition.y - metrics.screenSpan.y) / metrics.screenSpan.y };
	}

	void DrawHealthBar(IGuiRenderContext& gr, IHumanAI& human)
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

	class StatsPane : public IUIPaneSupervisor
	{
		Environment& e;

	public:
		StatsPane(Environment& _e) :
			e(_e)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual Relay OnTimestep(const TimestepEvent& clock)
		{
			return Relay_Next;
		}

		virtual Relay OnKeyboardEvent(const KeyboardEvent& ke)
		{
			return Relay_Next;
		}

		virtual Relay OnMouseEvent(const MouseEvent& me)
		{
			return Relay_Next;
		}

		virtual void OnPop()
		{

		}

		virtual void OnTop()
		{

		}

		virtual void RenderObjects(IRenderContext& rc)
		{
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			auto id = e.level.GetPlayerId();

			auto spec = e.level.GetHuman(id);
			if (spec.type)
			{
				DrawHealthBar(grc, *spec.ai);
			}
		}
	};
}

namespace Dystopia
{
	IUIPaneSupervisor* CreatePaneStats(Environment& e)
	{
		return new StatsPane(e);
	}
}
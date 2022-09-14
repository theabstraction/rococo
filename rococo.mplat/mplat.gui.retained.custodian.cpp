#include <rococo.gui.retained.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>
#include <rococo.maths.h>
#include <rococo.ui.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Graphics;

namespace ANON
{
	struct MPlatGR_Renderer : IGRRenderContext
	{
		IGuiRenderContext* rc = nullptr;
		GuiRect lastScreenDimensions;

		Vec2i cursorPos{ -1000,-1000 };

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			Rococo::Graphics::DrawRectangle(*rc, absRect, colour, colour);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			Rococo::Graphics::DrawBorderAround(*rc, absRect, Vec2i{ 1,1 }, colour1, colour2);
		}

		Vec2i CursorHoverPoint() const override
		{
			return cursorPos;
		}

		bool IsHovered(IGRPanel& panel) const override
		{
			return IsPointInRect(cursorPos, panel.AbsRect());
		}

		GuiRect ScreenDimensions() const override
		{
			return lastScreenDimensions;
		}

		void SetContext(IGuiRenderContext* rc)
		{
			this->rc = rc;
			if (rc)
			{
				GuiMetrics metrics;
				rc->Renderer().GetGuiMetrics(metrics);
				cursorPos = metrics.cursorPosition;
				lastScreenDimensions = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
			}
		}
	};

	struct MPlatCustodian : IMPlatGuiCustodianSupervisor, IGuiRetainedCustodian, IGREventHistory
	{
		MPlatGR_Renderer renderer;

		// Debugging materials:
		std::vector<IGRWidget*> history;
		EventRouting lastRoutingStatus;
		int64 eventCount = 0;

		void RecordWidget(IGRWidget& widget)
		{
			history.push_back(&widget);
		}

		void RouteMouseEvent(const MouseEvent& me, IGuiRetained& gr)
		{
			static_assert(sizeof CursorClick == sizeof uint16);

			history.clear();
			if (me.buttonFlags != 0)
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags };
				lastRoutingStatus = gr.RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				CursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(CursorClick*)&me.buttonFlags };
				lastRoutingStatus = gr.RouteCursorMoveEvent(cursorEvent);
			}
			eventCount++;
		}

		IGuiRetainedCustodian& Custodian()
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void Render(IGuiRenderContext& rc, IGuiRetained& gr) override
		{
			renderer.SetContext(&rc);
			
			if (renderer.lastScreenDimensions.right > 0 && renderer.lastScreenDimensions.bottom > 0)
			{
				gr.RenderGui(renderer);
			}

			renderer.SetContext(nullptr);
		}
	};
}

namespace Rococo::Gui
{
	IMPlatGuiCustodianSupervisor* CreateMPlatCustodian()
	{
		return new ANON::MPlatCustodian();
	}
}
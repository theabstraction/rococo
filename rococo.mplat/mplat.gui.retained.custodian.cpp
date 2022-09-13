#include <rococo.gui.retained.h>
#include <rococo.mplat.h>
#include <rococo.renderer.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Graphics;

namespace ANON
{
	struct MPlatGR_Renderer : IGRRenderContext
	{
		IGuiRenderContext* rc = nullptr;
		GuiRect lastScreenDimensions;

		Vec2i origin{ 0,0 };

		void DrawRect(Vec2i parentOffset, Vec2i span, RGBAb colour) override
		{
			Vec2i topLeft = origin + parentOffset;
			GuiRect rect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
			Rococo::Graphics::DrawRectangle(*rc, rect, colour, colour);
		}

		void DrawRectEdge(Vec2i parentOffset, Vec2i span, RGBAb colour1, RGBAb colour2) override
		{
			Vec2i topLeft = origin + parentOffset;
			GuiRect rect { topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
			Rococo::Graphics::DrawBorderAround(*rc, rect, Vec2i{ 1,1 }, colour1, colour2);
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
				lastScreenDimensions = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
			}
		}

		void SetOrigin(Vec2i origin) override
		{
			this->origin = origin;
		}
	};

	struct MPlatCustodian : IMPlatGuiCustodianSupervisor, IGuiRetainedCustodian
	{
		MPlatGR_Renderer renderer;

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
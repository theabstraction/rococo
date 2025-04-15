#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRGradientFill : IGRWidgetGradientFill, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		
		IGRImage* image = nullptr;
		HString imagePath;

		GRGradientFill(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		RGBAb topLeft = { 0,0,0,0 };
		RGBAb topRight = { 0,0,0,0 };
		RGBAb bottomLeft = { 0,0,0,0 };
		RGBAb bottomRight = { 0,0,0,0 };

		virtual ~GRGradientFill()
		{

		}

		void Free() override
		{
			delete this;
		}

		void SetBottomLeft(RGBAb c) override
		{
			bottomLeft = c;
		}

		void SetBottomRight(RGBAb c) override
		{
			bottomRight = c;
		}

		void SetTopLeft(RGBAb c) override
		{
			topLeft = c;
		}

		void SetTopRight(RGBAb c) override
		{
			topRight = c;
		}

		EGREventRouting OnCursorClick(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
		}

		EGREventRouting OnCursorMove(GRCursorEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			const auto& rect = panel.AbsRect();

			GRTriangle t[2];
			t[0].a = { TopLeft(rect), topLeft };
			t[0].b = { TopRight(rect), topRight };
			t[0].c = { BottomRight(rect), bottomRight };
			t[1].a = { BottomRight(rect), bottomRight };
			t[1].b = { BottomLeft(rect), bottomLeft };
			t[1].c = { TopLeft(rect), topLeft };

			g.DrawTriangles(t, 2);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const
		{
			Vec2i extraSpan;
			extraSpan.x = panel.Padding().left + panel.Padding().right;
			extraSpan.y = panel.Padding().top + panel.Padding().bottom;
			return image ? image->Span() + extraSpan : extraSpan;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetGradientFill, GRGradientFill>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRGradientFill";
		}
	};

	struct GRGradientFillFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRGradientFill(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetGradientFill::InterfaceId()
	{
		return "IGRWidgetGradientFill";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetGradientFill& CreateGradientFill(IGRWidget& parent)
	{
		GRANON::GRGradientFillFactory factory;
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), factory);
		IGRWidgetGradientFill* g = Cast<IGRWidgetGradientFill>(widget);
		return *g;
	}
}
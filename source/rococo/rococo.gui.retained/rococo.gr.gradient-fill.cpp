// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRGradientFill : IGRWidgetGradientFill, IGRWidgetSupervisor, IGRWidgetLayout
	{
		IGRPanel& panel;
		
		IGRImage* image = nullptr;
		HString imagePath;

		GRGradientFill(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		struct RGBAbQuad
		{
			RGBAb topLeft = {0,0,0,0};;
			RGBAb topRight = { 0,0,0,0 };;
			RGBAb bottomLeft = { 0,0,0,0 };;
			RGBAb bottomRight = { 0,0,0,0 };;
		} mainQuad;

		virtual ~GRGradientFill()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
		}

		void Free() override
		{
			delete this;
		}

		void LayoutBeforeFit() override
		{

		}

		void LayoutBeforeExpand() override
		{
			if (fitRule == EGRFitRule::FirstChild)
			{
				auto* child0 = panel.GetChild(0);
				if (!child0)
				{				
					panel.SetConstantHeight(2);
					return;
				}

				panel.SetConstantHeight(child0->Span().y);
			}
		}

		void LayoutAfterExpand() override
		{

		}

		void SetBottomLeft(RGBAb c) override
		{
			mainQuad.bottomLeft = c;
		}

		void SetBottomRight(RGBAb c) override
		{
			mainQuad.bottomRight = c;
		}

		void SetTopLeft(RGBAb c) override
		{
			mainQuad.topLeft = c;
		}

		void SetTopRight(RGBAb c) override
		{
			mainQuad.topRight = c;
		}

		EGRFitRule fitRule = EGRFitRule::None;

		void SetFitVertical(EGRFitRule fitRule) override
		{
			this->fitRule = fitRule;
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

		void BannerShade(const GuiRect& rect, IGRRenderContext& g)
		{
			Vec2i centre = Centre(rect);
			GuiRect leftRect(rect.left, rect.top, centre.x, rect.bottom);
			GuiRect rightRect(centre.x, rect.top, rect.right, rect.bottom);

			const RGBAbQuad& leftQuad = mainQuad;
			RGBAbQuad rightQuad { mainQuad.topRight, mainQuad.topLeft, mainQuad.bottomRight, mainQuad.bottomLeft };

			SmoothShade(leftRect, leftQuad, g);
			SmoothShade(rightRect, rightQuad, g);
		}

		void SmoothShade(const GuiRect& rect, const RGBAbQuad& quad, IGRRenderContext& g)
		{
			GRTriangle t[2];
			t[0].a = { TopLeft(rect), quad.topLeft };
			t[0].b = { TopRight(rect), quad.topRight };
			t[0].c = { BottomRight(rect), quad.bottomRight };
			t[1].a = { BottomRight(rect), quad.bottomRight };
			t[1].b = { BottomLeft(rect), quad.bottomLeft };
			t[1].c = { TopLeft(rect), quad.topLeft };

			g.DrawTriangles(t, 2);
		}

		void Render(IGRRenderContext& g) override
		{
			auto fillStyle = panel.FillStyle();
			const auto& rect = panel.AbsRect();

			switch (fillStyle)
			{
			case EGRFillStyle::SMOOTH:
				SmoothShade(rect, mainQuad, g);
				break;
			case EGRFillStyle::BANNER:
				BannerShade(rect, g);
				break;
			case EGRFillStyle::SOLID:
			default:
				g.DrawRect(panel.AbsRect(), mainQuad.topLeft);
				break;
			}
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
			auto result = QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}
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
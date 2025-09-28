#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRIcon : IGRWidgetIcon, IGRWidgetLayout, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		
		IGRImage* image = nullptr;
		HString imagePath;

		GRIcon(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRIcon()
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

		}

		double ImgAspectRatio() const
		{
			Vec2i span = ImageSpan();
			return span.y == 0 ? 0 : (double)span.x / (double)span.y;
		}


		void LayoutAfterExpand() override
		{
			int height = panel.Span().y;
			int width = 0;

			switch (presentation)
			{
			case EGRIconPresentation::ScaleAgainstFixedHeight:
				width = (int) (height * ImgAspectRatio());
				panel.SetConstantWidth(width);
				break;
			}
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
			GRAlignmentFlags alignment;

			if (!image)
			{
				return;
			}

			Vec2i span = image->Span();
			if (span.x == 0 || span.y == 0)
			{
				return;
			}

			GuiRect innerRect = panel.AbsRect();
			innerRect.left += imagePadding.left;
			innerRect.right -= imagePadding.right;
			innerRect.top += imagePadding.top;
			innerRect.bottom -= imagePadding.bottom;
			g.DrawImageStretched(*image, innerRect);
		}

		IGRWidgetIcon& SetImagePath(cstr imagePath) override
		{
			this->imagePath = imagePath ? imagePath : "";
			image = this->imagePath.length() == 0 ? nullptr : panel.Root().Custodian().CreateImageFromPath("portrait", imagePath);
			return *this;
		}

		Vec2i ImageSpan() const override
		{
			return image ? image->Span() : Vec2i{ 0,0 };
		}

		EGRIconPresentation presentation = EGRIconPresentation::ScaleAgainstFixedHeight;

		void SetPresentation(EGRIconPresentation presentation) override
		{
			presentation = EGRIconPresentation::ScaleAgainstFixedHeight;
		}

		GRAnchorPadding imagePadding{ 0,0,0,0 };

		void SetImagePadding(const GRAnchorPadding& padding) override
		{
			imagePadding = padding;
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
			auto result = QueryForParticularInterface<IGRWidgetLayout, GRIcon>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return EGRQueryInterfaceResult::SUCCESS;
			}
			
			return QueryForParticularInterface<IGRWidgetIcon, GRIcon>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRIcon";
		}
	};

	struct GRIconFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRIcon(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetIcon::InterfaceId()
	{
		return "IGRWidgetIcon";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetIcon& CreateIcon(IGRWidget& parent)
	{
		GRANON::GRIconFactory factory;
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), factory);
		IGRWidgetIcon* icon = Cast<IGRWidgetIcon>(widget);
		return *icon;
	}
}
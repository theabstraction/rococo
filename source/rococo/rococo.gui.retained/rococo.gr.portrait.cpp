#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRPortrait : IGRWidgetPortrait, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		
		IGRImage* image = nullptr;
		HString imagePath;

		GRPortrait(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRPortrait()
		{

		}

		void Free() override
		{
			delete this;
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

			double aspectRatio = span.x / (double) span.y;

			GuiRect targetRect = panel.AbsRect();
			Vec2i targetSpan = Span(targetRect);

			if (targetSpan.x == 0 || targetSpan.y == 0)
			{
				return;
			}

			double targetAspectRatio = targetSpan.x / (double)targetSpan.y;

			if (aspectRatio == targetAspectRatio)
			{
				g.DrawImageStretched(*image, targetRect);
				return;
			}

			Vec2i centre = Centre(targetRect);

			if (aspectRatio > targetAspectRatio)
			{
				int32 correctedHeight = (int) (targetSpan.x / aspectRatio);

				targetRect.top = centre.y - correctedHeight / 2;
				targetRect.bottom = centre.y + correctedHeight / 2;

				g.DrawImageStretched(*image, targetRect);
				return;
			}
			else
			{
				int32 correctedWidth = (int) (aspectRatio * targetSpan.y);

				targetRect.left = centre.x - correctedWidth / 2;
				targetRect.right = centre.x + correctedWidth / 2;

				g.DrawImageStretched(*image, targetRect);
				return;
			}
		}

		IGRWidgetPortrait& SetImagePath(cstr imagePath) override
		{
			this->imagePath = imagePath ? imagePath : "";
			image = this->imagePath.length() == 0 ? nullptr : panel.Root().Custodian().CreateImageFromPath("portrait", imagePath);
			return *this;
		}

		Vec2i ImageSpan() const override
		{
			return image ? image->Span() : Vec2i{ 0,0 };
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
			return QueryForParticularInterface<IGRWidgetPortrait, GRPortrait>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRPortrait";
		}
	};

	struct GRPortraitFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRPortrait(panel);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetPortrait::InterfaceId()
	{
		return "IGRWidgetPortrait";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetPortrait& CreatePortrait(IGRWidget& parent)
	{
		GRANON::GRPortraitFactory factory;
		auto& gr = parent.Panel().Root().GR();
		auto& widget = gr.AddWidget(parent.Panel(), factory);
		IGRWidgetPortrait* portrait = Cast<IGRWidgetPortrait>(widget);
		return *portrait;
	}
}
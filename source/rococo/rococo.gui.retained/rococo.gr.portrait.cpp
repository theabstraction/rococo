#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	struct GRPortrait : IGRWidgetPortrait, IGRWidgetLayout, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		IGRImage* image = nullptr;
		HString imagePath;

		IGRWidgetDivision* clientArea;

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

		void PostConstruct()
		{
			clientArea = &CreateDivision(*this);
			panel.SetExpandToParentHorizontally();
			panel.SetExpandToParentVertically();
		}

		void LayoutBeforeFit() override
		{

		}

		GuiRect imageRect{ 0, 0, 0, 0 };
		GuiRect band1Rect{ 0, 0, 0, 0 };
		GuiRect band2Rect{ 0, 0, 0, 0 };

		void ComputeRects()
		{
			GRAlignmentFlags alignment;

			imageRect = GuiRect{ 0, 0, 0, 0 };
			band1Rect = GuiRect{ 0, 0, 0, 0 };
			band2Rect = GuiRect{ 0, 0, 0, 0 };

			if (!image)
			{
				return;
			}

			Vec2i span = image->Span();
			if (span.x == 0 || span.y == 0)
			{
				return;
			}

			double aspectRatio = span.x / (double)span.y;

			GuiRect targetRect = panel.AbsRect();
			Vec2i targetSpan = Span(targetRect);

			if (targetSpan.x == 0 || targetSpan.y == 0)
			{
				return;
			}

			double targetAspectRatio = targetSpan.x / (double)targetSpan.y;

			if (aspectRatio == targetAspectRatio)
			{
				imageRect = targetRect;
				return;
			}

			Vec2i centre = Centre(targetRect);

			if (aspectRatio > targetAspectRatio)
			{
				int32 correctedHeight = (int)(targetSpan.x / aspectRatio);
				targetRect.top = centre.y - correctedHeight / 2;
				targetRect.bottom = centre.y + correctedHeight / 2;
				imageRect = targetRect;
				band1Rect = GuiRect{ targetRect.left, panel.AbsRect().top, targetRect.right, targetRect.top };
				band2Rect = GuiRect{ targetRect.left, targetRect.bottom, targetRect.right, panel.AbsRect().bottom };
			}
			else
			{
				int32 correctedWidth = (int)(aspectRatio * targetSpan.y);
				targetRect.left = centre.x - correctedWidth / 2;
				targetRect.right = centre.x + correctedWidth / 2;
				imageRect = targetRect;
				band1Rect = GuiRect{ panel.AbsRect().left, targetRect.top, targetRect.left, targetRect.bottom };
				band2Rect = GuiRect{ targetRect.right, targetRect.top, panel.AbsRect().right, targetRect.bottom };
			}
		}

		void LayoutBeforeExpand() override
		{
			ComputeRects();
			clientArea->Panel().SetConstantSpan(Span(imageRect));

			GRAnchorPadding padding;
			padding.left = imageRect.left - panel.AbsRect().left;
			padding.top = imageRect.top - panel.AbsRect().top;
			padding.right = 0;
			padding.bottom = 0;
			panel.Set(padding);
		}

		void LayoutAfterExpand() override
		{
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

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		void Render(IGRRenderContext& g) override
		{
			if (image)
			{
				g.DrawImageStretched(*image, imageRect);
			}

			RGBAb bandColour = clientArea->Panel().GetColour(EGRSchemeColourSurface::PORTRAIT_BAND_COLOUR, GRRenderState(false, false, false));
			g.DrawRect(band1Rect, bandColour);
			g.DrawRect(band2Rect, bandColour);
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
			auto result = QueryForParticularInterface<IGRWidgetLayout, GRPortrait>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}
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
			auto* portrait = new GRPortrait(panel);
			portrait->PostConstruct();
			return *portrait;
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
#include "SRococoGRHostWidget.h"
#include "SlateRenderContext.h"

SRococoGRHostWidget::SRococoGRHostWidget()
{
	custodian = Rococo::Gui::Create_UE5_GRCustodian();

	Rococo::Gui::GRConfig config;
	grSystem = Rococo::Gui::CreateGRSystem(config, *custodian);
	custodian->Bind(*grSystem);
}

void SRococoGRHostWidget::Construct(const FArguments& InArgs)
{

}

FVector2D SRococoGRHostWidget::ComputeDesiredSize(float) const
{
	return FVector2D(64, 64);
}

int32 SRococoGRHostWidget::OnPaint(const FPaintArgs& args,
	const FGeometry& allottedGeometry,
	const FSlateRect& cullingRect,
	OUT FSlateWindowElementList& drawElements,
	int32 layerId,
	const FWidgetStyle& widgetStyle,
	bool bParentEnabled) const
{
	Rococo::SlateRenderContext rc{ args, allottedGeometry, cullingRect, drawElements, layerId, widgetStyle, bParentEnabled };

	custodian->Render(rc);

	return 0;
}
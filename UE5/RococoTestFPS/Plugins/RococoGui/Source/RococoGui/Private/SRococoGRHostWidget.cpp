#include "SRococoGRHostWidget.h"
#include "SlateRenderContext.h"

#include <rococo.os.h>
#include <rococo.great.sex.h>
#include <rococo.allocators.h>

static Rococo::Gui::GRIdWidget s_HostFrame { "SRococoGRHostWidget.cpp-HostFrame" };

SRococoGRHostWidget::SRococoGRHostWidget()
{
	try
	{
		custodian = Rococo::Gui::Create_UE5_GRCustodian();
		Rococo::Gui::GRConfig config;
		grSystem = CreateGRSystem(config, *custodian);
		custodian->Bind(*grSystem);
	}
	catch (Rococo::IException& ex)
	{
		Rococo::LogExceptionAndQuit(ex, nullptr, nullptr);
	}
}

void SRococoGRHostWidget::Construct(const FArguments& InArgs)
{

}

FVector2D SRococoGRHostWidget::ComputeDesiredSize(float) const
{
	return FVector2D(64, 64);
}

void ClearFrame(Rococo::Gui::IGRWidgetMainFrame& frame)
{
}

void SRococoGRHostWidget::LoadFrame(const char* sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading)
{
	Rococo::AutoFree<Rococo::IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(128, 0, "sexml-allocator");
	
	struct ErrorHandler : Rococo::IEventCallback<Rococo::GreatSex::LoadFrameException>
	{
		Rococo::Gui::IUE5_GRCustodianSupervisor* custodian = nullptr;

		void OnEvent(Rococo::GreatSex::LoadFrameException& lfe)
		{
			custodian->AddLoadError(lfe);
		}
	} onError;

	onError.custodian = custodian;

	auto& frame = grSystem->BindFrame(s_HostFrame);
	ClearFrame(frame);
	Rococo::GreatSex::LoadFrame(custodian->Installation(), *allocator, sexmlPingPath, frame, onPrepForLoading, onError);
}

void DrawBackground(Rococo::SlateRenderContext& rc, const FWidgetStyle& style)
{
	ESlateDrawEffect drawEffects = rc.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	FSlateColorBrush solidBrush(rc.bEnabled ? style.GetForegroundColor() : style.GetSubduedForegroundColor());

	/*
	FSlateDrawElement::MakeBox(OUT rc.drawElements,
		++rc.layerId,
		rc.geometry,
		&solidBrush,
		drawEffects,
		solidBrush.GetTint(style)
	);
	*/
}

int32 SRococoGRHostWidget::OnPaint(const FPaintArgs& args,
	const FGeometry& allottedGeometry,
	const FSlateRect& cullingRect,
	OUT FSlateWindowElementList& drawElements,
	int32 layerId,
	const FWidgetStyle& widgetStyle,
	bool bParentEnabled) const
{
	bool bEnabled = ShouldBeEnabled(bParentEnabled);
	
	Rococo::SlateRenderContext rc{ args, allottedGeometry.ToPaintGeometry(), cullingRect, drawElements, layerId, widgetStyle, bEnabled};

	DrawBackground(rc, widgetStyle);

	custodian->Render(rc);

	return rc.layerId;
}
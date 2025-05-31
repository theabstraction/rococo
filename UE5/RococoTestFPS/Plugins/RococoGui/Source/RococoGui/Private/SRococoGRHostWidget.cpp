#include "SRococoGRHostWidget.h"
#include "SlateRenderContext.h"

#include <rococo.os.h>
#include <rococo.great.sex.h>
#include <rococo.allocators.h>

static Rococo::Gui::GRIdWidget s_HostFrame { "SRococoGRHostWidget.cpp-HostFrame" };

SRococoGRHostWidget::SRococoGRHostWidget()
{
}

void SRococoGRHostWidget::Construct(const FArguments& InArgs)
{

}

void SRococoGRHostWidget::SyncCustodian(TMapPathToTexture& mapPathToTexture, const FString& fontDirectory)
{
	try
	{
		if (!custodian)
		{
			custodian = Rococo::Gui::Create_UE5_GRCustodian(mapPathToTexture, fontDirectory);
			Rococo::Gui::GRConfig config;
			grSystem = CreateGRSystem(config, *custodian);
			custodian->Bind(*grSystem);
		}
	}
	catch (Rococo::IException& ex)
	{
		Rococo::LogExceptionAndQuit(ex, nullptr, nullptr);
	}
}

FVector2D SRococoGRHostWidget::ComputeDesiredSize(float) const
{
	return FVector2D(64, 64);
}

void UseTestColourScheme(Rococo::Gui::IGRWidgetMainFrame& frame)
{
	auto& framePanel = frame.Panel();

	using namespace Rococo;
	using namespace Rococo::Gui;

	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(192, 192, 192, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(160, 160, 160, 255), GRWRS());
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(225, 225, 225, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(32, 32, 32, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(224, 224, 224, 255), EGRColourSpec::ForAllRenderStates);
	framePanel.Set(EGRSchemeColourSurface::READ_ONLY_TEXT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_EVEN, RGBAb(240, 240, 240));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::ROW_COLOUR_ODD, RGBAb(255, 255, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::NAME_TEXT, RGBAb(0, 0, 0, 255));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::LABEL_BACKGROUND, RGBAb(255, 255, 255, 0));
	MakeTransparent(framePanel, EGRSchemeColourSurface::LABEL_SHADOW);
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDITOR, RGBAb(192, 192, 192));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::EDIT_TEXT, RGBAb(0, 0, 0));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN, RGBAb(255, 240, 240));
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD, RGBAb(240, 255, 240));
	framePanel.Set(EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllFocusedStates);
	framePanel.Set(EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllFocusedStates);
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_TEXT, RGBAb(0, 0, 0, 255));
	MakeTransparent(framePanel, EGRSchemeColourSurface::COLLAPSER_TITLE_SHADOW);
	SetUniformColourForAllRenderStates(framePanel, EGRSchemeColourSurface::VALUE_TEXT, RGBAb(0, 0, 0, 255));
	MakeTransparent(framePanel, EGRSchemeColourSurface::BUTTON_IMAGE_FOG);
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 32), GRWidgetRenderState(0, 1, 0));
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 48), GRWidgetRenderState(0, 0, 1));
	framePanel.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 64), GRWidgetRenderState(0, 1, 1));

	frame.Panel().Set(EGRSchemeColourSurface::BACKGROUND, RGBAb(255, 0, 0, 0), EGRColourSpec::ForAllRenderStates);
}

void ClearFrame(Rococo::Gui::IGRWidgetMainFrame& frame)
{
	struct : Rococo::IEventCallback<Rococo::Gui::IGRPanel>
	{
		void OnEvent(Rococo::Gui::IGRPanel& panel) override
		{
			panel.MarkForDelete();
		}
	} cb;
	frame.ClientArea().Panel().EnumerateChildren(&cb);
	frame.ClientArea().Panel().Root().GR().GarbageCollect();

	auto& scheme = frame.Panel().Root().Scheme();
	SetSchemeColours_ThemeGrey(scheme);
	UseTestColourScheme(frame);
}

void SRococoGRHostWidget::LoadFrame(const char* sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading)
{
	if (!custodian)
	{
		return;
	}

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
	
	Rococo::SlateRenderContext rc{ args, allottedGeometry, cullingRect, drawElements, layerId, widgetStyle, bEnabled};

	DrawBackground(rc, widgetStyle);

	if (custodian)
	{
		custodian->Render(rc);
	}

	return rc.layerId;
}
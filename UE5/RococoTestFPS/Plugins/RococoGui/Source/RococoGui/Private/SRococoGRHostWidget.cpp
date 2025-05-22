#include "SRococoGRHostWidget.h"
#include "SlateRenderContext.h"

#include <rococo.great.sex.h>
#include <rococo.allocators.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::GreatSex;

struct ResourceLoader: IGreatSexResourceLoader
{
	void LoadGreatSexResource(cstr resourcePath, Rococo::IO::ILoadEventsCallback& onLoad) override
	{
		Throw(0, __FUNCTION__ ": Not implemented");
	}
};

static GRIdWidget s_HostFrame { "SRococoGRHostWidget.cpp-HostFrame" };

SRococoGRHostWidget::SRococoGRHostWidget()
{
	custodian = Create_UE5_GRCustodian();
	Rococo::Gui::GRConfig config;
	grSystem = CreateGRSystem(config, *custodian);
	custodian->Bind(*grSystem);

	grSystem->BindFrame(s_HostFrame);
}

void SRococoGRHostWidget::Construct(const FArguments& InArgs)
{

}

FVector2D SRococoGRHostWidget::ComputeDesiredSize(float) const
{
	return FVector2D(64, 64);
}

void ClearFrame(IGRWidgetMainFrame& frame)
{
}

void SRococoGRHostWidget::LoadFrame(const char* sexmlPingPath, IEventCallback<IGreatSexGenerator>& onPrepForLoading)
{
	AutoFree<IAllocatorSupervisor> allocator = Rococo::Memory::CreateBlockAllocator(128, 0, "sexml-allocator");
	
	struct ErrorHandler : IEventCallback<LoadFrameException>
	{
		IUE5_GRCustodianSupervisor* custodian = nullptr;

		void OnEvent(LoadFrameException& lfe)
		{
			custodian->AddLoadError(lfe);
		}
	} onError;

	onError.custodian = custodian;

	auto& frame = grSystem->BindFrame(s_HostFrame);
	ClearFrame(frame);
	GreatSex::LoadFrame(custodian->Installation(), *allocator, sexmlPingPath, frame, onPrepForLoading, onError);
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
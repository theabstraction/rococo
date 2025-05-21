#include "RococoGRHostWidget.h"

void URococoGRHostWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	slateHostWidget.Reset();
}

TSharedRef<SWidget> URococoGRHostWidget::RebuildWidget()
{
	TSharedRef<SRococoGRHostWidget> host = SNew(SRococoGRHostWidget);
	return host;
}
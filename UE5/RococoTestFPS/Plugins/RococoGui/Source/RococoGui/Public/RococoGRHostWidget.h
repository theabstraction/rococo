#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include <Widgets/SWidget.h>
#include "SRococoGRHostWidget.h"

#include "RococoGuiAPI.h"
#include "RococoGRHostWidget.generated.h"

UCLASS(Experimental, meta = (DisplayName = "RococoGRHostWidget (Object)"))
class ROCOCOGUI_API URococoGRHostWidget : public UUserWidget
{
public:
	GENERATED_BODY()

	void ReleaseSlateResources(bool bReleaseChildren) override;
	TSharedRef<SWidget> RebuildWidget() override;

protected:
	TSharedPtr<SRococoGRHostWidget> slateHostWidget;
};

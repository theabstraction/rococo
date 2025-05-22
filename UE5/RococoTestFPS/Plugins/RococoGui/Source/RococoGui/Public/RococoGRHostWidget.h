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

	void LoadFrame(const FString& sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);
	void LoadFrame(const char* pingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);
protected:
	TSharedPtr<SRococoGRHostWidget> slateHostWidget;
};

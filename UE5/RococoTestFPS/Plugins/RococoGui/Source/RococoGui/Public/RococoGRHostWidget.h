#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include <Widgets/SWidget.h>
#include "SRococoGRHostWidget.h"

#include "RococoGuiAPI.h"
#include "RococoGRHostWidget.generated.h"

// The basic Rococo Gui Retained host widget. Needs C++ to get anywhere. For a blueprint driven system use URococoGRHostWidgetBuilder
UCLASS(BlueprintType, meta = (DisplayName = "RococoGRHostWidget (Object)"))
class ROCOCOGUI_API URococoGRHostWidget : public UUserWidget
{
public:
	GENERATED_BODY()

	void ReleaseSlateResources(bool bReleaseChildren) override;
	TSharedRef<SWidget> RebuildWidget() override;

	// FString/TCHAR version of LoadFrame.
	void LoadFrame(const FString& sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);

	// Tries to load a GreatSex seml file into the widget. OnPrepForLoading::OnEvent is called which allows the API consumer
	// to add handlers for custom widgets and as well as bind game options for use by Rococo game option widgets.
	void LoadFrame(const char* sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);

	// Returns a reference to the currently constructed GR custodian from the slate widget. Invalidated on a widget rebuild or destruction. Do not cache!
	// Custodian functions and anything linked to them may throw exceptions, so only call in a pluin with exceptions enabled and captured
	Rococo::Gui::IUE5_GRCustodianSupervisor* GetCurrentCustodian();
protected:
	TSharedPtr<SRococoGRHostWidget> slateHostWidget;

	// This caches the Rococo::Gui textures. The slate widget is volatile, so perhaps is not appropriate
	// We pass it to the slate widget by calling slateHostWidget->SyncCustodian(...) inside of RebuildWidget
	UPROPERTY(Transient)
	TMap<FString, UTexture2D*> mapPathToTexture;

	// The location where fonts are expected.
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "URococoFontSet"))
	FSoftObjectPath _FontAsset;
};

typedef void (*FN_GlobalPrepGenerator)(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);

// Rococo Gui retained host widget. Designed to be scripted in Blueprints 
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RococoGRHostWidgetBuilder (Object)"))
class ROCOCOGUI_API URococoGRHostWidgetBuilder : public URococoGRHostWidget
{
public:
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	FString _SexmlPingPath = TEXT("!tests/greatsex.test.sexml");

	// If set to true the builder will prep the GUI generator with a global function
	// Your project must define a C++ function void GlobalPrepGenerator(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);
	// The reference returned by that method must remain valid for the lifetime of the widget
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	bool _UseGlobalOptions = true;

	// Allows void PrepGlobalGenerator(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator) to be tuned according to this property
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	FString _GlobalOptionsKey = TEXT("default");

	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void ReloadFrame();

	// Tells the RococoGUI widget tree to handle a mouse mouse down event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	// Tells the RococoGUI widget tree to handle a mouse up event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
private:
	void OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator);
};
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
protected:
	TSharedPtr<SRococoGRHostWidget> slateHostWidget;

	// This caches the Rococo::Gui textures. The slate widget is volatile, so perhaps is not appropriate
	// We pass it to the slate widget by calling slateHostWidget->SyncCustodian(...) inside of RebuildWidget
	UPROPERTY(Transient)
	TMap<FString, UTexture2D*> mapPathToTexture;
};

typedef void (*FN_GlobalPrepGenerator)(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);

// Rococo Gui retained host widget. Designed to be scripted in Blueprints 
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RococoGRHostWidgetBuilder (Object)"))
class ROCOCOGUI_API URococoGRHostWidgetBuilder : public URococoGRHostWidget
{
public:
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Rococo.Gui")
	FString _SexmlPingPath = TEXT("!tests/greatsex.test.sexml");

	// If set to true the builder will prep the GUI generator with a global function
	// Your project must define a C++ function void GlobalPrepGenerator(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);
	// The reference returned by that method must remain valid for the lifetime of the widget
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Rococo.Gui")
	bool _UseGlobalOptions = true;

	// Allows void PrepGlobalGenerator(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator) to be tuned according to this property
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Rococo.Gui")
	FString _GlobalOptionsKey = TEXT("default");

	UFUNCTION(BlueprintCallable, Category = "Rococo.Gui")
	void ReloadFrame();
private:
	void OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator);
};
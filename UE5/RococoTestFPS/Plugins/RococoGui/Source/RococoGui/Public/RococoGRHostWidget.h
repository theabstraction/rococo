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
	// Custodian functions and anything linked to them may throw exceptions, so only call in a plugin with exceptions enabled and captured
	// Potentially returns nullptr if custodian could not be constructed or was not constructed.
	Rococo::Gui::IUE5_GRCustodianSupervisor* GetCurrentCustodian();

	// C++ is needed to set the event handler. If required, pass the widget to a C++ method that invokes this function.
	// The event handler must be valid for the lifetime of the widget. The event handler will be used following a RebuildWidget.
	// Thus invoke the method before adding the widget to a viewport
	void SetEventHandler(ISRococoGRHostWidgetEventHandler* slateEventHandler)
	{
		this->_SlateEventHandler = slateEventHandler;
	}
protected:
	TSharedPtr<SRococoGRHostWidget> slateHostWidget;

	// This caches the Rococo::Gui textures. The slate widget is volatile, so perhaps is not appropriate
	// We pass it to the slate widget by calling slateHostWidget->SyncCustodian(...) inside of RebuildWidget
	UPROPERTY(Transient)
	TMap<FString, UTexture2D*> mapPathToTexture;

	// The location where fonts are expected.
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "RococoFontSet"), Category = "RococoGui")
	FSoftObjectPath _FontAsset;

	// Defaults to true. If sets to true will use Rococo::Gui::GetDefaultFocusRenderer to hilight the focused widget
	// To implement your own, set false and override OnGRSystemConstructed(...) to call Rococo::Gui::IGRSystem::SetFocusOverlayRenderer
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	bool _UseDefaultFocusRenderer = true;

	ISRococoGRHostWidgetEventHandler* _SlateEventHandler = nullptr;
};

typedef void (*FN_GlobalPrepGenerator)(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator);

// Rococo Gui retained host widget. Designed to be scripted in Blueprints 
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RococoGRHostWidgetBuilder (Object)"))
class ROCOCOGUI_API URococoGRHostWidgetBuilder : public URococoGRHostWidget
{
public:
	GENERATED_BODY()

	// Specifies the sexml to load when the slate widget within is rebuilt
	// As with all ping paths the ping (!) represents the rococo content directory
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	FString _SexmlPingPath = TEXT("!tests/greatsex.test.sexml");

	// If set to true the builder will prep the GUI generator with a global function
	// Your project must call 'void Rococo::Gui::SetGlobalPrepGenerator(FN_GlobalPrepGenerator fnGlobalPrepGenerator)' 
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	bool _UseGlobalOptions = true;

	// Allows void PrepGlobalGenerator(const FString& key, Rococo::GreatSex::IGreatSexGenerator& generator) to be tuned according to this property
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	FString _GlobalOptionsKey = TEXT("default");

	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void ReloadFrame();

	// Tells the RococoGUI widget tree to handle a mouse mouse down event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// Tells the RococoGUI widget tree to handle a mouse up event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// Tells the RococoGUI widget tree to handle a mouse move event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// Tells the RococoGUI widget tree to handle a key-press or key-repeat event
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteKeyDown(const FGeometry& MyGeometry, FKeyEvent InKeyEvent);

	// Tells the RococoGUI widget tree to handle a key-release
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteKeyUp(const FGeometry& MyGeometry, FKeyEvent InKeyEvent);

	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
private:
	void OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator);
};
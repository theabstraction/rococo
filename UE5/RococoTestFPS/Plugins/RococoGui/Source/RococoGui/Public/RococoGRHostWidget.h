// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#pragma once

#include <CoreMinimal.h>
#include <Blueprint/UserWidget.h>

#include <Widgets/SWidget.h>
#include "SRococoGRHostWidget.h"

#include "RococoGui_BPEvents.h"

#include "RococoGuiAPI.h"
#include "RococoGRHostWidget.generated.h"

UENUM(BlueprintType)
enum class RococoControlCategory: uint8
{
	NONE,
	KEYBOARD,
	XBOX,
	PLAYSTATION
};

UENUM(BlueprintType)
enum class RococoSelectionChangeOrigin : uint8
{
	None,
	VMenuKeyNav,
	CarouselLRArrows,
	ButtonClick,
	ScalarChangeKey
};

// The basic Rococo Gui Retained host widget. Needs C++ to get anywhere. For a blueprint driven system use URococoGRHostWidgetBuilder
UCLASS(BlueprintType, meta = (DisplayName = "RococoGRHostWidget (Object)"))
class ROCOCO_GUI_API URococoGRHostWidget : public UUserWidget, public Rococo::Gui::IUE5_GlobalFontMetrics, public Rococo::Gui::IGRSelectionChangeHandler, public Rococo::Gui::IGREventHandler
{
public:
	GENERATED_BODY()

	void ReleaseSlateResources(bool bReleaseChildren) override;
	TSharedRef<SWidget> RebuildWidget() override;

	// FString/TCHAR version of LoadFrame.
	void LoadFrame(const FString& sexmlPingPath, Rococo::IEventCallback<Rococo::GreatSex::IGreatSexGenerator>& onPrepForLoading);

	// Tries to load a GreatSex sexml file into the host widget. The method invokes onPrepForLoading.OnEvent which allows the API consumer
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

	void SetCustodianManager(Rococo::Gui::IUE5_GlobalFontMetrics* metricsManager)
	{
		_GlobalFontMetrics = metricsManager;
	}

	void OnSelectionChanged(Rococo::Gui::IGRPanel& panel, Rococo::Gui::EGRSelectionChangeOrigin origin) override;

	Rococo::Gui::EGREventRouting OnGREvent(Rococo::Gui::GRWidgetEvent& ev) override;
protected:
	TSharedPtr<SRococoGRHostWidget> _SlateHostWidget;

	FDelegateHandle WindowActionNotificationHandle;

	// This caches the Rococo::Gui textures. The slate widget is volatile, so perhaps is not appropriate for persisting textures.
	// We pass it to the slate widget by calling slateHostWidget->SyncCustodian(...) inside of RebuildWidget
	UPROPERTY(Transient)
	TMap<FString, UTexture2D*> _MapPathToTexture;

	// The event handler methods are called by reflection in response to GR_PumpMessages
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	TObjectPtr<UObject> _GREventHandler;

	// Multiplier of the Rococo point size value for fonts to create the UE5 point size. The default value was determined empircally to best approximate the UE5
	// fonts to those of the Rococo GDI test application. The value is capped between 0.2 and 8.0
	UPROPERTY(EditAnywhere, Category = "RococoGui")
	float _FontPointSizeRatio = 0.7826f;

	// The location where fonts are expected.
	UPROPERTY(EditAnywhere, meta = (AllowedClasses = "/Script/RococoGui.RococoFontSet"), Category = "RococoGui")
	FSoftObjectPath _FontAsset;

	// Set to true to enable logging of GUI events to the screen
	UPROPERTY(EditAnywhere, Category = "RococoGui")
	bool _LogToScreen = false;

	// Set to true to enable logging of GUI events to the log
	UPROPERTY(EditAnywhere, Category = "RococoGui")
	bool _LogToFile = false;

	// Defaults to true. If set to true will use Rococo::Gui::GetDefaultFocusRenderer to hilight the focused widget
	// To implement your own, set false and override OnGRSystemConstructed(...) to call Rococo::Gui::IGRSystem::SetFocusOverlayRenderer
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	bool _UseDefaultFocusRenderer = true;

	ISRococoGRHostWidgetEventHandler* _SlateEventHandler = nullptr;

	Rococo::Gui::IUE5_GlobalFontMetrics* _GlobalFontMetrics = nullptr;

	int GetUE5PointSize(int rococoPointSize) override;

	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void SetControlCategory(RococoControlCategory category);

	// This should be called periodically from the main thread, it will invoke the _GREventHandler with messages from the GR message queue.
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void GRPumpMessages();

	RococoControlCategory lastCategory = RococoControlCategory::NONE;

	UFUNCTION(BlueprintImplementableEvent, Category = "RococoGui")
	void EvSelectionChanged(RococoSelectionChangeOrigin origin);
};

typedef void (*FN_GlobalPrepGenerator)(Rococo::GreatSex::IReflectedGameOptionsBuilder& builder, const TArray<UObject*>& context, Rococo::GreatSex::IGreatSexGenerator& generator);

// Rococo Gui retained host widget. Designed to be scripted in Blueprints 
UCLASS(BlueprintType, Blueprintable, meta = (DisplayName = "RococoGRHostWidgetBuilder (Object)"))
class ROCOCO_GUI_API URococoGRHostWidgetBuilder : public URococoGRHostWidget
{
public:
	GENERATED_BODY()

	URococoGRHostWidgetBuilder();
	~URococoGRHostWidgetBuilder();

	// Specifies the sexml to load when the slate widget within is rebuilt
	// As with all ping paths the ping (!) represents the rococo content directory
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	FString _SexmlPingPath = TEXT("!tests/greatsex.test.sexml");

	// If set to true the builder will prep the GUI generator with a global function
	// Your project must call 'void Rococo::Gui::SetGlobalPrepGenerator(FN_GlobalPrepGenerator fnGlobalPrepGenerator) before ReloadFrame' 
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	bool _UseGlobalOptions = true;

	// Passed to void PrepGlobalGenerator(const TArray<UObject*>& context, Rococo::GreatSex::IGreatSexGenerator& generator) as the first argument
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	TArray<UObject*> _GeneratorContext;

	// The emitted event handler, invoked when the gui widget does not trap the event (FEventReply.Unhandled)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RococoGui")
	TScriptInterface<IRococoEmittedUIEventHandler> _EmittedEventHandler;

	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void ReloadFrame();

	// Set the tab position to the first available focus slot
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	void FocusDefaultTab();

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

	// Tells the RococoGUI widget tree to handle a mouse wheel spin
	UFUNCTION(BlueprintCallable, Category = "RococoGui")
	FEventReply RouteMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
private:
	void OnPrepForLoading(Rococo::GreatSex::IGreatSexGenerator& generator);
	
	Rococo::GreatSex::IReflectedGameOptionsBuilder* optionsBuilder;
};
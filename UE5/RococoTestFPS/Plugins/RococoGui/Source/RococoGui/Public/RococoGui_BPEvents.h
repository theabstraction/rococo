#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RococoGui_BPEvents.generated.h"

UINTERFACE(Blueprintable)
class ROCOCO_GUI_API URococoReflectionEventHandler : public UInterface
{
	GENERATED_BODY()
};

class ROCOCO_GUI_API IRococoReflectionEventHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnBadMethod(const FString& errMsg, const FString& methodMsg, const FString& propertyMsg);

	virtual void RaiseBadMethod(const FString& errMsg, const FString& methodMsg, const FString& propertyMsg);
};

UENUM(BlueprintType)
enum class RococoGREventCode : uint8
{
	// Copied and pasted from rococo.gui.retained.h, Rococo::Gui::EGRWidgetEventType
	BUTTON_CLICK,
	BUTTON_KEYPRESS_DOWN, // // A key was pressed while the button had focus, and the meta data contains the vkey code
	BUTTON_KEYPRESS_UP, // A key was released while the button had focus, and the meta data contains the vkey code
	BOOL_CHANGED, // A boolean valued control was changed. The meta data int is 0 for false and not zero for true
	CHOICE_MADE, // A choice was selected, the meta data contains the key
	EDITOR_UPDATED, // Cast WidgetEvent to WidgetEvent_EditorUpdated
	DROP_DOWN_COLLAPSED, // The drop down control collapsed
	DROP_DOWN_EXPANDED, // The drop down control expanded
	BUTTON_CLICK_OUTSIDE, // A control captured a mouse click outside of its panel's AbsRect
	SCROLLER_RELEASED, // A scroll button was released by letting go of the mouse button 
	ON_HINT_HOVER, // A hint was hovered with a mouse move event
	GET_HINT_HOVER, // Retrieve the last hint that was hovered over
	SLIDER_HELD, // A slider was clicked down with the cursor
	SLIDER_NEW_POS, // Slider position changed
	UPDATED_CLIENTAREA_HEIGHT, // A viewport client-area control calculated its new height (passed to iMetaData). The viewport caches this and applies it during the next layout
	ARE_DESCENDANTS_OBSCURED,
	USER_DEFINED
};

USTRUCT(BlueprintType)
struct FRococoGREvent
{
	GENERATED_BODY()

	// Gives the cause of the event.
	UPROPERTY(BlueprintReadOnly)
	RococoGREventCode EventCode = RococoGREventCode::USER_DEFINED;

	// An integer version of the event code. In the event of USER_DEFINED codes, the value is 1025 + offset
	UPROPERTY(BlueprintReadOnly)
	int extendedEventCode = 0;

	// An integer id of the widget sending the event
	UPROPERTY(BlueprintReadOnly)
	int PanelId = -1;

	// Some numeric data associated with the event
	UPROPERTY(BlueprintReadOnly)
	int MetaDataInt = 0;

	// Gives the sender's panel description
	UPROPERTY(BlueprintReadOnly)
	FString SenderDesc;

	// Some meta data string associated with the event
	UPROPERTY(BlueprintReadOnly)
	FString MetaDataString;
};

UINTERFACE(Blueprintable)
class ROCOCO_GUI_API URococoGlobalUIEventHandler : public UInterface
{
	GENERATED_BODY()
};

class ROCOCO_GUI_API IRococoGlobalUIEventHandler
{
	GENERATED_BODY()

public:
	// A RococoGUI widget tree did not handle a mouse down event
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// A RococoGUI widget tree did not handle a mouse up event
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// A RococoGUI widget tree did not handle a mouse move event
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// A RococoGUI widget tree did not handle a mouse wheel event
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	// A RococoGUI widget tree did not handle a key-press or key-repeat event
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalKeyDown(const FGeometry& MyGeometry, FKeyEvent InKeyEvent);

	// A RococoGUI widget tree did not handle a key-release
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply OnGlobalKeyUp(const FGeometry& MyGeometry, FKeyEvent InKeyEvent);
};
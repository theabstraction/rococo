#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameOptionBuilder.generated.h"

UINTERFACE(Blueprintable)
class ROCOCO_GUI_API URococoGameOptionBuilder : public UInterface
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FRococoGameOptionChoiceQuantum
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Hint;

	UPROPERTY(EditAnywhere)
	FString Id;

	UPROPERTY(EditAnywhere)
	FString Text;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionChoice
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere)
	int SortOrder = 0;

	UPROPERTY(EditAnywhere)
	FString Hint;

	UPROPERTY(EditAnywhere)
	FString Title;

	UPROPERTY(EditAnywhere)
	TArray<FRococoGameOptionChoiceQuantum> Items;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionBool
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere)
	int SortOrder = 0;

	UPROPERTY(EditAnywhere)
	FString Hint;

	UPROPERTY(EditAnywhere)
	FString Title;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionScalar
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere)
	int SortOrder = 0;

	UPROPERTY(EditAnywhere)
	FString Hint;

	UPROPERTY(EditAnywhere)
	FString Title;

	UPROPERTY(EditAnywhere)
	double MinValue = 0.0;

	UPROPERTY(EditAnywhere)
	double MaxValue = 200.0;

	// The smallest increment or decrement
	UPROPERTY(EditAnywhere)
	double QuantumDelta = 1.0;

	// The number of digits after the decimal point to display
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0, UIMax = 9))
	int DecimalPlaces = 2;
};


class ROCOCO_GUI_API IRococoGameOptionBuilder
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);

	virtual void RaiseError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);

	UFUNCTION(BlueprintImplementableEvent)
	FString GetOptionId();

	virtual FString RaiseGetOptionId();

	UFUNCTION(BlueprintImplementableEvent)
	void OnRevert();

	virtual void Revert();

	UFUNCTION(BlueprintImplementableEvent)
	void OnAccept();

	virtual void Accept();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void InitOptions();

	virtual void InvokeInitOptions();
};

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
	void OnError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);

	virtual void RaiseError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);
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
	RococoGREventCode EventCode;

	// An integer version of the event code. In the event of USER_DEFINED codes, the value is 1025 + offset
	UPROPERTY(BlueprintReadOnly)
	int extendedEventCode;

	// An integer id of the widget sending the event
	UPROPERTY(BlueprintReadOnly)
	int PanelId;

	// Some numeric data associated with the event
	UPROPERTY(BlueprintReadOnly)
	int MetaDataInt;

	// Gives the sender's panel description
	UPROPERTY(BlueprintReadOnly)
	FString SenderDesc;

	// Some meta data string associated with the event
	UPROPERTY(BlueprintReadOnly)
	FString MetaDataString;
};
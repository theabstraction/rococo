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
};
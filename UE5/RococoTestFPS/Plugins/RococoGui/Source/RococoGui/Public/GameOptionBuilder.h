#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameOptionBuilder.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class URococoGameOptionBuilder : public UInterface
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

class IRococoGameOptionBuilder
{
	GENERATED_BODY()

	virtual TArray<FRococoGameOptionChoice> GetChoices();
};
// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
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

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Hint;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Id;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Text;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionChoice
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	int SortOrder = 0;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Hint;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Title;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	TArray<FRococoGameOptionChoiceQuantum> Items;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionBool
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	int SortOrder = 0;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Hint;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Title;
};

USTRUCT(BlueprintType)
struct FRococoGameOptionScalar
{
	GENERATED_BODY()

	// Gives the sort order, or priority of the choice.
	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	int SortOrder = 0;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Hint;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	FString Title;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	double MinValue = 0.0;

	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	double MaxValue = 200.0;

	// The smallest increment or decrement
	UPROPERTY(EditAnywhere, Category = "Rococo Options")
	double QuantumDelta = 1.0;

	// The number of digits after the decimal point to display
	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0, UIMax = 9), Category = "Rococo Options")
	int DecimalPlaces = 2;
};


class ROCOCO_GUI_API IRococoGameOptionBuilder
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Rococo Options")
	void OnError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);

	virtual void RaiseError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rococo Options")
	FString GetOptionId();

	virtual FString RaiseGetOptionId();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rococo Options")
	void OnRevert();

	virtual void Revert();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rococo Options")
	void OnAccept();

	virtual void Accept();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Rococo Options")
	void InitOptions();

	virtual void InvokeInitOptions();
};

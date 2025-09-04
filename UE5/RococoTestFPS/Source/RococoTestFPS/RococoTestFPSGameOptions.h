#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RococoTestFPSGameOptions.generated.h"

UCLASS()
class ROCOCOTESTFPS_API URococoTestFPSGameOptionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// Retrieves the volume level 0 - 1 of the background music
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio")
	static double GetMusicVolume();

	// Retrieves the volume level 0 - 1 of the narrator
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio")
	static double GetNarrationVolume();

	// Retrieves the volume level 0 - 1 of the UI FX 
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio")
	static double GetFXVolume();
};
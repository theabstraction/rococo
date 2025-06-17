// Copyright Epic Games, Inc. All Rights Reserved.

#include "RococoTestFPSGameMode.h"
#include "RococoTestFPSCharacter.h"
#include "UObject/ConstructorHelpers.h"

ARococoTestFPSGameMode::ARococoTestFPSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "RococoJPEGLib.h"

#define LOCTEXT_NAMESPACE "FRococoJPEGLibModule"

void FRococoJPEGLibModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRococoJPEGLibModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRococoJPEGLibModule, RococoJPEGLib)
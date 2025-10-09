// Copyright Epic Games, Inc. All Rights Reserved.

#include "RococoBuild.h"

#define LOCTEXT_NAMESPACE "FRococoBuildModule"

void FRococoBuildModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FRococoBuildModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRococoBuildModule, RococoBuild)
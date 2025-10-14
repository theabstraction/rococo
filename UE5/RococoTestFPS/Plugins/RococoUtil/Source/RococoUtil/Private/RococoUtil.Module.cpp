// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.

#include "RococoUtil.Module.h"

#define LOCTEXT_NAMESPACE "FRococoUtilModule"

void FRococoUtilModule::StartupModule()
{
}

void FRococoUtilModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRococoUtilModule, RococoUtil)
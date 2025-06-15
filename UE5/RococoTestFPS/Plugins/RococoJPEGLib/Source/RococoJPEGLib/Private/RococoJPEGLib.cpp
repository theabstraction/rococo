// Copyright Epic Games, Inc. All Rights Reserved.

#include "RococoJPEGLib.h"

#define LOCTEXT_NAMESPACE "FRococoJPEGLibModule"

#ifndef _WIN32
int _vsnprintf_s(char* message, size_t capacity, size_t nBytesToWrite, const char* format, va_list args)
{
	size_t size;

	if (nBytesToWrite == _TRUNCATE)
	{
		size = capacity;
	}
	else
	{
		size = nBytesToWrite < capacity ? nBytesToWrite : capacity;
	}

	return vsnprintf(message, size, format, args);
}
#endif

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
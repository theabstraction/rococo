// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#include <RococoGui_BPevents.h>

void IRococoReflectionEventHandler::RaiseBadMethod(const FString& errMsg, const FString& methodMsg, const FString& propMsg)
{
	OnBadMethod(errMsg, methodMsg, propMsg);
}

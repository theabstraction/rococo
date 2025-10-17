// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#include <GameOptionBuilder.h>

void IRococoGameOptionBuilder::RaiseError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg)
{
	OnError(methodMsg, propertyMsg, errMsg);
}

void IRococoGameOptionBuilder::Accept()
{
	OnAccept();
}

void IRococoGameOptionBuilder::Revert()
{
	OnRevert();
}

void IRococoGameOptionBuilder::InvokeInitOptions()
{
	InitOptions();
}

FString IRococoGameOptionBuilder::RaiseGetOptionId()
{
	return TEXT("IRococoGameOptionBuilder: RaiseGetOptionId - default");
}

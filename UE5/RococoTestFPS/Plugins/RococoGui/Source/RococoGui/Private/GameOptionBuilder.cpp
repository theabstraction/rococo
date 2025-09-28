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

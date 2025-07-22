#include <GameOptionBuilder.h>

void IRococoGameOptionBuilder::RaiseError(const FString& methodMsg, const FString& propertyMsg, const FString& errMsg)
{
	OnError(methodMsg, propertyMsg, errMsg);
}

FString IRococoGameOptionBuilder::RaiseGetOptionId()
{
	return TEXT("IRococoGameOptionBuilder: RaiseGetOptionId - default");
}

#include <RococoGui_BPevents.h>

void IRococoReflectionEventHandler::RaiseBadMethod(const FString& errMsg, const FString& methodMsg, const FString& propMsg)
{
	OnBadMethod(errMsg, methodMsg, propMsg);
}

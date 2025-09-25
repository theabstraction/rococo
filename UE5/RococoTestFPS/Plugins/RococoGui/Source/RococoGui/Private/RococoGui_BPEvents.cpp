#include <RococoGui_BPevents.h>

void IRococoReflectionEventHandler::RaiseBadMethod(const FString& errMsg, const FString& methodMsg, const FString& propMsg)
{
	OnBadMethod(errMsg, methodMsg, propMsg);
}

FEventReply IRococoReflectionEventHandler::RaiseGlobalKeyUp(int virtualKeyCode, int unicode)
{
	return OnGlobalKeyUp(virtualKeyCode, unicode);
}

FEventReply IRococoReflectionEventHandler::RaiseGlobalKeyDown(int virtualKeyCode, int unicode)
{
	return OnGlobalKeyDown(virtualKeyCode, unicode);
}
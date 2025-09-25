#include <RococoBP_Util_Lib.h>
#include "Kismet/KismetSystemLibrary.h"

void ROCOCO_UE5_EXPORT URococoUtilLibrary::PrintStringItems(
	const UObject* WorldContextObject, 
	const FString& s1,
	const FString& s2,
	const FString& s3,
	const FString& s4,
	const FString& s5,
	const FString& s6,
	bool bPrintToScreen,
	bool bPrintToLog, 
	FLinearColor TextColor, 
	float Duration, 
	const FName Key)
{
	FString s = FString::Printf(TEXT("%s%s%s%s%s%s"), *s1, *s2, *s3, *s4, *s5, *s6);
	UKismetSystemLibrary::PrintString(WorldContextObject, s, bPrintToScreen, bPrintToLog, TextColor, Duration, Key);
	
}

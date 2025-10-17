// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#include <RococoBP_Util_Lib.h>

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

bool ROCOCO_UE5_EXPORT URococoUtilLibrary::IsKey(const FKeyEvent& ev, ERococoVirtualKey vkCode)
{
	return ev.GetKeyCode() == static_cast<uint32>(vkCode);
}
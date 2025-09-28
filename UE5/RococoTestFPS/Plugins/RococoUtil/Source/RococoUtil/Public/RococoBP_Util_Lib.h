#pragma once

#include "CoreMinimal.h"
#include "rococo.UE5.h"
#include "RococoBP_Util_Lib.generated.h"

UENUM(BlueprintType, Category = "Rococo")
enum class ERococoVirtualKey : uint8
{
	None = 0,
	Escape = 27,
	Tab = 9
};

UCLASS(meta = (ScriptName = "RococoUtilLibrary"))
class ROCOCO_BPUTIL_API URococoUtilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Prints a string to the log, and optionally, to the screen
	 * If Print To Log is true, it will be visible in the Output Log window.  Otherwise it will be logged only as 'Verbose', so it generally won't show up.
	 *
	 * @param	s1		The 1st string to log out
	 * @param	s2		The 2nd string to log out
	 * @param	s3		The 3rd string to log out
	 * @param	s4		The 4th string to log out
	 * @param	s5		The 5th string to log out
	 * @param   s6		The 6th string to log out
	 * @param	bPrintToScreen	Whether or not to print the output to the screen
	 * @param	bPrintToLog		Whether or not to print the output to the log
	 * @param	bPrintToConsole	Whether or not to print the output to the console
	 * @param	TextColor		The color of the text to display
	 * @param	Duration		The display duration (if Print to Screen is True). Using negative number will result in loading the duration time from the config.
	 * @param	Key				If a non-empty key is provided, the message will replace any existing on-screen messages with the same key.
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext, Keywords = "log print", AdvancedDisplay = "2", DevelopmentOnly), Category = "Development")
	static void PrintStringItems(
		const UObject* WorldContextObject, 
		const FString& s1 = FString(TEXT("Rumplestiltskin")), 
		const FString& s2 = FString(TEXT("")),
		const FString& s3 = FString(TEXT("")),
		const FString& s4 = FString(TEXT("")),
		const FString& s5 = FString(TEXT("")),
		const FString& s6 = FString(TEXT("")),
		bool bPrintToScreen = true, 
		bool bPrintToLog = true,
		FLinearColor TextColor = FLinearColor(0.0f, 0.66f, 1.0f),
		float Duration = 2.f,
		const FName Key = NAME_None
	);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Rococo")
	static bool IsKey(const FKeyEvent& ev, ERococoVirtualKey vkCode);
};
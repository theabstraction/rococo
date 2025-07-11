#pragma once
#include <CoreMinimal.h>
#include <Engine/Font.h>
#include "RococoFontSet.generated.h"

// Rococo Font Set, links to a UFont and adds mapping of typeface names to other typefaces
// The motivation is that Rococo is implemented on platforms other than UE5 that might not have the UE5 fonts
// So it needs to be able to map these to something UE5 has.
UCLASS(Blueprintable, BlueprintType)
class ROCOCOGUI_API URococoFontSet: public UDataAsset
{
public:
	GENERATED_BODY()

protected:
	// The font asset our font set uses to search for fonts
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UFont> _FontAsset;

	// Map of typefaces requested by Rococo, to those available in the font asset above
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, FName> _RequestedTypeFaceToResultantTypeFace;

public:
	UFont* GetFontAsset()
	{
		return _FontAsset;
	}

	FName MapTypeface(const FName& typefaceName);
};

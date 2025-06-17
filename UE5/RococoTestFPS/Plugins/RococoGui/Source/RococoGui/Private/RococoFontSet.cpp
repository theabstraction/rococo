#include "RococoFontSet.h"
#include "rococo.GR.UE5.h"

FName URococoFontSet::MapTypeface(const FName& typefaceName)
{
	const FName* pResultantName = _RequestedTypeFaceToResultantTypeFace.Find(typefaceName);
	return pResultantName ? *pResultantName : NAME_None;
}
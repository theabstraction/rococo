// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#include "RococoFontSet.h"
#include "rococo.GR.UE5.h"

FName URococoFontSet::MapTypeface(const FName& typefaceName)
{
	const FName* pResultantName = _RequestedTypeFaceToResultantTypeFace.Find(typefaceName);
	return pResultantName ? *pResultantName : NAME_None;
}
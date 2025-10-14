// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com.
#pragma once

#include <Runtime\Core\Public\HAL\Platform.h>
#include <rococo.types.h>

class FPaintArgs;
struct FGeometry;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;

namespace Rococo
{
	struct SlateRenderContext
	{
		const FPaintArgs& args;
		const FGeometry& geometry;
		const FSlateRect& cullingRect;
		FSlateWindowElementList& drawElements;
		int32 layerId;
		const FWidgetStyle& widgetStyle;
		bool bEnabled;

		FVector2f ToSlatePosition(Rococo::Vec2i pos);
	};
}
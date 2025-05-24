#pragma once

#include <Runtime\Core\Public\HAL\Platform.h>

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
		const FPaintGeometry& geometry;
		const FSlateRect& cullingRect;
		FSlateWindowElementList& drawElements;
		int32 layerId;
		const FWidgetStyle& widgetStyle;
		bool bEnabled;
	};
}
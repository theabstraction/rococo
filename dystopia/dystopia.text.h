#pragma once

namespace Rococo
{
	struct RGBAb;
	struct Vec2i;
	struct IGuiRenderContext;
}

namespace Dystopia
{
	using namespace Rococo;
	void RenderHorizontalCentredText(IGuiRenderContext& gr, const wchar_t* txt, RGBAb colour, int fontSize, const Vec2i& topLeft);
}
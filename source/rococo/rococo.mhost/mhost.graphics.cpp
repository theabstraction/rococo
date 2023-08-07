#include "mhost.h"
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.fonts.h>

using namespace MHost;
using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Graphics::Textures;
using namespace Rococo::Graphics::Fonts;

void SetGuiQuadForBitmap(GuiTriangle t[2], const GuiRectf& txUV, int textureIndex)
{
	t[0].a.colour = RGBAb(0xFFFFFFFF);
	t[0].a.sd.lerpBitmapToColour = 0;
	t[0].a.sd.matIndex = 0;
	t[0].a.sd.textureIndex = (float)textureIndex;
	t[0].a.sd.lerpBitmapToColour = 0;
	t[0].a.sd.textureToMatLerpFactor = 0;
	t[0].a.vd.fontBlend = 0;

	t[0].b = t[0].c = t[0].a;
	t[1] = t[0];

	t[0].a.vd.uv = { txUV.left, txUV.top };
	t[0].b.vd.uv = { txUV.right, txUV.top };
	t[0].c.vd.uv = { txUV.left, txUV.bottom };

	t[1].a.vd.uv = { txUV.right, txUV.bottom };
	t[1].b.vd.uv = t[0].c.vd.uv;
	t[1].c.vd.uv = t[0].b.vd.uv;
}

void SetGuiQuadForBitmapWithColour(GuiTriangle t[2], const GuiRectf& txUV, int textureIndex, float spriteToColourLerpFactor, RGBAb colour)
{
	t[0].a.colour = colour;
	t[0].a.sd.matIndex = 0;
	t[0].a.sd.textureIndex = (float)textureIndex;
	t[0].a.sd.lerpBitmapToColour = spriteToColourLerpFactor;
	t[0].a.sd.textureToMatLerpFactor = 0;
	t[0].a.vd.fontBlend = 0;

	t[0].b = t[0].c = t[0].a;
	t[1] = t[0];

	t[0].a.vd.uv = { txUV.left, txUV.top };
	t[0].b.vd.uv = { txUV.right, txUV.top };
	t[0].c.vd.uv = { txUV.left, txUV.bottom };

	t[1].a.vd.uv = { txUV.right, txUV.bottom };
	t[1].b.vd.uv = t[0].c.vd.uv;
	t[1].c.vd.uv = t[0].b.vd.uv;
}

using namespace Rococo::Graphics;

struct Gui : public MHost::IGui
{
	IGuiRenderContext& gc;
	int64 id = 0;

	Gui(IGuiRenderContext& _gc) : gc(_gc) {}

	//   a --- b
	//   |   /
	//   |  /  = t0
	//   | /
	//   |/
	//   c
	//         c
	//       / |
	//      /  | = t1
	//     /   |
	//    /    |
	//   b-----a
	void DrawSprite(const Vec2& pixelPos, int32 alignmentFlags, const BitmapLocation& loc) override
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		Vec2 span = Span(txUV);

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags::Flip))
		{
			swap(txUV.top, txUV.bottom);
		}

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags::Mirror))
		{
			swap(txUV.left, txUV.right);
		}

		Vec2 topLeftPos = GetTopLeftPos(pixelPos, span, alignmentFlags);

		topLeftPos.x = floorf(topLeftPos.x);
		topLeftPos.y = floorf(topLeftPos.y);

		GuiTriangle t[2];
		SetGuiQuadForBitmap(t, txUV, loc.textureIndex);

		t[0].a.pos = topLeftPos;
		t[0].b.pos = { topLeftPos.x + span.x, topLeftPos.y };
		t[0].c.pos = { topLeftPos.x, topLeftPos.y + span.y };

		t[1].a.pos = topLeftPos + span;
		t[1].b.pos = t[0].c.pos;
		t[1].c.pos = t[0].b.pos;

		gc.AddTriangle(&t[0].a);
		gc.AddTriangle(&t[1].a);
	}

	void DrawScaledColouredSprite(const Vec2& pixelPos, int32 alignmentFlags, const Textures::BitmapLocation& loc, float blendFactor, RGBAb colour, float scaleFactor)
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		Vec2 span = Span(txUV);

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags::Flip))
		{
			swap(txUV.top, txUV.bottom);
		}

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags::Mirror))
		{
			swap(txUV.left, txUV.right);
		}

		span *= scaleFactor;

		Vec2 topLeftPos = GetTopLeftPos(pixelPos, span, alignmentFlags);

		topLeftPos.x = floorf(topLeftPos.x);
		topLeftPos.y = floorf(topLeftPos.y);

		GuiTriangle t[2];
		SetGuiQuadForBitmapWithColour(t, txUV, loc.textureIndex, blendFactor, colour);

		t[0].a.pos = topLeftPos;
		t[0].b.pos = { topLeftPos.x + span.x, topLeftPos.y };
		t[0].c.pos = { topLeftPos.x, topLeftPos.y + span.y };

		t[1].a.pos = topLeftPos + span;
		t[1].b.pos = t[0].c.pos;
		t[1].c.pos = t[0].b.pos;

		gc.AddTriangle(&t[0].a);
		gc.AddTriangle(&t[1].a);
	}

	void DrawColouredSprite(const Vec2& pixelPos, int32 alignmentFlags, const BitmapLocation& loc, float spriteToColourLerpFactor, RGBAb colour) override
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		Vec2 span = Span(txUV);

		Vec2 topLeftPos = GetTopLeftPos(pixelPos, span, alignmentFlags);

		topLeftPos.x = floorf(topLeftPos.x);
		topLeftPos.y = floorf(topLeftPos.y);

		GuiTriangle t[2];
		SetGuiQuadForBitmapWithColour(t, txUV, loc.textureIndex, spriteToColourLerpFactor, colour);

		t[0].a.pos = topLeftPos;
		t[0].b.pos = { topLeftPos.x + span.x, topLeftPos.y };
		t[0].c.pos = { topLeftPos.x, topLeftPos.y + span.y };

		t[1].a.pos = topLeftPos + span;
		t[1].b.pos = t[0].c.pos;
		t[1].c.pos = t[0].b.pos;

		gc.AddTriangle(&t[0].a);
		gc.AddTriangle(&t[1].a);
	}

	void FillRect(const Rococo::GuiRectf& rect, RGBAb colour) override
	{
		GuiQuad q;
		q.topLeft = GuiVertex{ { rect.left, rect.top }, {{0, 0}, 0}, {1.0f, 0.0f, 0.0f, 0.0f}, colour };
		q.bottomLeft = q.bottomRight = q.topRight = q.topLeft;
		q.topRight.pos = { rect.right, rect.top };
		q.bottomLeft.pos = { rect.left, rect.bottom };
		q.bottomRight.pos = { rect.right, rect.bottom };
		DrawQuad(q);
	}

	void StretchSprite(const GuiRectf& quad, const BitmapLocation& loc) override
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		GuiTriangle t[2];
		SetGuiQuadForBitmap(t, txUV, loc.textureIndex);

		t[0].a.pos = { quad.left, quad.top };
		t[0].b.pos = { quad.right, quad.top };
		t[0].c.pos = { quad.left, quad.bottom };

		t[1].a.pos = { quad.right, quad.bottom };
		t[1].b.pos = t[0].c.pos;
		t[1].c.pos = t[0].b.pos;

		gc.AddTriangle(&t[0].a);
		gc.AddTriangle(&t[1].a);
	}

	void DrawQuad(const GuiQuad& q) override
	{
		gc.AddTriangle(&q.topLeft);
		gc.AddTriangle(&q.topRight);
	}

	void DrawTriangle(const GuiTriangle& t) override
	{
		gc.AddTriangle(&t.a);
	}

	void DrawClippedText(const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect) override
	{
		Rococo::Graphics::DrawClippedText(gc, rect, alignmentFlags, text, fontIndex, colour, clipRect);
	}

	void DrawTextWithCaret(const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect, int32 caretPos) override
	{
		Rococo::Graphics::DrawTextWithCaret(gc, rect, alignmentFlags, text, fontIndex, colour, clipRect, caretPos);
	}

	void DrawLeftAligned(const Rococo::GuiRectf& rect, const fstring& text, int32 fontIndex, int32 fontHeight, RGBAb colour, float32 softRightEdge, float32 hardRightEdge) override
	{
		Rococo::Graphics::DrawLeftAligned(gc, rect, text, fontIndex, fontHeight, colour, softRightEdge, hardRightEdge);
	}

	void DrawText(const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour) override
	{
		Rococo::Graphics::DrawText(gc, rect, alignmentFlags, text, fontIndex, colour);
	}

	void GetScreenSpan(Vec2& span) const override
	{
		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);
		span = { (float)gm.screenSpan.x, (float)gm.screenSpan.y };
	}

	void GetCursorPos(Vec2& pos) const override
	{
		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);
		pos = { (float) gm.cursorPosition.x, (float)gm.cursorPosition.y };
	}

	void SetGuiShaders(const fstring& pixelShaderFilename) override
	{
		gc.SetGuiShader(pixelShaderFilename);
	}

	void DrawBorder(const Rococo::GuiRectf& rect, float pxThickness, RGBAb tl, RGBAb tr, RGBAb bl, RGBAb br) override
	{
		GuiQuad top;
		top.topLeft = GuiVertex{ { rect.left, rect.top }, {{0, 0}, 0}, {1.0f, 0.0f, 0.0f, 0.0f}, tl };
		top.bottomLeft = top.bottomRight = top.topRight = top.topLeft;
		top.topRight.pos = { rect.right, rect.top };
		top.bottomLeft.pos = { rect.left + pxThickness, rect.top + pxThickness };
		top.bottomRight.pos = { rect.right - pxThickness, rect.top + pxThickness };
		top.bottomRight.colour = top.topRight.colour = tr;
		DrawQuad(top);

		GuiQuad bottom;
		bottom.topLeft = GuiVertex{ { rect.left + pxThickness, rect.bottom - pxThickness }, {{0, 0}, 0}, {1.0f, 0.0f, 0.0f, 0.0f}, bl };
		bottom.bottomLeft = bottom.bottomRight = bottom.topRight = bottom.topLeft;
		bottom.topRight.pos = { rect.right - pxThickness, rect.bottom - pxThickness };
		bottom.bottomLeft.pos = { rect.left, rect.bottom };
		bottom.bottomRight.pos = { rect.right, rect.bottom };
		bottom.bottomRight.colour = bottom.topRight.colour = br;
		bottom.bottomLeft.colour = bl;
		DrawQuad(bottom);

		GuiQuad left;
		left.topLeft = GuiVertex{ { rect.left, rect.top }, {{0, 0}, 0}, {1.0f, 0.0f, 0.0f, 0.0f}, tl };
		left.bottomLeft = left.bottomRight = left.topRight = left.topLeft;
		left.topRight.pos = { rect.left + pxThickness, rect.top + pxThickness};
		left.bottomLeft.pos = { rect.left, rect.bottom };
		left.bottomRight.pos = { rect.left + pxThickness, rect.bottom - pxThickness };
		left.bottomLeft.colour = left.bottomRight.colour = bl;
		DrawQuad(left);

		GuiQuad right;
		right.topLeft = GuiVertex{ { rect.right - pxThickness, rect.top + pxThickness }, {{0, 0}, 0}, {1.0f, 0.0f, 0.0f, 0.0f}, tr };
		right.bottomLeft = right.bottomRight = right.topRight = right.topLeft;
		right.topRight.pos = { rect.right, rect.top };
		right.bottomLeft.pos = { rect.right - pxThickness, rect.bottom - pxThickness };
		right.bottomRight.pos = { rect.right, rect.bottom };
		right.bottomLeft.colour = right.bottomRight.colour = br;
		DrawQuad(right);
	}

	void EvalTextSpan(const fstring& text, int32 fontIndex, Vec2& pixelSpan) override
	{
		Rococo::Graphics::EvalTextSpan(gc, text, fontIndex, pixelSpan);
	}

	void GetFontDescription(int32 fontIndex, Strings::IStringPopulator& familyName, MHost::Graphics::FontDesc& desc) const override
	{
		auto& font = gc.Gui().FontMetrics();
		auto& glyphSet = font[fontIndex];
		desc.ascent = (float) glyphSet.FontAscent();
		desc.height = (float) glyphSet.FontHeight();
		familyName.Populate(glyphSet.Name());
	}

	int32 GetNumberOfFonts() const override
	{
		return gc.Gui().FontMetrics().NumberOfGlyphSets();
	}

	void SetScissorRect(const Rococo::GuiRect& rect) override
	{
		gc.SetScissorRect(rect);
	}

	void ClearScissorRect() override
	{
		gc.ClearScissorRect();
	}
};

#include <new>

namespace MHost
{
	using namespace Rococo;

	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc)
	{
		static_assert(sizeof(Gui) <= 64, "Increase buffer size for Gui");
		return new (buffer) Gui(gc);
	}
}
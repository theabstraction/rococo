#include "mhost.h"
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <rococo.fonts.h>
#include <limits>
#include <new>

using namespace MHost;
using namespace Rococo;
using namespace Rococo::Textures;
using namespace Rococo::Fonts;
using namespace Rococo::Graphics;

Vec2i GetTopLeftPos(const GuiRect& rect, Vec2i span, int32 alignmentFlags)
{
	Vec2i pos;

	if (IsFlagged(alignmentFlags, AlignmentFlags_Left) && !IsFlagged(alignmentFlags, AlignmentFlags_Right))
	{
		pos.x = rect.left;
	}
	else if (!IsFlagged(alignmentFlags, AlignmentFlags_Left) && IsFlagged(alignmentFlags, AlignmentFlags_Right))
	{
		pos.x = rect.right - span.x;
	}
	else
	{
		pos.x = ((rect.left + rect.right - span.x) >> 1);
	}

	if (IsFlagged(alignmentFlags, AlignmentFlags_Top) && !IsFlagged(alignmentFlags, AlignmentFlags_Bottom))
	{
		pos.y = rect.top;
	}
	else if (!IsFlagged(alignmentFlags, AlignmentFlags_Top) && IsFlagged(alignmentFlags, AlignmentFlags_Bottom))
	{
		pos.y = rect.bottom - span.y;
	}
	else
	{
		pos.y = ((rect.top + rect.bottom + span.y) >> 1) - span.y;
	}
	
	return pos;
}

Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags)
{
	Vec2 topLeftPos;

	if (HasFlag(alignmentFlags, AlignmentFlags_Left) && !HasFlag(alignmentFlags, AlignmentFlags_Right))
	{
		topLeftPos.x = pos.x;
	}
	else if (!HasFlag(alignmentFlags, AlignmentFlags_Left) && HasFlag(alignmentFlags, AlignmentFlags_Right))
	{
		topLeftPos.x = pos.x - span.x;
	}
	else // Centred horizontally
	{
		topLeftPos.x = pos.x - 0.5f * span.x;
	}

	if (HasFlag(alignmentFlags, AlignmentFlags_Top) && !HasFlag(alignmentFlags, AlignmentFlags_Bottom))
	{
		topLeftPos.y = pos.y;
	}
	else if (!HasFlag(alignmentFlags, AlignmentFlags_Top) && HasFlag(alignmentFlags, AlignmentFlags_Bottom))
	{
		topLeftPos.y = pos.y - span.y;
	}
	else // Centred vertically
	{
		topLeftPos.y = pos.y - 0.5f * span.y;
	}

	return topLeftPos;
}


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

struct GlyphArgs
{
	int32 index;
	GuiRectf rect;
};

class BasicTextJob : public IDrawTextJob
{
private:
	cstr text;
	FontColour colour;
	int fontIndex;
	int fontHeight;
	IEventCallback<GlyphArgs>* cb;
	int count = 0;
public:
	GuiRectf target;

	BasicTextJob(int _fontIndex, cstr _text, RGBAb _colour, int _fontHeight, IEventCallback<GlyphArgs>* _cb = nullptr) :
		text(_text),  colour((FontColour&)_colour), fontIndex(_fontIndex), fontHeight(_fontHeight), cb(_cb)
	{
		float minFloat = std::numeric_limits<float>::min();
		float maxFloat = std::numeric_limits<float>::max();
		target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
	}

	void Reset(IEventCallback<GlyphArgs>* cb)
	{
		count = 0;
		this->cb = cb;
	}

	void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
	{
		GuiRectf outputRect;
		builder.AppendChar(c, outputRect);

		if (cb)
		{
			GlyphArgs args;
			args.index = count++;
			args.rect = outputRect;
			cb->OnEvent(args);
		}

		if (outputRect.left < target.left) target.left = outputRect.left;
		if (outputRect.right > target.right) target.right = outputRect.right;
		if (outputRect.bottom > target.bottom) target.bottom = outputRect.bottom;
		if (outputRect.top < target.top) target.top = outputRect.top;
	}

	virtual void OnDraw(Fonts::IGlyphBuilder& builder)
	{
		builder.SetTextColour(colour);
		builder.SetShadow(false);
		builder.SetFontIndex(fontIndex);
		builder.SetFontHeight(fontHeight);

		Vec2 firstGlyphPos = builder.GetCursor();

		for (cstr p = text; *p != 0; p++)
		{
			char c = *p;

			if (c >= 255) c = '?';

			if (c == '\t')
			{
				for (int i = 0; i < 4; ++i)
				{
					DrawNextGlyph(' ', builder);
				}
			}
			else if (c == '\n')
			{
				Vec2 nextGlyphPos = builder.GetCursor();
				builder.SetCursor(Vec2{ firstGlyphPos.x, nextGlyphPos.y + fontHeight });
			}
			else
			{
				DrawNextGlyph((char)c, builder);
			}
		}
	}
};

class LeftAlignedTextJob : public IDrawTextJob
{
private:
	cstr text;
	FontColour colour;
	int fontIndex;
	int fontHeight;
	IEventCallback<GlyphArgs>* cb;
	int count = 0;
	GuiRectf bounds;

	float rightMaxReturnX;
	float rightAlignX;
public:
	GuiRectf target;

	LeftAlignedTextJob(int _fontIndex, float hardRightEdge, float softRightEdge, cstr _text, RGBAb _colour, const GuiRectf& _bounds, int32 _fontHeight, IEventCallback<GlyphArgs>* _cb = nullptr) :
		text(_text), colour((FontColour&)_colour), fontIndex(_fontIndex), fontHeight(_fontHeight), cb(_cb), bounds(_bounds)
	{
		float minFloat = std::numeric_limits<float>::min();
		float maxFloat = std::numeric_limits<float>::max();
		target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
		rightMaxReturnX = bounds.right - hardRightEdge;
		rightAlignX = bounds.right - softRightEdge;
	}

	void Reset(IEventCallback<GlyphArgs>* cb)
	{
		count = 0;
		this->cb = cb;
	}

	void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
	{
		GuiRectf outputRect;
		builder.AppendChar(c, outputRect);

		if (cb)
		{
			GlyphArgs args;
			args.index = count++;
			args.rect = outputRect;
			cb->OnEvent(args);
		}

		if (outputRect.left < target.left) target.left = outputRect.left;
		if (outputRect.right > target.right) target.right = outputRect.right;
		if (outputRect.bottom > target.bottom) target.bottom = outputRect.bottom;
		if (outputRect.top < target.top) target.top = outputRect.top;
	}

	virtual void OnDraw(Fonts::IGlyphBuilder& builder)
	{
		builder.SetTextColour(colour);
		builder.SetShadow(false);
		builder.SetFontIndex(fontIndex);
		builder.SetFontHeight(fontHeight);

		Vec2 firstGlyphPos = builder.GetCursor();

		for (cstr p = text; *p != 0; p++)
		{
			char c = *p;

			if (c >= 255) c = '?';

			Vec2 pos = builder.GetCursor();

			if (pos.x >= rightAlignX)
			{
				if (c <= 32)
				{
					pos.x = bounds.left;
					pos.y += (float) fontHeight;
					builder.SetCursor(pos);
					continue;
				}
				else if (pos.x >= rightMaxReturnX)
				{
					pos.x = bounds.left;
					pos.y += (float)fontHeight;
					builder.SetCursor(pos);
				}
			}

			if (c == '\t')
			{
				for (int i = 0; i < 4; ++i)
				{
					DrawNextGlyph(' ', builder);
				}

				pos = builder.GetCursor();

				if (pos.x >= rightAlignX)
				{
					pos.x = bounds.left;
					pos.y += (float)fontHeight;
					builder.SetCursor(pos);
				}
			}
			else if (c == '\n')
			{
				pos.x = bounds.left;
				pos.y += (float)fontHeight;
				builder.SetCursor(pos);
			}
			else
			{
				DrawNextGlyph((char)c, builder);
			}
		}
	}
};

struct Gui : public MHost::IGui
{
	Rococo::IGuiRenderContext& gc;
	int64 id = 0;

	Gui(Rococo::IGuiRenderContext& _gc) : gc(_gc) {}

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

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags_Flip))
		{
			std::swap(txUV.top, txUV.bottom);
		}

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags_Mirror))
		{
			std::swap(txUV.left, txUV.right);
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

	void DrawScaledColouredSprite(const Vec2& pixelPos, int32 alignmentFlags, const Rococo::Textures::BitmapLocation& loc, float blendFactor, RGBAb colour, float scaleFactor)
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		Vec2 span = Span(txUV);

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags_Flip))
		{
			std::swap(txUV.top, txUV.bottom);
		}

		if (HasFlag(alignmentFlags, MHost::AlignmentFlags_Mirror))
		{
			std::swap(txUV.left, txUV.right);
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

	void DrawQuad(const Rococo::GuiQuad& q) override
	{
		gc.AddTriangle(&q.topLeft);
		gc.AddTriangle(&q.topRight);
	}

	void DrawTriangle(const Rococo::GuiTriangle& t) override
	{
		gc.AddTriangle(&t.a);
	}

	void DrawClippedText(const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect) override
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			BasicTextJob job(fontIndex, text, colour, Height(iRect));
			Vec2i span = gc.EvalSpan({ 0,0 }, job);
			Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

			GuiRect clipRecti{ (int32)clipRect.left, (int32)clipRect.top, (int32)clipRect.right, (int32)clipRect.bottom };
			gc.RenderText(topLeft, job, &clipRecti);
		}
	}

	void DrawTextWithCaret(const Rococo::GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour, const Rococo::GuiRectf& clipRect, int32 caretPos) override
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };

		struct : IEventCallback<GlyphArgs>
		{
			int caretPos;

			GuiRectf caretGlyphRect = { -1,-1,-1,-1 };
			GuiRectf lastGlyphRect = { -1, -1, -1, -1 };

			void OnEvent(GlyphArgs& args) override
			{
				lastGlyphRect = args.rect;

				if (caretPos == args.index)
				{
					caretGlyphRect = args.rect;
				}
			}
		} onGlyph;

		onGlyph.caretPos = caretPos;

		BasicTextJob job(fontIndex, text, colour, Height(iRect), &onGlyph);
		Vec2i span = gc.EvalSpan({ 0,0 }, job);

		job.Reset(&onGlyph);

		Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

		GuiRect clipRecti{ (int32)clipRect.left, (int32)clipRect.top, (int32)clipRect.right, (int32)clipRect.bottom };
		gc.RenderText(topLeft, job, &clipRecti);

		if (onGlyph.caretGlyphRect.left > -1)
		{
			Vec2i caretStart = { (int32)onGlyph.caretGlyphRect.left,  (int32)onGlyph.caretGlyphRect.bottom };
			Vec2i caretEnd   = { (int32)onGlyph.caretGlyphRect.right, (int32)onGlyph.caretGlyphRect.bottom };
			Rococo::Graphics::DrawLine(gc, 2, caretStart, caretEnd, 0xFFFFFFFF);
		}
		else if (onGlyph.lastGlyphRect.left > -1)
		{
			auto& f = gc.Renderer().FontMetrics();
			auto& glyphs = f[fontIndex % f.NumberOfGlyphSets()];
			auto& glyph = glyphs['a'];
			auto span = glyph.B;
			Vec2i caretStart = { (int32)onGlyph.lastGlyphRect.right,  (int32)onGlyph.lastGlyphRect.bottom };
			Vec2i caretEnd = { caretStart.x + (int32) span, caretStart.y };
			Rococo::Graphics::DrawLine(gc, 2, caretStart, caretEnd, 0xFFFFFFFF);
		}
		else
		{
			auto& f = gc.Renderer().FontMetrics();
			auto& glyphs = f[fontIndex % f.NumberOfGlyphSets()];
			auto& glyph = glyphs['a'];
			auto span = glyph.B;

			Vec2i ds{ (int32) span, (int32) glyphs.FontHeight() };
			Vec2i caretStart = GetTopLeftPos(iRect, ds, alignmentFlags);
			caretStart.y += ds.y;

			Vec2i caretEnd = { caretStart.x + (int32)span, caretStart.y };
			Rococo::Graphics::DrawLine(gc, 2, caretStart, caretEnd, 0xFFFFFFFF);
		}
	}

	void DrawLeftAligned(const Rococo::GuiRectf& rect, const fstring& text, int32 fontIndex, int32 fontHeight, RGBAb colour, float32 softRightEdge, float32 hardRightEdge) override
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			LeftAlignedTextJob job(fontIndex, softRightEdge, hardRightEdge, text, colour, rect, fontHeight);

			Vec2i topLeft = { iRect.left, iRect.top };
			gc.RenderText(topLeft, job, &iRect);
		}
	}

	void DrawText(const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour) override
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			BasicTextJob job(fontIndex, text, colour, (int) Height(rect));
			Vec2i span = gc.EvalSpan({ 0,0 }, job);
			Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);

			if (HasFlag(alignmentFlags, AlignmentFlags_Clipped)) __debugbreak();

			gc.RenderText(topLeft, job, HasFlag(alignmentFlags, AlignmentFlags_Clipped) ? &iRect : nullptr);
		}
	}

	void GetScreenSpan(Vec2& span) override
	{
		GuiMetrics gm;
		gc.Renderer().GetGuiMetrics(gm);
		span = { (float)gm.screenSpan.x, (float)gm.screenSpan.y };
	}

	void GetCursorPos(Vec2& pos) override
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

	void EvalTextSpan(const fstring& text, int32 fontIndex, int fontHeight, Vec2& pixelSpan) override
	{
		BasicTextJob job(fontIndex, text, 0xFFFFFFFF, (int) pixelSpan.y);
		auto iSpan = gc.EvalSpan({ 0,0 }, job);
		pixelSpan.x = (float)iSpan.x;
		pixelSpan.y = (float)iSpan.y;
	}

	void GetFontDescription(int32 fontIndex, Rococo::IStringPopulator& familyName, MHost::Graphics::FontDesc& desc) override
	{
		auto& font = gc.Renderer().FontMetrics();
		auto& glyphSet = font[fontIndex];
		desc.ascent = (float) glyphSet.FontAscent();
		desc.height = (float) glyphSet.FontHeight();
		familyName.Populate(glyphSet.Name());
	}

	int32 GetNumberOfFonts() override
	{
		return gc.Renderer().FontMetrics().NumberOfGlyphSets();
	}

	void SetScissorRect(const Rococo::GuiRectf& rect) override
	{
		gc.SetScissorRect(rect);
	}

	void ClearScissorRect() override
	{
		gc.ClearScissorRect();
	}
};

namespace MHost
{
	using namespace Rococo;

	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc)
	{
		static_assert(sizeof(Gui) <= 64, "Increase buffer size for Gui");
		return new (buffer) Gui(gc);
	}
}
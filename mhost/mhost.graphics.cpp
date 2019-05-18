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
		pos.x = ((rect.left + rect.right) >> 1) - span.x;
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
		pos.y = ((rect.top + rect.bottom) >> 1) - span.y;
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

class BasicTextJob : public IDrawTextJob
{
private:
	cstr text;
	FontColour colour;
	int fontIndex;
	float lastCellHeight;
	IEventCallback<GlyphCallbackArgs>* cb;
	int count = 0;
public:
	GuiRectf target;

	BasicTextJob(int _fontIndex, cstr _text, RGBAb _colour, IEventCallback<GlyphCallbackArgs>* _cb = nullptr) :
		text(_text),  colour((FontColour&)_colour), fontIndex(_fontIndex), lastCellHeight(10.0f), cb(_cb)
	{
		float minFloat = std::numeric_limits<float>::min();
		float maxFloat = std::numeric_limits<float>::max();
		target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
	}

	void Reset(IEventCallback<GlyphCallbackArgs>* cb)
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
			Rococo::Graphics::GlyphCallbackArgs args;
			args.index = count++;
			args.rect.left = (int32)outputRect.left;
			args.rect.right = (int32)outputRect.right;
			args.rect.top = (int32)outputRect.top;
			args.rect.bottom = (int32)outputRect.bottom;
			cb->OnEvent(args);
		}

		lastCellHeight = outputRect.bottom - outputRect.top;

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
				builder.SetCursor(Vec2{ firstGlyphPos.x, nextGlyphPos.y + lastCellHeight });
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

	void DrawSprite(const Vec2& pixelPos, int32 alignmentFlags, const BitmapLocation& loc) override
	{
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

		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		Vec2 span = Span(txUV);

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

	void DrawText(const GuiRectf& rect, int32 alignmentFlags, const fstring& text, int32 fontIndex, RGBAb colour) override
	{
		GuiRect iRect{ (int32)rect.left, (int32)rect.top, (int32)rect.right, (int32)rect.bottom };
		if (text.length)
		{
			BasicTextJob job(fontIndex, text, colour);
			Vec2i span = gc.EvalSpan({ 0,0 }, job);
			Vec2i topLeft = GetTopLeftPos(iRect, span, alignmentFlags);
			gc.RenderText(topLeft, job, &iRect);
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
		pos = { (float) gm.cursorPosition.x, (float)gm.screenSpan.y };
	}
};

namespace MHost
{
	using namespace Rococo;

	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc)
	{
		return new (buffer) Gui(gc);
	}
}
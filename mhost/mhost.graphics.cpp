#include "mhost.h"
#include <rococo.renderer.h>
#include <rococo.textures.h>
#include <new>

using namespace MHost;
using namespace Rococo;
using namespace Rococo::Textures;

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


struct Gui : public MHost::IGui
{
	Rococo::IGuiRenderContext& gc;
	int64 id = 0;

	Gui(Rococo::IGuiRenderContext& _gc) : gc(_gc) {}

	void AdvanceFrame()
	{
		id++;
	}

	void DrawSprite(const Vec2i& pixelPos, int32 alignmentFlags, const BitmapLocation& loc) override
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

		Vec2 pxPos = { (float)pixelPos.x, (float)pixelPos.y };
		Vec2 topLeftPos = GetTopLeftPos(pxPos, span, alignmentFlags);

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

	void StretchSprite(const GuiRect& quad, const BitmapLocation& loc) override
	{
		GuiRectf txUV = { (float)loc.txUV.left,  (float)loc.txUV.top, (float)loc.txUV.right,  (float)loc.txUV.bottom };

		GuiTriangle t[2];
		SetGuiQuadForBitmap(t, txUV, loc.textureIndex);

		t[0].a.pos = { (float)quad.left, (float)quad.top };
		t[0].b.pos = { (float)quad.right, (float)quad.top };
		t[0].c.pos = { (float)quad.left, (float)quad.bottom };

		t[1].a.pos = { (float)quad.right, (float)quad.bottom };
		t[1].b.pos = t[0].c.pos;
		t[1].c.pos = t[0].b.pos;

		gc.AddTriangle(&t[0].a);
		gc.AddTriangle(&t[1].a);
	}

	void PushTriangle(const Rococo::GuiTriangle& t) override
	{
		gc.AddTriangle(&t.a);
	}
};

namespace MHost
{
	using namespace Rococo;

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

	IGui* CreateGuiOnStack(char buffer[64], IGuiRenderContext& gc)
	{
		return new (buffer) Gui(gc);
	}
}
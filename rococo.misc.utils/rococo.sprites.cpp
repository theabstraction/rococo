#include "rococo.api.h"
#include "rococo.textures.h"
#include "rococo.renderer.h"

using namespace Rococo;
using namespace Textures;

namespace Rococo
{
	namespace Graphics
	{
		void StretchBitmap(IGuiRenderContext& rc, const GuiRect& absRect, MaterialId id)
		{
			SpriteVertexData useMaterial{ 0.0f, 0.0f, id, 1.0f };

			GuiVertex v[6] =
			{
				{
					{ (float)absRect.left, (float)absRect.top},
					{ { 0, 0}, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ (float)absRect.right, (float)absRect.top },
					{ { 1, 0 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{(float)absRect.right,(float)absRect.bottom },
					{ { 1, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ (float)absRect.right, (float)absRect.bottom},
					{ { 1, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ (float)absRect.left, (float)absRect.bottom },
					{ { 0, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ (float)absRect.left, (float)absRect.top },
					{ { 0, 0 }, 0 },
					useMaterial,
					RGBAb(255,255,255),
				}
			};

			rc.AddTriangle(v);
			rc.AddTriangle(v + 3);
		}

		void RenderBitmap_ShrinkAndPreserveAspectRatio(IGuiRenderContext& rc, MaterialId id, const GuiRect& absRect)
		{
			MaterialArrayMetrics metrics;
			rc.Materials().GetMaterialArrayMetrics(metrics);

			Vec2 txSpan{ (float) metrics.Width,(float) metrics.Width };

			Vec2i rectSpan = Span(absRect);

			Vec2i centre = Centre(absRect);

			float aspectRatio = txSpan.x / txSpan.y;

			GuiRectf rect;

			if (aspectRatio > 1.0f)
			{
				if (rectSpan.x > txSpan.x)
				{
					rect.left = centre.x - floorf(0.5f * txSpan.x);
					rect.right = rect.left + txSpan.x;
					rect.top = centre.y - floorf(0.5f * txSpan.y);
					rect.bottom = rect.top + txSpan.y;
				}
				else
				{
					rect.left = (float) absRect.left;
					rect.right = (float)absRect.right;
					rect.top = (float) (centre.y - floorf(0.5f * rectSpan.y / aspectRatio));
					rect.bottom = (float)(rect.top + floorf(rectSpan.y / aspectRatio));
				}
			}
			else
			{
				if (rectSpan.y > txSpan.y)
				{
					rect.left = (float) (centre.x - floorf(0.5f * txSpan.x));
					rect.right = (float) (rect.left + txSpan.x);
					rect.top = (float)(centre.y - floorf(0.5f * txSpan.y));
					rect.bottom = (float)(rect.top + txSpan.y);
				}
				else
				{
					rect.top = (float)(absRect.top);
					rect.bottom = (float)(absRect.bottom);
					rect.left = centre.x - floorf(0.5f * rectSpan.y * aspectRatio);
					rect.right = rect.left + floorf(rectSpan.y * aspectRatio);
				}
			}

			SpriteVertexData useMaterial{ 0.0f, 0.0f, id, 1.0f };

			GuiVertex v[6] =
			{
				{
					{ rect.left,	rect.top },
					{ { 0, 0 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ rect.right, rect.top },
					{ { 1, 0 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ rect.right, rect.bottom },
					{ { 1, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ rect.right, rect.bottom },
					{ { 1, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ rect.left, rect.bottom },
					{ { 0, 1 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				},
				{
					{ rect.left, rect.top },
					{ { 0, 0 }, 0 },
					useMaterial,
					RGBAb(255,255,255)
				}
			};

			rc.AddTriangle(v);
			rc.AddTriangle(v + 3);
		}

		void DrawSprite(const Vec2i& topLeftBitmap, const BitmapLocation& location, IGuiRenderContext& gc)
		{
			Vec2 topLeft, bottomRight;
			topLeft.x = (float)topLeftBitmap.x;
			topLeft.y = (float)topLeftBitmap.y;
			bottomRight.x = (float)(topLeftBitmap.x + location.txUV.right - location.txUV.left);
			bottomRight.y = (float)(topLeftBitmap.y + location.txUV.bottom - location.txUV.top);

			// [0 1]
			// [3 2]
			GuiVertex quad[4] = { 0 };
			quad[0].pos = topLeft;
			quad[2].pos = bottomRight;

			quad[1].pos = { bottomRight.x, topLeft.y };
			quad[3].pos = { topLeft.x, bottomRight.y };	

			quad[0].vd.uv.x = quad[3].vd.uv.x = (float)location.txUV.left;
			quad[1].vd.uv.x = quad[2].vd.uv.x = (float)location.txUV.right;
			quad[0].vd.uv.y = quad[1].vd.uv.y = (float)location.txUV.top;
			quad[2].vd.uv.y = quad[3].vd.uv.y = (float)location.txUV.bottom;

			quad[0].vd.fontBlend = quad[1].vd.fontBlend = quad[2].vd.fontBlend = quad[3].vd.fontBlend = 0;
			quad[0].sd = quad[1].sd = quad[2].sd = quad[3].sd = { 0, (float)location.textureIndex, 0, 0 };
			quad[0].colour = quad[1].colour = quad[2].colour = quad[3].colour = RGBAb(0, 0, 0, 0);

			GuiVertex TL[3] = { quad[0], quad[1], quad[2] };
			GuiVertex BR[3] = { quad[2], quad[3], quad[0] };
			gc.AddTriangle(TL);
			gc.AddTriangle(BR);
		}

		void StretchBitmap(IGuiRenderContext& gc, const Textures::BitmapLocation& location, const GuiRect& absRect)
		{
			auto recti = Dequantize(absRect);
			// [0 1]
			// [3 2]
			GuiVertex quad[4] = { 0 };
			quad[0].pos = Vec2 { recti.left, recti.top };
			quad[2].pos = Vec2 { recti.right, recti.bottom };
			quad[1].pos = Vec2 { recti.right, recti.top };
			quad[3].pos = Vec2 { recti.left, recti.bottom };

			quad[0].vd.uv.x = quad[3].vd.uv.x = (float)location.txUV.left;
			quad[1].vd.uv.x = quad[2].vd.uv.x = (float)location.txUV.right;
			quad[0].vd.uv.y = quad[1].vd.uv.y = (float)location.txUV.top;
			quad[2].vd.uv.y = quad[3].vd.uv.y = (float)location.txUV.bottom;

			quad[0].vd.fontBlend = quad[1].vd.fontBlend = quad[2].vd.fontBlend = quad[3].vd.fontBlend = 0;
			quad[0].sd = quad[1].sd = quad[2].sd = quad[3].sd = { 0, (float)location.textureIndex, 0, 0 };
			quad[0].colour = quad[1].colour = quad[2].colour = quad[3].colour = RGBAb(0, 0, 0, 0);

			GuiVertex TL[3] = { quad[0], quad[1], quad[2] };
			GuiVertex BR[3] = { quad[2], quad[3], quad[0] };
			gc.AddTriangle(TL);
			gc.AddTriangle(BR);
		}

		void DrawSpriteCentred(const GuiRect& rect, const Textures::BitmapLocation& location, IGuiRenderContext& gc)
		{
			GuiMetrics metrics;
			gc.Renderer().GetGuiMetrics(metrics);

			Vec2i sprSpan = Span(location.txUV);
			Vec2i pos = Centre(rect) - Vec2i { sprSpan.x >> 1, sprSpan.y >> 1};
			DrawSprite(pos, location, gc);
		}
	}
} // Rococo
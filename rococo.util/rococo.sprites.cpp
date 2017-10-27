#include "rococo.api.h"
#include "rococo.textures.h"
#include "rococo.renderer.h"

using namespace Rococo;
using namespace Textures;

namespace Rococo
{
	namespace Graphics
	{
		void StretchBitmap(IGuiRenderContext& rc, const GuiRect& absRect)
		{
			GuiVertex v[6] =
			{
				{
					(float) absRect.left,
					(float) absRect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0,
					0,
					0
				},
				{
					(float) absRect.right,
					(float) absRect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					0,
					0
				},
				{
					(float) absRect.right,
					(float) absRect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					1.0f,
					0
				},
				{
					(float) absRect.right,
					(float) absRect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					1.0f,
					0
				},
				{
					(float) absRect.left,
					(float) absRect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0.0f,
					1.0f,
					0
				},
				{
					(float) absRect.left,
					(float) absRect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0,
					0,
					0
				}
			};

			rc.AddTriangle(v);
			rc.AddTriangle(v + 3);
		}

		void RenderBitmap_ShrinkAndPreserveAspectRatio(IGuiRenderContext& rc, ID_TEXTURE id, const GuiRect& absRect)
		{
			Vec2i txSpan = rc.SelectTexture(id);
			Vec2i rectSpan = Span(absRect);

			Vec2i centre = Centre(absRect);

			float aspectRatio = txSpan.x / (float)txSpan.y;

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

			GuiVertex v[6] =
			{
				{
					rect.left,
					rect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0,
					0,
					0
				},
				{
					rect.right,
					rect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					0,
					0
				},
				{
					rect.right,
					rect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					1.0f,
					0
				},
				{
					rect.right,
					rect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					1.0f,
					1.0f,
					0
				},
				{
					rect.left,
					rect.bottom,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0.0f,
					1.0f,
					0
				},
				{
					rect.left,
					rect.top,
					0.0f,
					0.0f,
					RGBAb(255,255,255),
					0,
					0,
					0
				}
			};

			rc.AddTriangle(v);
			rc.AddTriangle(v + 3);
		}

		void DrawSprite(const Vec2i& topLeftBitmap, const BitmapLocation& location, IGuiRenderContext& gc, bool alphaBlend)
		{
			Vec2 topLeft, bottomRight;
			topLeft.x = (float)topLeftBitmap.x;
			topLeft.y = (float)topLeftBitmap.y;
			bottomRight.x = (float)(topLeftBitmap.x + location.txUV.right - location.txUV.left);
			bottomRight.y = (float)(topLeftBitmap.y + location.txUV.bottom - location.txUV.top);

			// [0 1]
			// [2 3]
			GuiVertex quad[4] = { 0 };
			quad[0].x = quad[2].x = topLeft.x;
			quad[1].x = quad[3].x = bottomRight.x;
			quad[0].y = quad[1].y = topLeft.y;
			quad[2].y = quad[3].y = bottomRight.y;

			quad[0].u = quad[2].u = (float)location.txUV.left;
			quad[1].u = quad[3].u = (float)location.txUV.right;
			quad[0].v = quad[1].v = (float)location.txUV.top;
			quad[2].v = quad[3].v = (float)location.txUV.bottom;

			quad[0].textureIndex = quad[1].textureIndex = quad[2].textureIndex = quad[3].textureIndex = (float)location.textureIndex;

			gc.AddSpriteTriangle(alphaBlend, quad);
			gc.AddSpriteTriangle(alphaBlend, quad + 1);
		}
	}
} // Rococo
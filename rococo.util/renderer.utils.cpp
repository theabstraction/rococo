#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.fonts.h>

#include <limits>
#include <new>

#include <rococo.maths.h>

namespace
{
	using namespace Rococo;

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	GuiRectf GuiRectToQuad(const GuiRect& rect)
	{
		return GuiRectf((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}

	GuiRectf Intersect(const GuiRectf& a, const GuiRectf& b)
	{
		GuiRectf q;
		q.left = max(a.left, b.left);
		q.right = min(a.right, b.right);
		q.top = max(a.top, b.top);
		q.bottom = min(a.bottom, b.bottom);
		return q;
	}

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
	public:
		GuiRectf target;

		HorizontalCentredText(int _fontIndex, cstr _text, Fonts::FontColour _colour) :
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = GuiRectf(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;
			builder.AppendChar(c, outputRect);

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

			float fMin = -1000000.0f;
			float fMax = 1000000.0f;
			builder.SetClipRect(GuiRectf(fMin, fMin, fMax, fMax));

			Vec2 firstGlyphPos = builder.GetCursor();

			for (cstr p = text; *p != 0; p++)
			{
				rchar c = *p;

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

	class LeftAlignedText : public Fonts::IDrawTextJob
	{
	private:
		cstr text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
		GuiRectf targetRect;
		int retzone;
		int hypzone;
	public:
		LeftAlignedText(const GuiRect& _targetRect, int _retzone, int _hypzone, int _fontIndex, cstr _text, Fonts::FontColour _colour) :
			targetRect(GuiRectToQuad(_targetRect)), retzone(_retzone), hypzone(_hypzone),
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			GuiRectf outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);
			
			auto outerClipRect = builder.GetClipRect();
			auto innerClipRect = targetRect;
			
			builder.SetClipRect(Intersect(outerClipRect, innerClipRect));

			builder.SetCursor(Vec2{ targetRect.left, targetRect.top });

			const float retX = targetRect.right - retzone;
			const float hypX = targetRect.right - hypzone;

			for (cstr p = text; *p != 0; p++)
			{
				rchar c = *p;

				Vec2 nextGlyphPos = builder.GetCursor();

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else if (c == ' ')
				{
					if (nextGlyphPos.x > retX)
					{
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						continue;
					}

					DrawNextGlyph(' ', builder);
				}
				else if (c == '\n')
				{
					builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
				}
				else
				{
					if (nextGlyphPos.x > hypX)
					{
						DrawNextGlyph('-', builder);
						builder.SetCursor(Vec2{ targetRect.left, nextGlyphPos.y + lastCellHeight });
						DrawNextGlyph('-', builder);
					}
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

}

namespace Rococo
{
	namespace Graphics
	{
		Vec2i GetScreenCentre(const GuiMetrics& metrics)
		{
			return Vec2i{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
		}

		Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middleLeft)
		{
			HorizontalCentredText job(fontSize, text, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ middleLeft.x, middleLeft.y - (span.y >> 1) }, job);
			return span;
		}

		Vec2i RenderTopLeftAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topLeft)
		{
			HorizontalCentredText job(fontSize, text, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ topLeft.x, topLeft.y }, job);
			return span;
		}

		Vec2i RenderTopRightAlignedText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& topRight)
		{
			HorizontalCentredText job(fontSize, text, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ topRight.x - span.x, topRight.y }, job);
			return span;
		}

		Vec2i RenderHorizontalCentredText(IGuiRenderContext& grc, cstr txt, RGBAb colour, int fontSize, const Vec2i& topMiddle)
		{
			HorizontalCentredText job(fontSize, txt, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ topMiddle.x - (span.x >> 1), topMiddle.y }, job);
			return span;
		}

		Vec2i RenderCentredText(IGuiRenderContext& grc, cstr text, RGBAb colour, int fontSize, const Vec2i& middle)
		{
			HorizontalCentredText job(fontSize, text, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ middle.x - (span.x >> 1), middle.y - (span.y >> 1) }, job);
			return span;
		}

		void DrawLine(IGuiRenderContext& grc, int pixelthickness, Vec2i start, Vec2i end, RGBAb colour)
		{
			Vec2i delta = end - start;

			int dx2 = Sq(delta.x);
			int dy2 = Sq(delta.y);

			Vec2 offset;
			if (dy2 > dx2) // Move vertical than horizontal
			{
				offset = { (float)pixelthickness, 0 };
			}
			else
			{
				offset = { 0, (float)pixelthickness };
			}

			Vec2 bottomLeft = Vec2{ (float)start.x,  (float)start.y };
			Vec2 topLeft = bottomLeft + offset;

			Vec2 bottomRight = Vec2{ (float)end.x,  (float)end.y };
			Vec2 topRight = bottomRight + offset;

			BaseVertexData noFont{ {0,0}, 0 };
			SpriteVertexData solidColour{ 1.0f, 0.0f, 0.0f, 0.0f };

			GuiVertex q[] =
			{
				{ topLeft,      noFont, solidColour, colour },
				{ bottomLeft,   noFont, solidColour, colour },
				{ bottomRight,  noFont, solidColour, colour },
				{ bottomRight,  noFont, solidColour, colour },
				{ topRight,     noFont, solidColour, colour },
				{ topLeft,      noFont, solidColour, colour },
			};

			grc.AddTriangle(q);
			grc.AddTriangle(q + 3);
		}

		void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag)
		{
			GuiRectf rect{ (float)grect.left, (float)grect.top, (float)grect.right, (float)grect.bottom };
			BaseVertexData noFont{ {0, 0}, 0 };
			SpriteVertexData solid{ 1.0f, 0, 0, 0 };
			GuiVertex q[] =
			{
				{ {rect.left,  rect.top},    noFont, solid, diag },
				{ {rect.right, rect.top},    noFont, solid, backdiag },
				{ {rect.right, rect.bottom}, noFont, solid, diag },
				{ {rect.right, rect.bottom}, noFont, solid, diag },
				{ {rect.left,  rect.bottom}, noFont, solid, backdiag },
				{ {rect.left,  rect.top},    noFont, solid, diag }
			};

			grc.AddTriangle(q);
			grc.AddTriangle(q + 3);
		}

		void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag)
		{
			GuiRect topRect{ rect.left - width.x, rect.top, rect.right, rect.top + width.y };
			DrawRectangle(grc, topRect, diag, diag);

			GuiRect bottomRect{ rect.left - width.x, rect.bottom - width.y, rect.right, rect.bottom };
			DrawRectangle(grc, bottomRect, backdiag, backdiag);

			GuiRect leftRect{ rect.left - width.x, rect.top, rect.left, rect.bottom };
			DrawRectangle(grc, leftRect, backdiag, backdiag);

			GuiRect rightRect{ rect.right - width.x, rect.top, rect.right, rect.bottom };
			DrawRectangle(grc, rightRect, diag, diag);
		}

		Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, cstr text, RGBAb colour)
		{
			static_assert(sizeof(ss) > sizeof(HorizontalCentredText), "Increase buffer size");
			HorizontalCentredText *hct = new (&ss) HorizontalCentredText(fontIndex, text, FontColourFromRGBAb(colour));
			return *hct;
		}

		Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontIndex, cstr text, RGBAb colour)
		{
			static_assert(sizeof(ss) > sizeof(LeftAlignedText), "Increase buffer size");
			LeftAlignedText *lat = new (&ss) LeftAlignedText(targetRect, retzone, hypzone, fontIndex, text, FontColourFromRGBAb(colour));
			return *lat;
		}

		float GetAspectRatio(const IRenderer& renderer)
		{
			GuiMetrics metrics;
			renderer.GetGuiMetrics(metrics);
			float aspectRatio = metrics.screenSpan.y / float(metrics.screenSpan.x);
			return aspectRatio;
		}

		Vec2 PixelSpaceToScreenSpace(const Vec2i& v, IRenderer& renderer)
		{
			GuiMetrics metrics;
			renderer.GetGuiMetrics(metrics);

			return{ (2.0f * (float)metrics.cursorPosition.x - metrics.screenSpan.x) / metrics.screenSpan.x,
				-(2.0f * (float)metrics.cursorPosition.y - metrics.screenSpan.y) / metrics.screenSpan.y };
		}
	}
}
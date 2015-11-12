#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.fonts.h>

#include <limits>
#include <new>

namespace
{
	using namespace Rococo;

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	Quad GuiRectToQuad(const GuiRect& rect)
	{
		return Quad((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
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
			builder.SetClipRect(Quad(fMin, fMin, fMax, fMax));

			Vec2 firstGlyphPos = builder.GetCursor();

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

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
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;
		float lastCellHeight;
		Quad targetRect;
		int retzone;
		int hypzone;
	public:
		LeftAlignedText(const GuiRect& _targetRect, int _retzone, int _hypzone, int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) :
			targetRect(GuiRectToQuad(_targetRect)), retzone(_retzone), hypzone(_hypzone),
			text(_text), colour(_colour), fontIndex(_fontIndex), lastCellHeight(10.0f)
		{
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			lastCellHeight = outputRect.bottom - outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			builder.SetClipRect(Quad((float)targetRect.left, (float)targetRect.top, (float)targetRect.right, (float)targetRect.bottom));

			builder.SetCursor(Vec2{ targetRect.left, targetRect.top });

			const float retX = targetRect.right - retzone;
			const float hypX = targetRect.right - hypzone;

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

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
						DrawNextGlyph(' ', builder);
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

		Vec2i RenderVerticalCentredText(IGuiRenderContext& grc, int32 x, int32 top, RGBAb colour, const wchar_t* text, int fontIndex)
		{
			HorizontalCentredText job(fontIndex, text, FontColourFromRGBAb(colour));
			Vec2i span = grc.EvalSpan(Vec2i{ 0,0 }, job);
			grc.RenderText(Vec2i{ x - (span.x >> 1), top }, job);
			return span;
		}

		void RenderHorizontalCentredText(IGuiRenderContext& gr, const wchar_t* txt, RGBAb colour, int fontSize, const Vec2i& topLeft)
		{
			HorizontalCentredText hw(1, txt, FontColourFromRGBAb(RGBAb{ 255, 255, 255, 255 }));
			gr.RenderText(topLeft, hw);
		}

		void DrawRectangle(IGuiRenderContext& grc, const GuiRect& grect, RGBAb diag, RGBAb backdiag)
		{
			Quad rect{ (float)grect.left, (float)grect.top, (float)grect.right, (float)grect.bottom };
			GuiVertex q[] =
			{
				{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
				{ rect.right, rect.top,    1.0f, 0, backdiag, 0, 0, 0 },
				{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
				{ rect.right, rect.bottom, 1.0f, 0, diag,     0, 0, 0 },
				{ rect.left,  rect.bottom, 1.0f, 0, backdiag, 0, 0, 0 },
				{ rect.left,  rect.top,    1.0f, 0, diag,     0, 0, 0 },
			};

			grc.AddTriangle(q);
			grc.AddTriangle(q + 3);
		}

		void DrawBorderAround(IGuiRenderContext& grc, const GuiRect& rect, const Vec2i& width, RGBAb diag, RGBAb backdiag)
		{
			GuiRect topRect{ rect.left - width.x, rect.top - width.y, rect.right, rect.top };
			DrawRectangle(grc, topRect, diag, diag);

			GuiRect bottomRect{ rect.left - width.x, rect.bottom, rect.right, rect.bottom + width.y };
			DrawRectangle(grc, bottomRect, backdiag, backdiag);

			GuiRect leftRect{ rect.left - width.x, rect.top, rect.left, rect.bottom };
			DrawRectangle(grc, leftRect, backdiag, backdiag);

			GuiRect rightRect{ rect.right, rect.top - width.y, rect.right + width.x, rect.bottom + width.y };
			DrawRectangle(grc, rightRect, diag, diag);
		}

		Fonts::IDrawTextJob& CreateHorizontalCentredText(StackSpaceGraphics& ss, int fontIndex, const wchar_t* text, RGBAb colour)
		{
			static_assert(sizeof(ss) > sizeof(HorizontalCentredText), "Increase buffer size");
			HorizontalCentredText *hct = new (&ss) HorizontalCentredText(fontIndex, text, FontColourFromRGBAb(colour));
			return *hct;
		}

		Fonts::IDrawTextJob& CreateLeftAlignedText(StackSpaceGraphics& ss, const GuiRect& targetRect, int retzone, int hypzone, int fontIndex, const wchar_t* text, RGBAb colour)
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
#include <rococo.types.h>
#include <rococo.fonts.h>
#include <rococo.renderer.h>

#include <limits>

using namespace Rococo;
using namespace Rococo::Fonts;

namespace
{
	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;

	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) : text(_text), colour(_colour), fontIndex(_fontIndex)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

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
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};
}

namespace Dystopia
{
	void RenderHorizontalCentredText(IGuiRenderContext& gr, const wchar_t* txt, RGBAb colour, int fontSize, const Vec2i& topLeft)
	{
		HorizontalCentredText hw(1, txt, FontColourFromRGBAb(RGBAb{ 255, 255, 255, 255 }));
		gr.RenderText(topLeft, hw);
	}
}
#include <rococo.api.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.strings.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <new>

#include "csv.inl"

namespace
{
	using namespace Rococo;
	using namespace Rococo::Graphics::Fonts;

	class GlyphSet : public IGlyphSet
	{
	private:
		enum { MAX_INDEX = 255 };
		int specIndex;
		char specName[64];
		float fontHeight;
		int fontAscent;
		int defaultChar;
		Glyph glyphs[256];

	public:
		GlyphSet(CSVStream& csv, int specIndex)
		{
			this->specIndex = specIndex;

			int charsInSet;
			StringBuffer specCapture(specName, 64);
			csv >> specCapture;
			csv >> fontHeight;
			csv >> fontAscent;
			csv >> charsInSet;
			csv >> defaultChar;

			for (int i = 0; i < 256; ++i)
			{
				glyphs[i].topLeft.y = -1;
			}

			if (defaultChar < 0 || defaultChar > MAX_INDEX)
			{
				Throw(0, "Bad defaultChar %d in spec %d of %s", defaultChar, specIndex, csv.Filename());
			}

			for (int i = 0; i < charsInSet; ++i)
			{
				csv.AdvanceToNextLine();

				int charValue;
				csv >> charValue;

				if (charValue < 0 || charValue > MAX_INDEX)
				{
					Throw(0, "Bad charValue %d in spec %d of %s", charValue, specIndex, csv.Filename());
				}

				Glyph& g = glyphs[charValue];
				ValidateItem vABC("ABC");
				csv >> vABC;
				csv >> g.A;
				csv >> g.B;
				csv >> g.C;
				csv >> g.topLeft.x;
				csv >> g.topLeft.y;

				g.bottomRight.x = g.topLeft.x + g.B;
				g.bottomRight.y = g.topLeft.y + fontHeight;
			}

			if (glyphs[defaultChar].topLeft.y < 0) Throw(0, "defaultChar %d in spec %d of %s was not defined", defaultChar, specIndex, csv.Filename());

			for (int i = 0; i < 256; ++i)
			{
				if (glyphs[i].topLeft.y == -1) glyphs[i] = glyphs[defaultChar];
			}
		}

		const char* Name() const override { return specName; }
		float FontHeight() const override { return fontHeight; }
		int FontAscent() const override { return fontAscent; }
		const Glyph& operator[](unsigned char index) const override { return index < MAX_INDEX ? glyphs[index] : glyphs[defaultChar]; }
	};

	class GlyphsSpecSets : public IFontSupervisor
	{
	private:
		GlyphSet** glyphsets;
		int numberOfGlyphSets;

		Vec4 scale;
	public:
		GlyphsSpecSets(cstr sourceName, const char* csvData, size_t nBytes)
		{
			CSVStream csv(sourceName, csvData, nBytes);

			int32 width, height;

			ValidateItem vSpecs("NumberOfSpecs");
			csv >> vSpecs >> numberOfGlyphSets >> width >> height;

			scale.x = (float)width;
			scale.y = (float)height;
			scale.z = 1.0f / width;
			scale.w = 1.0f / height;

			glyphsets = new GlyphSet*[numberOfGlyphSets];

			for (int j = 0; j < numberOfGlyphSets; ++j)
			{
				csv.AdvanceToNextLine();
				int specIndex;
				ValidateItem vSpec("Spec");
				csv >> vSpec >> specIndex;

				if (specIndex != j) Throw(0, "Spec index %d did not match j = %d in %s", specIndex, j, sourceName);

				GlyphSet* set = new GlyphSet(csv, j);
				glyphsets[j] = set;
			}

			if (numberOfGlyphSets == 0)
			{
				Throw(0, "There are no glyphsets in the font '%s'", sourceName);
			}
		}

		virtual ~GlyphsSpecSets()
		{
			for (int i = 0; i < numberOfGlyphSets; ++i)
			{
				delete glyphsets[i];
			}

			delete[] glyphsets;
		}

		void Free() override
		{
			delete this;
		}

		int NumberOfGlyphSets() const override
		{ 
			return numberOfGlyphSets;
		}

		const IGlyphSet& operator[](int index) const override
		{
			if (index < 0 || index >= numberOfGlyphSets)
			{
				index = 0;
			}
			return *glyphsets[index]; 
		}

		const Vec4& TextureSpan() const override
		{
			return scale;
		}
	};

	struct IGlyphCallback
	{
		virtual void OnGlyph(int column, const GuiRectf* clippingRect, const Glyph& g, float scale, float targetHeight, FontColour colour, bool isShadowed, GuiRectf& outputRect) = 0;
		virtual void OnNewLine() = 0;
	};

	class GlyphBuilder : public IGlyphBuilder
	{
	public:
		FontColour colour;
		bool shadow;
		int fontIndex;
		int fontHeight;
		int spacesPerTab;
		Vec2 cursor;
		GuiRectf clipRect;
		const GlyphsSpecSets& fg;
		IGlyphCallback& glyphCallback;
		int column;

		GlyphBuilder(const IFont& _fg, IGlyphCallback& _glyphCallback) :
			fg(static_cast<const GlyphsSpecSets&>(_fg)),
			colour(0xFFFFFFFF),
			shadow(false),
			fontIndex(0),
			spacesPerTab(4),
			clipRect(0, 0, 0, 0),
			glyphCallback(_glyphCallback),
			column(0),
			fontHeight(10)
		{
		}

		void SetFontHeight(int _fontHeight) override
		{
			fontHeight = _fontHeight;
		}

		void AppendChar(char c, GuiRectf& outputRect) override
		{
			float specHeight = fg[fontIndex].FontHeight();
			const Glyph& g = fg[fontIndex][c];

			float scale = fontHeight / specHeight;

			column++;
			if (c == L'\t')
			{
				const Glyph& space = fg[fontIndex][' '];
				for (int i = 0; i < spacesPerTab; ++i)
				{
					glyphCallback.OnGlyph(column, Width(clipRect) != 0 ? &clipRect : NULL, space, scale, (float) fontHeight, colour, shadow, outputRect);
				}
			}
			else
			{
   				glyphCallback.OnGlyph(column, Width(clipRect) != 0 ? &clipRect : NULL, g, scale, (float) fontHeight, colour, shadow, outputRect);
			}
		}

		Vec2 GetCursor() override
		{
			return cursor;
		}

		GuiRectf GetClipRect() const override
		{
			return clipRect;
		}

		void SetClipRect(const GuiRectf& clipRect) override
		{
			this->clipRect = clipRect;
		}

		void SetFirstColumnIndex(int index) override
		{
			column = index;
		}

		void SetCursor(const Vec2& cursor) override
		{
			this->cursor = Vec2{ floorf(cursor.x), floorf(cursor.y) };
		}

		void SetTextColour(FontColour colour) override
		{
			this->colour = colour;
		}

		void SetShadow(bool shadow) override
		{
			this->shadow = shadow;
		}

		float SetFontIndex(int fontIndex) override
		{
			this->fontIndex = fontIndex;
			return fg[fontIndex].FontHeight();
		}

		void SetSpacesPerTab(int spacesPerTab) override
		{
			this->spacesPerTab = spacesPerTab;
		}
	};

	class GlyphBuilderAndProjector : private IGlyphCallback
	{
		enum { GLYPH_BUILDER_MEM = 128 };
		char glyphBuilderMemory[GLYPH_BUILDER_MEM];
		GlyphBuilder* builder;
		IGlyphRenderPipeline& pipeline;
		float lastHeight;
		float left;
		FontColour shadowColour;
	public:
		GlyphBuilderAndProjector(const IFont& _fg, IGlyphRenderPipeline& _pipeline) :
			pipeline(_pipeline), lastHeight(0)
		{
			static_assert(sizeof(GlyphBuilder) <= GLYPH_BUILDER_MEM, "Increase GlyphBuilderAndProjector::GLYPH_BUILDER_MEM");
			builder = new (glyphBuilderMemory)GlyphBuilder(_fg, *this);
			left = builder->cursor.x;
		}

		~GlyphBuilderAndProjector()
		{
			builder->~GlyphBuilder();
		}

		IGlyphBuilder& Builder() { return *builder; }

		void OnGlyph(int column, const GuiRectf* clipRect, const Glyph& g, float scale, float targetHeight, FontColour colour, bool isShadowed, GuiRectf& outputRect) override
		{
			UNUSED(column);
			bool isVisible = true;
			if (clipRect != nullptr)
			{
				if (builder->cursor.y + targetHeight < clipRect->top)
				{
					isVisible = false;
				}
				else if (builder->cursor.y > clipRect->bottom)
				{
					isVisible = false;
				}
				else if ((builder->cursor.x + scale * (g.A + g.B)) < clipRect->left)
				{
					isVisible = false;
				}
				else if (builder->cursor.x > clipRect->right)
				{
					isVisible = false;
				}
			}

			GuiRectf glyphClipRect = clipRect == NULL ? GuiRectf(0, 0, 16384.0f, 16384.0f) : *clipRect;

			// Since it is visible the lowest line in the glyph must be below the top line in the clip rect, and the top line in the glyph above the bottom line in the clip rect
			// also the left side of the glyph is to the left of the right clip rect. amd the right side of the glyph is to the right of the left clip rect

			float dx = scale * (g.bottomRight.x - g.topLeft.x);
			float dy = scale * (g.bottomRight.y - g.topLeft.y);

			if (isVisible && isShadowed)
			{
				Vec2 shadowCursor = builder->cursor;
				shadowCursor.x += 1.0f;
				shadowCursor.y += 1.0f;
				shadowCursor.x += scale * g.A;

				pipeline.ClipGlyph(glyphClipRect, shadowCursor, g.topLeft, g.bottomRight, scale, shadowColour);

				shadowCursor = builder->cursor;
				shadowCursor.x += 2.0f;
				shadowCursor.y += 2.0f;
				shadowCursor.x += scale * g.A;

				pipeline.ClipGlyph(glyphClipRect, shadowCursor, g.topLeft, g.bottomRight, scale, shadowColour);
			}

			builder->cursor.x += scale * g.A;

			outputRect = GuiRectf(builder->cursor.x, builder->cursor.y, builder->cursor.x + dx, builder->cursor.y + dy);

			if (isVisible)
			{
				pipeline.ClipGlyph(glyphClipRect, builder->cursor, g.topLeft, g.bottomRight, scale, colour);
			}

			builder->cursor.x += scale * (g.B + g.C);

			lastHeight = targetHeight;
		}

		void OnNewLine() override
		{
			left = builder->cursor.x;
			builder->cursor.y += lastHeight;
		}
	};

	class GlyphPipeline : public IGlyphRenderPipeline
	{
	public:
		IGlyphRenderer& renderer;

		GlyphPipeline(IGlyphRenderer& _renderer) : renderer(_renderer)
		{

		}

		void Free()
		{
			delete this;
		}

		void ClipGlyph(const GuiRectf& glyphClip, cr_vec2 cursor, cr_vec2 glyphTopLeft, cr_vec2 glyphBottomRight, float scale, FontColour rgbaColour) override
		{
			Vec2 t0 = glyphTopLeft, t1 = glyphBottomRight;
			Vec2 p = cursor;

			float du = t1.x - t0.x;
			float dv = t1.y - t0.y;

			float dx = du * scale;
			float dy = dv * scale;

			float ddx0 = 0;
			float ddx1 = 0;
			float ddy0 = 0;
			float ddy1 = 0;

			if (p.x >= glyphClip.right) return;
			if (p.x + dx <= glyphClip.left) return;
			if (p.y + dy <= glyphClip.top) return;
			if (p.y >= glyphClip.bottom) return;

			if (p.x < glyphClip.left)
			{
				ddx0 = glyphClip.left - p.x;
				p.x += ddx0;
				dx -= ddx0;
				t0.x += ddx0 / scale;
			}

			if ((p.x + dx) > glyphClip.right)
			{
				ddx1 = p.x + dx - glyphClip.right;
				dx -= ddx1 / scale;
			}

			if (p.y < glyphClip.top)
			{
				ddy0 = glyphClip.top - p.y;
				p.y += ddy0;
				dy -= ddy0;
				t0.y += ddy0 / scale;
			}

			if ((p.y + dy) > glyphClip.bottom)
			{
				ddy1 = (p.y + dy) - glyphClip.bottom;
				dy -= ddy1;
				t1.y -= ddy1 / scale;
			}

			Vec2 p1 = { p.x + dx, p.y + dy };
			renderer.DrawGlyph(t0, t1, p, p1, rgbaColour);
		}
	};
}

namespace Rococo::Graphics
{
	namespace Fonts
	{
		IFontSupervisor* LoadFontCSV(cstr srcName, const char* srcData, size_t srcLenBytes)
		{
			return new GlyphsSpecSets(srcName, srcData, srcLenBytes);
		}

		IGlyphRenderPipeline* CreateGlyphRenderPipeline(char* stackBuffer, size_t nBytes, IGlyphRenderer& _renderer)
		{
			if (nBytes < sizeof(GlyphPipeline))
			{
				Throw(0, "Insufficient buffer. Make sure it is sizeof(DeferredProjector) or greater");
			}
			return new (stackBuffer)GlyphPipeline(_renderer);
		}

		void RouteDrawTextBasic(const Vec2i& pos, IDrawTextJob& job, const IFont& font, IGlyphRenderPipeline& pipeline, const GuiRectf& clipRect)
		{
			GlyphBuilderAndProjector gbandp(font, pipeline);
			gbandp.Builder().SetCursor(Dequantize(pos));
			gbandp.Builder().SetClipRect(clipRect);
			job.OnDraw(gbandp.Builder());
		}
	}
}
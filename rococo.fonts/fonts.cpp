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
	using namespace Rococo::Fonts;

	bool IsPointIn(const GuiRectf& clipRect, const Vec2& p)
	{
		if (p.x >= clipRect.left && p.x <= clipRect.right && p.y >= clipRect.top && p.y <= clipRect.bottom)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	class GlyphSet: public IGlyphSet
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

		virtual const char* Name() const { return specName; }
		virtual float FontHeight() const { return fontHeight; }
		virtual int FontAscent() const { return fontAscent; }
		virtual const Glyph& operator[](unsigned char index) const { return index < MAX_INDEX ? glyphs[index] : glyphs[defaultChar]; }
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

		virtual void Free()
		{
			delete this;
		}

		virtual int NumberOfGlyphSets() const 
		{ 
			return numberOfGlyphSets;
		}

		virtual const IGlyphSet& operator[](int index) const
		{
			if (index < 0 || index >= numberOfGlyphSets)
			{
				index = 0;
			}
			return *glyphsets[index]; 
		}

		virtual const Vec4& TextureSpan() const 
		{
			return scale;
		}
	};

	struct IGlyphCallback
	{
		virtual void OnGlyph(int column, const GuiRectf* clippingRect, const Glyph& g, float height, FontColour colour, bool isShadowed, GuiRectf& outputRect) = 0;
		virtual void OnNewLine() = 0;
	};

	class GlyphBuilder : public IGlyphBuilder
	{
	public:
		FontColour colour;
		bool shadow;
		int fontIndex;
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
			column(0)
		{
		}

		virtual void AppendChar(char c, GuiRectf& outputRect)
		{
			float specHeight = fg[fontIndex].FontHeight();
			const Glyph& g = fg[fontIndex][c];

			column++;
			if (c == L'\t')
			{
				const Glyph& space = fg[fontIndex][' '];
				for (int i = 0; i < spacesPerTab; ++i)
				{
					glyphCallback.OnGlyph(column, Width(clipRect) != 0 ? &clipRect : NULL, space, specHeight, colour, shadow, outputRect);
				}
			}
			else
			{
				glyphCallback.OnGlyph(column, Width(clipRect) != 0 ? &clipRect : NULL, g, specHeight, colour, shadow, outputRect);
			}
		}

		virtual Vec2 GetCursor()
		{
			return cursor;
		}

		virtual GuiRectf GetClipRect() const
		{
			return clipRect;
		}

		virtual void SetClipRect(const GuiRectf& clipRect)
		{
			this->clipRect = clipRect;
		}

		virtual void SetFirstColumnIndex(int index)
		{
			column = index;
		}

		virtual void SetCursor(const Vec2& cursor)
		{
			this->cursor = Vec2{ floorf(cursor.x), floorf(cursor.y) };
		}

		virtual void SetTextColour(FontColour colour)
		{
			this->colour = colour;
		}

		virtual void SetShadow(bool shadow)
		{
			this->shadow = shadow;
		}

		virtual float SetFontIndex(int fontIndex)
		{
			this->fontIndex = fontIndex;
			return fg[fontIndex].FontHeight();
		}

		virtual void SetSpacesPerTab(int spacesPerTab)
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

		virtual void OnGlyph(int column, const GuiRectf* clipRect, const Glyph& g, float height, FontColour colour, bool isShadowed, GuiRectf& outputRect)
		{
			bool isVisible = clipRect == NULL || (IsPointIn(*clipRect, builder->cursor) || IsPointIn(*clipRect, builder->cursor + Vec2{ g.A + g.B, height }));
			GuiRectf glyphClipRect = clipRect == NULL ? GuiRectf(0, 0, 16384.0f, 16384.0f) : *clipRect;

			// Since it is visible the lowest line in the glyph must be below the top line in the clip rect, and the top line in the glyph above the bottom line in the clip rect
			// also the left side of the glyph is to the left of the right clip rect. amd the right side of the glyph is to the right of the left clip rect

			float dy = (g.bottomRight.y - g.topLeft.y);

			if (isVisible && isShadowed)
			{
				Vec2 shadowCursor = builder->cursor;
				shadowCursor.x += 1.0f;
				shadowCursor.y += 1.0f;
				shadowCursor.x += g.A;

				pipeline.ClipGlyph(glyphClipRect, shadowCursor, g.topLeft, g.bottomRight, shadowColour);

				shadowCursor = builder->cursor;
				shadowCursor.x += 2.0f;
				shadowCursor.y += 2.0f;
				shadowCursor.x += g.A;

				pipeline.ClipGlyph(glyphClipRect, shadowCursor, g.topLeft, g.bottomRight, shadowColour);
			}

			builder->cursor.x += g.A;

			outputRect = GuiRectf(builder->cursor.x, builder->cursor.y, builder->cursor.x + g.bottomRight.x - g.topLeft.x, builder->cursor.y + dy);

			if (isVisible)
			{
				pipeline.ClipGlyph(glyphClipRect, builder->cursor, g.topLeft, g.bottomRight, colour);
			}

			builder->cursor.x += g.B + g.C;

			lastHeight = max(lastHeight, height);
		}

		virtual void OnNewLine()
		{
			left = builder->cursor.x;
			builder->cursor.y += lastHeight;
			lastHeight = 0;
		}
	};

	class GlyphPipeline : public IGlyphRenderPipeline
	{
	public:
		IGlyphRenderer& renderer;

		GlyphPipeline(IGlyphRenderer& _renderer) : renderer(_renderer)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual void ClipGlyph(const GuiRectf& glyphClip, const Vec2& cursor, const Vec2& glyphTopLeft, const Vec2& glyphBottomRight, FontColour rgbaColour)
		{
			Vec2 t0 = glyphTopLeft, t1 = glyphBottomRight;
			Vec2 p = cursor;

			float dx = t1.x - t0.x;
			float dy = t1.y - t0.y;

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
				t0.x += ddx0;
			}

			if ((p.x + dx) > glyphClip.right)
			{
				ddx1 = p.x + dx - glyphClip.right;
				dx -= ddx1;
			}

			if (p.y < glyphClip.top)
			{
				ddy0 = glyphClip.top - p.y;
				p.y += ddy0;
				dy -= ddy0;
				t0.y += ddy0;
			}

			if ((p.y + dy) > glyphClip.bottom)
			{
				ddy1 = (p.y + dy) - glyphClip.bottom;
				dy -= ddy1;
			}

			renderer.DrawGlyph(t0, p, dx, dy, rgbaColour);
		}
	};
}

namespace Rococo
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
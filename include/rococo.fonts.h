#ifndef ROCOCO_FONTS_H
#define ROCOCO_FONTS_H

#ifndef Rococo_TYPES_H
# error include <rococo.types.h> before including this file
#endif

namespace Rococo
{
	namespace Fonts
	{
		typedef uint32 FontColour;

		struct IGlyphClipper
		{
			virtual void ClipGlyph(const Quad& glyphClipRect, const Vec2& p, const Vec2& t0, const Vec2& t1, FontColour colour) = 0;
		};

		struct IGlyphRenderer
		{
			virtual void DrawGlyph(const Vec2& uvTopLeft, const Vec2& posTopLeft, float dx, float dy, Fonts::FontColour fcolour) = 0;
		};

		struct IGlyphRenderPipeline : public IGlyphClipper
		{
		};

		IGlyphRenderPipeline* CreateGlyphRenderPipeline(char* stackBuffer, size_t nBytes, IGlyphRenderer& renderer);

		struct IGlyphBuilder
		{
			virtual void AppendChar(char c, Quad& outputRect) = 0;
			virtual void SetClipRect(const Quad& rect) = 0;
			virtual void SetCursor(const Vec2& bottomLeftOfNextGlyph) = 0;
			virtual void SetFirstColumnIndex(int index) = 0;
			virtual void SetTextColour(FontColour colour) = 0;
			virtual void SetShadow(bool isEnabled) = 0;
			virtual float SetFontIndex(int fontIndex) = 0; // Sets the font contained in the font set, and returns the font height
			virtual void SetSpacesPerTab(int spacesPerTab) = 0;
		};

		struct IDrawTextJob
		{
			virtual void OnDraw(IGlyphBuilder& builder) = 0;
		};

		struct Glyph // Gives the ABC widths of a glyph, and also the position of the glyph in the font texture
		{
			float A;
			float B;
			float C;
			Vec2 topLeft;
			Vec2 bottomRight;
		};

		struct IGlyphSet // represents a sequence of ASCII glyphs of the same height
		{
			virtual const char* Name() const = 0;
			virtual float FontHeight() const = 0;
			virtual int FontAscent() const = 0;
			virtual const Glyph& operator[](unsigned char index) const = 0;
		};

		struct IFont // represents a number of glyph sets
		{
			virtual int NumberOfGlyphSets() const = 0;
			virtual const IGlyphSet& operator[](int index) const = 0;
			virtual const Vec4& TextureSpan() const = 0;
		};

		struct IFontSupervisor : public IFont
		{
			virtual void Free() = 0;
		};

		IFontSupervisor* LoadFontCSV(const wchar_t* srcname, const char* data, size_t dataLength);
		void RouteDrawTextBasic(const Vec2i& pos, IDrawTextJob& job, const IFont& font, IGlyphRenderPipeline& pipeline, const Quad& clipRect);

		int FindFirstFont(IFont& font, const char* specToken, bool throwOnError);
		int GetFontMatchingHeight(IFont& font, const char* specToken, float height, bool throwOnError);
		int GetFontNearestHeight(IFont& font, const char* specToken, float height, bool throwOnError);
	} // Fonts
} // Bloke

#endif
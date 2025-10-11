// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#ifndef ROCOCO_FONTS_H
#define ROCOCO_FONTS_H

#ifndef Rococo_TYPES_H
# error include <rococo.types.h> before including this file
#endif

namespace Rococo::Graphics
{
	namespace Fonts
	{
		typedef uint32 FontColour;

		struct IGlyphClipper
		{
			virtual void ClipGlyph(const GuiRectf& glyphClipRect, cr_vec2 p, cr_vec2 topLeft, cr_vec2 bottomRight, float scale, FontColour colour) = 0;
		};

		struct IGlyphRenderer
		{
			virtual void DrawGlyph(cr_vec2 glyphTopLeft, cr_vec2 glyphBottomRight, cr_vec2 posTopLeft, cr_vec2 posBottomRight, Fonts::FontColour fcolour) = 0;
		};

		struct IGlyphRenderPipeline : public IGlyphClipper
		{
		};

		IGlyphRenderPipeline* CreateGlyphRenderPipeline(char* stackBuffer, size_t nBytes, IGlyphRenderer& renderer);

		struct IGlyphBuilder
		{
			virtual void AppendChar(char c, GuiRectf& outputRect) = 0;
			virtual Vec2 GetCursor() = 0;
			virtual GuiRectf GetClipRect() const = 0;
			virtual void SetClipRect(const GuiRectf& rect) = 0;
			virtual void SetCursor(const Vec2& topLeftOfNextGlyph) = 0;
			virtual void SetFirstColumnIndex(int index) = 0;
			virtual void SetTextColour(FontColour colour) = 0;
			virtual void SetShadow(bool isEnabled) = 0;
			virtual void SetFontHeight(int fontHeightPixels) = 0;
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

		IFontSupervisor* LoadFontCSV(cstr friendlyName, const char* data, size_t dataLength);
		void RouteDrawTextBasic(const Vec2i& pos, IDrawTextJob& job, const IFont& font, IGlyphRenderPipeline& pipeline, const GuiRectf& clipRect);

		int FindFirstFont(IFont& font, const char* specToken, bool throwOnError);
		int GetFontMatchingHeight(IFont& font, const char* specToken, float height, bool throwOnError);
		int GetFontNearestHeight(IFont& font, const char* specToken, float height, bool throwOnError);
	} // Fonts
} // Bloke

#endif
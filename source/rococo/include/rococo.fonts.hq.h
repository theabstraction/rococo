#pragma once

#include <rococo.types.h>

namespace Rococo::Imaging
{
	template<class T> struct IImagePopulator;
}

namespace Rococo::Graphics::Fonts
{
	ROCOCO_INTERFACE IFontGlyphBuilder
	{
		virtual void AddGlyph(wchar_t code) = 0;
		virtual void AddGlyph(unsigned char asciiValue) = 0;
	};

	struct ArrayGlyph
	{
		int32 A; // dx cursor rollback before rendering
		int32 B; // Width in pixels
		int32 C; // dx cursor advance after rendering
		int32 Index; // enumeration index
	};

	struct GlyphDesc
	{
		const ArrayGlyph& glyph;
		const char32_t charCode;

		GlyphDesc(const ArrayGlyph& _glyph, const char32_t _charCode) : glyph(_glyph), charCode(_charCode) {}
	};

	struct ArrayFontMetrics
	{
		int32 height;
		int32 ascent;
		int32 descent;
		int32 italic;
		int32 weight;
		int32 internalLeading;
		int32 imgWidth;
		int32 imgHeight;
	};

	ROCOCO_INTERFACE IArrayFont
	{
		virtual int32 NumberOfGlyphs() const = 0;

		virtual const ArrayGlyph& operator[](unsigned char) = 0;
		virtual const ArrayGlyph& operator[](wchar_t) = 0;
		virtual const ArrayGlyph& operator[](char32_t) = 0;

		virtual const ArrayFontMetrics& Metrics() const = 0;

		/* Enumerate the glyphs in the registered glyphs for this font
			The callback gives the ABC widths of each glyph as well as the UNICODE value
		*/
		virtual void ForEachGlyph(IEventCallback<const GlyphDesc>& cb) = 0;

		/*
			GenerateImage will work for characters outside the glyph set, but this is abuse of the API
			In general the consumer uses ForEachGlyph to iterate over all registered glyphs
			then calls GenerateImage in the callback to get an image, and saves it to memory or disk
		*/
		virtual void GenerateImage(const char32_t charCode, Imaging::IImagePopulator<GRAYSCALE>& populator) = 0;
	};

	ROCOCO_INTERFACE IArrayFontSupervisor : IArrayFont
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IArrayFontSet
	{
		virtual void Populate(IFontGlyphBuilder & builder);
	};

	struct FontSpec
	{
		cstr fontName;
		int32 height;
		int32 weight;
		boolean32 italic;
		int sysFontId = -1;
	};

	/*
		Implement your own IArrayFontSet. The populate function should call IFontGlyphBuilder::AddGlyph
		for every UNICODE value you want in the font image set. Since this can be rather expensive in
		memory for the full UNICODE set, I suggest you restrict glyphs to ASCII 32 to 126 and add whatever
		extra characters your particular application needs.
	*/
	IArrayFontSupervisor* CreateOSFont(IArrayFontSet& glyphSet, const FontSpec& spec);

	ROCOCO_INTERFACE IHQTextBuilder
	{
		virtual const Fonts::ArrayFontMetrics & Metrics() const = 0;
		virtual void SetColour(RGBAb colour) = 0;
		virtual void SetCursor(Vec2 bottomLeftNextCharCell) = 0;
		virtual void Write(char c, GuiRectf* outputBounds) = 0;
		virtual void Write(wchar_t c, GuiRectf* outputBounds) = 0;
		virtual void Write(char32_t c, GuiRectf* outputBounds) = 0;
	};

	ROCOCO_INTERFACE IHQTextJob
	{
		virtual void Render(IHQTextBuilder & builder) = 0;
	};
}


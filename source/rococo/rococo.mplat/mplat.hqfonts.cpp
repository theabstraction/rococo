#include <rococo.mplat.h>
#include <unordered_set>
#include <array>
#include <rococo.strings.h>
#include <rococo.fonts.hq.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

const char* const DEFAULT_FONT_NAME = "Tahoma";

struct HQFonts : IHQFontsSupervisor, Fonts::IArrayFontSet
{
	std::unordered_set<char32_t> glyphs;

	IHQFontResource& hq;
	HQFonts(IHQFontResource& _hq) : hq(_hq) {}
	virtual ~HQFonts() {}
	void Free() override { delete this; }

	HQFont hqFontType = HQFont::TitleFont;
	HString faceName = DEFAULT_FONT_NAME;
	int32 heightInPixels = 20;
	boolean32 italics = false;
	boolean32 bold = false;

	// This was added to facilitate Gui-Retained fonts, so is a little out-of-place
	ID_FONT BindFont(const HQFontDef& fontDef, const fstring& fontFamily) override
	{
		Fonts::FontSpec spec;
		spec.fontName = fontFamily;
		spec.height = fontDef.fontSize;
		spec.italic = fontDef.isItalic;
		spec.weight = fontDef.isBold ? 700 : 400;

		struct : Fonts::IArrayFontSet
		{
			void Populate(Fonts::IFontGlyphBuilder& builder) override
			{
				for(wchar_t c = 32; c <= 127; c++)
				{
					builder.AddGlyph(c);
				}
			}
		} asciiGlyphs;

		ID_FONT idFont = hq.CreateOSFont(asciiGlyphs, spec);
		return idFont;
	}

	int32 GetHeight(ID_FONT fontId) override
	{
		const auto& metrics = hq.GetFontMetrics(fontId);
		return metrics.height;
	}

	void Build(Rococo::Graphics::HQFont hqFont) override
	{
		hqFontType = hqFont;
		Clear();
	}

	void SetZoomLevel(float zoomLevel) override
	{
		hq.SetZoomLevel(zoomLevel);
	}

	void Clear() override
	{
		glyphs.clear();
		heightInPixels = 20;
		faceName = DEFAULT_FONT_NAME;
		italics = false;
		bold = false;
	}

	void SetFaceName(const fstring& name) override
	{
		faceName = name.length > 0 ? (cstr)name : DEFAULT_FONT_NAME;
		faceName = name;
	}

	void SetHeight(int32 dyPixels) override
	{
		heightInPixels = dyPixels;
	}

	void MakeItalics() override
	{
		italics = true;
	}

	void MakeBold() override
	{
		bold = true;
	}

	void AddUnicode32Char(int32 unicodeValue) override
	{
		if (unicodeValue < 0 || unicodeValue > 0x00007FFF)
		{
			Throw(0, "%s: Unicode character values must be between 0 and 0x7FFF", __ROCOCO_FUNCTION__);
		}

		glyphs.insert((uint32)unicodeValue);
	}

	void AddCharacters(const fstring& s) override
	{
		if (s.length == 0) return;

		for (auto* p = s.buffer; *p != 0; p++)
		{
			AddUnicode32Char((char32_t)(unsigned char) *p);
		}
	}

	void AddRange(int32 unicodeStartChar, int32 unicodeEndChar) override
	{
		if (unicodeStartChar > unicodeEndChar)
		{
			std::swap(unicodeStartChar, unicodeEndChar);
		}

		for (int32 i = unicodeStartChar; i <= unicodeEndChar; ++i)
		{
			AddUnicode32Char(i);
		}
	}

	ID_FONT Commit() override
	{
		Fonts::FontSpec spec;
		spec.fontName = faceName;
		spec.height = heightInPixels;
		spec.italic = italics;
		spec.weight = bold ? 700 : 400;
		spec.sysFontId = static_cast<int>(hqFontType);

		if (glyphs.empty())
		{
			Throw(0, "%s: no glyphs have been added", __ROCOCO_FUNCTION__);
		}

		ID_FONT idFont = hq.CreateOSFont(*this, spec);

		Clear();

		return idFont;
	}

	void Populate(Fonts::IFontGlyphBuilder& builder) override
	{
		for (auto c : glyphs)
		{
			builder.AddGlyph((wchar_t)c);
		}
	}
};

namespace Rococo
{
	namespace Graphics
	{
		IHQFontsSupervisor* CreateHQFonts(IHQFontResource& hq)
		{
			return new HQFonts(hq);
		}
	}
}
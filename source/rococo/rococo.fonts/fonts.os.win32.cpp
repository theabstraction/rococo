#define NOMINMAX

#include <rococo.api.h>
#include <rococo.strings.h>
#include <unordered_map>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.fonts.hq.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::Graphics::Fonts;

#pragma pack(push,1)
struct BGRAb
{
	uint8 blue = 0;
	uint8 green = 0;
	uint8 red = 0;
	uint8 alpha = 0;
};
#pragma pack(pop)

template<class T> struct AutoGDI
{
	T handle = nullptr;
	operator T() { return handle; }

	operator bool() const { return handle != nullptr; }

	AutoGDI& operator = (T _handle)
	{
		this->handle = _handle;
		return *this;
	}

	~AutoGDI()
	{
		if (!handle)
		{
			DeleteObject((HGDIOBJ)handle);
		}
	}
};

struct AutoDC
{
	HDC handle = nullptr;
	operator HDC() { return handle; }

	operator bool() const { return handle != nullptr; }

	~AutoDC()
	{
		if (!handle)
		{
			ReleaseDC(nullptr, handle);
		}
	}
};

struct FontGlyphs : IFontGlyphBuilder
{
	std::unordered_map<char32_t, ArrayGlyph> codes;

	void ForEachGlyph(IEventCallback<const GlyphDesc>& cb)
	{
		for (auto i : codes)
		{
			cb.OnEvent(GlyphDesc(i.second, i.first));
		}
	}

	void AddGlyph(unsigned char code) override
	{
		codes[code] = ArrayGlyph{ 0,0,0 };
	}

	void AddGlyph(wchar_t code) override
	{
		codes[code] = ArrayGlyph{ 0,0,0 };
	}

	int32 ComputeMaxABCWidth(HDC hDC, bool isFixedWidth)
	{
		int32 width = 0;

		int32 enumerationIndex = 0;
		for (auto& it : codes)
		{
			ABC abc;

			char32_t i = it.first;
			if (isFixedWidth)
			{
				INT charWidth;
				GetCharWidthW(hDC, i, i, &charWidth);

				it.second.A = 0;
				it.second.B = charWidth;
				it.second.C = 0;

				width = max(width, charWidth);
			}
			else if (GetCharABCWidthsW(hDC, i, i, &abc))
			{
				int a = abc.abcA;
				uint32 b = abc.abcB;
				int c = abc.abcC;

				it.second.A = a;
				it.second.B = b;
				it.second.C = c;

				int totalWidth = a + b + c;
				width = max(width, totalWidth);
			}

			it.second.Index = enumerationIndex++;
		}

		return width;
	}
};

struct WindowsArrayFont : IArrayFontSupervisor
{
	AutoGDI<HFONT> hFont;
	AutoDC hdcDesktop;
	AutoGDI<HDC> hMemDC;
	HFONT hOldFont = nullptr;
	HGDIOBJ hOldBitmap = nullptr;
	FontGlyphs glyphs;
	ArrayFontMetrics metrics;
	AutoGDI<HBITMAP> hBitmap;
	std::vector<BGRAb> pixels;
	std::vector<uint8> grayscale;
	AutoGDI<HBRUSH> hEraserBrush;

	int width = 0;

	enum { DEFAULT_CHAR = L'?' };

	WindowsArrayFont(IArrayFontSet& glyphSet, const FontSpec& spec) :
		hdcDesktop{ GetDC(nullptr) },
		hMemDC{ CreateCompatibleDC(hdcDesktop) }
	{
		LOGFONTA logFont = { 0 };

		if (spec.fontName && *spec.fontName != 0)
		{
			SafeFormat(logFont.lfFaceName, sizeof logFont.lfFaceName, "%s", spec.fontName);
		}
		else
		{
			SafeFormat(logFont.lfFaceName, sizeof logFont.lfFaceName, "Courier New");
		}

		logFont.lfItalic = (BYTE) spec.italic;
		logFont.lfWeight = spec.weight;

		if (!hMemDC)
		{
			Throw(GetLastError(), "CreateCompatibleDC returned NULL");
		}

		SetMapMode(hMemDC, MM_TEXT);

		//int32 DOTSY = GetDeviceCaps(hMemDC, LOGPIXELSY);
		//logFont.lfHeight = (int32) abs((spec.height / (double) DOTSY) * 72.0);

		logFont.lfHeight = spec.height;

		hFont = CreateFontIndirectA(&logFont);

		hOldFont = (HFONT)SelectObject(hMemDC, hFont);

		TEXTMETRICA tm;
		GetTextMetricsA(hMemDC, &tm);

		// If the TMPF_FIXED_PITCH flag is 0 then width is fixed: blame Microsoft.
		bool isFixedPitch = !HasFlag(TMPF_FIXED_PITCH, tm.tmPitchAndFamily);

		glyphSet.Populate(glyphs);

		auto i = glyphs.codes.find('?');
		if (i == glyphs.codes.end())
		{
			glyphs.AddGlyph((wchar_t) DEFAULT_CHAR);
		}

		metrics.ascent = tm.tmAscent;
		metrics.descent = tm.tmDescent;
		metrics.height = tm.tmHeight;
		metrics.imgWidth = 0;
		metrics.imgHeight = tm.tmHeight;
		metrics.internalLeading = tm.tmInternalLeading;
		metrics.italic = tm.tmItalic;
		metrics.weight = tm.tmWeight;
		metrics.imgWidth = width = glyphs.ComputeMaxABCWidth(hMemDC, isFixedPitch);
		hBitmap = CreateCompatibleBitmap(hdcDesktop, width, metrics.height);
		if (hBitmap == nullptr)
		{
			Throw(GetLastError(), "%s CreateCompatibleBitmap failed.", __FUNCTION__);
		}

		pixels.resize(width* metrics.height);
		grayscale.resize(width* metrics.height);

		auto hOldBitmap = SelectObject(hMemDC, hBitmap);
		UNUSED(hOldBitmap);

		RECT rect = { 0, 0, width, metrics.height };

		hEraserBrush = CreateSolidBrush(RGB(0, 0, 0));

		SetTextColor(hMemDC, RGB(255, 255, 255));
		SetBkColor(hMemDC, RGB(0, 0, 0));
	}

	~WindowsArrayFont()
	{
		if (hOldBitmap) SelectObject(hMemDC, hOldBitmap);
		if (hOldFont) SelectObject(hMemDC, hOldFont);
	}

	void Free() override
	{
		delete this;
	}

	int32 ComputeMaxABCWidth(bool isFixedPitch)
	{
		return glyphs.ComputeMaxABCWidth(hMemDC, isFixedPitch);
	}

	int32 NumberOfGlyphs() const override
	{
		return (int32) glyphs.codes.size();
	}

	const ArrayGlyph& operator[](unsigned char ascii) override
	{
		return this->operator[]((char32_t)ascii);
	}

	const ArrayGlyph& operator[](wchar_t unicode) override
	{
		return this->operator[]((char32_t)(uint32)unicode);
	}

	const ArrayGlyph& operator[](char32_t unicode) override
	{
		auto i = glyphs.codes.find((uint32)unicode);
		if (i == glyphs.codes.end())
		{
			i = glyphs.codes.find(DEFAULT_CHAR);
		}

		return i->second;
	}

	void ForEachGlyph(IEventCallback<const GlyphDesc>& cb) override
	{
		glyphs.ForEachGlyph(cb);
	}

	const ArrayFontMetrics& Metrics() const override
	{
		return metrics;
	}

	void GenerateImage(const char32_t charCode, IImagePopulator<GRAYSCALE>& populator) override
	{
		wchar_t text[2] = { 0,0 };
		text[0] = charCode > 32768 ? '?' : (wchar_t) charCode;

		RECT rect{ 0, 0, width, metrics.height };

		FillRect(hMemDC, &rect, hEraserBrush);

		// SelectObject(hMemDC, GetStockObject(WHITE_PEN));

		DrawTextW(hMemDC, text, 1, &rect, DT_BOTTOM | DT_LEFT | DT_NOCLIP | DT_SINGLELINE);

		BITMAPINFO info = { 0 };
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biWidth = width;
		info.bmiHeader.biHeight = metrics.height;

		SelectObject(hMemDC, hOldBitmap);

		GetDIBits(hMemDC, hBitmap, 0, 0, NULL, &info, DIB_RGB_COLORS);
		GetDIBits(hMemDC, hBitmap, 0, metrics.height, pixels.data(), &info, DIB_RGB_COLORS);

		SelectObject(hMemDC, hBitmap);

		for (int j = 0; j < metrics.height; j++)
		{
			for (int i = 0; i < width; i++)
			{
				const BGRAb& rgb = pixels[i + j * width];
				int32 sum = rgb.red + rgb.green + rgb.blue;
				grayscale[i + (metrics.height - j - 1) * width] = (uint8)(sum / 3);
			}
		}

		populator.OnImage(grayscale.data(), width, metrics.height);
	}
};


namespace Rococo::Graphics
{
	namespace Fonts
	{
		IArrayFontSupervisor* CreateOSFont(IArrayFontSet& glyphSet, const FontSpec& spec)
		{
			return new WindowsArrayFont(glyphSet, spec);
		}
	}
}
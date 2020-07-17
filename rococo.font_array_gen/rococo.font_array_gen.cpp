// rococo.font_array_gen.cpp : Defines the entry point for the application.
//

#define NOMINMAX
#include "framework.h"
#include "rococo.font_array_gen.h"

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.window.h>

#include <rococo.imaging.h>

#define MAX_LOADSTRING 100

#include <commdlg.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Windows;

cstr root = "C:\\work\\rococo\\content\\textures\\fonts\\";

#ifdef _DEBUG
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.tiff.debug.lib")
# pragma comment(lib, "rococo.jpg.debug.lib")
# pragma comment(lib, "rococo.zlib.debug.lib")
#else
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.tiff.lib")
# pragma comment(lib, "rococo.jpg.lib")
# pragma comment(lib, "rococo.zlib.lib")
#endif

struct RenderContext
{
	HBRUSH hEraserBrush;
	const TEXTMETRIC& metrics;
	HDC hdc;
	HBITMAP hBitmap;
	RECT rect;
	uint8* grayscaleBuffer;
	HBITMAP hOldBitmap;
};

#pragma pack(push,1)
struct BGRAb
{
	uint8 blue = 0;
	uint8 green = 0;
	uint8 red = 0;
	uint8 alpha = 0;
};
#pragma pack(pop)

int GenerateCharFile(int charCode, const LOGFONT& f, RenderContext& rc, cstr filename, FILE* abcFile)
{
	ABC abc;
	if (GetCharABCWidthsA(rc.hdc, charCode, charCode, &abc))
	{
		int a = abc.abcA;
		uint32 b = abc.abcB;
		int c = abc.abcC;

		char text[2] = { 0,0 };
		text[0] = charCode;

		int lenChars = fprintf(abcFile, " %d %u %d)", a, b, c);
		
		Vec2i span{ rc.rect.right, rc.rect.bottom };

		FillRect(rc.hdc, &rc.rect, rc.hEraserBrush);

		SetTextColor(rc.hdc, RGB(255, 255, 255));
		SetBkColor(rc.hdc, RGB(0, 0, 0));

		SelectObject(rc.hdc, GetStockObject(WHITE_PEN));

		DrawTextA(rc.hdc, text, 1, &rc.rect, DT_BOTTOM | DT_LEFT | DT_NOCLIP | DT_SINGLELINE);

		std::vector<BGRAb> rgbPixels;
		rgbPixels.resize(span.x * span.y);

		BITMAPINFO info = { 0 };
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB; 
		info.bmiHeader.biWidth = span.x;
		info.bmiHeader.biHeight = span.y;

		SelectObject(rc.hdc, rc.hOldBitmap);

		GetDIBits(rc.hdc, rc.hBitmap, 0, 0, NULL, &info, DIB_RGB_COLORS);

		GetDIBits(rc.hdc, rc.hBitmap, 0, span.y, rgbPixels.data(), &info, DIB_RGB_COLORS);

		SelectObject(rc.hdc, rc.hBitmap);

		for (int j = 0; j < span.y; j++)
		{
			for (int i = 0; i < span.x; i++)
			{
				const BGRAb& rgb = rgbPixels[i + j * span.x];
				int32 sum = rgb.red + rgb.green + rgb.blue;
				rc.grayscaleBuffer[i + (span.y - j - 1) * span.x] = (uint8) (sum / 3);
			}
		}

		rgbPixels.clear();

		Rococo::Imaging::SaveAsTiff(rc.grayscaleBuffer, span, filename);

		return lenChars;
	}
	else
	{
		Throw(0, "Bad ABCs for slot %d", charCode);
	}
}

template<class T> struct AutoGDI
{
	T handle = nullptr;
	operator T() { return handle; }

	operator bool() const { return handle != nullptr; }

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

void GenerateLogFont(cstr targetFolder, const LOGFONT& f)
{
	const int FIRST_ASCII_CHAR_CODE = 32;
	const int FINAL_ASCII_CHAR_CODE = 126;

	std::vector<unsigned char> charCodes;
	for (int i = FIRST_ASCII_CHAR_CODE; i <= FINAL_ASCII_CHAR_CODE; ++i)
	{
		charCodes.push_back(i);
	}

	charCodes.push_back('£');
	charCodes.push_back('€');

	AutoGDI<HFONT> hFont{ CreateFontIndirectW(&f) };
	AutoDC hdcDesktop{ GetDC(nullptr) };

	AutoGDI<HDC> hMemDC{ CreateCompatibleDC(hdcDesktop) };
	if (!hMemDC)
	{
		Throw(GetLastError(), "CreateCompatibleDC returned NULL");
	}

	HFONT hOldFont = (HFONT) SelectObject(hMemDC, hFont);

	int txWidth = 0;

	for (uint8 i: charCodes)
	{
		ABC abc;
		if (GetCharABCWidthsA(hMemDC, i, i, &abc))
		{
			int a = abc.abcA;
			uint32 b = abc.abcB;
			int c = abc.abcC;

			int totalWidth = a + b + c;
			txWidth = max(txWidth, totalWidth);
		}
	}

	TEXTMETRIC tm;
	GetTextMetrics(hMemDC, &tm);

	int txHeight = tm.tmHeight;

	AutoGDI<HBITMAP> hBitmap{ CreateCompatibleBitmap(hdcDesktop, txWidth, txHeight) };

	std::vector<RGBAb> pixels;
	pixels.resize(txWidth * txHeight);

	auto hOld = SelectObject(hMemDC, hBitmap);

	std::vector<uint8> grayscale;
	grayscale.resize(txWidth* txHeight);

	RECT rect = { 0, 0, txWidth, txHeight };
	AutoGDI<HBRUSH> hEraserBrush{ CreateSolidBrush(RGB(0,0,0)) };
	RenderContext rc{ hEraserBrush, tm, hMemDC, hBitmap, rect, grayscale.data(),(HBITMAP) hOld};

	U8FilePath descFile;
	SafeFormat(descFile.buf, descFile.CAPACITY, "%s\\desc.sxy", targetFolder);

	FILE* fp;
	auto err = fopen_s(&fp, descFile, "wt");
	if (err)
	{
		Throw(0, "Could not create description file");
	}

	fprintf_s(fp, "(' #file.type rococo.fontdef.sxy)\n");

	fprintf_s(fp, "(struct FontMetrics\n");
	fprintf_s(fp, "\t(Int32 ascent descent height internalLeading italic weight imgWidth imgHeight)\n");
	fprintf_s(fp, ")\n");
	fprintf_s(fp, "(alias FontMetrics Rococo.Graphics.FontMetrics)\n");
	fprintf_s(fp, "(using Rococo.Graphics)\n");
	
	fprintf_s(fp, "(namespace EntryPoint)\n");
	fprintf_s(fp, "\t(alias Main EntryPoint.Main)\n\n");
	fprintf_s(fp, "(function Main (Int32 id)->(Int32 exitCode):\n");
	fprintf_s(fp, "\t(FontMetrics metrics)\n");
	fprintf_s(fp, "\t(metrics.height = %d)\n", tm.tmHeight);
	fprintf_s(fp, "\t(metrics.ascent = %d)\n", tm.tmAscent);
	fprintf_s(fp, "\t(metrics.descent = %d)\n", tm.tmDescent);
	fprintf_s(fp, "\t(metrics.italic = %d)\n", tm.tmItalic);
	fprintf_s(fp, "\t(metrics.weight = %d)\n", tm.tmWeight);
	fprintf_s(fp, "\t(metrics.internalLeading = %d)\n", tm.tmInternalLeading);
	fprintf_s(fp, "\t(metrics.imgWidth = %d)\n", txWidth);
	fprintf_s(fp, "\t(metrics.imgHeight = %d)\n", txHeight);
	fprintf_s(fp, "\t(DeclareFontMetrics metrics)\n\n");
	
	fprintf_s(fp, "\t // (AddGlyph <ascii> <image-file> <A> <B> <C>)\n");

	for (uint8 i : charCodes)
	{
		U8FilePath targetFile;
		SafeFormat(targetFile.buf, targetFile.CAPACITY, "%s\\c%04u.tiff", targetFolder, i);
	
		fprintf_s(fp, "\t(AddGlyph %d \"c%04u.tiff\"", i, i);

		int nLineChars = GenerateCharFile(i, f, rc, targetFile, fp);

		int nSpaces = max(0, 12 - nLineChars);
		for (int i = 0; i < nSpaces; ++i)
		{
			fprintf_s(fp, " ");
		}

		if (i == '&')
		{
			fprintf_s(fp, "// &&\n");
		}
		else
		{
			fprintf_s(fp, "// %c\n", i);
		}
	}
	fprintf(fp, ")\n");

	SelectObject(hMemDC, hOldFont);
	SelectObject(hMemDC, hOld);
}

void Main()
{
	CHOOSEFONTW cf = { 0 };
	cf.lStructSize = sizeof(cf);

	LOGFONT logFont = { 0 };
	cf.lpLogFont = &logFont;

	cf.Flags |= CF_EFFECTS;

	if (ChooseFontW(&cf))
	{
		U8FilePath targetFolder;
		SafeFormat(targetFolder.buf, targetFolder.CAPACITY, "%s%S%d", root, logFont.lfFaceName, logFont.lfHeight);

		if (!CreateDirectoryA(targetFolder, nullptr))
		{
			HRESULT hr = GetLastError();
			if (hr != ERROR_ALREADY_EXISTS)
			{
				Throw(hr, "Error creating directory\r\n%s", targetFolder);
			}
		}

		GenerateLogFont(targetFolder, logFont);
	}
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	try
	{
		Main();
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, "Font Array Gen Error");
		return ex.ErrorCode();
	}
}


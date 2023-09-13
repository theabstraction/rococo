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

#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::Fonts;

#ifdef _WIN32
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.tiff.lib")
# pragma comment(lib, "rococo.jpg.lib")
# pragma comment(lib, "rococo.zlib.lib")
# pragma comment(lib, "rococo.fonts.lib")
#endif

struct : IArrayFontSet
{
	void Populate(IFontGlyphBuilder& builder)
	{
		const unsigned char FIRST_ASCII_CHAR_CODE = 32;
		const unsigned char FINAL_ASCII_CHAR_CODE = 126;

		for (unsigned char i = FIRST_ASCII_CHAR_CODE; i <= FINAL_ASCII_CHAR_CODE; ++i)
		{
			builder.AddGlyph(i);
		}

		builder.AddGlyph(L'£');
		builder.AddGlyph(L'€');
	}
} asciiGlyphsAndCurrencySymbols;

void GenerateLogFont(cstr targetFolder, const LOGFONTA& logFont)
{
	FontSpec spec{ logFont.lfFaceName, logFont.lfHeight, logFont.lfWeight, logFont.lfItalic };
	AutoFree<IArrayFontSupervisor> font = Fonts::CreateOSFont(asciiGlyphsAndCurrencySymbols, spec);

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

	auto& tm = font->Metrics();
	
	fprintf_s(fp, "(namespace EntryPoint)\n");
	fprintf_s(fp, "\t(alias Main EntryPoint.Main)\n\n");
	fprintf_s(fp, "(function Main (Int32 id)->(Int32 exitCode):\n");
	fprintf_s(fp, "\t(FontMetrics metrics)\n");
	fprintf_s(fp, "\t(metrics.height = %d)\n", tm.height);
	fprintf_s(fp, "\t(metrics.ascent = %d)\n", tm.ascent);
	fprintf_s(fp, "\t(metrics.descent = %d)\n", tm.descent);
	fprintf_s(fp, "\t(metrics.italic = %d)\n", tm.italic);
	fprintf_s(fp, "\t(metrics.weight = %d)\n", tm.weight);
	fprintf_s(fp, "\t(metrics.internalLeading = %d)\n", tm.internalLeading);
	fprintf_s(fp, "\t(metrics.imgWidth = %d)\n", tm.imgWidth);
	fprintf_s(fp, "\t(metrics.imgHeight = %d)\n", tm.imgHeight);
	fprintf_s(fp, "\t(DeclareFontMetrics metrics)\n\n");
	
	fprintf_s(fp, "\t // (AddGlyph <ascii> <image-file> <A> <B> <C>)\n");

	struct : IEventCallback<const GlyphDesc>
	{
		FILE* fp;
		cstr targetFolder;
		IArrayFont* font;

		void OnEvent(const GlyphDesc& gd) override
		{
			U8FilePath targetFile;
			SafeFormat(targetFile.buf, targetFile.CAPACITY, "%s\\c%04u.tiff", targetFolder, (uint32) gd.charCode);

			struct : IImagePopulator<GRAYSCALE>
			{
				cstr filename;

				void OnImage(const GRAYSCALE* grayscale, int width, int height) override
				{
					Rococo::Imaging::SaveAsTiff(grayscale, { width, height }, filename);
				}
			} saveAsTiff;
			saveAsTiff.filename = targetFile;

			font->GenerateImage(gd.charCode, saveAsTiff);

			fprintf_s(fp, "\t(AddGlyph %u \"c%04u.tiff\"", (uint32) gd.charCode, (uint32) gd.charCode);

			int nLineChars = fprintf(fp, " %d %u %d)", gd.glyph.A, gd.glyph.B, gd.glyph.C);
				
			int nSpaces = max(0, 12 - nLineChars);
			for (int i = 0; i < nSpaces; ++i)
			{
				fprintf_s(fp, " ");
			}

			if (gd.charCode == '&')
			{
				fprintf_s(fp, "// &&\n");
			}
			else
			{
				if (gd.charCode < 127)
				{
					fprintf_s(fp, "// %c\n", (unsigned char) gd.charCode);
				}
				else
				{
					fprintf_s(fp, "\n");
				}
			}
		}
	} appendGlyphToDescFile;

	appendGlyphToDescFile.fp = fp;
	appendGlyphToDescFile.targetFolder = targetFolder;
	appendGlyphToDescFile.font = font;

	font->ForEachGlyph(appendGlyphToDescFile);
	
	fprintf(fp, ")\n");
}

void Main()
{
	CHOOSEFONTA cf = { 0 };
	cf.lStructSize = sizeof(cf);

	LOGFONTA logFont = { 0 };
	cf.lpLogFont = &logFont;

	cf.Flags |= CF_EFFECTS;

	cstr CONTENT_FONT = "C:\\work\\rococo\\content\\textures\\fonts\\";

	if (ChooseFontA(&cf))
	{
		U8FilePath targetFolder;
		SafeFormat(targetFolder.buf, targetFolder.CAPACITY, "%s%s%d", CONTENT_FONT, logFont.lfFaceName, logFont.lfHeight);

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


// WindowFontAPI.h

#include <malloc.h>

using namespace System;
using namespace System::Drawing;

namespace Gisephone
{
	namespace Fonts
	{	
		public ref class FontMetrics
		{
		private:
			Int32  m_Height;
			Int32  m_Ascent;
			Int32  m_Descent;
			Int32  m_InternalLeading;
			Int32  m_ExternalLeading;
			Int32  m_AveCharWidth;
			Int32  m_MaxCharWidth;
			Int32  m_Weight;
			Int32  m_Overhang;
			Int32  m_DigitizedAspectX;
			Int32  m_DigitizedAspectY;
			Char   m_FirstChar;
			Char   m_LastChar;
			Char   m_DefaultChar;
			Char   m_BreakChar;
			byte   m_Italic;
			byte   m_Underlined;
			byte   m_StruckOut;
			byte   m_PitchAndFamily;
			byte   m_CharSet;
			array<Boolean>^ m_ValidAsciiGlyphs;

			ABC* m_pAbcWidths;

		protected:
			~FontMetrics()
			{
				this->!FontMetrics();
			}

			!FontMetrics()
			{
				delete[] m_pAbcWidths;
			}

		public:
			const static int MAX_CHARS = 256;

			static FontMetrics^ From(Graphics^ g, Font^ f)
			{
				IntPtr hdc = g->GetHdc();
				IntPtr hFont = f->ToHfont();
				FontMetrics^ fm = gcnew FontMetrics(hdc, hFont);
				
				g->ReleaseHdc(hdc);
				return fm;
			}

			FontMetrics(IntPtr hdc, IntPtr phFont)
			{
				HFONT hFont = (HFONT) phFont.ToPointer();
				HDC dc = (HDC) hdc.ToPointer();

				HGDIOBJ old = SelectObject(dc, (HGDIOBJ) hFont);

				TEXTMETRIC tm;
				if (!GetTextMetrics(dc, &tm))
				{
					throw gcnew System::Exception("Could not get text metrics from device context");
				}

				DWORD sizeofGlyphData = GetFontUnicodeRanges(dc,nullptr);

				GLYPHSET* gs = (GLYPHSET*) _alloca(sizeofGlyphData);
				gs->cbThis = sizeofGlyphData;

				m_ValidAsciiGlyphs = gcnew array<Boolean>(256);
				for(int i = 0; i < 256; ++i)
				{
					m_ValidAsciiGlyphs[i] = false;
				}

				GetFontUnicodeRanges(dc,gs);

				const WCRANGE* ranges = gs->ranges;

				for (DWORD i = 0; i < gs->cRanges; ++i)
				{
					const WCRANGE& range = ranges[i];
					for(USHORT c = range.wcLow; c < 256 && c < range.wcLow + range.cGlyphs; ++c)
					{
						m_ValidAsciiGlyphs[c] = true;
					}
					if (range.wcLow > 255) break;
				}

				m_pAbcWidths = new ABC[MAX_CHARS];
				GetCharABCWidthsW(dc, 0, MAX_CHARS-1, m_pAbcWidths);

				SelectObject(dc, old);	
				DeleteObject((HGDIOBJ) hFont);

				m_Ascent = tm.tmAscent;
				m_AveCharWidth = tm.tmAveCharWidth;
				m_BreakChar = tm.tmBreakChar;
				m_CharSet = tm.tmCharSet;
				m_DefaultChar = tm.tmDefaultChar;
				m_Descent = tm.tmDescent;
				m_DigitizedAspectX = tm.tmDigitizedAspectX;
				m_DigitizedAspectY = tm.tmDigitizedAspectY;
				m_ExternalLeading = tm.tmExternalLeading;
				m_FirstChar = tm.tmFirstChar;
				m_Height = tm.tmHeight;
				m_InternalLeading = tm.tmInternalLeading;
				m_Italic = tm.tmItalic;
				m_LastChar = tm.tmLastChar;
				m_MaxCharWidth = tm.tmMaxCharWidth;
				m_PitchAndFamily = tm.tmPitchAndFamily;
				m_Overhang = tm.tmOverhang;
				m_PitchAndFamily = tm.tmPitchAndFamily;
				m_StruckOut = tm.tmStruckOut;
				m_Underlined = tm.tmUnderlined;
				m_Weight = tm.tmWeight;				
			}	

			int GetA(Char index)
			{
				if (index < 0 || index > MAX_CHARS) return 0;

				return m_pAbcWidths[index].abcA;
			}

			int GetB(Char index)
			{
				if (index < 0 || index > MAX_CHARS) return 0;

				return m_pAbcWidths[index].abcB;
			}

			int GetC(Char index)
			{
				if (index < 0 || index > MAX_CHARS) return 0;

				return m_pAbcWidths[index].abcC;
			}

			Boolean IsGlyphAvailable(Int32 asciiValue)
			{
				return asciiValue >=0 && asciiValue < 256 && m_ValidAsciiGlyphs[asciiValue];
			}
			
			property Int32  Height				{ Int32 get() { return m_Height; }}
			property Int32  Ascent				{ Int32 get() { return m_Ascent; }}
			property Int32  Descent				{ Int32 get() { return m_Descent; }}
			property Int32  InternalLeading		{ Int32 get() { return m_InternalLeading; }}
			property Int32  ExternalLeading		{ Int32 get() { return m_ExternalLeading; }}
			property Int32  AveCharWidth		{ Int32 get() { return m_AveCharWidth; }}
			property Int32  MaxCharWidth		{ Int32 get() { return m_MaxCharWidth; }}
			property Int32  Weight				{ Int32 get() { return m_Weight; }}
			property Int32  Overhang			{ Int32 get() { return m_Overhang; }}
			property Int32  DigitizedAspectX	{ Int32 get() { return m_DigitizedAspectX; }}
			property Int32  DigitizedAspectY	{ Int32 get() { return m_DigitizedAspectY; }}
			property Char   FirstChar			{ Char  get() { return m_FirstChar; }}
			property Char   LastChar			{ Char  get() { return m_LastChar; }}
			property Char   DefaultChar			{ Char  get() { return m_DefaultChar; }}
			property Char   BreakChar			{ Char  get() { return m_BreakChar; }}
			property Boolean IsItalic			{ Boolean  get() { return m_Italic != 0; }}
			property Boolean IsUnderlined		{ Boolean  get() { return m_Underlined != 0; }}
			property Boolean IsStruckOut		{ Boolean  get() { return m_StruckOut != 0; }}
			property Byte   PitchAndFamily		{ Byte  get() { return m_PitchAndFamily; }}
			property Byte   CharSet				{ Byte  get() { return m_CharSet; }}
		};
	}
}

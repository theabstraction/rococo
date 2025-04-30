#include <rococo.types.h>

#define ROCOCO_GR_GDI_API ROCOCO_API_EXPORT

#include <rococo.gr.win32-gdi.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <rococo.hashtable.h>
#include <rococo.io.h>
#include <rococo.ui.h>
#include <rococo.os.h>
#include <rococo.window.h>
#include <rococo.imaging.h>
#include <rococo.hashtable.h>
#include <rococo.time.h>
#include <sexy.types.h>
#include <rococo.time.h>
#include <rococo.great.sex.h>
#include <Sexy.S-Parser.h>
#include <vector>
#include <rococo.vkeys.h>

#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

#include <Richedit.h>

#include <rococo.ui.joystick.h>

using namespace Rococo;
using namespace Rococo::GR;
using namespace Rococo::GR::Win32;
using namespace Rococo::Gui;
using namespace Rococo::GreatSex;
using namespace Rococo::Joysticks;
using namespace Rococo::Sex;
using namespace Gdiplus;

namespace GRANON
{
	class GDISolidBrush
	{
	private:
		HBRUSH hBrush;

	public:
		GDISolidBrush(RGBAb colour)
		{
			hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
		}

		~GDISolidBrush()
		{
			DeleteObject(hBrush);
		}

		operator HBRUSH ()
		{
			return hBrush;
		}
	};

	class UseBrush
	{
		HDC dc;
		HBRUSH hOldBrush;

	public:
		UseBrush(HDC _dc, HBRUSH hBrush): dc(_dc)
		{
			hOldBrush = (HBRUSH) SelectObject(dc, hBrush);
		}

		~UseBrush()
		{
			SelectObject(dc, hOldBrush);
		}
	private:
	};

	Vec2i GetTopLeftFromAlignment(GRAlignmentFlags alignment, const GuiRect& rect, Vec2i span)
	{
		int x;

		if (alignment.IsHCentred())
		{
			x = Centre(rect).x - (span.x >> 1);
		}
		else if (alignment.HasSomeFlags(EGRAlignment::Left))
		{
			x = rect.left;
		}
		else
		{
			x = rect.right - span.x;
		}

		int y;

		if (alignment.IsVCentred())
		{
			y = Centre(rect).y - (span.y >> 1);
		}
		else if (alignment.HasSomeFlags(EGRAlignment::Top))
		{
			y = rect.top;
		}
		else
		{
			y = rect.bottom - span.y;
		}

		return { x, y };
	}

	class GDIPen
	{
	private:
		HPEN hPen;

	public:
		GDIPen(RGBAb colour, int lineThickness = 1)
		{
			hPen = CreatePen(PS_SOLID, lineThickness, RGB(colour.red, colour.green, colour.blue));
		}

		~GDIPen()
		{
			DeleteObject(hPen);
		}

		operator HPEN ()
		{
			return hPen;
		}
	};

	class UsePen
	{
		HDC dc;
		HPEN hOldPen;

	public:
		UsePen(HDC _dc, HPEN hPen) : dc(_dc)
		{
			hOldPen = (HPEN)SelectObject(dc, hPen);
		}

		~UsePen()
		{
			SelectObject(dc, hOldPen);
		}
	private:
	};

	class UseClipRect
	{
	public:
		UseClipRect(HDC _dc, const GuiRect& rect) : dc(_dc)
		{
			if (rect.IsNormalized())
			{
				hRegion = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
				SelectClipRgn(dc, hRegion);
			}
		}

		~UseClipRect()
		{
			if (hRegion)
			{
				SelectClipRgn(dc, nullptr);
				DeleteObject(hRegion);
			}
		}
	private:
		HDC dc = 0;
		HRGN hRegion = 0;
	};

	class UseFont
	{
		HDC dc;
		HFONT hOldFont;

	public:
		UseFont(HDC _dc, HFONT hFont) : dc(_dc)
		{
			hOldFont = (HFONT)SelectObject(dc, hFont);
		}

		~UseFont()
		{
			SelectObject(dc, hOldFont);
		}
	private:
	};

	class UseBkMode
	{
		HDC dc;
		int oldMode;

	public:
		UseBkMode(HDC _dc, int mode) : dc(_dc)
		{
			oldMode = SetBkMode(dc, mode);
		}

		~UseBkMode()
		{
			SetBkMode(dc, oldMode);
		}
	private:
	};

	UINT FormatWin32DrawTextAlignment(GRAlignmentFlags alignment)
	{
		UINT format = 0;

		if (alignment.HasAllFlags(EGRAlignment::HCentre) || !alignment.HasSomeFlags(EGRAlignment::HCentre))
		{
			format += DT_CENTER;
		}
		else
		{
			if (alignment.HasSomeFlags(EGRAlignment::Left))
			{
				format += DT_LEFT;
			}
			else if (alignment.HasSomeFlags(EGRAlignment::Right))
			{
				format += DT_RIGHT;
			}
		}

		if (alignment.HasAllFlags(EGRAlignment::VCentre) || !alignment.HasSomeFlags(EGRAlignment::VCentre))
		{
			format += DT_VCENTER;
		}
		else
		{
			if (alignment.HasSomeFlags(EGRAlignment::Top))
			{
				format += DT_TOP;
			}
			else if (alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				format += DT_BOTTOM;
			}
		}

		return format;
	}

	struct GDICustodian;

	GRFontId BindFontId(GDICustodian& custodian, const FontSpec& desc);
	HFONT DefaultFont(GDICustodian& custodian);
	void SelectFont(GDICustodian& custodian, GRFontId fontId, HDC paintDC);

	struct GDIImage : IGRImageSupervisor
	{
		Vec2i span{ 8, 8 };
		HBITMAP hImage = NULL;
		Strings::HString hint;

		GDIImage(cstr _hint, cstr imagePath, IO::IInstallation& installation): hint(_hint)
		{
			struct ImageParser : Imaging::IImageLoadEvents
			{
				HBITMAP hBitmap = 0;
				Vec2i span{ 0,0 };

				void OnError(const char* message) override
				{
					err = message;
				}

				void OnRGBAImage(const Vec2i& span, const RGBAb* data) override
				{
					HDC screenDC = GetDC(NULL);
					HDC bitmapDC = CreateCompatibleDC(screenDC);

					if (!bitmapDC)
					{
						Throw(GetLastError(), "CreateCompatibleDC returned NULL");
					}

					SetMapMode(bitmapDC, MM_TEXT);

					hBitmap = CreateCompatibleBitmap(screenDC, span.x, span.y);
					if (hBitmap == nullptr)
					{
						DeleteDC(bitmapDC);
						Throw(GetLastError(), "%s CreateCompatibleBitmap failed.", __FUNCTION__);
					}

					// We have the const buffer, we don't change its size, only its content.
					// This allows us to avoid duplicating the buffer to convert to BGRA format

					auto* pMutableData = const_cast<RGBAb*>(data);
					size_t nElements = span.x * span.y;
					
					for (size_t i = 0; i < nElements; i++)
					{
						RGBAb& col = pMutableData[i];
						std::swap(col.blue, col.red);
						// This swizzles us into BGRA format, which is what Windows wants
					}
					
					BITMAPINFO info = { 0 };
					info.bmiHeader.biSize = sizeof(info);
					info.bmiHeader.biWidth = span.x;
					info.bmiHeader.biHeight = -span.y;
					info.bmiHeader.biPlanes = 1;
					info.bmiHeader.biBitCount = 32;
					info.bmiHeader.biCompression = BI_RGB;
					SetDIBits(bitmapDC, hBitmap, 0, span.y, data, &info, DIB_RGB_COLORS);
					DeleteDC(bitmapDC);

					this->span = span;
				}

				void OnAlphaImage(const Vec2i& span, const uint8* data) override
				{
					UNUSED(span);
					UNUSED(data);
					err = "8bpp images not supported";
				}

				Strings::HString err;
			} parser;

			if (Strings::EndsWithI(imagePath, ".tiff") || (Strings::EndsWithI(imagePath, ".tif")))
			{
				AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
				installation.LoadResource(imagePath, *buffer, 32_megabytes);
				Rococo::Imaging::DecompressTiff(parser, buffer->GetData(), buffer->Length());
			}
			else if (Strings::EndsWithI(imagePath, ".jpeg") || (Strings::EndsWithI(imagePath, ".jpg")))
			{
				AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(64_kilobytes);
				installation.LoadResource(imagePath, *buffer, 32_megabytes);
				Rococo::Imaging::DecompressJPeg(parser, buffer->GetData(), buffer->Length());
			}
			else
			{
				Throw(0, "Could not load image: %s. Only jpg and tiff files are recognized", imagePath);
			}

			if (parser.err.length() > 0)
			{
				Throw(0, "Could not parse image data for %s. Error: %s", imagePath, parser.err.c_str());
			}

			hImage = parser.hBitmap;
			span = parser.span;
		}

		virtual ~GDIImage()
		{
			if (hImage)
			{
				DeleteObject(hImage);
			}
		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& g) override;

		void Free() override
		{
			delete this;
		}

		Vec2i Span() const override
		{
			return span;
		}
	};


	struct SceneRenderer: Gui::IGRRenderContext
	{
		HWND hWnd;
		HDC paintDC = 0;
		HDC bitmapDC = 0;

		// DC and bitmap for hacking alpha blending into GDI for functions such as GradientFill
		struct AlphaBuilder
		{
			HDC DC = 0;
			HBITMAP hBitmap = 0;
			Gdiplus::Graphics* g = nullptr;
		} alphaBuilder;

		GDICustodian& custodian;
		Gdiplus::Graphics g;
	

		Vec2i cursor{ 0,0 };

		int64 captureId;
		
		SceneRenderer(GDICustodian& _custodian, HWND _hWnd, HDC dc, int64 _captureId): hWnd(_hWnd), custodian(_custodian), g(dc), paintDC(dc), captureId(_captureId)
		{
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p);
			cursor = { p.x,p.y };
		}

		~SceneRenderer()
		{
			if (bitmapDC)
			{
				DeleteDC(bitmapDC);
			}

			if (alphaBuilder.DC)
			{
				DeleteDC(alphaBuilder.DC);
			}

			if (alphaBuilder.hBitmap)
			{
				DeleteObject(alphaBuilder.hBitmap);
			}

			if (alphaBuilder.g)
			{
				delete alphaBuilder.g;
			}
		}

		// Get some kind of hover point for the cursor
		Vec2i CursorHoverPoint() const override
		{
			return cursor;
		}

		// It is up to the renderer to decide if a panel is hovered.
		bool IsHovered(IGRPanel& panel) const override
		{
			if (captureId > 0)
			{
				bool isCaptured = false;

				for (auto* p = &panel; p != nullptr; p = p->Parent())
				{
					if (p->Id() == captureId)
					{
						isCaptured = true;
						break;
					}
				}

				if (!isCaptured)
				{
					return false;
				}
			}
			
			return IsPointInRect(cursor, panel.AbsRect());
		}

		// Get the screen dimensions
		GuiRect ScreenDimensions() const override
		{
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			return GuiRect(clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);
		}

		/* heading: 0 = N, E = 90 etc */
		void DrawDirectionArrow(const GuiRect& absRect, RGBAb colour, Degrees heading) override
		{
			POINT v[3];

			if (heading == 0)
			{
				// North

				v[0] = { absRect.left, absRect.bottom };
				v[1] = { absRect.right, absRect.bottom };
				v[2] = { (absRect.left + absRect.right) >> 1, absRect.top };
			}
			else if (heading == 90)
			{
				// East

				v[0] = { absRect.left, absRect.top };
				v[1] = { absRect.left, absRect.bottom };
				v[2] = { absRect.right, (absRect.top + absRect.bottom) >> 1 };
			}
			else if (heading == 180)
			{
				// South

				v[0] = { absRect.left, absRect.top };
				v[1] = { absRect.right, absRect.top };
				v[2] = { (absRect.left + absRect.right) >> 1, absRect.bottom };
			}
			else
			{
				// West
				v[0] = { absRect.right, absRect.top };
				v[1] = { absRect.right, absRect.bottom };
				v[2] = { absRect.left, (absRect.top + absRect.bottom) >> 1 };
			}

			GDISolidBrush brush(colour);
			UseBrush useBrush(paintDC, brush);

			int oldMode = SetPolyFillMode(paintDC, ALTERNATE);
			Polygon(paintDC, v, 3);		
			SetPolyFillMode(paintDC, oldMode);
		}

		void DrawImageStretched(IGRImage& image, const GuiRect& absRect) override
		{
			HBITMAP hImage = (HBITMAP) static_cast<GDIImage&>(image).hImage;

			if (!bitmapDC)
			{
				bitmapDC = CreateCompatibleDC(paintDC);
			}

			UseClipRect useClip(paintDC, lastScissorRect);

			HBITMAP hOldBitmap = (HBITMAP)SelectObject(bitmapDC, hImage);

			auto span = image.Span();

			BLENDFUNCTION blendFunction;
			blendFunction.AlphaFormat = AC_SRC_ALPHA;
			blendFunction.BlendFlags = 0;
			blendFunction.BlendOp = AC_SRC_OVER;
			blendFunction.SourceConstantAlpha = 255;

			AlphaBlend(paintDC, absRect.left, absRect.top, Width(absRect), Height(absRect), bitmapDC, 0, 0, span.x, span.y, blendFunction);

			SelectObject(bitmapDC, hOldBitmap);
		}

		void DrawImageUnstretched(IGRImage& image, const GuiRect& absRect, GRAlignmentFlags alignment) override
		{
			HBITMAP hImage = (HBITMAP) static_cast<GDIImage&>(image).hImage;

			if (!bitmapDC)
			{
				bitmapDC = CreateCompatibleDC(paintDC);
			}

			UseClipRect useClip(paintDC, lastScissorRect);

			HBITMAP hOldBitmap = (HBITMAP) SelectObject(bitmapDC, hImage);

			auto span = image.Span();

			BLENDFUNCTION blendFunction;
			blendFunction.AlphaFormat = AC_SRC_ALPHA;
			blendFunction.BlendFlags = 0;
			blendFunction.BlendOp = AC_SRC_OVER;
			blendFunction.SourceConstantAlpha = 255;

			Vec2i p = GetTopLeftFromAlignment(alignment, absRect, span);

			AlphaBlend(paintDC, p.x, p.y, span.x, span.y, bitmapDC, 0, 0, span.x, span.y, blendFunction);

			SelectObject(bitmapDC, hOldBitmap);
		}

		GuiRect MergeWithScissorRect(const GuiRect& targetRect)
		{
			GuiRect scissorRect;
			if (TryGetScissorRect(OUT scissorRect))
			{
				return IntersectNormalizedRects(targetRect, scissorRect);
			}

			return targetRect;
		}

		void DrawSharpRect(const GuiRect& visibleRect, RGBAb colour)
		{
			if (colour.alpha < 255)
			{
				g.ResetClip();
				SolidBrush solidBrush(Color(colour.alpha, colour.red, colour.green, colour.blue));
				g.FillRectangle(&solidBrush, visibleRect.left, visibleRect.top, Width(visibleRect), Height(visibleRect));
			}
			else
			{
				const RECT& rect = reinterpret_cast<const RECT&>(visibleRect);

				GDISolidBrush brush(colour);
				FillRect(paintDC, &rect, brush);
			}
		}

		std::vector<Gdiplus::Point> pointsCache;

		static inline Gdiplus::Rect ToGdiRect(const GuiRect& rect)
		{
			return Gdiplus::Rect(rect.left, rect.top, Width(rect), Height(rect));
		}

		void DrawBlurRect(const GuiRect& absRect, const GuiRect& visibleRect, int cornerRadius, RGBAb colour)
		{
			if (colour.alpha == 0 || Height(visibleRect) == 0 || Width(visibleRect) == 0)
			{
				return;
			}
			
			const int delta = cornerRadius;

			GuiRect innerRect = Expand(absRect, -delta);
		
			GuiRect innerVisibleRect = MergeWithScissorRect(innerRect);
			DrawSharpRect(innerVisibleRect, colour);

			Gdiplus::Rect clipRect(visibleRect.left, visibleRect.top, Width(visibleRect), Height(visibleRect));
			g.SetClip(clipRect);

			RGBAb transparent(0, 0, 0, 0);

			GuiRect leftRect{ absRect.left, absRect.top + delta, absRect.left + delta, absRect.bottom - delta};
			PushRect(leftRect, transparent, colour, transparent, colour);
			CommitTriangles();

			GuiRect rightRect{ absRect.right - delta, absRect.top + delta, absRect.right, absRect.bottom - delta };
			PushRect(rightRect, colour, transparent, colour, transparent);
			CommitTriangles();

			GuiRect topRect{ absRect.left + delta, absRect.top, absRect.right - delta, absRect.top + delta };
			PushRect(topRect, transparent, transparent, colour, colour);

			GRTriangle topLeft;
			topLeft.a.position = { absRect.left + delta, absRect.top + delta};
			topLeft.a.colour = colour;
			topLeft.b.position = { absRect.left + delta, absRect.top };
			topLeft.b.colour = transparent;
			topLeft.c.position = { absRect.left, absRect.top + delta };
			topLeft.c.colour = transparent;
			PushTriangle(topLeft);

			GRTriangle topRight;
			topRight.a.position = { absRect.right - delta, absRect.top + delta };
			topRight.a.colour = colour;
			topRight.b.position = { absRect.right - delta, absRect.top };
			topRight.b.colour = transparent;
			topRight.c.position = { absRect.right, absRect.top + delta };
			topRight.c.colour = transparent;
			PushTriangle(topRight);

			CommitTriangles();

			GuiRect bottomRect{ absRect.left + delta, absRect.bottom - delta, absRect.right - delta, absRect.bottom };
			PushRect(bottomRect, colour, colour, transparent, transparent);

			GRTriangle bottomLeft;
			bottomLeft.a.position = { absRect.left + delta, absRect.bottom - delta };
			bottomLeft.a.colour = colour;
			bottomLeft.b.position = { absRect.left + delta, absRect.bottom };
			bottomLeft.b.colour = transparent;
			bottomLeft.c.position = { absRect.left, absRect.bottom - delta };
			bottomLeft.c.colour = transparent;
			PushTriangle(bottomLeft);

			GRTriangle bottomRight;
			bottomRight.a.position = { absRect.right - delta, absRect.bottom - delta };
			bottomRight.a.colour = colour;
			bottomRight.b.position = { absRect.right - delta, absRect.bottom };
			bottomRight.b.colour = transparent;
			bottomRight.c.position = { absRect.right, absRect.bottom - delta };
			bottomRight.c.colour = transparent;
			PushTriangle(bottomRight);

			CommitTriangles();

			g.ResetClip();
		}

		void DrawRoundedRect(const GuiRect& absRect, const GuiRect& visibleRect, int cornerRadius, RGBAb colour)
		{
			if (colour.alpha == 0 || Height(visibleRect) == 0 || Width(visibleRect) == 0)
			{
				return;
			}

			if (colour.alpha == 255)
			{
				GDIPen pen(colour);
				UsePen usePen(paintDC, pen);

				GDISolidBrush brush(colour);
				UseBrush useBrush(paintDC, brush);

				UseClipRect useClip(paintDC, visibleRect);

				RoundRect(paintDC, absRect.left, absRect.top, absRect.right, absRect.bottom, cornerRadius, cornerRadius);
			}
			else
			{
				/*
				   ----topRect-----
				----------------------
				|                    |
				|                    |
				|     centreRect     |
				|                    |
				|                    |
				----------------------
                   ---bottomRect----
				*/


				Gdiplus::SolidBrush brush(Color(colour.alpha, colour.red, colour.green, colour.blue));

				Gdiplus::Rect rects[3];
				rects[0] = Gdiplus::Rect(absRect.left + cornerRadius, absRect.top, Width(absRect) - 2 * cornerRadius - 1, cornerRadius); // top
				rects[1] = Gdiplus::Rect(absRect.left, absRect.top + cornerRadius, Width(absRect), Height(absRect) - 2 * cornerRadius - 1); // centre
				rects[2] = Gdiplus::Rect(absRect.left + cornerRadius, absRect.bottom - cornerRadius - 1, Width(absRect) - 2 * cornerRadius -1, cornerRadius); // bottom

				g.SetClip(ToGdiRect(visibleRect));
				g.FillRectangles(&brush, rects, 3);

				float R = 2.0f * (float)cornerRadius;

				// corners
				g.FillPie(&brush, (float)absRect.left, (float)absRect.top, R, R, 180.0f, 90.0f);
				g.FillPie(&brush, (float)absRect.right - R - 1, (float)absRect.top, R, R, 270.0f, 90.0f);
				g.FillPie(&brush, (float)absRect.left, (float)absRect.bottom - R - 1, R, R, 90.0f, 90.0f);
				g.FillPie(&brush, (float)absRect.right - R - 1, (float)absRect.bottom - R - 1, R, R, 0, 90.0f);
			}
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour, EGRRectStyle rectStyle, int cornerRadius) override
		{
			if (colour.alpha == 0)
			{
				return;
			}

			GuiRect visibleRect = MergeWithScissorRect(absRect);

			switch (rectStyle)
			{
			case EGRRectStyle::SHARP:
				DrawSharpRect(visibleRect, colour);
				break;
			case EGRRectStyle::ROUNDED:
				DrawRoundedRect(absRect, visibleRect, cornerRadius, colour);
				break;
			case EGRRectStyle::ROUNDED_WITH_BLUR:
				DrawBlurRect(absRect, visibleRect, cornerRadius, colour);
				break;
			}
		}

		void DrawGDILines(Gdiplus::Point* points, int nVertices, RGBAb colour)
		{
			GDIPen pen(colour);
			UsePen usePen(paintDC, pen);

			UseClipRect useClip(paintDC, lastScissorRect);

			MoveToEx(paintDC, points[0].X, points[0].Y, NULL);

			for (int i = 1; i < nVertices; i++)
			{
				LineTo(paintDC, points[i].X, points[i].Y);
			}
		}

		void DrawLine(Vec2i start, Vec2i end, RGBAb colour) override
		{
			Gdiplus::Point p[2];
			p[0].X = start.x;
			p[0].Y = start.y;
			p[1].X = end.x;
			p[1].Y = end.y;

			if (colour.alpha < 255)
			{
				Gdiplus::Pen topLeftPen(Gdiplus::Color(colour.alpha, colour.red, colour.green, colour.blue));
				Gdiplus::Rect clipRect(lastScissorRect.left, lastScissorRect.top, Width(lastScissorRect), Height(lastScissorRect));
				g.SetClip(clipRect);
				g.DrawLines(&topLeftPen, p, 2);
			}
			else
			{
				DrawGDILines(p, 2, colour);
			}
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour, EGRRectStyle rectStyle, int cornerRadius) override
		{
			switch (rectStyle)
			{
			case EGRRectStyle::SHARP:
			case EGRRectStyle::ROUNDED_WITH_BLUR:
				DrawSharpRectEdge(absRect, topLeftColour, bottomRightColour);
				break;
			case EGRRectStyle::ROUNDED:
				DrawRoundedEdge(absRect, topLeftColour, bottomRightColour, cornerRadius);
				break;
			}
		}

		void DrawRoundedEdge(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour, int cornerRadius)
		{
			int R = cornerRadius;

			if (topLeftColour.alpha == 255 && bottomRightColour.alpha == 255 && *(int*) &topLeftColour == *(int*) &bottomRightColour)
			{
				UseClipRect clipRect(paintDC, lastScissorRect);
				RoundRect(paintDC, absRect.left, absRect.top, absRect.right, absRect.bottom, R, R);
				return;
			}

			Gdiplus::Pen tlPen(Gdiplus::Color(topLeftColour.alpha, topLeftColour.red, topLeftColour.green, topLeftColour.blue), 1.0f);

			Gdiplus::Rect clipRect(lastScissorRect.left, lastScissorRect.top, Width(lastScissorRect), Height(lastScissorRect));
			g.SetClip(clipRect);

			g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
			g.DrawArc(&tlPen, absRect.left, absRect.top, R, R, 180.0f, 90.0f);
			g.DrawArc(&tlPen, absRect.right - R, absRect.top, R, R, 270, 90.0f);
			g.DrawLine(&tlPen, absRect.left, absRect.top + R / 2, absRect.left, absRect.bottom - R/2);
			g.DrawLine(&tlPen, absRect.left + R/2, absRect.top, absRect.right - R/2, absRect.top);
			
			Gdiplus::Pen brPen(Gdiplus::Color(bottomRightColour.alpha, bottomRightColour.red, bottomRightColour.green, bottomRightColour.blue), 1.0f);
			g.DrawLine(&brPen, absRect.left + R/2, absRect.bottom-1, absRect.right - R /2, absRect.bottom-1);
			g.DrawLine(&brPen, absRect.right -1, absRect.top + R/2, absRect.right, absRect.bottom - R/2);
			g.DrawArc(&brPen, absRect.right - R - 1, absRect.bottom - R - 1, R, R, 0, 90.0f);
			g.DrawArc(&brPen, absRect.left, absRect.bottom - R - 1, R, R, 90.0f, 90.0f);
			g.SetSmoothingMode(Gdiplus::SmoothingModeDefault);

			g.ResetClip();
		}

		void DrawSharpRectEdge(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour)
		{
			if (topLeftColour.alpha > 0)
			{
				Gdiplus::Point topLeftPoints[3] =
				{
					{ absRect.right - 1, absRect.top   },
					{ absRect.left,  absRect.top   },
					{ absRect.left, absRect.bottom - 1 }
				};

				if (topLeftColour.alpha < 255)
				{
					Gdiplus::Pen topLeftPen(Gdiplus::Color(topLeftColour.alpha, topLeftColour.red, topLeftColour.green, topLeftColour.blue));
					Gdiplus::Rect clipRect(lastScissorRect.left, lastScissorRect.top, Width(lastScissorRect), Height(lastScissorRect));
					g.SetClip(clipRect);
					g.DrawLines(&topLeftPen, topLeftPoints, 3);					
				}
				else
				{
					DrawGDILines(topLeftPoints, 3, topLeftColour);
				}
			}

			if (bottomRightColour.alpha > 0)
			{
				Gdiplus::Pen bottomRightPen(Gdiplus::Color(bottomRightColour.alpha, bottomRightColour.red, bottomRightColour.green, bottomRightColour.blue));
				Gdiplus::Point bottomRightPoints[3] =
				{
					{ absRect.left, absRect.bottom - 1 },
					{ absRect.right - 1, absRect.bottom - 1 },
					{ absRect.right - 1, absRect.top }
				};

				if (bottomRightColour.alpha < 255)
				{
					Gdiplus::Rect clipRect(lastScissorRect.left, lastScissorRect.top, Width(lastScissorRect), Height(lastScissorRect));
					g.SetClip(clipRect);
					g.DrawLines(&bottomRightPen, bottomRightPoints, 3);
				}
				else
				{
					DrawGDILines(bottomRightPoints, 3, bottomRightColour);
				}
			}
		}

		struct HilightRect
		{
			GuiRect absRect;
			RGBAb colour1;
			RGBAb colour2;
		};

		std::vector<HilightRect> hilightRects;

		static inline void CopyColour(TRIVERTEX& t, const GRVertex& v)
		{
			t.Alpha = v.colour.alpha << 8;
			t.Blue = v.colour.blue << 8;
			t.Green = v.colour.green << 8;
			t.Red = v.colour.red << 8;
		}

		static GuiRect GetEnclosingRect(const std::vector<TRIVERTEX>& vertices)
		{
			GuiRect r;
			r.left = INT_MAX;
			r.right = INT_MIN;
			r.top = INT_MAX;
			r.bottom = INT_MIN;

			for (auto& v: vertices)
			{
				ExpandZoneToContain(r, Vec2i{ v.x, v.y });
			}

			return r;
		}

		// Head any code sections that use the alpha builder with this function.
		// After the blend is completed you must pass the [oldBitmap] return value to SelectObject(alphaBuilderDC, oldBitmap);
		HBITMAP /* [oldBitmap] */ CacheAlphaBuilder()
		{
		cacheAlphaBuilder:
			auto ds = Span(ScreenDimensions());
			if (!alphaBuilder.DC)
			{
				alphaBuilder.DC = CreateCompatibleDC(paintDC);
				if (alphaBuilder.DC == NULL)
				{
					Throw(GetLastError(), "CreateCompatibleDC failed");
				}

				// CreateCompatibleDC will not work, as the screen is not of RGBA format.
				// So we have to explicitly create a bitmap with a 32-bit RGBA specification

				BITMAPINFO info = { 0 };
				info.bmiHeader.biSize = sizeof(info);
				info.bmiHeader.biWidth = ds.x;
				info.bmiHeader.biHeight = -ds.y;
				info.bmiHeader.biPlanes = 1;
				info.bmiHeader.biBitCount = 32;
				info.bmiHeader.biCompression = BI_RGB;
				info.bmiHeader.biSizeImage = ds.x * ds.y * 4;
				LPVOID bits = NULL;
				alphaBuilder.hBitmap = CreateDIBSection(alphaBuilder.DC, &info, DIB_RGB_COLORS, &bits, NULL, 0x0);
				if (alphaBuilder.hBitmap == NULL)
				{
					Throw(GetLastError(), "CreateDIBSection RGBA failed");
				}

				SetBitmapDimensionEx(alphaBuilder.hBitmap, ds.x, ds.y, NULL);
			}

			SIZE dimensions;
			GetBitmapDimensionEx(alphaBuilder.hBitmap, &dimensions);
			if (dimensions.cx != ds.x || dimensions.cy != ds.y)
			{
				// Screen dimensions no longer match the bitmap, so scratch the bitmap and cache again
				delete alphaBuilder.g;
				DeleteObject(alphaBuilder.hBitmap);
				DeleteDC(alphaBuilder.DC);
				alphaBuilder.hBitmap = 0;
				alphaBuilder.DC = 0;
				alphaBuilder.g = nullptr;
				goto cacheAlphaBuilder;
			}

			HBITMAP oldBitmap = (HBITMAP) SelectObject(alphaBuilder.DC, alphaBuilder.hBitmap);

			if (alphaBuilder.g == nullptr)
			{
				alphaBuilder.g = new Gdiplus::Graphics(alphaBuilder.DC);
			}

			return oldBitmap;
		}

		struct AutoReleaseBitmap
		{
			HBITMAP oldBitmap;
			HDC dc;

			AutoReleaseBitmap(HBITMAP _oldBitmap, HDC _dc): oldBitmap(_oldBitmap), dc(_dc)
			{

			}

			~AutoReleaseBitmap()
			{
				SelectObject(dc, oldBitmap);
			}
		};

		std::vector<TRIVERTEX> trivertexCache;
		std::vector<GRADIENT_TRIANGLE> triIndexCache;

		void PushTriangle(const GRTriangle& t)
		{
			TRIVERTEX& v = trivertexCache.emplace_back();
			v.x = t.a.position.x;
			v.y = t.a.position.y;
			CopyColour(v, t.a);

			TRIVERTEX& v1 = trivertexCache.emplace_back();
			v1.x = t.b.position.x;
			v1.y = t.b.position.y;
			CopyColour(v1, t.b);

			TRIVERTEX& v2 = trivertexCache.emplace_back();
			v2.x = t.c.position.x;
			v2.y = t.c.position.y;
			CopyColour(v2, t.c);

			ULONG index = (ULONG) (3 * triIndexCache.size());
			GRADIENT_TRIANGLE& indices = triIndexCache.emplace_back();
			indices.Vertex1 = index ++;
			indices.Vertex2 = index ++;
			indices.Vertex3 = index;
		}

		void PushRect(const GuiRect& rect, RGBAb tl, RGBAb tr, RGBAb bl, RGBAb br)
		{
			GRTriangle upper;
			upper.a.colour = tl;
			upper.a.position = TopLeft(rect);
			upper.b.colour = tr;
			upper.b.position = TopRight(rect);
			upper.c.colour = br;
			upper.c.position = BottomRight(rect);

			PushTriangle(upper);

			GRTriangle lower;
			lower.a.colour = br;
			lower.a.position = BottomRight(rect);
			lower.b.colour = bl;
			lower.b.position = BottomLeft(rect);
			lower.c.colour = tl;
			lower.c.position = TopLeft(rect);

			PushTriangle(lower);
		}

		void CommitTriangles()
		{
			if (trivertexCache.empty())
			{
				return;
			}

			GuiRect r = GetEnclosingRect(trivertexCache);

			AutoReleaseBitmap arb(CacheAlphaBuilder(), alphaBuilder.DC);

			Gdiplus::SolidBrush brush(Gdiplus::Color(0, 0, 0, 0));
			Gdiplus::Status status = alphaBuilder.g->FillRectangle(&brush, r.left, r.top, Width(r), Height(r));

			BOOL isOK = FALSE;

			if (status == Gdiplus::Status::Ok)
			{
				isOK = GradientFill(alphaBuilder.DC, trivertexCache.data(), (ULONG)trivertexCache.size(), triIndexCache.data(), (ULONG)triIndexCache.size(), GRADIENT_FILL_TRIANGLE);
			}
			
			trivertexCache.clear();
			triIndexCache.clear();

			if (!isOK)
			{
				return;
			}

			BLENDFUNCTION blendFunction;
			blendFunction.AlphaFormat = AC_SRC_ALPHA;
			blendFunction.BlendFlags = 0;
			blendFunction.BlendOp = AC_SRC_OVER;
			blendFunction.SourceConstantAlpha = 255;

			UseClipRect clipRect(paintDC, lastScissorRect);

			GuiRect targetRect = IntersectNormalizedRects(lastScissorRect, r);

			if (!AlphaBlend(paintDC, targetRect.left, targetRect.top, Width(r), Height(r), alphaBuilder.DC, targetRect.left, targetRect.top, Width(r), Height(r), blendFunction))
			{
				HRESULT nErr = GetLastError();
				if (nErr)
				{
					Throw(nErr, "AlphaBlend failed");
				}
				return;
			}
		}

		void DrawTriangles(const GRTriangle* triangles, size_t nTriangles) override
		{
			for (size_t i = 0; i < nTriangles; i++)
			{
				const auto& t = triangles[i];
				PushTriangle(t);
			}
		
			CommitTriangles();
		}

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			hilightRects.push_back({ absRect, colour1, colour2 });
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& textRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour, const CaretSpec& caret) override
		{
			UseClipRect useClip(paintDC, lastScissorRect);

			SelectFont(custodian, fontId, paintDC);

			TEXTMETRICA tm;
			GetTextMetricsA(paintDC, &tm);

			int caretLeftColumn = 0;
			int caretRightColumn = 0;

			if (caret.CaretPos == 0)
			{
				// Caret is at the start
				// Zero.

				if (text.length > 0)
				{
					// Caret right side is the width of the first glyph
					SIZE ds;
					if (GetTextExtentPoint32A(paintDC, text, 1, &ds))
					{
						caretRightColumn = ds.cx;
					}
				}
			}
			else if (caret.CaretPos < 0 || caret.CaretPos >= text.length)
			{
				// Caret is at the end
				SIZE ds;
				if (GetTextExtentPoint32A(paintDC, text, text.length, &ds))
				{
					caretLeftColumn = ds.cx;
				}

				// Caret is the span of the glyph 'A'
				if (GetTextExtentPoint32A(paintDC, "A", 1, &ds))
				{
					caretRightColumn = caretLeftColumn + ds.cx;
				}
			}
			else
			{
				SIZE ds;
				if (GetTextExtentPoint32A(paintDC, text, caret.CaretPos, &ds))
				{
					caretLeftColumn = ds.cx;
				}

				if (GetTextExtentPoint32A(paintDC, text, caret.CaretPos + 1, &ds))
				{
					caretRightColumn = ds.cx;
				}
			}

			// To calculate caretPos using GDI:
			//    Get text metrics of clipped text before caretPos and after the caretPos to get dimensions of caret 
			//    MoveToEx + LineTo to draw it.

			UINT format = FormatWin32DrawTextAlignment(alignment);

			format += DT_END_ELLIPSIS;
			format += DT_SINGLELINE;
			format += DT_NOPREFIX;
			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));

			RECT rect{ textRect.left, textRect.top, textRect.right, textRect.bottom };

			int caretY = 0;

			if (alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.top -= tm.tmInternalLeading;
				rect.top += spacing.y;

				caretY = rect.top + tm.tmHeight;
			}

			if (!alignment.HasSomeFlags(EGRAlignment::Top) && alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.bottom -= spacing.y;
				caretY = rect.bottom - tm.tmInternalLeading;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Left) && !alignment.HasSomeFlags(EGRAlignment::Right))
			{
				rect.left += spacing.x;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Right) && !alignment.HasSomeFlags(EGRAlignment::Left))
			{
				rect.right -= spacing.x;
			}

			if (alignment.HasAllFlags(EGRAlignment::VCentre))
			{
				rect.top -= (tm.tmInternalLeading >> 1);
				caretY = ((rect.top + rect.bottom + tm.tmHeight) / 2);
			}

			DrawTextA(paintDC, text, text.length, &rect, format);

			SetTextColor(paintDC, oldColour);

			RGBAb caretColour;
			
			if (caret.BlinksPerSecond <= 0)
			{
				caretColour = caret.CaretColour1;
			}
			else if (caret.BlinksPerSecond > 10)
			{
				caretColour = caret.CaretColour2;
			}
			else
			{
				auto now = Time::TickCount();
				auto t = now / (Time::TickHz() / caret.BlinksPerSecond);
				caretColour = (t % caret.BlinksPerSecond == 0) ? caret.CaretColour1 : caret.CaretColour2;
			}
			
			if (caret.IsInserting)
			{
				GDIPen pen(caretColour, caret.LineThickness);
				UsePen usePen(paintDC, pen);
				MoveToEx(paintDC, rect.left + caretLeftColumn, caretY, NULL);
				LineTo(paintDC, rect.left + caretRightColumn, caretY);
			}
			else
			{
				GuiRect caretRect;
				caretRect.left = rect.left + caretLeftColumn;
				caretRect.top = caretY - tm.tmHeight;
				caretRect.right = rect.left + caretRightColumn;
				caretRect.bottom = caretY;
				DrawRectEdge(caretRect, caretColour, caretColour, EGRRectStyle::SHARP, 0);
			}
		}

		void DrawParagraph(GRFontId fontId, const GuiRect& targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			if (colour.alpha == 0)
			{
				return;
			}

			UseClipRect useClip(paintDC, lastScissorRect);

			SelectFont(custodian, fontId, paintDC);

			TEXTMETRICA tm;
			GetTextMetricsA(paintDC, &tm);

			RECT rect{ targetRect.left, targetRect.top, targetRect.right, targetRect.bottom };

			if (alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.top -= tm.tmInternalLeading;
				rect.top += spacing.y;
			}

			if (!alignment.HasSomeFlags(EGRAlignment::Top) && alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.bottom -= spacing.y;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Left) && !alignment.HasSomeFlags(EGRAlignment::Right))
			{
				rect.left += spacing.x;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Right) && !alignment.HasSomeFlags(EGRAlignment::Left))
			{
				rect.right -= spacing.x;
			}

			if (alignment.HasAllFlags(EGRAlignment::VCentre))
			{
				rect.top -= (tm.tmInternalLeading >> 1);
			}

			UINT format = FormatWin32DrawTextAlignment(alignment);

			if (!alignment.HasSomeFlags(EGRAlignment::AutoFonts))
			{
				format += DT_END_ELLIPSIS;
			}

			format += DT_NOPREFIX;
			format += DT_NOCLIP;

			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));
			DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&rect), format);
			SetTextColor(paintDC, oldColour);
		}

		void DrawText(GRFontId fontId, const GuiRect & targetRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring & text, RGBAb colour)
		{
			if (colour.alpha == 0)
			{
				return;
			}

			UseClipRect useClip(paintDC, lastScissorRect);

			SelectFont(custodian, fontId, paintDC);

			TEXTMETRICA tm;
			GetTextMetricsA(paintDC, &tm);

			RECT rect{ targetRect.left, targetRect.top, targetRect.right, targetRect.bottom };

			if (alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.top -= tm.tmInternalLeading;
				rect.top += spacing.y;
			}

			if (!alignment.HasSomeFlags(EGRAlignment::Top) && alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.bottom -= spacing.y;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Left) && !alignment.HasSomeFlags(EGRAlignment::Right))
			{
				rect.left += spacing.x;
			}

			if (alignment.HasSomeFlags(EGRAlignment::Right) && !alignment.HasSomeFlags(EGRAlignment::Left))
			{
				rect.right -= spacing.x;
			}

			if (alignment.HasAllFlags(EGRAlignment::VCentre))
			{
				rect.top -= (tm.tmInternalLeading >> 1);
			}

			UINT format = FormatWin32DrawTextAlignment(alignment);

			if (!alignment.HasSomeFlags(EGRAlignment::AutoFonts))
			{
				format += DT_END_ELLIPSIS;
			}

			format += DT_SINGLELINE;
			format += DT_NOPREFIX;
			format += DT_NOCLIP;

			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));

			if (alignment.HasSomeFlags(EGRAlignment::AutoFonts) && fontId > GRFontId::MENU_FONT)
			{
				RECT calculatedRect = rect;
				DrawTextA(paintDC, text, text.length, &calculatedRect, format | DT_CALCRECT);

				if (calculatedRect.right >= rect.right)
				{
					SelectFont(custodian, GRFontId::MENU_FONT, paintDC);
					DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&rect), format | DT_END_ELLIPSIS);
					goto end;
				}
			}

			DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&rect), format);

		end:
			SetTextColor(paintDC, oldColour);
		}

		// Causes all render operations to complete
		void Flush() override
		{

		}

		GuiRect lastScissorRect = GuiRect(0,0,0,0);

		void EnableScissors(const GuiRect& scissorRect) override
		{
			lastScissorRect = scissorRect;
		}

		void DisableScissors() override
		{
			lastScissorRect = GuiRect{ 0,0,0,0 };
		}

		bool TryGetScissorRect(GuiRect& scissorRect) const override
		{
			scissorRect = lastScissorRect;
			return lastScissorRect.IsNormalized();
		}

		IGRFonts& Fonts() override;
		IGRImages& Images() override;

		void Render(IGR2DScene& scene)
		{
			UseFont useFont(paintDC, DefaultFont(custodian));

			scene.Render(*this);

			for (auto& h : hilightRects)
			{
				DrawRectEdgeLast(h.absRect, h.colour1, h.colour2);
			}

			hilightRects.clear();
		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, HANDLE hImage)
		{
			SIZE size;
			if (!GetBitmapDimensionEx((HBITMAP)hImage, &size))
			{
				return false;
			}

			bool isLeftAligned = alignment.HasSomeFlags(EGRAlignment::Left) && !alignment.HasSomeFlags(EGRAlignment::Right);
			bool isRightAligned = !alignment.HasSomeFlags(EGRAlignment::Left) && alignment.HasSomeFlags(EGRAlignment::Right);

			int32 x = 0;

			GuiRect rect = panel.AbsRect();

			Vec2i ds = { size.cx, size.cy };

			if (isLeftAligned)
			{
				x = rect.left + spacing.x;
			}
			else if (isRightAligned)
			{
				x = rect.right - spacing.x - ds.x;
			}
			else
			{
				x = (rect.left + rect.right - ds.x) >> 1;
			}

			int y = 0;
			bool isTopAligned = alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom);
			bool isBottomAligned = !alignment.HasSomeFlags(EGRAlignment::Top) && alignment.HasSomeFlags(EGRAlignment::Bottom);

			if (isTopAligned)
			{
				y = rect.top + spacing.y;
			}
			else if (isBottomAligned)
			{
				y = rect.bottom - spacing.x - ds.y;
			}
			else
			{
				y = (rect.top + rect.bottom - ds.y) >> 1;
			}

			HDC hCompatibleDC = CreateCompatibleDC(paintDC);
			HANDLE hOld = SelectObject(hCompatibleDC, hImage);
			BitBlt(paintDC, x, y, ds.x, ds.y, hCompatibleDC, 0, 0, SRCCOPY);
			SelectObject(hCompatibleDC, hOld);
			DeleteDC(hCompatibleDC);

			return true;
		}
	};


	bool GDIImage::Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, bool isStretched, IGRRenderContext& g)
	{
		GuiRect rect = panel.AbsRect();

		if (alignment.HasSomeFlags(EGRAlignment::Left))
		{
			rect.left += spacing.x;
		}

		if (alignment.HasSomeFlags(EGRAlignment::Right))
		{
			rect.right -= spacing.x;
		}

		if (alignment.HasSomeFlags(EGRAlignment::Top))
		{
			rect.top += spacing.y;
		}

		if (alignment.HasSomeFlags(EGRAlignment::Bottom))
		{
			rect.bottom -= spacing.y;
		}

		if (isStretched)
		{
			g.DrawImageStretched(*this, rect);
		}
		else
		{
			g.DrawImageUnstretched(*this, rect, alignment);
		}
		return true;
	}

	enum class ERenderTaskType
	{
		Edge
	};

	struct RenderTask
	{
		ERenderTaskType type;
		GuiRect target;
		RGBAb colour1;
		RGBAb colour2;
	};

	const stringmap<cstr> macroToPingPath =
	{
		{ "$(COLLAPSER_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_COLLAPSE)", "!textures/toolbars/MAT/collapsed.tif" },
		{ "$(COLLAPSER_ELEMENT_EXPAND)", "!textures/toolbars/MAT/expanded.tif" },
		{ "$(COLLAPSER_ELEMENT_INLINE)", "!textures/toolbars/MAT/collapsed.tif" },
	};

	struct GDICustodian : IWin32GDICustodianSupervisor, IGRCustodian, IGREventHistory, IGRFonts, IGRImages, IGRKeyState
	{
		// Debugging materials:
		std::vector<IGRWidget*> history;
		EGREventRouting lastRoutingStatus = EGREventRouting::Terminate;
		int64 eventCount = 0;

		AutoFree<IO::IOSSupervisor> os;
		AutoFree<IO::IInstallationSupervisor> installation;

		mutable HDC screenDC = nullptr;

		stringmap<GDIImage*> images;

		GDICustodian()
		{
			os = IO::GetIOS();
			installation = IO::CreateInstallation(L"content.indicator.txt", *os);
			FontSpec defaultSpec;
			defaultSpec.FontName = "Tahoma";
			BindFontId(defaultSpec);
			defaultFont = knownFonts[0].handle;
		}

		IGRFonts& Fonts() override
		{
			if (screenDC == nullptr)
			{
				SyncToScreen();
			}
			return *this;
		}

		Strings::HString lastKnownControlType;

		cstr GetLastKnownControlType() const override
		{
			return lastKnownControlType;
		}

		void SetControlType(cstr lastKnownControlType) override
		{
			this->lastKnownControlType = lastKnownControlType;
		}

		IGRKeyState& Keys() override
		{
			return *this;
		}

		void AlertNoActionForKey() override
		{
			MessageBeep(0xFFFFFFFF);
		}

		bool IsCtrlPressed() const override
		{
			return GetAsyncKeyState(VK_CONTROL) != 0;
		}

		struct KnownFont
		{
			LOGFONTA creator;
			HFONT handle;
			TEXTMETRICA metrics;
		};

		std::vector<KnownFont> knownFonts;

		void ReleaseScreenDC()
		{
			if (screenDC)
			{
				ReleaseDC(NULL, screenDC);
				screenDC = NULL;
			}
		}

		virtual ~GDICustodian()
		{
			for (auto f : knownFonts)
			{
				DeleteObject(f.handle);
			}

			for (auto i : images)
			{
				i.second->Free();
			}

			ReleaseScreenDC();
		}

		HFONT defaultFont = NULL;

		HFONT DefaultFont() const
		{
			return defaultFont;
		}

		const KnownFont* GetFont(GRFontId id) const
		{
			size_t arrayIndex = 0;

			switch (id)
			{
			case GRFontId::NONE:
			case GRFontId::MENU_FONT:
				arrayIndex = 0;
				break;
			default:
				arrayIndex = static_cast<size_t>(id) - 2;
			}

			if (arrayIndex >= knownFonts.size())
			{
				return nullptr;
			}

			return &knownFonts[arrayIndex];
		}

		int GetFontHeight(GRFontId id) const override
		{
			auto* f = GetFont(id);
			return f ? f->metrics.tmHeight : 0;
		}

		Vec2i EvaluateMinimalSpan(GRFontId fontId, const fstring& text) const override
		{
			auto* f = GetFont(fontId);
			if (!f)
			{
				return { 0,0 };
			}

			UseFont font(screenDC, f->handle);

			SIZE span;
			if (!GetTextExtentPoint32A(screenDC, text, text.length, OUT & span))
			{
				return { 0,0 };
			}
			else
			{
				return { span.cx, span.cy };
			}
		}

		void SyncToScreen()
		{
			ReleaseScreenDC();
			screenDC = GetDC(NULL);
		}

		void OnPaint(IGR2DScene& scene, HWND hWnd, HDC paintDC) override
		{
			SyncToScreen();
			SceneRenderer renderer(*this, hWnd, paintDC, 0);
			UseBkMode bkMode(paintDC, TRANSPARENT);
			renderer.Render(scene);
		}

		void RenderGui(IGRSystem& gr, HWND hWnd, HDC paintDC) override
		{
			auto captureId = gr.Root().CapturedPanelId();

			SyncToScreen();
			SceneRenderer renderer(*this, hWnd, paintDC, captureId);
			UseBkMode bkMode(paintDC, TRANSPARENT);
			gr.RenderAllFrames(renderer);
		}

		static bool Match(const LOGFONTA& a, const LOGFONT& b)
		{
			if (!Strings::EqI(a.lfFaceName, b.lfFaceName))
			{
				return false;
			}

			bool match =
				a.lfHeight == b.lfHeight
				&& a.lfItalic == b.lfItalic
				&& a.lfUnderline == b.lfUnderline
				&& a.lfWeight == b.lfWeight;
			return match;
		}

		GRFontId BindFontId(const FontSpec& desc) override
		{
			LOGFONTA f = { 0 };
			Strings::SafeFormat(f.lfFaceName, "%s", desc.FontName);
			f.lfCharSet = (BYTE)desc.CharSet;
			f.lfHeight = desc.CharHeight;
			f.lfItalic = desc.Italic ? 1 : 0;
			f.lfWeight = desc.Bold ? FW_BOLD : FW_NORMAL;
			f.lfUnderline = desc.Underlined ? 1 : 0;

			for (size_t i = 0; i < knownFonts.size(); ++i)
			{
				if (Match(f, knownFonts[i].creator))
				{
					return static_cast<GRFontId>(i + 2);
				}
			}

			HFONT hFont = CreateFontIndirectA(&f);
			if (!hFont)
			{
				return GRFontId::NONE;
			}

			TEXTMETRICA tm = { 0 };
			HDC dc = GetDC(nullptr);
			if (!dc)
			{
				RaiseError(nullptr, EGRErrorCode::Generic, __FUNCTION__, "Cannot bind to font. GetDC returned nullptr");
			}
			else
			{
				auto oldFont = SelectObject(dc, hFont);
				bool success = GetTextMetricsA(dc, &tm);
				SelectObject(dc, oldFont);

				DeleteDC(dc);

				if (!success)
				{
					RaiseError(nullptr, EGRErrorCode::Generic, __FUNCTION__, "Cannot bind to font. GetTextMetricsA returned false. Error code %d", GetLastError());
				}
			}

			knownFonts.push_back({ f, hFont, tm });
			return static_cast<GRFontId>(knownFonts.size() + 1);
		}

		void SelectFont(GRFontId fontId, HDC dc)
		{
			auto* f = GetFont(fontId);
			if (f)
			{
				SelectObject(dc, f->handle);
			}
		}

		IGRImageSupervisor* CreateImageFromPath(cstr debugHint, cstr codedImagePath) override
		{
			auto i = macroToPingPath.find(codedImagePath);
			cstr imagePath = i != macroToPingPath.end() ? imagePath = i->second : codedImagePath;

			auto it = images.find(imagePath);
			if (it == images.end())
			{
				auto* newImage = new GDIImage(debugHint, imagePath, *installation);
				it = images.insert(imagePath, newImage).first;
			}

			return it->second;
		}

		void RecordWidget(IGRWidget& widget) override
		{
			history.push_back(&widget);
		}

		void RouteKeyboardEvent(const KeyboardEvent& key, IGRSystem& gr) override
		{
			GRKeyEvent keyEvent{ *this, eventCount, key };
			lastRoutingStatus = gr.RouteKeyEvent(keyEvent);
			if (lastRoutingStatus == EGREventRouting::NextHandler)
			{
				gr.ApplyKeyGlobally(keyEvent);
			}
		}

		EGRCursorIcon currentIcon = EGRCursorIcon::Arrow;

		void RouteMouseEvent(const MouseEvent& me, IGRSystem& gr) override
		{
			static_assert(sizeof GRCursorClick == sizeof uint16);

			history.clear();
			if (me.buttonFlags != 0)
			{
				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Unspecified, (int)(int16)me.buttonData };
				lastRoutingStatus = gr.RouteCursorClickEvent(cursorEvent);
			}
			else
			{
				GRCursorEvent cursorEvent{ *this, me.cursorPos, eventCount, *(GRCursorClick*)&me.buttonFlags, EGRCursorIcon::Arrow, 0 };
				lastRoutingStatus = gr.RouteCursorMoveEvent(cursorEvent);

				if (currentIcon != cursorEvent.nextIcon)
				{
					currentIcon = cursorEvent.nextIcon;

					/*
					switch (currentIcon)
					{
					case EGRCursorIcon::Arrow:
						sysRenderer.GuiResources().SetSysCursor(EWindowCursor_Default);
						break;
					case EGRCursorIcon::LeftAndRightDragger:
						sysRenderer.GuiResources().SetSysCursor(EWindowCursor_HDrag);
						break;
					}

					*/
				}
			}
			eventCount++;
		}

		IGRCustodian& Custodian() override
		{
			return *this;
		}

		IO::IInstallation& Installation() override
		{
			return *installation;
		}

		void Free() override
		{
			delete this;
		}

		void RaiseError(const Sex::ISExpression* associatedSExpression, EGRErrorCode, cstr function, cstr format, ...) override
		{
			char message[1024];
			va_list args;
			va_start(args, format);
			Strings::SafeVFormat(message, sizeof message, format, args);
			va_end(args);

			if (associatedSExpression)
			{
				Throw(*associatedSExpression, "%s: %s", function, message);
			}
			else
			{
				Throw(0, "%s: %s", function, message);
			}
		}

		std::vector<char> copyAndPasteBuffer;

		void TranslateToEditor(const GRKeyEvent& keyEvent, IGREditorMicromanager& manager) override
		{
			if (!keyEvent.osKeyEvent.IsUp())
			{
				switch (keyEvent.osKeyEvent.VKey)
				{
				case IO::VirtualKeys::VKCode_BACKSPACE:
					manager.BackspaceAtCaret();
					return;
				case IO::VirtualKeys::VKCode_DELETE:
					manager.DeleteAtCaret();
					return;
				case IO::VirtualKeys::VKCode_ENTER:
					manager.Return();
					return;
				case IO::VirtualKeys::VKCode_LEFT:
					manager.AddToCaretPos(-1);
					return;
				case IO::VirtualKeys::VKCode_RIGHT:
					manager.AddToCaretPos(1);
					return;
				case IO::VirtualKeys::VKCode_HOME:
					manager.AddToCaretPos(-100'000'000);
					return;
				case IO::VirtualKeys::VKCode_END:
					manager.AddToCaretPos(100'000'000);
					return;
				case IO::VirtualKeys::VKCode_C:
					if (IO::IsKeyPressed(IO::VirtualKeys::VKCode_CTRL))
					{
						// Note that GetTextAndLength is guaranteed to be at least one character, and if so, the one character is the nul terminating the string
						copyAndPasteBuffer.resize(manager.GetTextAndLength(nullptr, 0));
						manager.GetTextAndLength(copyAndPasteBuffer.data(), (int32)copyAndPasteBuffer.size());
						//Rococo::OS::CopyStringToClipboard(copyAndPasteBuffer.data());
						copyAndPasteBuffer.clear();
						return;
					}
					else
					{
						break;
					}
				case IO::VirtualKeys::VKCode_V:
					if (IO::IsKeyPressed(IO::VirtualKeys::VKCode_CTRL))
					{
						manager.GetTextAndLength(copyAndPasteBuffer.data(), (int32)copyAndPasteBuffer.size());

						/*
						struct : IStringPopulator
						{
							IGREditorMicromanager* manager = nullptr;
							void Populate(cstr text) override
							{
								for (cstr p = text; *p != 0; p++)
								{
									manager->AppendCharAtCaret(*p);
								}
							}
						} cb;
						cb.manager = &manager;
						Rococo::OS::PasteStringFromClipboard(cb);
						*/
						return;
					}
					else
					{
						break;
					}
				}

				if (keyEvent.osKeyEvent.unicode >= 32 && keyEvent.osKeyEvent.unicode <= 127)
				{
					manager.AppendCharAtCaret((char)keyEvent.osKeyEvent.unicode);
				}
			}
		}
	};

	IGRFonts& SceneRenderer::Fonts()
	{
		return custodian;
	}

	IGRImages& SceneRenderer::Images()
	{
		return custodian;
	}

	HFONT DefaultFont(GDICustodian& custodian)
	{
		return custodian.DefaultFont();
	}

	void SelectFont(GDICustodian& custodian, GRFontId fontId, HDC paintDC)
	{
		custodian.SelectFont(fontId, paintDC);
	}

	struct Win32GDIApp: IWin32GDIApp
	{
		Gdiplus::GdiplusStartupInput input;
		ULONG_PTR token;

		Win32GDIApp()
		{
			// Initialize GDI+.
			Gdiplus::GdiplusStartup(&token, &input, NULL);

		}

		virtual ~Win32GDIApp()
		{
			Gdiplus::GdiplusShutdown(token);
		}

		void Free()
		{
			delete this;
		}
	};

	static cstr clientClassName = "GR-Win32-GDI-CLIENT";

	struct GR_Win32_EmptyScene : IGR2DScene
	{
		void Render(IGRRenderContext& rc) override
		{
			GuiRect windowRect = rc.ScreenDimensions();
			rc.DrawRect(windowRect, RGBAb(0, 0, 0));
		}
	};

	class UsePaint
	{
		PAINTSTRUCT ps;
		HWND hWnd;
	public:
		UsePaint(HWND _hWnd) : hWnd(_hWnd)
		{
			BeginPaint(hWnd, &ps);
		}

		~UsePaint()
		{
			EndPaint(hWnd, &ps);
		}

		HDC DC() const
		{
			return ps.hdc;
		}
	};


	struct GRClientWindow: IGRGDIClientWindowSupervisor, IGreatSexResourceLoader, IEventCallback<const Joysticks::JoystickButtonEvent>
	{
		HWND hWnd = 0;
		AutoFree<Rococo::Gui::IGRSystemSupervisor> grSystem;
		AutoFree<Rococo::GR::Win32::IWin32GDICustodianSupervisor> gdiCustodian;

		IGR2DScene* scene = nullptr;
		GR_Win32_EmptyScene emptyScene;
		GRConfig config;

		HDC hMemDC = 0;
		HBITMAP hBackBuffer = 0;
		Vec2i lastSpan{ 0,0 };

		HWND hErrorWnd = 0;
		HWND hMessageWnd = 0;

		AutoFree<IJoystick_XBOX360_Supervisor> xbox360Controller;

		static void PopulateClientClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
		{
			classDef = { 0 };
			classDef.cbSize = sizeof(classDef);
			classDef.style = 0;
			classDef.cbWndExtra = 0;
			classDef.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
			classDef.hInstance = hInstance;
			classDef.lpszClassName = clientClassName;
			classDef.lpszMenuName = NULL;
			classDef.lpfnWndProc = DefWindowProc;
		}

		struct UsingBackBuffer
		{
			GRClientWindow& w;

			HGDIOBJ oldTarget;

			UsingBackBuffer(GRClientWindow& _w): w(_w)
			{
				oldTarget = SelectObject(w.hMemDC, w.hBackBuffer);
			}

			~UsingBackBuffer()
			{
				SelectObject(w.hMemDC, oldTarget);
			}
		};

		void OnPaint()
		{
			UsePaint paint(hWnd);

			if (hErrorWnd)
			{
				return;
			}

			if (!hMemDC)
			{
				hMemDC = CreateCompatibleDC(paint.DC());
			}

			RECT rect;
			GetClientRect(hWnd, &rect);

			if (hBackBuffer)
			{
				if (lastSpan.x != rect.right || lastSpan.y != rect.bottom)
				{
					DeleteObject(hBackBuffer);
					hBackBuffer = NULL;
				}
			}

			if (!hBackBuffer)
			{
				hBackBuffer = CreateCompatibleBitmap(paint.DC(), rect.right, rect.bottom);
				lastSpan = { rect.right, rect.bottom };
			}

			UsingBackBuffer ubb(*this);

			char debugText[Time::Timer::FORMAT_CAPACITY];

			Time::Timer t1("Custodian Paint");
			t1.Start();
			gdiCustodian->OnPaint(*scene, hWnd, hMemDC);
			t1.End();

			t1.FormatMillisecondsWithName(debugText);

			Time::Timer t2("Custodian Gui");
			t2.Start();
			gdiCustodian->RenderGui(*grSystem, hWnd, hMemDC);
			t2.End();

			t2.FormatMillisecondsWithName(debugText);

		//	DrawTextA(hMemDC, debugText, (int)strlen(debugText), &rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

			Time::Timer t3("BitBlt");
			t3.Start();
			BitBlt(paint.DC(), 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, SRCCOPY);
			t3.End();

			// BitBlt appears to take around 0.25-0.5 milliseconds, which is ~ DirectX speed.
			// t3.FormatMillisecondsWithName(debugText);
			// DrawTextA(paint.DC(), debugText, (int) strlen(debugText), &rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		}

		BYTE keystate[256] = { 0 };

		int32 VcodeToUnicode(int32 virtualKeyCode, int32 scancode, HKL layout)
		{
			if (!GetKeyboardState(keystate))
			{
				// Oh well, assume some previous state was relatively unchanged and valid
			}

			WCHAR buffer[4] = { 0,0,0,0 };
			UINT flags = 0;
			int charsRead = ToUnicodeEx(virtualKeyCode, scancode, keystate, buffer, 4, flags, layout);
			return (charsRead == 1) ? buffer[0] : 0;
		}

		std::vector<uint8> rawInputBuffer;

		bool hasFocus = false;

		HKL hKeyboardLayout = 0;

		IGRAppControl* sink = nullptr;

		void InterceptVKeys(IGRAppControl& sink) override
		{
			this->sink = &sink;
		}

		LRESULT OnRawInput(WPARAM wParam, LPARAM lParam)
		{
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hWnd, &p);

			UINT sizeofBuffer = 0;
			if (NO_ERROR != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sizeofBuffer, sizeof(RAWINPUTHEADER)))
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			rawInputBuffer.resize(sizeofBuffer);

			auto* buffer = rawInputBuffer.data();

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &sizeofBuffer, sizeof(RAWINPUTHEADER)) == (UINT)-1)
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			RAWINPUT& raw = *((RAWINPUT*)buffer);
			RAWINPUT* pRaw = &raw;

			if (hasFocus && hErrorWnd == 0)
			{
				if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					MouseEvent ev;
					ev.buttons = raw.data.mouse.ulButtons;
					ev.dx = raw.data.mouse.lLastX;
					ev.dy = raw.data.mouse.lLastY;
					ev.flags = raw.data.mouse.usFlags;
					ev.cursorPos = { p.x, p.y };
					gdiCustodian->RouteMouseEvent(ev, *grSystem);
					QueuePaint();
				}
				else if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					KeyboardEvent key;
					((RAWKEYBOARD&)key) = raw.data.keyboard;

					if (sink)
					{
						if (EGREventRouting::Terminate == sink->OnRawVKey(key.VKey))
						{
							return 0;
						}
					}

					key.unicode = VcodeToUnicode(key.VKey, key.scanCode, hKeyboardLayout);
					gdiCustodian->RouteKeyboardEvent(key, *grSystem);
					gdiCustodian->SetControlType("Keyboard");
					QueuePaint();
				}

				return 0;
			}
			else
			{
				return DefRawInputProc(&pRaw, 1, sizeof(RAWINPUTHEADER));
			}
		}

		void RegisterRawInput()
		{
			hKeyboardLayout = GetKeyboardLayout(0);

			RAWINPUTDEVICE mouseDesc;
			mouseDesc.hwndTarget = hWnd;
			mouseDesc.dwFlags = 0;
			mouseDesc.usUsage = 0x02;
			mouseDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc)))
			{
				Throw(GetLastError(), "RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc) failed");
			}

			RAWINPUTDEVICE keyboardDesc;
			keyboardDesc.hwndTarget = hWnd;
			keyboardDesc.dwFlags = 0;
			keyboardDesc.usUsage = 0x06;
			keyboardDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)))
			{
				Throw(GetLastError(), "RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)) failed");
			}
		}

		void OnGainFocus()
		{
			hasFocus = true;
			RegisterRawInput();
		}

		void OnLoseFocus()
		{
			hasFocus = false;
		}

		void OnSize(WPARAM, LPARAM)
		{
			RECT clientRect;
			if (hErrorWnd && GetClientRect(hWnd, &clientRect))
			{
				int messageHeight = 100;
				MoveWindow(hMessageWnd, 0, 0, clientRect.right, messageHeight, TRUE);
				MoveWindow(hErrorWnd, 0, messageHeight, clientRect.right, clientRect.bottom - messageHeight, TRUE);
			}
		}

		void InsertKeyboardEvent(uint16 vCode, bool isUp, uint16 unicode) override
		{
			KeyboardEvent kbe;
			kbe.VKey = vCode;
			kbe.Flags = isUp ? RI_KEY_BREAK : RI_KEY_MAKE;
			kbe.Message = 0;
			kbe.Reserved = 0;
			kbe.scanCode = 0;
			kbe.unicode = unicode;
			kbe.extraInfo = 0;				
			gdiCustodian->RouteKeyboardEvent(kbe, *grSystem);
		}

		void OnEvent(const Joysticks::JoystickButtonEvent& jbe) override
		{
			auto i = mapJStickToKeyboard.find(jbe.vkCode);
			if (i != mapJStickToKeyboard.end())
			{
				if (jbe.IsKeyUp() && !jbe.IsRepeat())
				{
					InsertKeyboardEvent(i->second, true, jbe.unicodeValue);
				}
				else if (!jbe.IsKeyUp() && (jbe.IsKeyDown() || jbe.IsRepeat()))
				{
					InsertKeyboardEvent(i->second, false, jbe.unicodeValue);
				}

				gdiCustodian->SetControlType("XBOX");
			}
		}

		void OnTick()
		{
			if (!hErrorWnd) InvalidateRect(hWnd, NULL, FALSE);

			xbox360Controller->Poll(*this);
		}

		static LRESULT GDIProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto* This = reinterpret_cast<GRClientWindow*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

			try
			{
				switch (msg)
				{
				case WM_SETFOCUS:
					This->OnGainFocus();
					return 0L;
				case WM_KILLFOCUS:
					This->OnLoseFocus();
					return 0L;
				case WM_PAINT:
					This->OnPaint();
					return 0L;
				case WM_ERASEBKGND:
					return 0L;
				case WM_INPUT:
					This->OnRawInput(wParam, lParam);
					break;
				case WM_SIZE:
					This->OnSize(wParam, lParam);
					return 0L;
				case WM_TIMER:
					This->OnTick();					
					break;
				}
			}
			catch (IException& ex)
			{
				Rococo::Windows::THIS_WINDOW parent(GetParent(hWnd));
				SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) DefWindowProc);
				Rococo::Windows::ShowErrorBox(parent, ex, "Exception caught in " __FUNCTION__);
				PostQuitMessage(0);
				return 0L;
			}

			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		GRClientWindow()
		{
			gdiCustodian = GR::Win32::CreateGDICustodian();
			grSystem = Gui::CreateGRSystem(config, gdiCustodian->Custodian());
			scene = &emptyScene;
			xbox360Controller = CreateJoystick_XBox360Proxy();
		}

		virtual ~GRClientWindow()
		{
			if (hErrorWnd)
			{
				DestroyWindow(hErrorWnd);
			}

			if (hErrFont)
			{
				DeleteObject(hErrFont);
			}

			if (hBackBuffer)
			{
				DeleteObject(hBackBuffer);
			}

			if (hMemDC)
			{
				DeleteDC(hMemDC);
			}

			if (timerId && hWnd)
			{
				KillTimer(hWnd, timerId);
			}

			DestroyWindow(hWnd);
		}

		void Create(HWND hParentWnd)
		{
			if (hParentWnd == nullptr)
			{
				Throw(GetLastError(), "%s: parent was NULL", __FUNCTION__);
			}

			auto hInstance = GetModuleHandleA(NULL);

			WNDCLASSEXA clientInfo;
			PopulateClientClass(hInstance, clientInfo);
			ATOM clientClassAtom = RegisterClassExA(&clientInfo);
			if (clientClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create %s class atom", clientInfo.lpszClassName);
			}

			RECT clientRect;
			GetClientRect(hParentWnd, &clientRect);

			hWnd = CreateWindowExA(
				0,
				clientInfo.lpszClassName,
				"rococo.gr Win32-GDI Test Window",
				WS_CHILD | WS_VISIBLE,
				0, // x
				0, // y
				clientRect.right, // width
				clientRect.bottom, // height
				hParentWnd,
				nullptr,
				hInstance,
				nullptr);

			if (hWnd == nullptr)
			{
				Throw(GetLastError(), "%s: could not create client window for %s", __FUNCTION__, clientInfo.lpszClassName);
			}

			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) this);
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) GDIProc);

			timerId = SetTimer(hWnd, 1024, 5, NULL);
		}

		UINT_PTR timerId = 0;

		void LinkScene(IGR2DScene* scene)
		{
			if (scene)
			{
				this->scene = scene;
			}
			else
			{
				this->scene = &emptyScene;
			}
		}

		bool LoadFrame(cstr sexmlFile, IGRWidgetMainFrame& frameWidget, IEventCallback<IGreatSexGenerator>& onGenerate) override
		{
			AutoFree<IAllocatorSupervisor> allocator = Memory::CreateBlockAllocator(64, 0, "GreatSexAllocator");
			AutoFree<IGreatSexGeneratorSupervisor> greatSex = CreateGreatSexGenerator(*allocator, *this);

			onGenerate.OnEvent(*greatSex);

			Auto<ISParser> sParser = Sex::CreateSexParser_2_0(*allocator);
			AutoFree<IExpandingBuffer> buffer = CreateExpandingBuffer(4_kilobytes);

			Installation().LoadResource(sexmlFile, *buffer, 16_megabytes);

			Auto<ISourceCode> src = sParser->ProxySourceBuffer((cstr)buffer->GetData(), (int)buffer->Length(), { 0,0 }, sexmlFile, nullptr);

			try
			{
				Auto<ISParserTree> tree = sParser->CreateTree(*src);
				cr_sex s = tree->Root();

				greatSex->AppendWidgetTreeFromSexML(s, frameWidget.ClientArea().Widget());
				frameWidget.Panel().PrepPanelAndDescendants();
				frameWidget.Panel().ClearAssociatedExpressions();
				return true;
			}
			catch (ParseException& pex)
			{
				ShowError(pex.Start(), pex.End(), pex.Name(), (cstr)buffer->GetData(), pex.Message());
				return false;
			}
		}

		void LoadGreatSexResource(cstr resourcePath, Rococo::IO::ILoadEventsCallback& onLoad) override
		{
			Installation().LoadResource(resourcePath, onLoad);
		}

		void MapXCCode(cstr jkeyName, uint16 keyboardCode)
		{
			auto jcode = xbox360Controller->GetVKeyCode(jkeyName);
			if (jcode == 0)
			{
				Throw(0, "%s: Error mapping %s", __FUNCTION__, jkeyName);
			}

			mapJStickToKeyboard[jcode] = keyboardCode;
		}

		void BindStandardXBOXControlsToVKeys() override
		{
			MapXCCode("Back", VK_ESCAPE);
			MapXCCode("A", VK_RETURN);
			MapXCCode("Shoulder.R", VK_TAB);
			MapXCCode("Shoulder.L", Rococo::IO::VirtualKeys::VKCode_ANTITAB);
			MapXCCode("B", VK_ESCAPE);
			MapXCCode("DPAD.U", VK_UP);
			MapXCCode("DPAD.D", VK_DOWN);
			MapXCCode("DPAD.L", VK_LEFT);
			MapXCCode("DPAD.R", VK_RIGHT);
			MapXCCode("X", VK_NEXT);
			MapXCCode("Y", VK_PRIOR);
		}

		std::unordered_map<uint16, uint16> mapJStickToKeyboard;

		void MapJoystickVirtualKeyToVirtualKeyboardKey(uint16 joystickVirtualKeyCode, uint16 keyboardVirtualKeyCode) override
		{
			mapJStickToKeyboard[joystickVirtualKeyCode] = keyboardVirtualKeyCode;
		}

		void QueuePaint() override
		{
			InvalidateRect(hWnd, NULL, TRUE);
		}

		IGREventHandler* SetEventHandler(Gui::IGREventHandler* eventHandler) override
		{
			return grSystem->SetEventHandler(eventHandler);
		}

		Gui::IGRSystem& GRSystem() override
		{
			return *grSystem;
		}

		operator HWND () override
		{
			return hWnd;
		}

		IO::IInstallation& Installation() override
		{
			return gdiCustodian->Installation();
		}

		Joysticks::IJoysticks& GetXBoxControllers() override
		{
			return *xbox360Controller;
		}

		void Free() override
		{
			delete this;
		}

		struct Err
		{
			Vec2i origin{ 0,0 };
			Vec2i start{ 0,0 };
			Vec2i end{ 0,0 };
			Strings::HString sourceBuffer;
			Strings::HString message;
		} err;

		HFONT hErrFont = NULL;

		void Hilight(const Vec2i& start, const Vec2i& end, COLORREF backColour, COLORREF foreColour)
		{
			int startIndex = (int)SendMessage(hErrorWnd, EM_LINEINDEX, start.y, 0) + start.x;
			int endIndex = (int)SendMessage(hErrorWnd, EM_LINEINDEX, end.y, 0) + end.x;

			SendMessage(hErrorWnd, EM_SETSEL, startIndex, endIndex);

			CHARFORMAT2W format;
			ZeroMemory(&format, sizeof(format));
			format.cbSize = sizeof(format);
			format.dwMask = CFM_BOLD | CFM_COLOR | CFM_BACKCOLOR;
			format.dwEffects = CFE_BOLD;
			format.crTextColor = foreColour;
			format.crBackColor = backColour;

			SendMessage(hErrorWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);

			SendMessage(hErrorWnd, EM_SETSEL, endIndex, endIndex);
			SendMessage(hErrorWnd, EM_SCROLLCARET, 0, 0);
		}

		void ShowError(Vec2i start, Vec2i end, cstr nameRef, cstr sourceBuffer, cstr message) override
		{
			err.start = start;
			err.end = end;
			err.sourceBuffer = sourceBuffer;
			err.message = message;

			if (hErrorWnd == nullptr)
			{
				cstr dllName = "Riched20.dll";
				if (!LoadLibraryA(dllName))
				{
					Throw(GetLastError(), "Unable to load %s", dllName);
				}

				auto hInstance = GetModuleHandleA(NULL);

				RECT clientRect;
				GetClientRect(hWnd, &clientRect);

				int messageHeight = 100;

				DWORD style = ES_MULTILINE | ES_READONLY;
				DWORD exstyle = WS_EX_CLIENTEDGE;
				hErrorWnd = CreateWindowExW(exstyle, L"RichEdit20W", L"", style | WS_VISIBLE | WS_CHILD, 0, messageHeight, clientRect.right, clientRect.bottom - messageHeight, hWnd, NULL, hInstance, 0);

				if (hErrorWnd == nullptr)
				{
					Throw(GetLastError(), "Unable to create rich edit window");
				}

				if (hErrFont == NULL)
				{
					hErrFont = CreateFontA(-12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");
				}

				SendMessage(hErrorWnd, WM_SETFONT, (WPARAM)hErrFont, TRUE);
				SendMessage(hErrorWnd, EM_SHOWSCROLLBAR, SB_VERT, TRUE);

				AppendText(hErrorWnd, RGB(0, 0, 0), RGB(255, 255, 255), err.sourceBuffer, strlen(err.sourceBuffer));

				Hilight(start, end, RGB(224, 224, 224), RGB(128, 0, 0));

				hMessageWnd = CreateWindowExW(exstyle, L"RichEdit20W", L"", style | WS_VISIBLE | WS_CHILD, 0, 0, clientRect.right, messageHeight, hWnd, NULL, hInstance, 0);

				if (hMessageWnd == nullptr)
				{
					Throw(GetLastError(), "Unable to create rich edit window");
				}

				SendMessage(hMessageWnd, WM_SETFONT, (WPARAM)hErrFont, TRUE);
				SendMessage(hMessageWnd, EM_SHOWSCROLLBAR, SB_VERT, TRUE);
				SendMessage(hMessageWnd, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 0));

				char prompt[256];
				Strings::SafeFormat(prompt, "Error in: %s\n line %d pos %d to line %d pos %d:\n", nameRef, start.y, start.x, end.y, end.x);
				AppendText(hMessageWnd, RGB(0, 0, 64), RGB(255, 255, 0), prompt, strlen(prompt));
				AppendText(hMessageWnd, RGB(0, 0, 0), RGB(255, 255, 0), err.message, strlen(err.message));

				int32 y = (int32)SendMessage(hErrorWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
				SendMessage(hErrorWnd, EM_LINESCROLL, 0, -y);
				SendMessage(hErrorWnd, EM_LINESCROLL, 0, start.y > 2 ? start.y -1 : 0 );
			}
		}

		void ResetContent()
		{
			DestroyWindow(hErrorWnd);
			hErrorWnd = nullptr;
		}

		void AppendText(HWND hErrorWnd, COLORREF foreground, COLORREF background, cstr text, size_t nChars)
		{
			CHARFORMAT2 c;
			memset(&c, 0, sizeof(c));
			c.cbSize = sizeof(c);
			c.dwMask = CFM_COLOR | CFM_BACKCOLOR;
			c.crBackColor = background;
			c.crTextColor = foreground;

			CHARRANGE cr;
			cr.cpMin = -1;
			cr.cpMax = -1;

			size_t len = min(strlen(text), nChars);

			const char* source = text;

			while (len > 0)
			{
				enum { SEGMENT_CAPACITY = 4096 };
				char segmentBuffer[SEGMENT_CAPACITY];

				size_t delta = (len >= SEGMENT_CAPACITY) ? SEGMENT_CAPACITY - 1 : len;

				memcpy(segmentBuffer, source, delta);
				segmentBuffer[delta] = 0;
				source += delta;
				len -= delta;

				SendMessage(hErrorWnd, EM_EXSETSEL, 0, (LPARAM)&cr);

				CHARRANGE rangeBeforeAppend;
				rangeBeforeAppend.cpMin = -1; // This will give the starting character for the range of added characters
				rangeBeforeAppend.cpMax = -1;
				SendMessage(hErrorWnd, EM_GETSEL, (WPARAM)&rangeBeforeAppend.cpMin, (LPARAM)&rangeBeforeAppend.cpMax);

				SendMessage(hErrorWnd, EM_REPLACESEL, 0, (LPARAM)segmentBuffer);

				CHARRANGE rangeAfterAppend;
				rangeAfterAppend.cpMin = -1;
				rangeAfterAppend.cpMax = -1; // This will give the end character positiion for the range of added characters
				SendMessage(hErrorWnd, EM_GETSEL, (WPARAM)&rangeAfterAppend.cpMin, (LPARAM)&rangeAfterAppend.cpMax);

				// Select everything we just added
				CHARRANGE cr4;
				cr4.cpMin = rangeBeforeAppend.cpMin;
				cr4.cpMax = rangeAfterAppend.cpMax;
				SendMessage(hErrorWnd, EM_EXSETSEL, 0, (LPARAM)&cr4);

				// Then assign the colours
				SendMessage(hErrorWnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&c);
			}
		}
	};

	struct GRMainFrameWindow : IGRMainFrameWindowSupervisor
	{
		HWND hWnd = 0;

		AutoFree<GR::Win32::IGRGDIClientWindowSupervisor> grClientWindow;

		GRMainFrameWindow()
		{

		}

		static BOOL OnParentResized(HWND hWnd, LPARAM /* lParam */)
		{
			char className[256];
			if (RealGetWindowClassA(hWnd, className, (UINT)sizeof className) != 0)
			{
				if (strcmp(className, GR::Win32::GetGRClientClassName()) == 0)
				{
					HWND hParent = GetParent(hWnd);
					RECT clientRect;
					GetClientRect(hParent, &clientRect);
					MoveWindow(hWnd, 0, 0, clientRect.right, clientRect.bottom, TRUE);
					InvalidateRect(hWnd, NULL, TRUE);
				}
			}

			return TRUE; /* unused according to Microsoft Docs */
		}

		static LRESULT OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			EnumChildWindows(hWnd, OnParentResized, 0);
			return DefWindowProc(hWnd, WM_SIZE, wParam, lParam);
		}

		static LRESULT MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				{
					PAINTSTRUCT ps;
					BeginPaint(hWnd, &ps);
					EndPaint(hWnd, &ps);
				}
				return 0L;
			case WM_ERASEBKGND:
				return 0L;
			case WM_GETMINMAXINFO:
			{
				auto* m = (MINMAXINFO*)lParam;
				m->ptMinTrackSize = POINT{ 800, 600 };
			}
			return 0L;
			case WM_CLOSE:
				PostQuitMessage(0);
				break;
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		static void PopulateMainClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
		{
			classDef = { 0 };
			classDef.cbSize = sizeof(classDef);
			classDef.style = 0;
			classDef.cbWndExtra = 0;
			classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
			classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
			classDef.hInstance = hInstance;
			classDef.lpszClassName = "GR-Win32-GDI-APP";
			classDef.lpszMenuName = NULL;
			classDef.lpfnWndProc = MainProc;
		}


		void Create(HWND hOwner, const GRMainFrameConfig& config)
		{
			HMODULE hInstance = GetModuleHandle(NULL);

			WNDCLASSEXA info;
			PopulateMainClass(hInstance, info);
			info.hIcon = config.hLargeIconPath;
			info.hIconSm = config.hSmallIconPath;
			ATOM appClassAtom = RegisterClassExA(&info);
			if (appClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create %s class atom", info.lpszClassName);
			}

			HWND hWnd = CreateWindowExA(
				WS_EX_APPWINDOW,
				info.lpszClassName,
				"rococo.gr Win32-GDI Test Window",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT, // x
				CW_USEDEFAULT, // y
				CW_USEDEFAULT, // width
				CW_USEDEFAULT, // height
				hOwner,
				config.hMainWindowMenu,
				hInstance,
				nullptr);

			if (hWnd == nullptr)
			{
				Throw(GetLastError(), "%s: could not create overlapped window for %s", __FUNCTION__, info.lpszClassName);
			}

			grClientWindow = GR::Win32::CreateGRClientWindow(hWnd);

			PostMessage(*grClientWindow, WM_SETFOCUS, 0, 0);
		}

		virtual ~GRMainFrameWindow()
		{
			DestroyWindow(hWnd);
		}

		void Free() override
		{
			delete this;
		}

		IGRGDIClientWindow& Client() override
		{
			return *grClientWindow;
		}

		operator HWND() override
		{
			return hWnd;
		}
	};
}

namespace Rococo::GR::Win32
{
	ROCOCO_API_EXPORT IWin32GDICustodianSupervisor* CreateGDICustodian()
	{
		return new GRANON::GDICustodian();
	}

	ROCOCO_API_EXPORT IWin32GDIApp* CreateWin32GDIApp()
	{
		return new GRANON::Win32GDIApp();
	}

	ROCOCO_API_EXPORT IGRGDIClientWindowSupervisor* CreateGRClientWindow(HWND hParentWnd)
	{
		AutoFree<GRANON::GRClientWindow> window = new GRANON::GRClientWindow();
		window->Create(hParentWnd);
		return window.Detach();
	}

	ROCOCO_API_EXPORT cstr GetGRClientClassName()
	{
		return GRANON::clientClassName;
	}

	ROCOCO_API_EXPORT IGRMainFrameWindowSupervisor* CreateGRMainFrameWindow(HWND hOwner, const GRMainFrameConfig& config)
	{
		AutoFree<GRANON::GRMainFrameWindow> window = new GRANON::GRMainFrameWindow();
		window->Create(hOwner, config);
		return window.Detach();
	}
}

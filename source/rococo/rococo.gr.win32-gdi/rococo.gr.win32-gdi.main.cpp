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
#include <rococo.imaging.h>

#include <sexy.types.h>

#include <vector>

#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

using namespace Rococo::Gui;
using namespace Gdiplus;

namespace Rococo::GR::Win32::Implementation
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
		GDIPen(RGBAb colour)
		{
			hPen = CreatePen(PS_SOLID, 1, RGB(colour.red, colour.green, colour.blue));
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
					info.bmiHeader.biHeight = span.y;
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
			else
			{
				Throw(0, "Could not load image: %s. Only tiff files are recognized", imagePath);
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

		}

		bool Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, IGRRenderContext& g) override;

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

		GDICustodian& custodian;
		Graphics g;

		Vec2i cursor{ 0,0 };
		
		SceneRenderer(GDICustodian& _custodian, HWND _hWnd, HDC dc): hWnd(_hWnd), custodian(_custodian), g(dc), paintDC(dc)
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
		}

		// Get some kind of hover point for the cursor
		Vec2i CursorHoverPoint() const override
		{
			return cursor;
		}

		// It is up to the renderer to decide if a panel is hovered.
		bool IsHovered(IGRPanel& panel) const override
		{
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

		void DrawImageStretched(IGRImage& image, const GuiRect& absRect, const GuiRect& clipRect)
		{
			HBITMAP hImage = (HBITMAP) static_cast<GDIImage&>(image).hImage;

			if (!bitmapDC)
			{
				bitmapDC = CreateCompatibleDC(paintDC);
			}

			UseClipRect useClip(paintDC, clipRect);

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

		void DrawImageUnstretched(IGRImage& image, const GuiRect& absRect, const GuiRect& clipRect, GRAlignmentFlags alignment) override
		{
			HBITMAP hImage = (HBITMAP) static_cast<GDIImage&>(image).hImage;

			if (!bitmapDC)
			{
				bitmapDC = CreateCompatibleDC(paintDC);
			}

			UseClipRect useClip(paintDC, clipRect);

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

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			SolidBrush solidBrush(Color(colour.red, colour.green, colour.blue, colour.alpha));
			g.FillRectangle(&solidBrush, absRect.left, absRect.top, Width(absRect), Height(absRect));
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb topLeftColour, RGBAb bottomRightColour) override
		{
			GDIPen topLeftPen(topLeftColour);
			UsePen usePen(paintDC, topLeftPen);
			
			MoveToEx(paintDC, absRect.right, absRect.top, NULL);
			LineTo(paintDC, absRect.left, absRect.top);
			LineTo(paintDC, absRect.left, absRect.bottom);

			GDIPen bottomRightPen(bottomRightColour);
			UsePen usePen2(paintDC, bottomRightPen);
			
			LineTo(paintDC, absRect.right, absRect.bottom);
			LineTo(paintDC, absRect.right, absRect.top);
		}

		struct HilightRect
		{
			GuiRect absRect;
			RGBAb colour1;
			RGBAb colour2;
		};

		std::vector<HilightRect> hilightRects;

		// Queues an edge rect for rendering after everything else of lower priority has been rendered. Used for highlighting
		void DrawRectEdgeLast(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			hilightRects.push_back({ absRect, colour1, colour2 });
		}

		void DrawEditableText(GRFontId fontId, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, int32 caretPos, RGBAb colour) override
		{
			UNUSED(caretPos);
			UNUSED(spacing);

			UseClipRect useClip(paintDC, clipRect);

			SelectFont(custodian, fontId, paintDC);

			// To calculate caretPos using GDI:
			//    Get text metrics of clipped text before caretPos and after the caretPos to get dimensions of caret 
			//    MoveToEx + LineTo to draw it.

			RECT rect{ clipRect.left, clipRect.top, clipRect.right, clipRect.bottom };

			UINT format = FormatWin32DrawTextAlignment(alignment);

			format += DT_END_ELLIPSIS;
			format += DT_SINGLELINE;
			format += DT_NOPREFIX;
			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));

			DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&rect), format);

			SetTextColor(paintDC, oldColour);
		}
		void DrawText(GRFontId fontId, const GuiRect& targetRect, const GuiRect& clipRect, GRAlignmentFlags alignment, Vec2i spacing, const fstring& text, RGBAb colour) override
		{
			UNUSED(spacing);

			UseClipRect useClip(paintDC, clipRect);

			SelectFont(custodian, fontId, paintDC);

			TEXTMETRICA tm;
			GetTextMetricsA(paintDC, &tm);

			RECT rect{ targetRect.left, targetRect.top, targetRect.right, targetRect.bottom };

			if (alignment.HasSomeFlags(EGRAlignment::Top) && !alignment.HasSomeFlags(EGRAlignment::Bottom))
			{
				rect.top -= tm.tmInternalLeading;
			}
			if (alignment.HasAllFlags(EGRAlignment::VCentre))
			{
				rect.top -= (tm.tmInternalLeading >> 1);
			}

			UINT format = FormatWin32DrawTextAlignment(alignment);

			format += DT_END_ELLIPSIS;
			format += DT_SINGLELINE;
			format += DT_NOPREFIX;
			format += DT_NOCLIP;

			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));

			DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&rect), format);
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

		void RenderInPaintStruct(IGR2DScene& scene)
		{
			UseBkMode bkMode(paintDC, TRANSPARENT);
			UseFont useFont(paintDC, DefaultFont(custodian));

			scene.Render(*this);

			for (auto& h : hilightRects)
			{
				DrawRectEdgeLast(h.absRect, h.colour1, h.colour2);
			}

			hilightRects.clear();
		}

		IGRFonts& Fonts() override;
		IGRImages& Images() override;

		void Render(IGR2DScene& scene)
		{
			UsePaint usePaint(hWnd);
			paintDC = usePaint.DC();
			RenderInPaintStruct(scene);
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


	bool GDIImage::Render(IGRPanel& panel, GRAlignmentFlags alignment, Vec2i spacing, IGRRenderContext& g)
	{
		GuiRect rect = panel.AbsRect();
		rect.left += spacing.x;
		rect.right -= spacing.x;
		rect.top += spacing.y;
		rect.bottom -= spacing.y;
		g.DrawImageUnstretched(*this, rect, GuiRect{ 0,0,0,0 }, alignment);
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
		{ "$(COLLAPSER_EXPAND)", "!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff" },
		{ "$(COLLAPSER_COLLAPSE)", "!textures/toolbars/3rd-party/www.aha-soft.com/Forward.tiff" },
		{ "$(COLLAPSER_ELEMENT_EXPAND)", "!textures/toolbars/expanded_state.tiff" },
		{ "$(COLLAPSER_ELEMENT_INLINE)", "!textures/toolbars/inline_state.tiff" },
	};

	struct GDICustodian : IWin32GDICustodianSupervisor, IGRCustodian, IGREventHistory, IGRFonts, IGRImages
	{
		// Debugging materials:
		std::vector<IGRWidget*> history;
		EGREventRouting lastRoutingStatus = EGREventRouting::Terminate;
		int64 eventCount = 0;

		AutoFree<IO::IOSSupervisor> os;
		AutoFree<IO::IInstallationSupervisor> installation;

		mutable HDC screenDC = nullptr;

		GDICustodian()
		{
			os = IO::GetIOS();
			installation = IO::CreateInstallation(L"content.indicator.txt", *os);
			FontSpec defaultSpec;
			defaultSpec.FontName = "Tahoma";
			BindFontId(defaultSpec);
			defaultFont = knownFonts[0].handle;
		}

		struct KnownFont
		{
			LOGFONTA creator;
			HFONT handle;
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

		void OnPaint(IGR2DScene& scene, HWND hWnd) override
		{
			SyncToScreen();
			UsePaint usePaint(hWnd);
			SceneRenderer renderer(*this, hWnd, usePaint.DC());
			renderer.Render(scene);
		}

		void RenderGui(IGRSystem& gr, HWND hWnd) override
		{
			SyncToScreen();
			UsePaint usePaint(hWnd);
			SceneRenderer renderer(*this, hWnd, usePaint.DC());
			UseBkMode bkMode(usePaint.DC(), TRANSPARENT);
			gr.RenderGui(renderer);
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
			knownFonts.push_back({ f, hFont });

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
			return new GDIImage(debugHint, imagePath, *installation);
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
}

namespace Rococo::GR::Win32
{
	ROCOCO_API_EXPORT IWin32GDICustodianSupervisor* CreateGDICustodian()
	{
		return new Rococo::GR::Win32::Implementation::GDICustodian();
	}
}

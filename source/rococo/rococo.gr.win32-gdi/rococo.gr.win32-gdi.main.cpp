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

#include <sexy.types.h>

#include <vector>

#pragma comment(lib, "Msimg32.lib")

#include <gdiplus.h>
#pragma comment (lib,"Gdiplus.lib")

using namespace Rococo;
using namespace Rococo::GR;
using namespace Rococo::GR::Win32;
using namespace Rococo::Gui;
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
		Gdiplus::Graphics g;

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
			SolidBrush solidBrush(Color(colour.alpha, colour.red, colour.green, colour.blue));
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

		void OnPaint(IGR2DScene& scene, HWND hWnd, HDC paintDC) override
		{
			SyncToScreen();
			SceneRenderer renderer(*this, hWnd, paintDC);
			UseBkMode bkMode(paintDC, TRANSPARENT);
			renderer.Render(scene);
		}

		void RenderGui(IGRSystem& gr, HWND hWnd, HDC paintDC) override
		{
			SyncToScreen();
			SceneRenderer renderer(*this, hWnd, paintDC);
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


	struct GRClientWindow: IGRClientWindowSupervisor
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
			gdiCustodian->OnPaint(*scene, hWnd, hMemDC);
			gdiCustodian->RenderGui(*grSystem, hWnd, hMemDC);
			BitBlt(paint.DC(), 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, SRCCOPY);
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

			if (hasFocus)
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
					key.unicode = VcodeToUnicode(key.VKey, key.scanCode, hKeyboardLayout);
					gdiCustodian->RouteKeyboardEvent(key, *grSystem);
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
		}

		virtual ~GRClientWindow()
		{
			if (hBackBuffer)
			{
				DeleteObject(hBackBuffer);
			}

			if (hMemDC)
			{
				DeleteDC(hMemDC);
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
		}

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

		void QueuePaint() override
		{
			InvalidateRect(hWnd, NULL, TRUE);
		}

		Gui::IGRSystem& GRSystem() override
		{
			return *grSystem;
		}

		operator HWND () override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}
	};

	struct GRMainFrameWindow : IGRMainFrameWindowSupervisor
	{
		HWND hWnd = 0;

		AutoFree<GR::Win32::IGRClientWindowSupervisor> grClientWindow;

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
			case WM_ERASEBKGND:
				return 0L;
			case WM_GETMINMAXINFO:
			{
				auto* m = (MINMAXINFO*)lParam;
				m->ptMinTrackSize = POINT{ 640, 480 };
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

		IGRClientWindow& Client() override
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

	ROCOCO_API_EXPORT IGRClientWindowSupervisor* CreateGRClientWindow(HWND hParentWnd)
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

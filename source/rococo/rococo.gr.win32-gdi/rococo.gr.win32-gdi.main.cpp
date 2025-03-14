#include <rococo.types.h>

#define ROCOCO_GR_GDI_API ROCOCO_API_EXPORT

#include <rococo.gr.win32-gdi.h>
#include <rococo.strings.h>
#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <rococo.maths.i32.h>
#include <vector>

using namespace Rococo::Gui;

namespace Rococo::GR::Win32::Implementation
{
	class SolidBrush
	{
	private:
		HBRUSH hBrush;

	public:
		SolidBrush(RGBAb colour)
		{
			hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
		}

		~SolidBrush()
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

	class Pen
	{
	private:
		HPEN hPen;

	public:
		Pen(RGBAb colour)
		{
			hPen = CreatePen(PS_SOLID, 1, RGB(colour.red, colour.green, colour.blue));
		}

		~Pen()
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

	struct SceneRenderer: Gui::IGRRenderContext
	{
		HWND hWnd;
		HDC paintDC = 0;
		
		SceneRenderer(HWND _hWnd): hWnd(_hWnd)
		{

		}

		// Get some kind of hover point for the cursor
		Vec2i CursorHoverPoint() const override
		{
			return { 0,0 };
		}

		// It is up to the renderer to decide if a panel is hovered.
		bool IsHovered(IGRPanel& panel) const override
		{
			UNUSED(panel);
			return false;
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

			SolidBrush brush(colour);
			UseBrush useBrush(paintDC, brush);

			int oldMode = SetPolyFillMode(paintDC, ALTERNATE);
			Polygon(paintDC, v, 3);		
			SetPolyFillMode(paintDC, oldMode);
		}

		void DrawRect(const GuiRect& absRect, RGBAb colour) override
		{
			SolidBrush brush(colour);
			FillRect(paintDC, reinterpret_cast<const RECT*>(&absRect), brush);
		}

		void DrawRectEdge(const GuiRect& absRect, RGBAb colour1, RGBAb colour2) override
		{
			Pen topLeftPen(colour1);
			UsePen usePen(paintDC, topLeftPen);
			
			MoveToEx(paintDC, absRect.left, absRect.top, NULL);
			LineTo(paintDC, absRect.right, absRect.top);
			LineTo(paintDC, absRect.right, absRect.bottom);

			Pen bottomRightPen(colour2);
			UsePen usePen2(paintDC, bottomRightPen);
			
			LineTo(paintDC, absRect.left, absRect.bottom);
			LineTo(paintDC, absRect.left, absRect.top);
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
			UNUSED(fontId);
			UNUSED(caretPos);
			UNUSED(spacing);

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
			UNUSED(fontId);
			UNUSED(spacing);

			RECT rect{ targetRect.left, targetRect.top, targetRect.right, targetRect.bottom };

			UINT format = FormatWin32DrawTextAlignment(alignment);

			format += DT_END_ELLIPSIS;
			format += DT_SINGLELINE;
			format += DT_NOPREFIX;
			COLORREF oldColour = SetTextColor(paintDC, RGB(colour.red, colour.green, colour.blue));

			GuiRect finalRect = targetRect;
			if (clipRect.IsNormalized() && targetRect.IsNormalized())
			{
				finalRect = Rococo::IntersectNormalizedRects(targetRect, clipRect);
			}
			
			DrawTextA(paintDC, text, text.length, reinterpret_cast<RECT*>(&finalRect), format);
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

		void Render(IGR2DScene& scene)
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);

			paintDC = ps.hdc;

			scene.Render(*this);

			for (auto& h : hilightRects)
			{
				DrawRectEdgeLast(h.absRect, h.colour1, h.colour2);
			}

			hilightRects.clear();

			EndPaint(hWnd, &ps);
		}
	};

	struct SceneHandler : IGR2DSceneHandlerSupervisor
	{
		void OnPaint(IGR2DScene& scene, HWND hWnd) override
		{
			SceneRenderer renderer(hWnd);
			renderer.Render(scene);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::GR::Win32
{
	ROCOCO_API_EXPORT IGR2DSceneHandlerSupervisor* CreateSceneHandler()
	{
		return new Implementation::SceneHandler();
	}
}
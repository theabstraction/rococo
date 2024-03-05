#include <rococo.editors.h>

using namespace Rococo::Editors;

namespace Rococo::Windows
{
	class DC_Renderer : public Rococo::Editors::IFlatGuiRenderer
	{
		HDC dc;
		HBRUSH hBrush;
		HBRUSH hOldBrush = nullptr;
		HPEN hPen;
		HPEN hOldPen = nullptr;

		Vec2i span{ 0,0 };
		Vec2i cursorPosition{ 0,0 };
	public:

		DC_Renderer(HDC _dc, HWND hWnd) : dc(_dc), hBrush(GetStockBrush(WHITE_BRUSH)), hPen((HPEN)GetStockObject(WHITE_PEN))
		{
			hOldBrush = (HBRUSH) SelectObject(dc, hBrush);
			hOldPen = (HPEN)SelectObject(dc, hPen);

			POINT p;
			if (GetCursorPos(&p) && ScreenToClient(hWnd, &p))
			{
				cursorPosition = { p.x, p.y };
			}

			RECT rect;
			if (GetClientRect(hWnd, &rect))
			{
				span = { rect.right, rect.bottom };
			}
		}

		~DC_Renderer()
		{
			SelectObject(dc, hOldBrush);
			SelectObject(dc, hOldPen);
		}

		Vec2i CursorPosition() const override
		{
			return cursorPosition;
		}

		Vec2i Span() const override
		{
			return span;
		}

		void DrawCircle(const GuiRect& rect, RGBAb edgeColour, int thickness, RGBAb fillColour) override
		{
			HBRUSH hBrush = fillColour.alpha > 0 ? CreateSolidBrush(RGB(fillColour.red, fillColour.green, fillColour.blue)) : GetStockBrush(NULL_BRUSH);
			HBRUSH hOldBrush = (HBRUSH) SelectObject(dc, hBrush);
			HPEN hPen = edgeColour.alpha > 0 ? CreatePen(PS_SOLID, thickness, RGB(edgeColour.red, edgeColour.green, edgeColour.blue)) : GetStockPen(NULL_PEN);
			HPEN hOldPen = (HPEN)SelectObject(dc, hPen);
			::Ellipse(dc, rect.left, rect.top, rect.right, rect.bottom);
			SelectObject(dc, hOldBrush);
			SelectObject(dc, hOldPen);
			DeleteObject(hBrush);
			DeleteObject(hPen);
		}

		void DrawSpline(Vec2i start, Vec2i startDirection, Vec2i end, Vec2i endDirection, RGBAb colour) override
		{
			HPEN hPen = CreatePen(PS_SOLID, 2, RGB(colour.red, colour.green, colour.blue));
			HPEN hOldPen = (HPEN) SelectObject(dc, hPen);

			POINT points[4];
			points[0] = { start.x, start.y };
			points[1] = { start.x + startDirection.x, start.y + startDirection.y };
			points[2] = { end.x + endDirection.x, end.y + endDirection.y };
			points[3] = { end.x, end.y };
			
			::PolyBezier(dc, points, sizeof points / sizeof POINT);

			SelectObject(dc, hOldPen);
			DeleteObject(hPen);
		}

		void DrawText(const GuiRect& rect, cstr text, uint32 flags) override
		{
			RECT textRect{ rect.left, rect.top, rect.right, rect.bottom };

			DWORD dtText = DT_SINGLELINE;

			if (HasFlag(EFGAF_Right, flags))
			{
				dtText |= DT_RIGHT;
			}

			if (HasFlag(EFGAF_Bottom, flags))
			{
				dtText |= DT_BOTTOM;
			}

			if (HasFlag(EFGAF_VCentre, flags))
			{
				dtText |= DT_VCENTER;
			}

			::DrawTextA(dc, text, (int) strlen(text), &textRect, dtText);
		}

		void DrawFilledRect(const GuiRect& rect) override
		{
			::FillRect(dc, reinterpret_cast<const RECT*>(&rect), hBrush);
		}

		void DrawRoundedRect(const GuiRect& rect, int border, RGBAb edgeColour) override
		{
			HPEN hPen = CreatePen(PS_SOLID, 1, RGB(edgeColour.red, edgeColour.green, edgeColour.blue));
			HPEN hOldPen = (HPEN) SelectObject(dc, hPen);

			Vec2i span = { rect.right - rect.left, rect.bottom - rect.top };
			::RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, border, border);

			SelectObject(dc, hOldPen);
			DeleteObject(hPen);
		}

		void DrawLineTo(Vec2i pos, RGBAb edgeColour, int thickness) override
		{
			if (edgeColour.alpha == 0) return;

			HPEN hPen = CreatePen(PS_SOLID, thickness, RGB(edgeColour.red, edgeColour.green, edgeColour.blue));
			HPEN hOldPen = (HPEN) SelectObject(dc, hPen);
			::LineTo(dc, pos.x, pos.y);
			SelectObject(dc, hOldPen);
			DeleteObject(hPen);
		}

		void MoveLineStartTo(Vec2i pos) override
		{
			::MoveToEx(dc, pos.x, pos.y, NULL);
		}

		void SetTextOptions(RGBAb backColour, RGBAb textColour) override
		{
			SetBkColor(dc, RGB(backColour.red, backColour.green, backColour.blue));
			SetTextColor(dc, RGB(textColour.red, textColour.green, textColour.blue));
		}

		void SetFillOptions(RGBAb colour) override
		{
			DeleteObject(hBrush);
			hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
			SelectObject(dc, hBrush);
		}
	};
}
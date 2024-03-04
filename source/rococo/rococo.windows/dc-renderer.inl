#include <rococo.editors.h>

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

		void DrawText(const GuiRect& rect, cstr text) override
		{
			RECT textRect{ rect.left, rect.top, rect.right, rect.bottom };

			::DrawTextA(dc, text, (int) strlen(text), &textRect, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
		}

		void DrawFilledRect(const GuiRect& rect) override
		{
			::FillRect(dc, reinterpret_cast<const RECT*>(&rect), hBrush);
		}

		void DrawRoundedRect(const GuiRect& rect, int border) override
		{
			Vec2i span = { rect.right - rect.left, rect.bottom - rect.top };
			::RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, border, border);
		}

		void DrawLineTo(Vec2i pos) override
		{
			::LineTo(dc, pos.x, pos.y);
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

		void SetLineOptions(RGBAb colour) override
		{
			DeleteObject(hPen);
			hPen = CreatePen(PS_SOLID, 1, RGB(colour.red, colour.green, colour.blue));
			SelectObject(dc, hPen);
		}

		void SetFillOptions(RGBAb colour) override
		{
			DeleteObject(hBrush);
			hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
			SelectObject(dc, hBrush);
		}
	};
}
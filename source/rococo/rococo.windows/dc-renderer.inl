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

		void FillRect(const GuiRect& rect) override
		{
			::FillRect(dc, reinterpret_cast<const RECT*>(&rect), hBrush);
		}

		void LineTo(Vec2i pos) override
		{
			::LineTo(dc, pos.x, pos.y);
		}

		void MoveTo(Vec2i pos) override
		{
			::MoveToEx(dc, pos.x, pos.y, NULL);
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
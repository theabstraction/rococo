#pragma once

namespace Rococo::Windows
{
	class Pen
	{
		HPEN hPen = nullptr;
	public:
		Pen(int iStyle, int width, COLORREF color)
		{
			hPen = CreatePen(iStyle, width, color);
		}

		~Pen()
		{
			DeleteObject(hPen);
		}

		operator HPEN()
		{
			return hPen;
		}
	};

	class PenContext
	{
	public:
		HPEN oldPen = nullptr;
		HDC dc;

		PenContext(HPEN hPen, HDC _dc): dc(_dc)
		{
			oldPen = (HPEN) SelectObject(dc, hPen);
		}

		~PenContext()
		{
			SelectObject(dc, oldPen);
		}
	};

	class Grid_2D : public IUI2DGridSlateSupervisor, private IWindowHandler
	{
	private:
		AutoFree<IParentWindowSupervisor> window;
		IUI2DGridEvents& eventHandler;

		Grid_2D(IUI2DGridEvents& _handler) :
			eventHandler(_handler)
		{
		}

		void OnEraseBackground(HDC dc)
		{
			HBRUSH hBrush = GetStockBrush(BLACK_BRUSH);

			RECT rect;
			GetClientRect(*window, &rect);
			FillRect(dc, &rect, hBrush);
		}

		void OnPaint(HDC dc, const RECT& subRect)
		{
			Pen pen(PS_SOLID, 1, RGB(128, 128, 255));
			PenContext usePen(pen, dc);

			for (double x = left; x < right; x += smallestGradation)
			{
				int iX = (int)x;
				MoveToEx(dc, iX, (int) -bottom, NULL);
				LineTo(dc, iX, (int) -top);
			}

			for (double y = bottom; y < top; y += smallestGradation)
			{
				int iY = -(int)y;
				MoveToEx(dc, (int) left, iY, NULL);
				LineTo(dc, (int) right, iY);
			}
		}

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_ERASEBKGND:
				OnEraseBackground((HDC)wParam);
				return 1L;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(*window, &ps);

				OnPaint(hdc, ps.rcPaint);

				EndPaint(*window, &ps);
			}
			return 0;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND, WPARAM wParam, LPARAM lParam)
		{
			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			UNUSED(width);
			UNUSED(height);

			switch (wParam)
			{
			case SIZE_RESTORED:
			case SIZE_MAXSHOW:
			case SIZE_MAXIMIZED:
			case SIZE_MAXHIDE:
				break;
			}

			return 0L;
		}

		double scaleFactor = 1.0;
		double left = -4096.0;
		double right = 4096.0;
		double top = 4096.0;
		double bottom = -4096.0;
		double originX = 0.0;
		double originY = 0.0;
		double smallestGradation = 32.0;

		double ScaleFactor() const override
		{
			return scaleFactor;
		}

		void SetScaleFactor(double newValue) override
		{
			scaleFactor = newValue;
		}

		void SetHorizontalDomain(double left, double right) override
		{
			this->left = left;
			this->right = right;
		}

		void SetVerticalDomain(double top, double bottom) override
		{
			this->top = top;
			this->bottom = bottom;
		}

		void SetCentrePosition(double x, double y) override
		{
			originX = x;
			originY = y;
		}

		void SetSmallestGradation(double gradationDelta) override
		{
			smallestGradation = gradationDelta;
		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		void ResizeToParent() override
		{
			RECT rect;
			GetClientRect(GetParent(*window), &rect);
			MoveWindow(*window, 0, 0, rect.right, rect.bottom, TRUE);
		}
	public:
		static Grid_2D* Create(const WindowConfig& config, IUI2DGridEvents& eventHandler)
		{
			Grid_2D* p = new Grid_2D(eventHandler);
			p->window = CreateChildWindow(config, static_cast<IWindowHandler*>(p));

			return p;
		}

		virtual IWindowHandler& Handler()
		{
			return *this;
		}

		virtual operator HWND () const
		{
			return *window;
		}

		void Free() override
		{
			delete this;
		}
	};
}
#pragma once

#include <cmath>

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

	struct GVec2
	{
		double x;
		double y;
	};

	class Grid_2D : public IUI2DGridSlateSupervisor, private IWindowHandler
	{
	private:
		AutoFree<IParentWindowSupervisor> window;
		IUI2DGridEvents& eventHandler;

		int16 wheelDeltaSum = 0;

		double viewScaleFactor = 1.0; // Number of pixels per unit distance in the world
		double worldLeft = -4096.0;
		double worldRight = 4096.0;
		double worldTop = 4096.0;
		double worldBottom = -4096.0;
		GVec2 viewOrigin{ 0, 0 }; // World co-ordinates of the screen centre
		double mapSmallestGradation = 100.0; // Width of grid lines in world units
		double minPixelSpan = 8.0; // Smallest visible gradation span
		GVec2 viewSpan = { 1.0, 1.0 };

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

		GVec2 Quantize(const GVec2& worldSpace, double delta)
		{
			double integralComponentX = 0;
			double remainderX = std::modf(worldSpace.x / delta, &integralComponentX);

			double integralComponentY = 0;
			double remainderY = std::modf(worldSpace.y / delta, &integralComponentY);

			return GVec2{ integralComponentX * delta, integralComponentY * delta };
		}

		void PaintGradation(HDC dc, Pen& pen, double worldGradationSpan)
		{
			double pixelSpan = worldGradationSpan * viewScaleFactor;
			if (pixelSpan < minPixelSpan)
			{
				return;
			}

			PenContext usePen(pen, dc);

			GVec2 bottomLeft{ 0,viewSpan.y };
			GVec2 screenInWorldSpaceBottomLeft = ScreenToWorld(bottomLeft);

			GVec2 topRight{ viewSpan.x, 0 };
			GVec2 screenInWorldSpaceTopRight = ScreenToWorld(topRight);

			GVec2 bl = Quantize(screenInWorldSpaceBottomLeft, worldGradationSpan);
			GVec2 tr = Quantize(screenInWorldSpaceTopRight, worldGradationSpan);
			tr.x += worldGradationSpan;
			tr.y += worldGradationSpan;
			bl.x -= worldGradationSpan;
			bl.y -= worldGradationSpan;

			for (double x = bl.x; x < tr.x; x += worldGradationSpan)
			{
				GVec2 worldLineTop{ x, worldTop };
				GVec2 worldLineButtom{ x, worldBottom };

				GVec2 viewLineTop = WorldToScreen(worldLineTop);
				GVec2 viewLineBottom = WorldToScreen(worldLineButtom);

				MoveToEx(dc, (int)viewLineTop.x, (int)viewLineTop.y, NULL);
				LineTo(dc, (int)viewLineBottom.x, (int)viewLineBottom.y);
			}

			for (double y = bl.y; y < tr.y; y += worldGradationSpan)
			{
				GVec2 worldLineLeft{ worldLeft, y };
				GVec2 worldLineRight{ worldRight, y };

				GVec2 viewLineLeft = WorldToScreen(worldLineLeft);
				GVec2 viewLineRight = WorldToScreen(worldLineRight);

				MoveToEx(dc, (int)viewLineLeft.x, (int)viewLineLeft.y, NULL);
				LineTo(dc, (int)viewLineRight.x, (int)viewLineRight.y);
			}
		}

		void OnPaint(HDC dc, const RECT& subRect)
		{
			UNUSED(subRect);
			
			Pen pen1(PS_SOLID, 1, RGB(16, 16, 32));
			Pen pen2(PS_SOLID, 1, RGB(32, 32, 64));
			Pen pen3(PS_SOLID, 1, RGB(64, 64, 128));

			PaintGradation(dc, pen1, mapSmallestGradation / 10.0);
			PaintGradation(dc, pen2, mapSmallestGradation);
			PaintGradation(dc, pen3, mapSmallestGradation * 10.0);
		}

		const GVec2& GetCurrentlyVisibleOrigin() const
		{
			return previewOrigin.x < 1.e30 ? previewOrigin : viewOrigin;
		}

		GVec2 WorldToScreen(const GVec2& p) const
		{
			return GVec2
			{
				 p.x - GetCurrentlyVisibleOrigin().x * viewScaleFactor + 0.5 * viewSpan.x,
				 -(p.y - GetCurrentlyVisibleOrigin().y) * viewScaleFactor + 0.5 * viewSpan.y,
			};
		}

		GVec2 ScreenToWorld(const GVec2& pixelPos) const
		{
			return GVec2
			{
				 (pixelPos.x - 0.5 * viewSpan.x) / viewScaleFactor + GetCurrentlyVisibleOrigin().x,
				 -1.0  * (pixelPos.y - 0.5 * viewSpan.y) / viewScaleFactor + GetCurrentlyVisibleOrigin().y,
			};
		}

		Vec2i dragStart = { -1,-1 };

		GVec2 previewOrigin{ 1e40,1e40 };

		void BeginDrag(Vec2i referencePixelPosition) override
		{
			dragStart = referencePixelPosition;
		}

		void EndDrag(Vec2i referencePixelPosition) override
		{
			if (dragStart.x != -1)
			{
				Vec2i delta = dragStart - referencePixelPosition;
				dragStart = { -1,-1 };

				GVec2 worldCentreInPixels = WorldToScreen(viewOrigin);
				GVec2 newWorldCentreInPixels = { worldCentreInPixels.x + (double)delta.x,  worldCentreInPixels.y + (double)delta.y };
				GVec2 newWorldCentreInWorldUnits = ScreenToWorld(newWorldCentreInPixels);
				viewOrigin = newWorldCentreInWorldUnits;
				previewOrigin = { 1e40,1e40 };
				InvalidateRect(*window, NULL, TRUE);
			}
		}

		void PreviewDrag(Vec2i referencePixelPosition)
		{
			if (dragStart.x != -1)
			{
				Vec2i delta = dragStart - referencePixelPosition;
				GVec2 worldCentreInPixels = WorldToScreen(viewOrigin);
				GVec2 newWorldCentreInPixels = { worldCentreInPixels.x + (double)delta.x,  worldCentreInPixels.y + (double)delta.y };
				GVec2 newWorldCentreInWorldUnits = ScreenToWorld(newWorldCentreInPixels);
				previewOrigin = newWorldCentreInWorldUnits;
				InvalidateRect(*window, NULL, TRUE);
			}
		}

		void CaptureCursorInput() override
		{
			::SetCapture(*window);
		}

		void ReleaseCapture() override
		{
			::SetCapture(nullptr);
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
			case WM_LBUTTONDOWN:
				{
					uint16 fwKeys = GET_KEYSTATE_WPARAM(wParam);
					int32 xPos = GET_X_LPARAM(lParam);
					int32 yPos = GET_Y_LPARAM(lParam);
					eventHandler.GridEvent_OnLeftButtonDown(fwKeys, Vec2i{ xPos, yPos });
				}
				return 0L;
			case WM_LBUTTONUP:
				{
					uint16 fwKeys = GET_KEYSTATE_WPARAM(wParam);
					int32 xPos = GET_X_LPARAM(lParam);
					int32 yPos = GET_Y_LPARAM(lParam);
					eventHandler.GridEvent_OnLeftButtonUp(fwKeys, Vec2i{ xPos, yPos });
				}
				return 0L;
			case WM_MOUSEWHEEL:
				{
					uint16 fwKeys = GET_KEYSTATE_WPARAM(wParam);
					int16 wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
					wheelDeltaSum += wheelDelta;

					int32 clicks = 0;

					while (wheelDeltaSum >= 120)
					{
						clicks++;
						wheelDeltaSum -= 120;
					}

					while (wheelDeltaSum <= -120)
					{
						clicks--;
						wheelDeltaSum += 120;
					}

					if (clicks != 0)
					{
						int32 xPos = GET_X_LPARAM(lParam);
						int32 yPos = GET_Y_LPARAM(lParam);

						eventHandler.GridEvent_OnControlWheelRotated(clicks, fwKeys, Vec2i { xPos, yPos });
					}
				}
				return 0L;
			case WM_MOUSEMOVE:
					{
						uint16 fwKeys = GET_KEYSTATE_WPARAM(wParam);
						int32 xPos = GET_X_LPARAM(lParam);
						int32 yPos = GET_Y_LPARAM(lParam);
						eventHandler.GridEvent_OnCursorMove(fwKeys, Vec2i{ xPos, yPos });
					}
					return 0L;
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(*window, &ps);
				//int oldMode = SetGraphicsMode(dc, GM_ADVANCED);				
				OnPaint(dc, ps.rcPaint);
				//SetGraphicsMode(dc, oldMode);
				EndPaint(*window, &ps);
			}
			return 0;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		LRESULT OnSize(HWND, WPARAM wParam, LPARAM lParam)
		{
			UNUSED(wParam);

			DWORD width = LOWORD(lParam);
			DWORD height = HIWORD(lParam);

			viewSpan.x = (double)width;
			viewSpan.y = (double)height;

			return 0L;
		}

		double ScaleFactor() const override
		{
			return viewScaleFactor;
		}

		void SetScaleFactor(double newValue) override
		{
			viewScaleFactor = newValue;
			InvalidateRect(*window, NULL, TRUE);
		}

		void SetHorizontalDomain(double left, double right) override
		{
			this->worldLeft = left;
			this->worldRight = right;
		}

		void SetVerticalDomain(double top, double bottom) override
		{
			this->worldTop = top;
			this->worldBottom = bottom;
		}

		void SetCentrePosition(double x, double y) override
		{
			viewOrigin.x = x;
			viewOrigin.y = y;
		}

		void SetSmallestGradation(double gradationDelta) override
		{
			mapSmallestGradation = gradationDelta;
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
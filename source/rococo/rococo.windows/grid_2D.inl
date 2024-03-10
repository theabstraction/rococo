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

	class Grid_2D : public IUI2DGridSlateSupervisor, public IDesignSpace, private IWindowHandler
	{
	private:
		AutoFree<IParentWindowSupervisor> window;
		IUI2DGridEvents& eventHandler;

		int16 wheelDeltaSum = 0;

		double viewScaleFactor = 1.0; // Number of pixels per unit distance in the world
		double worldLeft = -4096.0;
		double worldRight = 4096.0;
		double worldTop = -4096.0;
		double worldBottom = 4096.0;
		DesignerVec2 viewOrigin{ 0, 0 }; // World co-ordinates of the screen centre
		double mapSmallestGradation = 100.0; // Width of grid lines in world units
		double minPixelSpan = 8.0; // Smallest visible gradation span
		Vec2i viewSpan = { 1, 1 }; // Span of the window in pixels

		Grid_2D(IUI2DGridEvents& _handler, bool isDoubleBuffered) :
			eventHandler(_handler), isBuffered(isDoubleBuffered)
		{
		}

		~Grid_2D()
		{
			if (memDC)
			{
				DeleteDC(memDC);
			}

			if (hMemBitmap)
			{
				DeleteBitmap(hMemBitmap);
			}

			if (indexDC)
			{
				DeleteDC(indexDC);
			}

			if (hOldIndexBitmap)
			{
				DeleteBitmap(hOldIndexBitmap);
			}

			if (hIndexBitmap)
			{
				DeleteBitmap(hIndexBitmap);
			}
		}

		bool TryGetIndicesAt(Vec2i position, OUT RGBAb& indices) const override
		{
			if (indexDC)
			{
				COLORREF colour = GetPixel(indexDC, position.x, position.y);
				indices.red = GetRValue(colour);
				indices.green = GetGValue(colour);
				indices.blue = GetBValue(colour);
				indices.alpha = 255;
				return true;
			}

			return false;
		}

		IDesignSpace& DesignSpace() override
		{
			return *this;
		}

		void OnEraseBackground(HDC dc)
		{
			HBRUSH hBrush = GetStockBrush(BLACK_BRUSH);

			RECT rect;
			GetClientRect(*window, &rect);
			FillRect(dc, &rect, hBrush);
		}

		DesignerVec2 Quantize(const DesignerVec2& worldSpace, double delta)
		{
			double integralComponentX = 0;
			double remainderX = std::modf(worldSpace.x / delta, &integralComponentX);
			UNUSED(remainderX);

			double integralComponentY = 0;
			double remainderY = std::modf(worldSpace.y / delta, &integralComponentY);
			UNUSED(remainderY);

			return DesignerVec2{ integralComponentX * delta, integralComponentY * delta };
		}

		void PaintGradation(HDC dc, Pen& pen, double worldGradationSpan)
		{
			double pixelSpan = worldGradationSpan * viewScaleFactor;
			if (pixelSpan < minPixelSpan)
			{
				return;
			}

			PenContext usePen(pen, dc);

			Vec2i topLeft{ 0, 0 };
			DesignerVec2 screenInWorldSpaceTopLeft = ScreenToWorld(topLeft);

			Vec2i bottomRight{ viewSpan.x, viewSpan.y };
			DesignerVec2 screenInWorldSpaceBottomRight = ScreenToWorld(bottomRight);

			DesignerVec2 tl = Quantize(screenInWorldSpaceTopLeft, worldGradationSpan);
			DesignerVec2 br = Quantize(screenInWorldSpaceBottomRight, worldGradationSpan);
			tl.x -= worldGradationSpan;
			tl.y -= worldGradationSpan;
			br.x += worldGradationSpan;
			br.y += worldGradationSpan;

			for (double x = tl.x; x < br.x; x += worldGradationSpan)
			{
				DesignerVec2 worldLineTop{ x, worldTop };
				DesignerVec2 worldLineBottom{ x, worldBottom };

				Vec2i viewLineTop = WorldToScreen(worldLineTop);
				Vec2i viewLineBottom = WorldToScreen(worldLineBottom);

				MoveToEx(dc, viewLineTop.x, viewLineTop.y, NULL);
				LineTo(dc, viewLineBottom.x, viewLineBottom.y);
			}

			for (double y = tl.y; y < br.y; y += worldGradationSpan)
			{
				DesignerVec2 worldLineLeft{ worldLeft, y };
				DesignerVec2 worldLineRight{ worldRight, y };

				Vec2i viewLineLeft = WorldToScreen(worldLineLeft);
				Vec2i viewLineRight = WorldToScreen(worldLineRight);

				MoveToEx(dc, viewLineLeft.x, viewLineLeft.y, NULL);
				LineTo(dc, viewLineRight.x, viewLineRight.y);
			}
		}

		HDC memDC = NULL;
		HBITMAP hMemBitmap = NULL;
		Vec2i memSpan{ 0,0 };
		HDC indexDC = NULL;
		HBITMAP hIndexBitmap = NULL;
		HBITMAP hOldIndexBitmap = NULL;

		void OnPaintBuffered(HDC dc, const RECT& subRect)
		{
			RECT windowRect;
			GetClientRect(*window, &windowRect);

			if (windowRect.right != memSpan.x || windowRect.bottom != memSpan.y || memDC == nullptr)
			{
				if (indexDC)
				{
					DeleteDC(indexDC);
					indexDC = nullptr;
				}

				if (hIndexBitmap)
				{
					DeleteBitmap(hIndexBitmap);
					hIndexBitmap = nullptr;
				}

				if (hOldIndexBitmap)
				{
					DeleteBitmap(hOldIndexBitmap);
					hOldIndexBitmap = nullptr;
				}

				if (memDC)
				{					
					DeleteDC(memDC);					
					memDC = nullptr;
				}

				if (hMemBitmap)
				{
					DeleteBitmap(hMemBitmap);
					hMemBitmap = nullptr;
				}

				memDC = CreateCompatibleDC(dc);
				if (!memDC)
				{
					OnPaintRGB(dc, subRect);
					return;
				}

				memSpan.x = windowRect.right;
				memSpan.y = windowRect.bottom;

				hMemBitmap = CreateCompatibleBitmap(dc, memSpan.x, memSpan.y);
				if (!hMemBitmap)
				{
					DeleteDC(memDC);
					memDC = nullptr;
					OnPaintRGB(dc, subRect);
					return;
				}

				indexDC = CreateCompatibleDC(dc);
				if (indexDC)
				{
					hIndexBitmap = CreateBitmap(memSpan.x, memSpan.y, 4, 8, NULL);
					hOldIndexBitmap = (HBITMAP) SelectObject(indexDC, hIndexBitmap);
				}
			}

			auto oldBitmap = SelectObject(memDC, hMemBitmap);

			BITMAPINFO info = { 0 };
			info.bmiHeader.biSize = sizeof info;

			int result = GetDIBits(memDC, hMemBitmap, 0, 0, NULL, &info, DIB_RGB_COLORS);
			UNUSED(result);

			HBRUSH hBackBrush = CreateSolidBrush(RGB(0, 0, 0));
			FillRect(memDC, &windowRect, hBackBrush);
			FillRect(indexDC, &windowRect, hBackBrush);
			DeleteObject(hBackBrush);
				
			OnPaintRGB(memDC, subRect);
			OnPaintIndices(indexDC, subRect);

			BitBlt(dc, 0, 0, memSpan.x, memSpan.y, memDC, 0, 0, SRCCOPY);

			SelectObject(memDC, oldBitmap);
		}

		void OnPaintRGB(HDC dc, const RECT& subRect)
		{
			UNUSED(subRect);

			Pen pen1(PS_SOLID, 1, RGB(16, 16, 32));
			Pen pen2(PS_SOLID, 1, RGB(32, 32, 64));
			Pen pen3(PS_SOLID, 1, RGB(64, 64, 128));

			PaintGradation(dc, pen1, mapSmallestGradation / 10.0);
			PaintGradation(dc, pen2, mapSmallestGradation);
			PaintGradation(dc, pen3, mapSmallestGradation * 10.0);

			DC_Renderer renderer(dc, *window);
			eventHandler.GridEvent_PaintForeground(renderer);
		}

		void OnPaintIndices(HDC dc, const RECT& subRect)
		{
			UNUSED(subRect);
			DC_Renderer renderer(dc, *window);
			eventHandler.GridEvent_PaintForegroundIndices(renderer);
		}

		const DesignerVec2& GetCurrentlyVisibleOrigin() const
		{
			return previewOrigin.x < 1.e30 ? previewOrigin : viewOrigin;
		}

		Vec2i WorldToScreen(const DesignerVec2& p) const override
		{
			DesignerVec2 q =
			{
				 (p.x - GetCurrentlyVisibleOrigin().x) * viewScaleFactor + 0.5 * viewSpan.x,
				 (p.y - GetCurrentlyVisibleOrigin().y) * viewScaleFactor + 0.5 * viewSpan.y,
			};

			return Vec2i{ (int32)q.x, (int32)q.y };
		}

		DesignerVec2 ScreenToWorld(Vec2i pixelPos) const override
		{
			return DesignerVec2
			{
				 (pixelPos.x - 0.5 * viewSpan.x) / viewScaleFactor + GetCurrentlyVisibleOrigin().x,
				 (pixelPos.y - 0.5 * viewSpan.y) / viewScaleFactor + GetCurrentlyVisibleOrigin().y,
			};
		}

		DesignerVec2 ScreenDeltaToWorldDelta(Vec2i pixelDelta) const override
		{
			return DesignerVec2
			{
				 pixelDelta.x / viewScaleFactor,
				 pixelDelta.y / viewScaleFactor,
			};
		}

		Vec2i dragStart = { -1,-1 };

		DesignerVec2 previewOrigin{ 1e40,1e40 };

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

				Vec2i worldCentreInPixels = WorldToScreen(viewOrigin);
				Vec2i newWorldCentreInPixels = { worldCentreInPixels.x + delta.x,  worldCentreInPixels.y + delta.y };
				DesignerVec2 newWorldCentreInWorldUnits = ScreenToWorld(newWorldCentreInPixels);
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
				Vec2i worldCentreInPixels = WorldToScreen(viewOrigin);
				Vec2i newWorldCentreInPixels = { worldCentreInPixels.x + delta.x,  worldCentreInPixels.y + delta.y };
				DesignerVec2 newWorldCentreInWorldUnits = ScreenToWorld(newWorldCentreInPixels);
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

		Vec2i GetDesktopPositionFromGridPosition(Vec2i gridPosition) override
		{
			POINT p = { gridPosition.x, gridPosition.y };
			return ClientToScreen(*window, &p) ? Vec2i{ p.x, p.y } : Vec2i{ -1,-1 };
		}

		const bool isBuffered;

		LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
		{
			switch (uMsg)
			{
			case WM_SIZE:
				return OnSize(hWnd, wParam, lParam);
			case WM_ERASEBKGND:
				if (!isBuffered) OnEraseBackground((HDC)wParam);
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
			case WM_RBUTTONUP:
			{
				uint16 fwKeys = GET_KEYSTATE_WPARAM(wParam);
				int32 xPos = GET_X_LPARAM(lParam);
				int32 yPos = GET_Y_LPARAM(lParam);
				eventHandler.GridEvent_OnRightButtonUp(fwKeys, Vec2i{ xPos, yPos });
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

				if (isBuffered)
				{
					OnPaintBuffered(dc, ps.rcPaint);
				}
				else
				{
					OnPaintRGB(dc, ps.rcPaint);
				}

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

			viewSpan.x = width;
			viewSpan.y = height;

			InvalidateRect(*window, NULL, TRUE);

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

		void SetCentrePosition(const DesignerVec2&  pos) override
		{
			viewOrigin = pos;
		}

		void SetSmallestGradation(double gradationDelta) override
		{
			mapSmallestGradation = gradationDelta;
		}

		void OnPretranslateMessage(MSG&) override
		{

		}

		void QueueRedraw() override
		{
			InvalidateRect(*window, NULL, TRUE);
		}

		void ResizeToParent() override
		{
			RECT rect;
			GetClientRect(GetParent(*window), &rect);
			MoveWindow(*window, 0, 0, rect.right, rect.bottom, TRUE);
		}
	public:
		static Grid_2D* Create(const WindowConfig& config, IUI2DGridEvents& eventHandler, bool useDoubleBuffering)
		{
			Grid_2D* p = new Grid_2D(eventHandler, useDoubleBuffering);
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

namespace Rococo::Editors
{
	ROCOCO_WINDOWS_API GuiRect WorldToScreen(const DesignerRect& designerRect, IDesignSpace& designSpace)
	{
		Vec2i topLeft = designSpace.WorldToScreen({ designerRect.left, designerRect.top });
		Vec2i bottomRight = designSpace.WorldToScreen({ designerRect.right, designerRect.bottom });
		return GuiRect{ topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
	}

	ROCOCO_WINDOWS_API DesignerRect ScreenToWorld(const GuiRect& screenRect, IDesignSpace& designSpace)
	{
		DesignerVec2 topLeft = designSpace.ScreenToWorld({ screenRect.left, screenRect.top });
		DesignerVec2 bottomRight = designSpace.ScreenToWorld({ screenRect.right, screenRect.bottom });
		return DesignerRect{ topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
	}

	ROCOCO_WINDOWS_API bool DesignerRect::Contains(const DesignerVec2& pt)
	{
		return pt.x > left && pt.x <= right && pt.y > top && pt.y < bottom;
	}
}
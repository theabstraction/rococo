#include "dx11.renderer.h"

using namespace Rococo;

namespace Rococo::DX11
{
	void ShowWindowVenue(HWND hWnd, IMathsVisitor& visitor)
	{
		POINT pt;
		GetCursorPos(&pt);

		visitor.ShowString("Abs Mouse Cursor:", "(%d %d)", pt.x, pt.y);

		ScreenToClient(hWnd, &pt);

		visitor.ShowString("Rel Mouse Cursor:", "(%d %d)", pt.x, pt.y);

		visitor.ShowPointer("HANDLE", hWnd);
		RECT rect;
		GetClientRect(hWnd, &rect);
		visitor.ShowString("-> Client Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

		GetWindowRect(hWnd, &rect);
		visitor.ShowString("-> Window Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

		LONG x = GetWindowLongA(hWnd, GWL_STYLE);
		visitor.ShowString("-> GWL_STYLE", "0x%8.8X", x);

		x = GetWindowLongA(hWnd, GWL_EXSTYLE);
		visitor.ShowString("-> GWL_EXSTYLE", "0x%8.8X", x);

		HWND hParent = GetParent(hWnd);
		if (hParent) visitor.ShowPointer("-> Parent", hParent);
		else			visitor.ShowString("-> Parent", "None (top-level window)");
	}
}
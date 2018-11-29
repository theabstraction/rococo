#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "rococo.cute.h"

#include "cute.sxh.h"

using namespace Rococo;
using namespace Rococo::Cute;

struct CuteChildProxy : IChildSupervisor, IWindowBase
{
	HWND hChildWnd;

	CuteChildProxy(HWND _hChildWnd) : hChildWnd(_hChildWnd)
	{

	}

	void Free() override
	{
		delete this;
	}

	size_t GetWindowHandle() override
	{
		return (size_t)hChildWnd;
	}

	IWindowBase* Window() override
	{
		return this;
	}
};


struct CuteChild : IChildSupervisor, IWindowBase
{
	HWND hParentWnd;
	HWND hChildWnd;

	CuteChild(HWND _hParentWnd, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy) : hParentWnd(_hParentWnd)
	{
		auto hInstance = (HINSTANCE)GetWindowLongPtrA(hParentWnd, GWLP_HINSTANCE);

		WINDOWINFO info;
		info.cbSize = sizeof(info);
		if (!GetWindowInfo(hParentWnd, &info))
		{
			Throw(GetLastError(), "CuteChild::GetWindowInfo(...) failed");
		}

		cstr wc = (cstr) info.atomWindowType;
		hChildWnd = CreateWindowExA(exStyle, wc, "", style, x, y, dx, dy, hParentWnd, NULL, hInstance, NULL);
		if (hChildWnd == NULL)
		{
			Throw(GetLastError(), "CuteChild::CreateWindowExA(...) failed");
		}
	}

	void Free() override
	{
		delete this;
	}

	size_t GetWindowHandle() override
	{
		return (size_t)hChildWnd;
	}

	IWindowBase* Window() override
	{
		return this;
	}
};

namespace Rococo
{
	namespace Cute
	{
		IChildSupervisor* CreateChildProxy(HWND hWnd)
		{
			return new CuteChildProxy(hWnd);
		}

		IChildSupervisor* CreateChild(HWND hParentWnd, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy)
		{
			return new CuteChild(hParentWnd, style, exStyle, x, y, dx, dy);
		}

		IChildSupervisor* CreateChild(IWindowBase& window, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy)
		{
			auto hParentWnd = (HWND)window.GetWindowHandle();
			return new CuteChild(hParentWnd, style, exStyle, x, y, dx, dy);
		}
	}
}
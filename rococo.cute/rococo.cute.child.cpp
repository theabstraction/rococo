#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>

using namespace Rococo;
using namespace Rococo::Cute;

struct CuteChildProxy : IChildSupervisor
{
	HWND hChildWnd;

	CuteChildProxy(HWND _hChildWnd) : hChildWnd(_hChildWnd)
	{
		SetMasterProc(ToRef(hChildWnd), this);
	}

	void Close() override
	{

	}

	void Free() override
	{
		delete this;
	}

	void OnResize(Vec2i span, ResizeType type) override
	{

	}

	WindowRef Handle() override
	{
		return ToRef(hChildWnd);
	}
};


struct CuteChild : IChildSupervisor
{
	HWND hParentWnd;
	HWND hChildWnd;

	CuteChild(HWND _hParentWnd, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy) : hParentWnd(_hParentWnd)
	{
		auto hInstance = (HINSTANCE)GetWindowLongPtrA(hParentWnd, GWLP_HINSTANCE);

		char name[256];
		if (0 >= GetClassNameA(_hParentWnd, name, sizeof(name)))
		{
			Throw(GetLastError(), "CuteChild::GetClassNameA(_hParentWnd) failed");
		}

		hChildWnd = CreateWindowExA(exStyle, name, "", style, x, y, dx, dy, hParentWnd, NULL, hInstance, NULL);
		if (hChildWnd == NULL)
		{
			Throw(GetLastError(), "CuteChild::CreateWindowExA(...) failed");
		}

		SetMasterProc(ToRef(hChildWnd), this);
	}

	void Close() override
	{

	}

	void Free() override
	{
		delete this;
	}

	WindowRef Handle() override
	{
		return ToRef(hChildWnd);
	}

	void OnResize(Vec2i span, ResizeType type) override
	{

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
			auto hParentWnd = ToHWND(window.Handle());
			return new CuteChild(hParentWnd, style, exStyle, x, y, dx, dy);
		}
	}
}
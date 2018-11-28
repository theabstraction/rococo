#include "rococo.cute.h"
#include "cute.sxh.h"

#include <rococo.os.win32.h>
#include <rococo.window.h>

#include <vector>

#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Cute;

struct BaseWindow : public IWindowBase
{
	HWND hWnd;

	void GetWindowRect(Vec2i& pos, Vec2i& span) override
	{
		RECT rect;
		::GetWindowRect(hWnd, &rect);
		pos = { rect.left, rect.top };
		span = { rect.right - rect.left, rect.bottom - rect.top };
	}

	void GetSpan(Vec2i& span) override
	{
		RECT rect;
		::GetClientRect(hWnd, &rect);
		span = { rect.right - rect.left, rect.bottom - rect.top };
	}

	void SetText(const fstring& text) override
	{
		::SetWindowTextA(hWnd, text);
	}

	int32 /* stringLength */ GetText(IStringPopulator& sb)override
	{
		int32 len = GetWindowTextLengthA(hWnd);

		char* text = (char*)alloca(len + 1);
		GetWindowTextA(hWnd, text, len);
		sb.Populate(text);
		return len;
	}
};

struct MasterWindow : public IMasterWindow
{
	BaseWindow baseWindow;

	MasterWindow(ATOM atom, HINSTANCE hInstance, HWND hParent, Vec2i pos, Vec2i span, cstr title)
	{
		DWORD exStyle = 0;
		DWORD style = WS_OVERLAPPEDWINDOW;
		int x = pos.x == -1 ? CW_USEDEFAULT : pos.x;
		int y = pos.y == -1 ? CW_USEDEFAULT : pos.y;
		int width = 640;
		int height = 480;
		baseWindow.hWnd = CreateWindowExA(exStyle, (cstr)atom, title, style, x, y, width, height,
			hParent, nullptr, hInstance, nullptr);
	}

	virtual ~MasterWindow()
	{
		DestroyWindow(baseWindow.hWnd);
		baseWindow.hWnd = nullptr;
	}

	void GetWindowRect(Vec2i& pos, Vec2i& span) override
	{
		baseWindow.GetWindowRect(pos, span);
	}

	void GetSpan(Vec2i& span) override
	{
		baseWindow.GetSpan(span);
	}

	void SetText(const fstring& text) override
	{
		baseWindow.SetText(text);
	}

	LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			PostQuitMessage(-1);
			return 0;
		default:
			break;
		}
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	}

	static LRESULT MasterWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto* This = (MasterWindow*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		return This->OnMessage(hWnd, uMsg, wParam, lParam);
	}

	void Show()
	{
		SetWindowLongPtrA(baseWindow.hWnd, GWLP_USERDATA, (LONG_PTR) this);
		SetWindowLongPtrA(baseWindow.hWnd, GWLP_WNDPROC, (LONG_PTR) MasterWindowProc);
		ShowWindow(baseWindow.hWnd, SW_SHOW);
	}

	int32 /* stringLength */ GetText(IStringPopulator& sb)override
	{
		return baseWindow.GetText(sb);
	}
};

struct MasterWindowFactory : public IMasterWindowFactory
{
	ATOM atom;
	std::vector<MasterWindow*> masters;
	size_t validWindows = 0;
	HINSTANCE hInstance;
	HWND hParent;

	MasterWindowFactory(HINSTANCE _hInstance, HWND _hParent): hInstance(_hInstance), hParent(_hParent)
	{
		char classname[48];
		static int index = 1;
		SafeFormat(classname, sizeof(classname), "Rococo::Cute::IMasterWindowFactory_%d", index++);
		WNDCLASSEXA wc = { 0 };
		wc.cbSize = sizeof(wc);
		wc.hInstance = hInstance;
		wc.lpfnWndProc = DefWindowProcA;
		wc.lpszClassName = classname;
		atom = RegisterClassExA(&wc);
		if (atom == 0)
		{
			Throw(GetLastError(), "MasterWindowFactory construction failed. RegisterClassExA returned 0");
		}
	}

	virtual ~MasterWindowFactory()
	{
		for (auto i : masters)
		{
			delete i;
		}

		UnregisterClassA((LPCSTR)atom, hInstance);
	}

	void Free() override
	{
		delete this;
	}

	IMasterWindow* CreateMaster(cstr title, const Vec2i& pos, const Vec2i& span) override
	{
		auto* master = new MasterWindow(atom, hInstance, hParent, pos, span, title);
		masters.push_back(master);
		return master;
	}

	void Commit() override
	{
		for (size_t i = validWindows; i < masters.size(); ++i)
		{
			masters[i]->Show();
		}

		validWindows = masters.size();
	}

	void Revert() override
	{
		for (size_t i = validWindows; i < masters.size(); ++i)
		{
			delete masters[i];
		}

		masters.resize(validWindows);
	}

	bool HasInstances() const
	{
		return !masters.empty();
	}
};

namespace Rococo
{
	namespace Cute
	{
		IMasterWindowFactory* CreateMasterWindowFactory(HINSTANCE hInstance, HWND hParent)
		{
			return new MasterWindowFactory(hInstance, hParent);
		}
	}
}
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>
#include <rococo.strings.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Cute;

static auto factoryPrefix = "Rococo::Cute::IMasterWindowFactory"_fstring;

namespace Rococo
{
	namespace Cute 
	{
		namespace Native
		{
			void GetWindowRect(WindowRef ref, Vec2i& pos, Vec2i& span)
			{
				HWND hWnd = ToHWND(ref);
				RECT rect;
				::GetWindowRect(hWnd, &rect);
				pos = { rect.left, rect.top };
				span = { rect.right - rect.left, rect.bottom - rect.top };
			}

			void GetSpan(WindowRef ref, Vec2i& span)
			{
				HWND hWnd = ToHWND(ref);
				RECT rect;
				::GetClientRect(hWnd, &rect);
				span = { rect.right - rect.left, rect.bottom - rect.top };
			}

			void SetText(WindowRef ref, const fstring& text)
			{
				HWND hWnd = ToHWND(ref);
				::SetWindowTextA(hWnd, text);
			}

			int32 /* stringLength */ GetText(WindowRef ref, IStringPopulator& sb)
			{
				HWND hWnd = ToHWND(ref);
				int32 len = GetWindowTextLengthA(hWnd);

				char* text = (char*)alloca(len + 1);
				GetWindowTextA(hWnd, text, len);
				sb.Populate(text);
				return len;
			}

			void SetColourTarget(WindowRef ref, ColourTarget target, RGBAb colour)
			{
				auto hWnd = ToHWND(ref);
				if (IsWindow(hWnd))
				{
					HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrA(hWnd, GWLP_HINSTANCE);

					char className[256];
					int result = GetClassNameA(hWnd, className, sizeof(className));
					if (result && Compare(className, factoryPrefix, factoryPrefix.length) == 0)
					{
						LONG_PTR val = GetWindowLongPtrA(hWnd, DWLP_USER);
						auto& data = *(CuteWindowExData*)(&val);

						switch (target)
						{
						case ColourTarget_NormalBackground:
							data.normalBackgroundColour = colour;
							break;
						case ColourTarget_HilightBackground:
							data.hilighBackgroundColour = colour;
							break;
						}

						SetWindowLongPtrA(hWnd, DWLP_USER, val);
					}
					else if (IsDebuggerPresent())
					{
						Rococo::OS::TripDebugger();
					}
				}
				else if (IsDebuggerPresent())
				{
					Rococo::OS::TripDebugger();
				}
			}
		} // Native
	} // Cute
} // Rococo

static void EraseBackground(HWND hWnd)
{
	LONG_PTR ldata = GetWindowLongPtrA(hWnd, DWLP_USER);
	auto& data = *(CuteWindowExData*)&ldata;

	RECT wrect;
	GetWindowRect(hWnd, &wrect);

	char title[256];
	GetWindowTextA(hWnd, title, 256);
	char msg[256];
	SafeFormat(msg, 256, "%s: erase {%d,%d} to {%d,%d}\n", title, wrect.left, wrect.top, wrect.right, wrect.bottom);
	OutputDebugStringA(msg);

	if (data.normalBackgroundColour.alpha == 0) return;

	HDC dc = GetDC(hWnd);
	RECT rect;
	GetClientRect(hWnd, &rect);

	POINT pt;
	GetCursorPos(&pt);

	ScreenToClient(hWnd, &pt);

	auto& c = data.normalBackgroundColour;
	HBRUSH hBrush = CreateSolidBrush(RGB(c.red, c.green, c.blue));
	HGDIOBJ old = SelectObject(dc, hBrush);
	FillRect(GetDC(hWnd), &rect, hBrush);
	SelectObject(dc, old);
	DeleteObject(hBrush);
	ReleaseDC(hWnd, dc);
}

static void OnSize(HWND hWnd, WPARAM wParam, LPARAM lParam, IWindowSupervisor& window)
{
	ResizeType type;

	switch (wParam)
	{
	case SIZE_MAXIMIZED:
		type = ResizeTo_Full;
		break;
	case SIZE_MINIMIZED:
		type = ResizeTo_Minimize;
		break;
	case SIZE_RESTORED:
		type = ResizeTo_Normal;
		break;
	default:
		return;
	}

	WORD width = LOWORD(lParam);
	WORD height = HIWORD(lParam);

	window.OnResize({ width,height }, type);
}

static LRESULT MasterWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto* WS = (IWindowSupervisor*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
	if (WS)
	{
		switch (uMsg)
		{
		case WM_SIZE:
			OnSize(hWnd, wParam, lParam, *WS);
			return 0L;
		case WM_CLOSE:
			WS->Close();
			return 0L;
		}
	}

	switch (uMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		EraseBackground(hWnd);
		return 0L;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

namespace Rococo
{
	namespace Cute
	{
		void SetMasterProc(WindowRef ref, IWindowSupervisor* window)
		{
			auto hWnd = ToHWND(ref);
			if (IsWindow(hWnd))
			{
				char className[256];
				int result = GetClassNameA(hWnd, className, sizeof(className));
				if (result && Compare(className, factoryPrefix, factoryPrefix.length) == 0)
				{
					SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)window);
					SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR) MasterWindowProc);
				}
			}
		}
	}
}

struct MasterWindow : IMasterWindow
{
	AutoFree<IMenuSupervisor> menuManager;
	std::vector<IWindowSupervisor*> children;
	HWND hWindow;
	ATOM atom;
	Vec2i minSize = { -1,-1 };
	Vec2i maxSize = { -1,-1 };

	MasterWindow(ATOM _atom, HINSTANCE hInstance, HWND hParent, Vec2i pos, Vec2i span, cstr title) : atom(_atom)
	{
		DWORD exStyle = 0;
		DWORD style = WS_OVERLAPPEDWINDOW;
		int x = pos.x == -1 ? CW_USEDEFAULT : pos.x;
		int y = pos.y == -1 ? CW_USEDEFAULT : pos.y;
		int width = span.x;
		int height = span.y;
		hWindow = CreateWindowExA(exStyle, (cstr)atom, title, style, x, y, width, height,
			hParent, nullptr, hInstance, nullptr);

		menuManager = Rococo::Cute::CreateCuteMenu(hWindow);

		CuteWindowExData data;
		data.normalBackgroundColour = RGBAb(224, 224, 224);
		data.hilighBackgroundColour = RGBAb(224, 224, 224);
		SetWindowLongPtrA(hWindow, DWLP_USER, *(LONG_PTR*)&data);
	}

	MasterWindow(ATOM _atom, HINSTANCE hInstance, HWND hParent, Vec2i pos, Vec2i span, cstr title, DWORD style, DWORD exStyle) : atom(_atom)
	{
		int x = pos.x == -1 ? CW_USEDEFAULT : pos.x;
		int y = pos.y == -1 ? CW_USEDEFAULT : pos.y;
		int width = span.x;
		int height = span.y;
		hWindow = CreateWindowExA(exStyle, (cstr)atom, title, style, x, y, width, height,
			hParent, nullptr, hInstance, nullptr);

		menuManager = Rococo::Cute::CreateCuteMenu(hWindow);

		CuteWindowExData data;
		data.normalBackgroundColour = RGBAb(224, 224, 224);
		data.hilighBackgroundColour = RGBAb(224, 224, 224);
		SetWindowLongPtrA(hWindow, DWLP_USER, *(LONG_PTR*)&data);
	}

	virtual ~MasterWindow()
	{
		DestroyWindow(hWindow);
	}

	void OnMinMax(WPARAM wParam, LPARAM lParam)
	{
		auto* s = (MINMAXINFO*)(lParam);
		if (maxSize.x > 0) s->ptMaxTrackSize.x = maxSize.x;
		if (maxSize.y > 0) s->ptMaxTrackSize.y = maxSize.y;
		if (minSize.x > 0) s->ptMinTrackSize.x = minSize.x;
		if (minSize.y > 0) s->ptMinTrackSize.y = minSize.y;
	}

	static LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto* This = (MasterWindow*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
		if (This)
		{
			switch (uMsg)
			{
			case WM_SIZE:
				::OnSize(hWnd, wParam, lParam, *static_cast<IWindowSupervisor*>(This));
				return 0L;
			case WM_CLOSE:
				This->Close();
				return 0L;
			case WM_ERASEBKGND:
				EraseBackground(hWnd);
				return 0L;
			case WM_GETMINMAXINFO:
				This->OnMinMax(wParam, lParam);
				return 0L;
			}
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	void SetMasterProxyProc(WindowRef ref)
	{
		SetWindowLongPtrA(hWindow, GWLP_USERDATA, (LONG_PTR) this);
		SetWindowLongPtrA(hWindow, GWLP_WNDPROC, (LONG_PTR)OnMessage);
	}

	void Close() override
	{
		PostQuitMessage(0);
	}

	void Free() override
	{
		delete this;
	}

	void AddChild(IWindowSupervisor* child)
	{
		children.push_back(child);
	}

	ISplit* SplitIntoLeftAndRight(int32 pixelSplit, int32 splitterWidth, int32 minLeftSplit, int32 maxRightSplit, boolean32 draggable) override
	{
		auto* s = CreateSplit(atom, *this, pixelSplit, minLeftSplit, maxRightSplit,splitterWidth, draggable, true);
		AddChild(s);
		return &s->Split();
	}

	ISplit*  SplitIntoTopAndBottom(int32 pixelSplit, int32 splitterHeight, int32 minTopSplit, int32 maxBottomSplit, boolean32 draggable) override
	{
		auto* s = CreateSplit(atom, *this, pixelSplit, minTopSplit, maxBottomSplit, splitterHeight, draggable, false);
		AddChild(s);
		return &s->Split();
	}

	void Show()
	{
		SetMasterProxyProc(ToRef(hWindow));
		ShowWindow(hWindow, SW_SHOW);
	}

	WindowRef Handle() override
	{
		return ToRef(hWindow);
	}

	IMenu* Menu() override
	{
		return &menuManager->Menu();
	}

	void OnResize(Vec2i span, ResizeType type) override
	{
		for (auto i : children)
		{
			auto hWnd = ToHWND(i->Handle());
			RECT r;
			GetClientRect(hWindow, &r);
			MoveWindow(hWnd, 0, 0, r.right, r.bottom, TRUE);
		}
	}

	void SetMinimumSize(int32 dx, int32 dy) override
	{
		minSize = { dx, dy };
	}

	void SetMaximumSize(int32 dx, int32 dy) override
	{
		maxSize = { dx, dy };
	}

	ITree* AddTree()
	{
		auto* tree = CreateTree(*this);
		AddChild(tree);
		return tree;
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
		SafeFormat(classname, sizeof(classname), "%s_%d", (cstr) factoryPrefix, index++);
		WNDCLASSEXA wc = { 0 };
		wc.cbSize = sizeof(wc);
		wc.hInstance = hInstance;
		wc.lpfnWndProc = DefWindowProcA;
		wc.lpszClassName = classname;
		wc.hCursor = LoadCursorA(nullptr, (cstr) IDC_ARROW);
		wc.cbWndExtra = DLGWINDOWEXTRA + sizeof(CuteWindowExData);
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
			i->Free();
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

		IParentWindow* CreateParent(IWindowSupervisor& parent, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy)
		{
			char className[256];
			int result = GetClassNameA(ToHWND(parent.Handle()), className, sizeof(className));
			if (result && Compare(className, factoryPrefix, factoryPrefix.length) == 0)
			{
				auto hInstance = (HINSTANCE) GetWindowLongPtrA(ToHWND(parent.Handle()), GWLP_HINSTANCE);
				WINDOWINFO info = { 0 };
				info.cbSize = sizeof(info);
				GetWindowInfo(ToHWND(parent.Handle()), &info);

				auto* m = new MasterWindow(info.atomWindowType,
											hInstance, 
											ToHWND(parent.Handle()), 
											{ x,y },
											{ dx,dy },
											"",
											style,
											exStyle);

				SetWindowLongPtrA(m->hWindow, GWLP_WNDPROC, (LONG_PTR)MasterWindowProc);
				parent.AddChild(m);
				return m;
			}
			else
			{
				Throw(0, "CreateMaster -> owner must be of class %s", (cstr)factoryPrefix);
				return nullptr;
			}
		}
	}
}
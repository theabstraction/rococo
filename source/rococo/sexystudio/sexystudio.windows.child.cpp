#include "sexystudio.impl.h"
#include <rococo.maths.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;

namespace Rococo::SexyStudio
{
	uint64 GetFileLength(cstr filename)
	{
		WIN32_FILE_ATTRIBUTE_DATA data;
		if (!GetFileAttributesExA(filename, GetFileExInfoStandard, &data))
		{
			return 0;
		}

		ULARGE_INTEGER len;
		len.HighPart = data.nFileSizeHigh;
		len.LowPart = data.nFileSizeLow;

		return len.QuadPart;
	}
	
	void AppendAncestorsToString(IWindow& window, StringBuilder& sb)
	{
		HWND hWnd = window;
		while (hWnd !=  nullptr)
		{
			char buf[256];
			if (!GetWindowTextA(hWnd, buf, sizeof buf))
			{
				sb.AppendFormat("(HWND 0x%llX)\r\n", hWnd);
			}
			else
			{
				sb.AppendFormat("(HWND '%s')\r\n", buf);
			}

			hWnd = GetParent(hWnd);
		}
	}

	void AppendAncestorsAndRectsToString(IWindow& window, StringBuilder& sb)
	{
		HWND hWnd = window;
		while (hWnd != nullptr)
		{
			char buf[256];
			if (!GetWindowTextA(hWnd, buf, sizeof buf))
			{
				sb.AppendFormat("(HWND 0x%llX", hWnd);
			}
			else
			{
				sb.AppendFormat("(HWND '%s'", buf);
			}

			RECT rect;
			GetWindowRect(window, &rect);
			sb.AppendFormat(": %d %d %d %d)\r\n", hWnd, rect.left, rect.top, rect.right, rect.bottom);

			hWnd = GetParent(hWnd);
		}
	}

	void AppendDescendantsAndRectsToStringRecursive(HWND hWnd, StringBuilder& sb, int indent)
	{
		for (int i = 0; i < indent; ++i)
		{
			sb.AppendChar(' ');
		}

		char className[128];
		RealGetWindowClassA(hWnd, className, sizeof className);

		char buf[256];
		if (!GetWindowTextA(hWnd, buf, sizeof buf))
		{
			sb.AppendFormat("(HWND %s 0x%llX", className, hWnd);
		}
		else
		{
			sb.AppendFormat("(HWND %s '%s'", className, buf);
		}

		RECT rect;
		GetWindowRect(hWnd, &rect);
		sb.AppendFormat(": %d %d %d %d)\r\n", rect.left, rect.top, rect.right, rect.bottom);

		if (Eq("SysTreeView32", className))
		{
			return;
		}

		struct Context
		{
			static BOOL OnWindow(HWND hChildWnd, LPARAM param)
			{
				auto* context = (Context*)param;
				if (GetParent(hChildWnd) == context->hParentWnd)
				{
					AppendDescendantsAndRectsToStringRecursive(hChildWnd, context->sb, context->indent + 1);
				}

				return TRUE;
			}

			StringBuilder& sb;
			int indent;
			HWND hParentWnd;
		} context{ sb, indent, hWnd };

		EnumChildWindows(hWnd, Context::OnWindow, (LPARAM) &context);
	}

	void AppendDescendantsAndRectsToString(IWindow& root, StringBuilder& sb)
	{
		AppendDescendantsAndRectsToStringRecursive(root, sb, 0);
	}

	Font::Font(LOGFONTA logFont)
	{
		hFont = CreateFontIndirectA(&logFont);
		if (hFont == nullptr)
		{
			Throw(GetLastError(), "%s: error creating font", __ROCOCO_FUNCTION__);
		}

	}

	Font::~Font()
	{
		DeleteFont(hFont);
	}

	void PaintDoubleBuffered(HWND hWnd, IWin32Painter& paint)
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hWnd, &ps);

		RECT rect;
		GetClientRect(hWnd, &rect);

		HDC hNewDC;
		BP_PAINTPARAMS args = { 0 };
		args.cbSize = sizeof BP_PAINTPARAMS;
		args.dwFlags = BPPF_ERASE;
		HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(dc, &rect, BPBF_COMPATIBLEBITMAP, &args, &hNewDC);
		if (hBufferedPaint)
		{
			paint.OnPaint(hNewDC);
			EndBufferedPaint(hBufferedPaint, TRUE);
		}
		else
		{
			paint.OnPaint(dc);
		}
		EndPaint(hWnd, &ps);
	}

	const char* GetChildClassName()
	{
		return  "SexyStudioChild";
	}

	void PopulatChildClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
	{
		classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH) COLOR_WINDOW;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = nullptr;
		classDef.hIconSm = nullptr;
		classDef.hInstance = hInstance;
		classDef.lpszClassName = GetChildClassName();
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;
	}

	static ATOM childClassAtom = 0;

	Win32ChildWindow::Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& _handler, DWORD extraStyleFlags):
		handler(_handler)
	{
		if (childClassAtom == 0)
		{
			WNDCLASSEXA childInfo;
			PopulatChildClass(GetMainInstance(), childInfo);
			childClassAtom = RegisterClassExA(&childInfo);
			if (childClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create child window class atom");
			}
		}

		DWORD exStyle = extraStyleFlags;
		DWORD style = WS_CHILD;
		HMENU hMenu = nullptr;
	
		hWnd = CreateWindowExA(
			exStyle,
			"SexyStudioChild",
			"",
			style,
			0, // x
			0, // y
			0, // width
			0, // height
			hParent,
			hMenu,
			GetMainInstance(),
			nullptr);

		if (hWnd == nullptr)
		{
			Throw(GetLastError(), "%s: could not create child window", __ROCOCO_FUNCTION__);
		}

		struct ANON
		{
			static LRESULT WINAPI RouteMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
			{
				auto* handler = (IWindowMessageHandler*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

				try
				{
					return handler->ProcessMessage(msg, wParam, lParam);
				}
				catch (IException& ex)
				{
					Rococo::SexyStudio::SetLastMessageError(ex);
					return DefWindowProc(hWnd, msg, wParam, lParam);
				}
			}

		};

		SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)&handler);
		SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR) ANON::RouteMessage);
	}

	Win32ChildWindow::~Win32ChildWindow()
	{
		DestroyWindow(hWnd);
	}

	Win32PopupWindow::Win32PopupWindow(HWND hParent, IWin32WindowMessageLoopHandler& _handler, DWORD extraStyleFlags) :
		handler(_handler)
	{
		if (childClassAtom == 0)
		{
			WNDCLASSEXA childInfo;
			PopulatChildClass(GetMainInstance(), childInfo);
			childClassAtom = RegisterClassExA(&childInfo);
			if (childClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create child window class atom");
			}
		}

		DWORD exStyle = extraStyleFlags;
		DWORD style = WS_POPUP;
		HMENU hMenu = nullptr;

		hWnd = CreateWindowExA(
			exStyle,
			"SexyStudioChild",
			"",
			style,
			0, // x
			0, // y
			0, // width
			0, // height
			hParent,
			hMenu,
			GetMainInstance(),
			nullptr);

		if (hWnd == nullptr)
		{
			Throw(GetLastError(), "%s: could not create child window", __ROCOCO_FUNCTION__);
		}

		struct ANON
		{
			static LRESULT WINAPI RouteMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
			{
				auto* handler = (IWindowMessageHandler*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

				try
				{
					return handler->ProcessMessage(msg, wParam, lParam);
				}
				catch (IException& ex)
				{
					Rococo::SexyStudio::SetLastMessageError(ex);
					return DefWindowProc(hWnd, msg, wParam, lParam);
				}
			}

		};

		SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)&handler);
		SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)ANON::RouteMessage);
	}

	Win32PopupWindow::~Win32PopupWindow()
	{
		DestroyWindow(hWnd);
	}

	namespace Widgets
	{
		GuiRect GetScreenRect(IWindow& window)
		{
			RECT rect;
			GetWindowRect(window, &rect);
			return GuiRect{ rect.left, rect.top, rect.right, rect.bottom };
		}

		void ExpandToFillParentSpace(IWindow& window)
		{
			Vec2i span = GetParentSpan(window);
			MoveWindow(window, 0, 0, span.x, span.y, TRUE);
		}

		Vec2i GetParentSpan(IWindow& window)
		{
			RECT rect;
			GetClientRect(GetParent(window), &rect);
			return { rect.right, rect.bottom };
		}

		Vec2i GetSpan(IWindow& window)
		{
			RECT rect;
			GetClientRect(window, &rect);
			return { rect.right, rect.bottom };
		}

		GuiRect MapScreenToWindowRect(const GuiRect& rect, IWindow& window)
		{
			POINT p1 = { rect.left, rect.top };
			POINT p2 = { rect.right, rect.bottom };

			ScreenToClient(GetParent(window), &p1);
			ScreenToClient(GetParent(window), &p2);

			return GuiRect{ p1.x, p1.y, p2.x, p2.y };
		}

		void SetText(IWindow& window, const char* text)
		{
			SetWindowTextA(window, text);
		}

		void SetSpan(IWindow& window, int32 dx, int32 dy)
		{
			MoveWindow(window, 0, 0, dx, dy, TRUE);
		}

		void SetWidgetPosition(IWindow& widget, const GuiRect& rect)
		{
			MoveWindow(widget, rect.left, rect.top, Width(rect), Height(rect), TRUE);
		}

		void Maximize(IWindow& window)
		{
			ShowWindow(window, SW_MAXIMIZE);
		}

		void Minimize(IWindow& window)
		{
			ShowWindow(window, SW_MINIMIZE);
		}
	} // Widgets
} // Roococ::SexyStudio
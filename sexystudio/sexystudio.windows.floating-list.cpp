#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <shobjidl.h>
#include <shlobj_core.h>
#include <rococo.auto-release.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

#include <vector>

namespace
{
	enum { ID_VISIBILITY_TICK_TEST = 10001 };

	struct FloatingListWidget : IFloatingListWidget, IWin32WindowMessageLoopHandler
	{
		Win32PopupWindow backWindow;
		HWNDProxy hWndList;
		WNDPROC defaultFloatingListProc = nullptr;
		UINT_PTR timerId = 0;
		HWND hEditor = nullptr;
		EventIdRef evDoubleClick = { 0 };
		WidgetContext wc;

		static LRESULT CALLBACK ProcessListMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto& This = *(FloatingListWidget*)GetWindowLongPtr(wnd, GWLP_USERDATA);
			return CallWindowProc(This.defaultFloatingListProc, wnd, msg, wParam, lParam);
		}

		FloatingListWidget(IWindow& window, WidgetContext& _wc) : 
			wc(_wc), backWindow(GetWindowOwner(window), *this, WS_EX_TOPMOST)
		{
			DWORD style = WS_CHILD | ES_AUTOHSCROLL | LBS_HASSTRINGS | LBS_DISABLENOSCROLL | WS_VSCROLL | WS_HSCROLL | LBS_NOTIFY;
			hWndList = CreateWindowExA(0, WC_LISTBOXA, "", style, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndList == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "FloatingListWidgetBackground");

			SendMessage(hWndList, WM_SETFONT, (WPARAM)(HFONT)wc.fontSmallLabel, 0);

			auto oldUserData = SetWindowLongPtr(hWndList, GWLP_USERDATA, (LONG_PTR)this);
			if (oldUserData != 0)
			{
				DestroyWindow(hWndList);
				Throw(0, "ListWidget -> this version of Windows appears to have hoarded the WC_LISTBOX GWLP_USERDATA pointer");

			}
			defaultFloatingListProc = (WNDPROC)SetWindowLongPtr(hWndList, GWLP_WNDPROC, (LONG_PTR)ProcessListMessage);
		}

		void SetDoubleClickEvent(EventIdRef id) override
		{
			evDoubleClick = id;
		}

		void AppendItem(cstr text) override
		{
			SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)text);
		}

		void ClearItems() override
		{
			ListBox_ResetContent(hWndList);
		}

		void ResizeChildToParent()
		{
			RECT rect;
			GetClientRect(backWindow, &rect);
			MoveWindow(hWndList, 0, 0, rect.right, rect.bottom, TRUE);
		}

		void CancelPopup()
		{
			SetVisible(false);

			TRACKMOUSEEVENT ev;
			ev.cbSize = sizeof ev;
			ev.hwndTrack = backWindow;
			ev.dwHoverTime = 0;
			ev.dwFlags = TME_CANCEL | TME_NONCLIENT;
			TrackMouseEvent(&ev);

			hEditor = nullptr;
		}

		bool IsCursorOutsideDomain() const
		{
			POINT p;
			GetCursorPos(&p);

			RECT rect;
			GetWindowRect(backWindow, &rect);

			if (PtInRect(&rect, p))
			{
				return false;
			}

			if (IsWindow(hEditor))
			{
				GetWindowRect(hEditor, &rect);

				if (GetFocus() == hEditor || PtInRect(&rect, p))
				{
					return false;
				}
			}

			return true;
		}

		std::vector<char> scratch;

		void OnDoubleClick()
		{
			if (evDoubleClick.name)
			{
				int index = ListBox_GetCurSel(hWndList);

				if (index >= 0)
				{
					int len = ListBox_GetTextLen(hWndList, index);
					if (len >= 0)
					{
						scratch.resize(len + 1);
					}

					SendMessageA(hWndList, LB_GETTEXT, index, (LPARAM)scratch.data());

					TEventArgs<std::pair<cstr,int>> args;
					args.value = std::make_pair(scratch.data(),index);
					wc.publisher.Publish(args, evDoubleClick);
				}
			}
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_SIZE:
				ResizeChildToParent();
				break;
			case WM_COMMAND:
				{
					WORD notify = HIWORD(wParam);
					switch (notify)
					{
					case LBN_DBLCLK:
						OnDoubleClick();
						return 0L;
					}
				}
				break;
			case WM_TIMER:
				if (IsCursorOutsideDomain())
				{
					SetVisible(false);
				}
				return 0L;
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		IWindow& OSList() override
		{
			return hWndList;
		}

		void Layout() override
		{
		}

		void AddLayoutModifier(ILayout* l) override
		{
			Throw(0, "Not implemented");
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(backWindow, isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(hWndList, isVisible ? SW_SHOW : SW_HIDE);

			if (isVisible)
			{
				if (timerId == 0)
				{
					timerId = SetTimer(backWindow, ID_VISIBILITY_TICK_TEST, 250, NULL);
				}
			}
			else
			{
				if (timerId != 0)
				{
					KillTimer(backWindow, timerId);
					timerId = 0;
				}
			}
		}

		IWidgetSet* Children()
		{
			return nullptr;
		}

		IWindow& Window()
		{
			return backWindow;
		}

		void RenderWhileMouseInEditorOrList(IWindow& editorWindow)
		{
			hEditor = editorWindow;
		}
	};
}

namespace Rococo::SexyStudio
{
	IFloatingListWidget* CreateFloatingListWidget(IWindow& window, WidgetContext& wc)
	{
		return new FloatingListWidget(window, wc);
	}
}
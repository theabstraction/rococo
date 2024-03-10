#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cfgs.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::Windows;

namespace ANON
{
	struct Popup : ICFGSContextPopupSupervisor, StandardWindowHandler, IListViewEvents
	{
		HWND hHostWindow;
		ICFGSDatabase& db;
		Vec2i referencePosition{ 0,0 };

		AutoFree<IParentWindowSupervisor> window;
		AutoFree<IListViewSupervisor> listView;

		Popup(HWND _hHostWindow, ICFGSDatabase& _db) : hHostWindow(_hHostWindow), db(_db)
		{

		}

		void Free() override
		{
			delete this;
		}

		void MakeVisibleAt(Vec2i desktopPosition)
		{
			if (IsWindowVisible(*window))
			{
				ShowWindow(*window, SW_HIDE);
				return;
			}

			referencePosition = Vec2i {desktopPosition.x + 16, desktopPosition.y };
			Layout();

			ShowWindow(*window, SW_SHOW);
		}

		void Create()
		{
			Windows::WindowConfig config;
			DWORD style = 0;
			SetPopupWindowConfig(config, GuiRect{ 0, 0, 0, 0 }, hHostWindow, "ContextPopup", style, 0);
			window = Windows::CreateChildWindow(config, this);

			GuiRect nullRect{ 0,0,0,0 };
			listView = AddListView(*window, nullRect, "PopupListView", *this, WS_VISIBLE | WS_CHILD, WS_VISIBLE | WS_CHILD, 0);

			Layout();
		}

		void Layout()
		{
			enum { Width = 640, Height = 480 };

			int x = 0;
			int y = 0;

			HWND hDesktopWnd = GetDesktopWindow();

			RECT desktopWindowRect;
			GetWindowRect(hDesktopWnd, &desktopWindowRect);

			if (referencePosition.x + Width < desktopWindowRect.right)
			{
				x = referencePosition.x;
			}
			else
			{
				x = referencePosition.x - Width;
			}

			if (referencePosition.y + Height < desktopWindowRect.bottom)
			{
				y = referencePosition.y;
			}
			else
			{
				y = referencePosition.y - Height;
			}

			MoveWindow(*window, x, y, Width, Height, FALSE);
			MoveWindow(*listView, 0, 0, Width, Height, FALSE);
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			default:
			break;
			}

			return StandardWindowHandler::OnMessage(hWnd, msg, wParam, lParam);
		}

		void OnItemChanged(int index) override
		{

		}

		void OnDrawItem(DRAWITEMSTRUCT& dis) override
		{

		}

		void OnMeasureItem(MEASUREITEMSTRUCT& mis) override
		{

		}

		void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) override
		{
			Layout();
		}
	};
}

extern "C" CFGS_CONTEXT_POPUP_API ICFGSContextPopupSupervisor* CFGSCreateWin32ContextPopup(HWND hHostWindow, ICFGSDatabase& db)
{
	auto *popup = new ANON::Popup(hHostWindow, db);
	popup->Create();
	return popup;
}

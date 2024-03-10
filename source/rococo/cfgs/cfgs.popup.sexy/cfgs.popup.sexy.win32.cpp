#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cfgs.h>
#include <vector>
#include <CommCtrl.h>

using namespace Rococo;
using namespace Rococo::CFGS;
using namespace Rococo::Windows;

namespace ANON
{
	struct Popup : ICFGSContextPopupSupervisor, StandardWindowHandler, IListViewEvents
	{
		struct NodeOptionHeader
		{
			cstr visibleName;
			cstr url;
		};

		using OPTION_METHOD = void (Popup::*)(const NodeOptionHeader& header);

		struct NodeOption
		{
			NodeOptionHeader header;
			OPTION_METHOD method;
		};

		HWND hHostWindow;
		ICFGSDatabase& db;
		Vec2i referencePosition{ 0,0 };
		Editors::DesignerVec2 designPosition;

		AutoFree<IParentWindowSupervisor> window;
		AutoFree<IListViewSupervisor> listView;

		std::vector<NodeOption> options =
		{
			{ "Int32.Add",		"Sys.Maths.Int32.Add", &Popup::SelectAdd },
			{ "Int32.Subtract", "Sys.Maths.Int32.Subtract", &Popup::SelectSubtract },
			{ "Int32.Multiply", "Sys.Maths.Int32.Multiply", &Popup::SelectMultiply },
			{ "Int32.Divide",	 "Sys.Maths.Int32.Divide", &Popup::SelectDivide },
		};

		enum { Width = 640, Height = 480 };

		Popup(HWND _hHostWindow, ICFGSDatabase& _db) : hHostWindow(_hHostWindow), db(_db)
		{

		}

		void SelectAdd(const NodeOptionHeader& header)
		{
			auto& node = db.Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::MakeNewUniqueId() });
			ShowWindow(*window, SW_HIDE);
			InvalidateRect(hHostWindow, NULL, TRUE);
		}

		void SelectSubtract(const NodeOptionHeader& header)
		{
			auto& node = db.Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::MakeNewUniqueId() });
			ShowWindow(*window, SW_HIDE);
			InvalidateRect(hHostWindow, NULL, TRUE);
		}

		void SelectMultiply(const NodeOptionHeader& header)
		{
			auto& node = db.Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::MakeNewUniqueId() });
			ShowWindow(*window, SW_HIDE);
			InvalidateRect(hHostWindow, NULL, TRUE);
		}

		void SelectDivide(const NodeOptionHeader& header)
		{
			auto& node = db.Nodes().Builder().AddNode(header.visibleName, designPosition, NodeId{ Rococo::MakeNewUniqueId() });
			ShowWindow(*window, SW_HIDE);
			InvalidateRect(hHostWindow, NULL, TRUE);
		}

		void Free() override
		{
			delete this;
		}

		void OpenPopupForDesignerSpace(Vec2i desktopPosition, Rococo::Editors::DesignerVec2 designPosition) override
		{
			if (IsWindowVisible(*window))
			{
				ShowWindow(*window, SW_HIDE);
				return;
			}

			this->designPosition = designPosition;

			referencePosition = Vec2i {desktopPosition.x + 16, desktopPosition.y };
			Layout();

			SetCursor(LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_WAIT)));

			listView->UIList().ClearRows();

			cstr row[2] = { "", nullptr };

			for (auto& opt : options)
			{
				row[0] = opt.header.visibleName;
				listView->UIList().AddRow(row);
			}

			SetCursor(LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_ARROW)));

			ShowWindow(*window, SW_SHOW);
		}

		void Create()
		{
			Windows::WindowConfig config;
			DWORD style = 0;
			SetPopupWindowConfig(config, GuiRect{ 0, 0, 0, 0 }, hHostWindow, "ContextPopup", style, 0);
			window = Windows::CreateChildWindow(config, this);

			GuiRect nullRect{ 0,0,0,0 };
			listView = AddListView(*window, nullRect, "", *this, LVS_REPORT | WS_VISIBLE | WS_CHILD, WS_VISIBLE | WS_CHILD, 0);

			cstr columns[] = { "Add Node...", nullptr };
			int32 widths[] = { Width, 0 };
			listView->UIList().SetColumns(columns, widths);

			Layout();
		}

		void Layout()
		{
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
			if (index < 0 || index >= (int32)options.size())
			{
				return;
			}

			auto& option = options[index];
			(this->*option.method)(option.header);
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

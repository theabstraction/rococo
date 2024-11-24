#include "sexystudio.impl.h"
#include <rococo.hashtable.h>
#include <rococo.os.h>

using namespace Rococo::Strings;
using namespace Rococo::Windows;

namespace Rococo::SexyStudio
{
	struct ReportWidget : IReportWidget, IWin32WindowMessageLoopHandler
	{
		IReportWidgetEvent& eventHandler;
		IVariableList& variableList;
		Win32ChildWindow eventSinkWindow;
		HWNDProxy hReportView;
		AutoFree<ILayoutSet> layouts = CreateLayoutSet();
		HFONT hFont = NULL;
		int fontSize = -11;
		HIMAGELIST hImages;

		struct ColumnDesc
		{
			HString title;
			int width;
			int index;
		};

		stringmap<ColumnDesc> columns;

		ReportWidget(IVariableList& _variables, IReportWidgetEvent& _eventHandler):
			variableList(_variables), eventHandler(_eventHandler),
			eventSinkWindow(_variables.Window(), *this)
		{
			DWORD exStyle = 0;
			DWORD win32Style = LVS_SINGLESEL | LVS_REPORT | LVS_ALIGNLEFT | WS_BORDER | WS_TABSTOP | WS_CHILD | WS_VSCROLL;

			hReportView = CreateWindowExA(exStyle, WC_LISTVIEWA, "", win32Style, 0, 0, 100, 100, eventSinkWindow, NULL, NULL, NULL);
			if (!hReportView)
			{
				Throw(GetLastError(), "%s: Could not create report view window", __FUNCTION__);
			}

			U8FilePath sysPath;
			variableList.Resolver().PingPathToSysPath("!textures/sexy-studio/report-checkboxes", sysPath);
			hImages = CreateImageList(sysPath, 2);
			ListView_SetImageList(hReportView, hImages, LVSIL_SMALL);

			int nImages = ImageList_GetImageCount(hImages);
			if (nImages != 2)
			{
				Throw(0, "Expecting two images in image list");
			}
		}


		void SetFont(int size, cstr name) override
		{
			hFont = SexyStudio::SetFont(size, name, hFont, hReportView);
		}

		LRESULT OnNotifyFromReportView(NMHDR* header, bool& callDefProc)
		{
			callDefProc = true;

			switch (header->code)
			{
				case NM_RCLICK:
				{
					DWORD dwpos = GetMessagePos();
					POINT pos{ (LONG)GET_X_LPARAM(dwpos), (LONG)GET_Y_LPARAM(dwpos) };
					POINT clientPos = pos;
					ScreenToClient(hReportView, &clientPos);

					LV_HITTESTINFO data = { 0 };
					data.pt = clientPos;
					ListView_HitTest(hReportView, &data);

					callDefProc = false;

					if (data.flags & LVHT_ONITEM && data.iItem >= 0 && data.iSubItem >= 0)
					{
						eventHandler.OnItemRightClicked(data.iItem, data.iSubItem, *this);
					}
					return 0L;
				}
				case NM_CLICK:
				{
					DWORD dwpos = GetMessagePos();
					POINT pos{ (LONG)GET_X_LPARAM(dwpos), (LONG)GET_Y_LPARAM(dwpos) };
					POINT clientPos = pos;
					ScreenToClient(hReportView, &clientPos);

					LV_HITTESTINFO data = { 0 };
					data.pt = clientPos;
					ListView_HitTest(hReportView, &data);

					callDefProc = false;

					if (data.flags & LVHT_ONITEM && data.iItem >= 0)
					{
						eventHandler.OnItemLeftClicked(data.iItem, data.iSubItem, *this);
					}
				}
			}

			return 0L;
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
			{
				auto width = LOWORD(lParam);
				auto height = HIWORD(lParam);
				MoveWindow(hReportView, 0, 0, width, height, TRUE);
			}
			break;
			case WM_NOTIFY:
			{
				auto* pHeader = (NMHDR*)(lParam);
				if (pHeader->hwndFrom == hReportView)
				{
					bool callDefProc = false;
					LRESULT retValue = OnNotifyFromReportView(pHeader, callDefProc);
					if (callDefProc)
					{
						break;
					}
					else
					{
						return retValue;
					}
				}
				break;
			}
			case WM_COMMAND:
				if (HIWORD(wParam) == 0)
				{
					WORD id = LOWORD(wParam);
					UNUSED(id)
					//eventHandler.OnCommand(id);
					return 0L;
				}
			}

			return DefWindowProcA(eventSinkWindow, msg, wParam, lParam);
		}

		IWindow& OSListView() override
		{
			return hReportView;
		}

		void AddColumn(cstr uniqueId, cstr header, int width) override
		{
			ColumnDesc desc;
			desc.title = header;
			desc.width = width;
			auto insertStatus = columns.insert(uniqueId, desc);
			if (insertStatus.second)
			{
				LVCOLUMNA col = { 0 };
				col = { 0 };
				col.mask = LVCF_TEXT | LVCF_WIDTH;
				col.pszText = (char*)header;
				col.cx = width;
				insertStatus.first->second.index = ListView_InsertColumn(hReportView, 100000, &col);
				if (insertStatus.first->second.index == -1)
				{
					Throw(0, "%s: ListView_InsertColumn returned -1", __FUNCTION__);
				}
			}
			else
			{
				Throw(0, "%s: a column with id %s already exists", __FUNCTION__, uniqueId);
			}
		}

		int SetItem(cstr columnId, cstr text, int row, int imageIndex) override
		{
			auto i = columns.find(columnId);
			if (i == columns.end())
			{
				Throw(0, "%s: no column found with id %s ", __FUNCTION__, columnId);
			}

			int columnIndex = i->second.index;

			int nRows = ListView_GetItemCount(hReportView);
			if (row >= nRows || row < 0)
			{
				LV_ITEMA item = { 0 };
				item.mask = LVIF_TEXT | LVIF_IMAGE;
				item.pszText =  (char*) text;
				item.iItem = 0x7FFFFFFF;
				item.iImage = imageIndex;
				item.cchTextMax = 256;

				row = ListView_InsertItem(hReportView, &item);

				return row;
			}

			ListView_SetItemText(hReportView, row, columnIndex, (char*)text);
			return row;
		}

		int GetImageIndex(int index, int subindex) override
		{
			LV_ITEMA item = { 0 };
			item.mask = LVIF_IMAGE;
			item.iItem = index;
			item.iImage = -1;
			item.iSubItem = subindex;
			ListView_GetItem(hReportView, &item);
			return item.iImage;
		}
		
		void SetImageIndex(int index, int subindex, int imageIndex) override
		{
			LV_ITEMA item = { 0 };
			item.mask = LVIF_IMAGE;
			item.iItem = index;
			item.iImage = imageIndex;
			item.iSubItem = subindex;
			ListView_SetItem(hReportView, &item);
		}

		int GetNumberOfRows() const override
		{
			return ListView_GetItemCount(hReportView);
		}

		bool GetText(U8FilePath& text, int row, int column) override
		{
			LVITEMA item = { 0 };
			item.cchTextMax = U8FilePath::CAPACITY - 1;
			item.pszText = text.buf;
			item.mask = LVIF_TEXT;
			item.iItem = row;
			item.iSubItem = column;
			return ListView_GetItem(hReportView, &item) ? true : false;
		}

		void ClearItems() override
		{
			ListView_DeleteAllItems(hReportView);
		}

		void Layout() override
		{
			layouts->Layout(*this);
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layouts->Add(l);
		}

		void Free() override
		{
			delete this;
		}

		int layoutHeight = 0;

		// Specify a layout height, for parents that modify their children's layout
		void SetDefaultHeight(int height)
		{
			layoutHeight = height;
		}

		// return a layout height. If unknown the result is <= 0
		int GetDefaultHeight() const
		{
			return layoutHeight;
		}

		// Modify visibility of the widget
		void SetVisible(bool isVisible) override
		{
			ShowWindow(eventSinkWindow, isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(hReportView, isVisible ? SW_SHOW : SW_HIDE);
		}

		//  the set of children if it can possess children, otherwise returns nullptr
		IWidgetSet* Children() override
		{
			return nullptr;
		}

		IWindow& Window() override
		{
			return eventSinkWindow;
		}
	};

	IReportWidget* CreateReportWidget(IVariableList& variables, IReportWidgetEvent& eventHandler)
	{
		return new ReportWidget(variables, eventHandler);
	}
}
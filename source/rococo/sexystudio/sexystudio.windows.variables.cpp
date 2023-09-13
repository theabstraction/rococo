#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <shobjidl.h>
#include <shlobj_core.h>
#include <rococo.auto-release.h>
#include <rococo.io.h>
#include <rococo.os.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace Rococo::SexyStudio
{
	IReportWidget* CreateReportWidget(IVariableList& variables, IReportWidgetEvent& eventHandler);
}

namespace ANON
{
	void ResizeEditor(IVariableList& variables, HWND hEditor)
	{
		RECT rect;
		GetClientRect(GetParent(hEditor), &rect);
		int32 x = variables.NameSpan();
		MoveWindow(hEditor, x, 0, rect.right - x, rect.bottom, TRUE);
	}

	void PaintName(const Theme& theme, IGuiWidgetEditor& editor, IVariableList& variables, HBRUSH backBrush)
	{
		struct Painter: IWin32Painter
		{
			const Theme* theme;
			cstr name;
			HBRUSH backBrush;
			IVariableList* variables;

			void OnPaint(HDC dc) override
			{
				HFONT hOldFont = (HFONT) SelectObject(dc, variables->Children()->Context().fontSmallLabel);

				auto oldColor = SetBkColor(dc, ToCOLORREF(theme->normal.bkColor));
				auto oldTxColor = SetTextColor(dc, ToCOLORREF(theme->normal.txColor));

				FillRect(dc, &rect, backBrush);

				DrawTextA(dc, name, StringLength(name), &rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

				SetTextColor(dc, oldTxColor);
				SetBkColor(dc, oldColor);
				SelectObject(dc, hOldFont);
			}

			RECT rect;
		} painter;

		painter.theme = &theme;
		painter.name = editor.Name();
		GetClientRect(editor.Window(), &painter.rect);
		painter.rect.right = variables.NameSpan();
		painter.backBrush = backBrush;
		painter.variables = &variables;

		PaintDoubleBuffered(editor.Window(), painter);
	}

	struct DropDownList : IDropDownList, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HString name;
		HWNDProxy hWndDropDown;
		Theme theme;
		HBRUSH backBrush;
		WNDPROC defaultDropDownProc = nullptr;
		EventIdRef evCharUpdateEvent = { 0 };

		static LRESULT CALLBACK ProcessDropDownMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto& This = *(DropDownList*)GetWindowLongPtr(wnd, GWLP_USERDATA);

			switch (msg)
			{
			case WM_KEYDOWN:
				switch (wParam)
				{
				case VK_RETURN:
					return 0;
				default:
					LRESULT result = CallWindowProc(This.defaultDropDownProc, wnd, msg, wParam, lParam);
					This.PublishCharChange();
					return result;
				}
			}
			return CallWindowProc(This.defaultDropDownProc, wnd, msg, wParam, lParam);
		}

		void PublishCharChange()
		{
			if (evCharUpdateEvent.name != nullptr)
			{
			}
		}

		DropDownList(IVariableList& _variables, HBRUSH _backBrush, bool addTextEditor, int32 dropDownHeight) :
			variables(_variables),
			backWindow(variables.Children()->Parent(), *this),
			backBrush(_backBrush)
		{
			DWORD style = WS_CHILD | ES_AUTOHSCROLL | CBS_HASSTRINGS;
			style |= addTextEditor ? CBS_DROPDOWN : CBS_SIMPLE;
			hWndDropDown = CreateWindowExA(0, WC_COMBOBOXA, "", style, 0, 0, 100, dropDownHeight, backWindow, NULL, NULL, NULL);
			if (hWndDropDown == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "DropDownList");

			if (!addTextEditor)
			{
				ComboBox_ShowDropdown(hWndDropDown, TRUE);
			}

			SendMessage(hWndDropDown, WM_SETFONT, (WPARAM)(HFONT)variables.Children()->Context().fontSmallLabel, 0);

			theme = GetTheme(_variables.Children()->Context().publisher);

			auto oldUserData = SetWindowLongPtr(hWndDropDown, GWLP_USERDATA, (LONG_PTR)this);
			if (oldUserData != 0)
			{
				DestroyWindow(hWndDropDown);
				Throw(0, "DropDownList -> this version of Windows appears to have hoarded the WC_EDIT GWLP_USERDATA pointer");

			}
			defaultDropDownProc = (WNDPROC)SetWindowLongPtr(hWndDropDown, GWLP_WNDPROC, (LONG_PTR)ProcessDropDownMessage);
		}

		void AppendItem(cstr text) override
		{
			if (CB_ERR == ComboBox_AddString(hWndDropDown, text))
			{
				Throw(GetLastError(), "Error adding [%s] to drop down list", text);
			}
		}

		void ClearItems() override
		{
			ComboBox_ResetContent(hWndDropDown);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintName(theme, *this, variables, backBrush);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				ResizeEditor(variables, hWndDropDown);
				break;
			case WM_COMMAND:
				break;
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		IWindow& OSDropDown() override
		{
			return hWndDropDown;
		}

		cstr Name() const override
		{
			return name;
		}

		void SetName(cstr name) override
		{
			SetWindowTextA(backWindow, name);
			this->name = name;
		}

		void Layout() override
		{
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

		void AddLayoutModifier(ILayout*) override
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
			ShowWindow(hWndDropDown, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children()
		{
			return nullptr;
		}

		IWindow& Window()
		{
			return backWindow;
		}
	};

	struct ListWidget : IListWidget, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HString name;
		HWNDProxy hWndList;
		Theme theme;
		HBRUSH backBrush;
		WNDPROC defaultListProc = nullptr;

		static LRESULT CALLBACK ProcessListMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto& This = *(DropDownList*)GetWindowLongPtr(wnd, GWLP_USERDATA);
			return CallWindowProc(This.defaultDropDownProc, wnd, msg, wParam, lParam);
		}

		ListWidget(IVariableList& _variables, HBRUSH _backBrush) :
			variables(_variables),
			backWindow(variables.Children()->Parent(), *this),
			backBrush(_backBrush)
		{
			DWORD style = WS_CHILD | ES_AUTOHSCROLL | LBS_HASSTRINGS;
			hWndList = CreateWindowExA(0, WC_LISTBOXA, "", style, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndList == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "ListWidgetBackground");

			SendMessage(hWndList, WM_SETFONT, (WPARAM)(HFONT)variables.Children()->Context().fontSmallLabel, 0);

			theme = GetTheme(_variables.Children()->Context().publisher);

			auto oldUserData = SetWindowLongPtr(hWndList, GWLP_USERDATA, (LONG_PTR)this);
			if (oldUserData != 0)
			{
				DestroyWindow(hWndList);
				Throw(0, "ListWidget -> this version of Windows appears to have hoarded the WC_EDIT GWLP_USERDATA pointer");

			}
			defaultListProc = (WNDPROC)SetWindowLongPtr(hWndList, GWLP_WNDPROC, (LONG_PTR)ProcessListMessage);
		}

		void AppendItem(cstr text) override
		{
			SendMessageA(hWndList, LB_ADDSTRING, 0, (LPARAM)text);
		}

		void ClearItems() override
		{
			ListBox_ResetContent(hWndList);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintName(theme, *this, variables, backBrush);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				ResizeEditor(variables, hWndList);
				break;
			case WM_COMMAND:
				break;
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		IWindow& OSList() override
		{
			return hWndList;
		}

		cstr Name() const override
		{
			return name;
		}

		void SetName(cstr name) override
		{
			SetWindowTextA(backWindow, name);
			this->name = name;
		}

		void Layout() override
		{
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

		void AddLayoutModifier(ILayout*) override
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
		}

		IWidgetSet* Children()
		{
			return nullptr;
		}

		IWindow& Window()
		{
			return backWindow;
		}
	};

	struct AsciiStringEditor : IAsciiStringEditor, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HString name;

		char* boundBuffer = nullptr;
		size_t capacity = 0;

		HWNDProxy hWndEditor;

		Theme theme;

		HBRUSH backBrush;

		EventIdRef evChangedEvent = { 0 };
		EventIdRef evCharChangedEvent = { 0 };
		EventIdRef evMouseMoved = { 0 };

		WNDPROC defaultEditProc = nullptr;

		static LRESULT CALLBACK ProcessEditorMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto& This = *(AsciiStringEditor*)  GetWindowLongPtr(wnd, GWLP_USERDATA);

			switch (msg)
			{
			case WM_KEYDOWN:
				switch (wParam)
				{
				case VK_RETURN:
					SetFocus(nullptr);
					This.PublishChange();
					return 0;
				}
			case WM_MOUSEMOVE:
			{
				Vec2i p = { GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam) };
				This.OnMouseMove(p);
				break;
			}
			}
			return CallWindowProc(This.defaultEditProc, wnd, msg, wParam, lParam);
		}

		void OnMouseMove(Vec2i mousePosition)
		{
			if (evMouseMoved.name != nullptr)
			{
				TEventArgs<Vec2i> args;
				args.value = mousePosition;
				variables.Children()->Context().publisher.Publish(args, evMouseMoved);
			}
		}

		void PublishCharChange()
		{
			if (evCharChangedEvent.name != nullptr)
			{
				TEventArgs<cstr> args;
				args.value = boundBuffer;
				GetWindowTextA(hWndEditor, boundBuffer, (int)capacity);
				variables.Children()->Context().publisher.Publish(args, evCharChangedEvent);
			}
		}

		void PublishChange()
		{
			if (evChangedEvent.name != nullptr)
			{
				TEventArgs<cstr> args;
				args.value = boundBuffer;
				GetWindowTextA(hWndEditor, boundBuffer, (int)capacity);
				variables.Children()->Context().publisher.Publish(args, evChangedEvent);
			}
		}

		AsciiStringEditor(IVariableList& _variables, HBRUSH _backBrush):
			variables(_variables),
			backWindow(variables.Children()->Parent(), *this),
			backBrush(_backBrush)
		{
			hWndEditor = CreateWindowExA(0, WC_EDITA, "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndEditor == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "AsciiStringEditor");

			SendMessage(hWndEditor, WM_SETFONT, (WPARAM) (HFONT) variables.Children()->Context().fontSmallLabel, 0);

			theme = GetTheme(_variables.Children()->Context().publisher);

			auto oldUserData = SetWindowLongPtr(hWndEditor, GWLP_USERDATA, (LONG_PTR)this);
			if (oldUserData != 0)
			{
				DestroyWindow(hWndEditor);
				Throw(0, "AsciiStringEditor -> this version of Windows appears to have hoarded the WC_EDIT GWLP_USERDATA pointer");

			}
			defaultEditProc = (WNDPROC)SetWindowLongPtr(hWndEditor, GWLP_WNDPROC, (LONG_PTR) ProcessEditorMessage);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintName(theme, *this, variables, backBrush);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				ResizeEditor(variables, hWndEditor);
				break;
			case WM_COMMAND:
				if (HIWORD(wParam) == EN_CHANGE)
				{
					PublishCharChange();
				}
				break;
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		void Bind(char* buffer, size_t capacityBytes) override
		{
			if (capacityBytes > 1_megabytes)
			{
				Throw(0, "%s: sanity check failed. Capacity > 1 meg", __FUNCTION__);
			}

			this->boundBuffer = buffer;
			this->capacity = capacityBytes;
			SendMessageA(hWndEditor, EM_SETLIMITTEXT, capacityBytes, 0);
			SetText(buffer);
		}

		IWindow& OSEditor() override
		{
			return hWndEditor;
		}

		void SetUpdateEvent(EventIdRef id) override
		{
			evChangedEvent = id;
		}

		void SetMouseMoveEvent(EventIdRef id) override
		{
			evMouseMoved = id;
		}

		void SetCharacterUpdateEvent(EventIdRef id) override
		{
			evCharChangedEvent = id;
		}

		void SetText(cstr text) override
		{
			SendMessageA(hWndEditor, WM_SETTEXT, 0, (LPARAM) text);
		}

		cstr Text() const override
		{
			SendMessageA(hWndEditor, WM_GETTEXT, capacity, (LPARAM) boundBuffer);
			return boundBuffer;
		}

		cstr Name() const override
		{
			return name;
		}

		void SetName(cstr name) override
		{
			SetWindowTextA(backWindow, name);
			this->name = name;
		}

		void Layout() override
		{
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

		void AddLayoutModifier(ILayout*) override
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
			ShowWindow(hWndEditor, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children()
		{
			return nullptr;
		}

		IWindow& Window()
		{
			return backWindow;
		}
	};

	struct FilePathEditor : IFilePathEditor, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HBRUSH backBrush;
		HString name;
		EFilePathType pathType;
		U8FilePath* filePath = nullptr;
		uint32 maxPathLength = 0;
		HWNDProxy hWndEditor;
		HWNDProxy hWndButton;
		Theme theme;
		EventIdRef evChangedEvent = { 0 };
		WNDPROC defaultEditProc = nullptr;

		FilePathEditor(IVariableList& _variables, HBRUSH _backBrush, EFilePathType _pathType):
			variables(_variables), backBrush(_backBrush), backWindow(_variables.Children()->Parent(), *this), pathType(_pathType)
		{
			hWndEditor = CreateWindowExA(0, WC_EDITA, "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndEditor == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "FilePathEditor");

			SendMessage(hWndEditor, WM_SETFONT, (WPARAM)(HFONT)variables.Children()->Context().fontSmallLabel, 0);

			theme = GetTheme(_variables.Children()->Context().publisher);

			auto oldUserData = SetWindowLongPtr(hWndEditor, GWLP_USERDATA, (LONG_PTR)this);
			if (oldUserData != 0)
			{
				DestroyWindow(hWndEditor);
				Throw(0, "AsciiStringEditor -> this version of Windows appears to have hoarded the WC_EDIT GWLP_USERDATA pointer");

			}

			defaultEditProc = (WNDPROC)SetWindowLongPtr(hWndEditor, GWLP_WNDPROC, (LONG_PTR)ProcessEditorMessage);

			hWndButton = CreateWindowExA(0, WC_BUTTONA, "...", WS_CHILD, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
		}

		void ResizeEditor()
		{
			RECT rect;
			GetClientRect(backWindow, &rect);
			int32 namespan = variables.NameSpan();
			int32 buttonWidth = 30;
			int32 editorWidth = (rect.right - rect.left) - buttonWidth - namespan;
			MoveWindow(hWndEditor, namespan, 0, editorWidth, rect.bottom, TRUE);
			MoveWindow(hWndButton, rect.right - buttonWidth, 0, buttonWidth, rect.bottom, TRUE);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintName(theme, *this, variables, backBrush);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				ResizeEditor();
				break;
			case WM_COMMAND:
				if (wParam == BN_CLICKED && lParam == (LPARAM)hWndButton.hWnd)
				{
					if (filePath)
					{
						U8FilePath oldPath = *filePath;

						U8FilePath sysPath;

						if (pathType == EFilePathType::PING_PATHS)
						{
							try
							{
								variables.Resolver().PingPathToSysPath(*filePath, sysPath);
							}
							catch (IException& ex)
							{
								THIS_WINDOW owner(GetAncestor(hWndEditor, GA_ROOT));
								Rococo::Windows::ShowErrorBox(owner, ex, "SexyStudio file search error");
							}
						}
						else
						{
							sysPath = *filePath;
						}

						try
						{
							char title[128];
							SafeFormat(title, "SexyStudio - %s selection", name.c_str());
							if (IO::ChooseDirectory(sysPath.buf, U8FilePath::CAPACITY, title))
							{
								if (pathType == EFilePathType::PING_PATHS)
								{
									variables.Resolver().SysPathToPingPath(sysPath, *filePath);
								}
								else
								{
									*filePath = sysPath;
								}

								SetWindowTextA(hWndEditor, *filePath);
								PublishChange();
							}
						}
						catch (IException&)
						{
							*filePath = oldPath;
						}
					}
				}
				break;
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		void Bind(U8FilePath& path, uint32 maxPathLength) override
		{
			filePath = &path;
			this->maxPathLength = min(maxPathLength, (uint32) U8FilePath::CAPACITY);
			SendMessageA(hWndEditor, EM_SETLIMITTEXT, maxPathLength, 0);
			SendMessageA(hWndEditor, WM_SETTEXT, 0, (LPARAM)(cstr) filePath);
		}

		void UpdateText() override
		{
			SendMessageA(hWndEditor, WM_SETTEXT, 0, (LPARAM)(cstr)filePath);
		}

		void SetUpdateEvent(EventIdRef id) override
		{
			evChangedEvent = id;
		}

		static LRESULT CALLBACK ProcessEditorMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			auto& This = *(FilePathEditor*)GetWindowLongPtr(wnd, GWLP_USERDATA);

			switch (msg)
			{
			case WM_KEYDOWN:
				switch (wParam)
				{
				case VK_RETURN:
					SetFocus(nullptr);
					This.PublishChange();
					return 0;
				}
			}
			return CallWindowProc(This.defaultEditProc, wnd, msg, wParam, lParam);
		}

		void PublishChange()
		{
			if (filePath && evChangedEvent.name != nullptr)
			{
				TEventArgs<cstr> args;
				args.value = *filePath;
				GetWindowTextA(hWndEditor, filePath->buf, maxPathLength);
				variables.Children()->Context().publisher.Publish(args, evChangedEvent);
			}
		}

		cstr Name() const override
		{
			return name;
		}

		void SetName(cstr name) override
		{
			SetWindowTextA(backWindow, name);
			this->name = name;
		}

		void Layout() override
		{
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

		void AddLayoutModifier(ILayout*) override
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
			ShowWindow(hWndEditor, isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(hWndButton, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children()
		{
			return nullptr;
		}

		IWindow& Window()
		{
			return backWindow;
		}
	};

	struct VariableList : IVariableList, IWin32WindowMessageLoopHandler, IWin32Painter
	{
		Win32ChildWindow window;
		AutoFree<IWidgetSetSupervisor> children;
		Theme theme;
		Brush bkBrush;
		AutoFree<ILayoutSet> layoutRules;
		IPingPathResolver& resolver;

		VariableList(IWidgetSet& widgets, IPingPathResolver& _resolver):
			resolver(_resolver),
			window(widgets.Parent(), *this)
		{
			children = CreateDefaultWidgetSet(window, widgets.Context());
			theme = GetTheme(widgets.Context().publisher);
			bkBrush = ToCOLORREF(theme.normal.bkColor);
			layoutRules = CreateLayoutSet();
			SetWindowTextA(window, "VariableList");
		}

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(window, &rect);
			FillRect(dc, &rect, bkBrush);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintDoubleBuffered(window, *this);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		IAsciiStringEditor* AddAsciiEditor() override
		{
			auto* editor = new AsciiStringEditor(*this, bkBrush);
			children->Add(editor);
			return editor;
		}

		IReportWidget* AddReportWidget(IReportWidgetEvent& eventHandler) override
		{
			auto* reportWidget = CreateReportWidget(*this, eventHandler);
			children->Add(reportWidget);
			return reportWidget;
		}

		IDropDownList* AddDropDownList(bool addTextEditor) override
		{
			auto* dropDown = new DropDownList(*this, bkBrush, addTextEditor, 100);
			children->Add(dropDown);
			return dropDown;
		}

		IListWidget* AddListWidget() override
		{
			auto* widget = new ListWidget(*this, bkBrush);
			children->Add(widget);
			return widget;
		}

		IFilePathEditor* AddFilePathEditor(EFilePathType pathType) override
		{
			auto* editor = new FilePathEditor(*this, bkBrush, pathType);
			children->Add(editor);
			return editor;
		}

		void Layout() override
		{
			layoutRules->Layout(*this);

			Vec2i span = Widgets::GetSpan(window);

			int32 x = 0;
			int32 y = 4;
			int32 width = span.x - 8;
			int32 ySpacing = 4;

			for (auto* child : *children)
			{
				int32 height = child->GetDefaultHeight();
				height = height == 0 ? 18 : height;

				MoveWindow(child->Window(), x, y, width, height, TRUE);
				y += height;
				y += ySpacing;
			}
		}

		// Specify a layout height, for parents that modify their children's layout
		void SetDefaultHeight(int height)
		{
			UNUSED(height);
			Throw(0, "Not implemented");
		}

		// return a layout height. If unknown the result is <= 0
		int GetDefaultHeight() const
		{
			int totalHeight = 0;
			for (const auto* child : *children)
			{
				int32 height = child->GetDefaultHeight();
				height = height == 0 ? 18 : height;
				totalHeight += height;
			}

			return totalHeight;
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layoutRules->Add(l);
		}

		void Free() override
		{

		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(window, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children() override
		{
			return children;
		}

		IWindow& Window() override
		{
			return window;
		}
		 
		int NameSpan() const override
		{
			return 128;
		}

		IPingPathResolver& Resolver() override
		{
			return resolver;
		}
	};
}

namespace Rococo::SexyStudio
{
	EventIdRef evGetFont_LineEditor = "evGetFont_LineEditor"_event;

	IVariableList* CreateVariableList(IWidgetSet& widgets, IPingPathResolver& resolver)
	{
		auto* v = new ANON::VariableList(widgets, resolver);
		widgets.Add(v);
		return v;
	}
}
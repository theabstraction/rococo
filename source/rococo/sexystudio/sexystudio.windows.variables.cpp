#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <shobjidl.h>
#include <shlobj_core.h>
#include <rococo.auto-release.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
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

		DropDownList(IVariableList& _variables, HBRUSH _backBrush, bool addTextEditor) :
			variables(_variables),
			backWindow(variables.Children()->Parent(), *this),
			backBrush(_backBrush)
		{
			DWORD style = WS_CHILD | ES_AUTOHSCROLL | CBS_HASSTRINGS;
			style |= addTextEditor ? CBS_DROPDOWN : CBS_SIMPLE;
			hWndDropDown = CreateWindowExA(0, WC_COMBOBOXA, "", style, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndDropDown == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SetWindowTextA(backWindow, "DropDownList");

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
			SendMessageA(hWndDropDown, CB_ADDSTRING, 0, (LPARAM) text);
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

	void BrowseFolder(U8FilePath& path, cstr title, HWND hWndParent)
	{
		BROWSEINFOA bi = { 0 };
		bi.hwndOwner = hWndParent;
		bi.lpszTitle = title;
		LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
		if (pidl != 0)
		{
			SHGetPathFromIDListA(pidl, path.buf);

			IMalloc* imalloc = 0;
			if (SUCCEEDED(SHGetMalloc(&imalloc)))
			{
				imalloc->Free(pidl);
				imalloc->Release();
			}
		}
	}

	struct FilePathEditor : IFilePathEditor, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HBRUSH backBrush;
		HString name;
		U8FilePath* filePath = nullptr;
		uint32 maxPathLength = 0;
		HWNDProxy hWndEditor;
		HWNDProxy hWndButton;
		Theme theme;
		EventIdRef evChangedEvent = { 0 };
		WNDPROC defaultEditProc = nullptr;

		FilePathEditor(IVariableList& _variables, HBRUSH _backBrush):
			variables(_variables), backBrush(_backBrush), backWindow(_variables.Children()->Parent(), *this)
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
						try
						{
							char title[128];
							SafeFormat(title, "SexyStudio - %s", name.c_str());
							BrowseFolder(*filePath, title, hWndButton);
							SetWindowTextA(hWndEditor, *filePath);
							PublishChange();
						}
						catch (IException&)
						{

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

		VariableList(IWidgetSet& widgets):
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

		IDropDownList* AddDropDownList(bool addTextEditor) override
		{
			auto* dropDown = new DropDownList(*this, bkBrush, addTextEditor);
			children->Add(dropDown);
			return dropDown;
		}

		IListWidget* AddListWidget() override
		{
			auto* widget = new ListWidget(*this, bkBrush);
			children->Add(widget);
			return widget;
		}

		IFilePathEditor* AddFilePathEditor() override
		{
			auto* editor = new FilePathEditor(*this, bkBrush);
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
			int32 height = 18;

			for (auto* child : *children)
			{
				MoveWindow(child->Window(), x, y, width, height, TRUE);
				y += height;
			}
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
	};
}

namespace Rococo::SexyStudio
{
	EventIdRef evGetFont_LineEditor = "evGetFont_LineEditor"_event;

	IVariableList* CreateVariableList(IWidgetSet& widgets)
	{
		auto* v = new VariableList(widgets);
		widgets.Add(v);
		return v;
	}
}
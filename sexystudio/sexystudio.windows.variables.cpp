#include "sexystudio.impl.h"
#include <rococo.strings.h>

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

	void PaintName(const Theme& theme, IGuiWidgetEditor& editor, IVariableList& variables, HBRUSH backBrush, HFONT hFont)
	{
		struct Painter: IWin32Painter
		{
			const Theme* theme;
			cstr name;
			HBRUSH backBrush;
			HFONT hFont;
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

	struct AsciiStringEditor : IAsciiStringEditor, IWin32WindowMessageLoopHandler
	{
		IVariableList& variables;
		Win32ChildWindow backWindow;
		HString name;

		char* boundBuffer = nullptr;
		size_t capacity = 0;

		HWND hWndEditor;

		Theme theme;

		HBRUSH backBrush;
		HFONT hFont = nullptr;

		AsciiStringEditor(IVariableList& _variables, HBRUSH _backBrush):
			variables(_variables),
			backWindow(variables.Children()->Parent(), *this),
			backBrush(_backBrush)
		{
			hWndEditor = CreateWindowExA(0, WC_EDITA, "", WS_CHILD, 0, 0, 100, 100, backWindow, NULL, NULL, NULL);
			if (hWndEditor == NULL)
			{
				Throw(GetLastError(), "%s: failed to create window", __FUNCTION__);
			}

			SendMessage(hWndEditor, WM_SETFONT, (WPARAM) (HFONT) variables.Children()->Context().fontSmallLabel, 0);

			theme = GetTheme(_variables.Children()->Context().publisher);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintName(theme, *this, variables, backBrush, hFont);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_SIZE:
				ResizeEditor(variables, hWndEditor);
			}
			return DefWindowProc(backWindow, msg, wParam, lParam);
		}

		void Bind(char* buffer, size_t capacityBytes) override
		{
			this->boundBuffer = buffer;
			this->capacity = capacityBytes;
			SendMessageA(hWndEditor, EM_SETLIMITTEXT, capacityBytes, 0);
			SetText(buffer);
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
			this->name = name;
		}

		void Layout() override
		{
		}

		void AddLayoutModifier(ILayout* preprocessor) override
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

	struct VariableList : IVariableList, IWin32WindowMessageLoopHandler, IWin32Painter
	{
		Win32ChildWindow window;
		AutoFree<IWidgetSetSupervisor> children;
		Theme theme;
		Brush bkBrush;

		VariableList(IWidgetSet& widgets):
			window(widgets.Parent(), *this)
		{
			children = CreateDefaultWidgetSet(window, widgets.Context());
			theme = GetTheme(widgets.Context().publisher);
			bkBrush = ToCOLORREF(theme.normal.bkColor);
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

		IAsciiStringEditor* AddAsciiString() override
		{
			auto* editor = new AsciiStringEditor(*this, bkBrush);
			children->Add(editor);
			return editor;
		}

		void Layout() override
		{
			Vec2i span = Widgets::GetParentSpan(window);
			MoveWindow(window, 0, 0, span.x, span.y, TRUE);

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

		void AddLayoutModifier(ILayout* preprocessor) override
		{
			Throw(0, "Not implemented");
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
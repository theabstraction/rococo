#include "sexystudio.api.h"
#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include "resource.h"

#include <rococo.events.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;
using namespace Rococo::Windows;

namespace
{
	struct SplitScreen : ISplitScreen, IWin32WindowMessageLoopHandler, IWindow
	{
		IPublisher& publisher;

		ISplitScreen* firstHalf = nullptr;
		ISplitScreen* secondHalf = nullptr;
		Win32ChildWindow window;

		AutoFree<IWidgetSetSupervisor> children;
		AutoFree<ILayoutSet> layoutRules;

		enum class ESplitDirection
		{
			None,
			Rows,
			Columns
		} splitAs = ESplitDirection::None;

		int32 splitPos = 0;

		Brush bkBrush;

		SplitScreen(IPublisher& _publisher, IWidgetSet& widgets):
			publisher(_publisher),
			window(widgets.Parent(), *this)
		{
			children = CreateDefaultWidgetSet(window);
			bkBrush.hBrush = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
			layoutRules = CreateLayoutSet();
		}

		void OnPaint(HDC dc)
		{
			RECT rect;
			GetClientRect(window, &rect);
			FillRect(dc, &rect, bkBrush);
			DrawEdge(dc, &rect, BDR_RAISEDINNER | BDR_RAISEDOUTER, 0);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC dc = BeginPaint(window, &ps);
				OnPaint(dc);
				EndPaint(window, &ps);
				return 0L;
			}
			}
			return DefWindowProcA(window, msg, wParam, lParam);
		}

		ISplitScreen* GetFirstHalf() override
		{
			return firstHalf;
		}

		ISplitScreen* GetSecondHalf()  override
		{
			return secondHalf;
		}

		GuiRect GetRect()  override
		{
			return Widgets::GetRect(*this);
		}

		void Split()
		{
			if (!firstHalf)
			{
				firstHalf = CreateSplitScreen(publisher, *children);
				secondHalf = CreateSplitScreen(publisher, *children);

				children->Add(firstHalf);
				children->Add(secondHalf);
			}
		}

		void SplitIntoColumns(int32 firstSpan) override
		{
			Split();
			splitAs = ESplitDirection::Columns;
			splitPos = firstSpan;
		}

		void SplitIntoRows(int32 firstSpan) override
		{
			Split();
			splitAs = ESplitDirection::Rows;
			splitPos = firstSpan;
		}

		void Merge()  override
		{

		}

		void Layout() override
		{
			layoutRules->Layout(*this);

			RECT rect;
			GetClientRect(GetParent(window), &rect);

			if (firstHalf)
			{
				Vec2i spanA;
				Vec2i spanB;
				Vec2i a, b;

				switch (splitAs)
				{
				case ESplitDirection::Columns:
					a = { 0, 0 };
					b = { splitPos + 1, 0 };
					spanA = { splitPos, rect.bottom - rect.top };
					spanB = { rect.right - rect.left - splitPos, rect.bottom - rect.top };
					break;
				default:  // rows
					a = { 0, 0 };
					b = { 0, splitPos + 1 };
					spanA = { rect.right - rect.left, splitPos };
					spanB = { rect.right - rect.left, rect.bottom - rect.top - splitPos};
				};

				MoveWindow(firstHalf->Window(), a.x, a.y, spanA.x, spanA.y, TRUE);
				MoveWindow(secondHalf->Window(), b.x, b.y, spanB.x, spanB.y, TRUE);
				firstHalf->Layout();
				secondHalf->Layout();
			}
			else
			{
				for (auto* child : *children)
				{
					child->Layout();
				}
			}
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layoutRules->Add(l);
		}

		operator HWND() const override
		{
			return window;
		}

		IWindow& Window() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(window, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children() override
		{
			return children;
		}

		void SetBackgroundColour(RGBAb colour) override
		{
			DeleteObject((HGDIOBJ)bkBrush.hBrush);
			bkBrush.hBrush = CreateSolidBrush(RGB(colour.red, colour.green, colour.blue));
			InvalidateRect(window, NULL, TRUE);
		}
	};
}

namespace Rococo::SexyStudio
{
	ISplitScreen* CreateSplitScreen(IPublisher& publisher, IWidgetSet& widgets)
	{
		return new SplitScreen(publisher, widgets);
	}
}
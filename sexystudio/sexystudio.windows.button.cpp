#include "sexystudio.impl.h"
#include <rococo.strings.h>

#include <rococo.events.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	struct Button : IButtonWidget, IWin32WindowMessageLoopHandler, ILayoutControl
	{
		Rococo::Events::IPublisher& publisher;
		Win32ChildWindow eventSinkWindow;

		HWNDProxy hbuttonWnd = nullptr;

		IWidgetSet& widgets;
		HString text;

		Rococo::Events::EventIdRef evOnClick;

		AutoFree<ILayoutSet> layouts = CreateLayoutSet();

		Button(Rococo::Events::IPublisher& _publisher, IWidgetSet& _widgets, cstr _text, Rococo::Events::EventIdRef _evOnClick):
			publisher(_publisher),
			eventSinkWindow(_widgets.Parent(), *this),
			widgets(_widgets), text(_text),
			evOnClick(_evOnClick)
		{
			MoveWindow(eventSinkWindow, 0, 0, 20, 20, TRUE);

			hbuttonWnd = CreateWindowExA(
				0,
				"BUTTON",
				_text, 
				WS_CHILD | WS_VISIBLE, 
				0, 0, 
				20, 20,
				eventSinkWindow, NULL, GetMainInstance(), NULL);

			if (hbuttonWnd == NULL)
			{
				Throw(GetLastError(), "%s: Could not create Button", __FUNCTION__);
			}

			ShowWindow(eventSinkWindow, SW_SHOW);

			_widgets.Add(this);
		}
		
		~Button()
		{
			DestroyWindow(hbuttonWnd);
		}

		void AttachLayoutModifier(ILayout* l) override
		{
			layouts->Add(l);
		}

		void Free() override
		{
			delete this;
		}

		void Layout() override
		{
			layouts->Layout(eventSinkWindow);
		}

		ILayoutControl& Layouts() override
		{
			return *this;
		}

		IWindow& Window() override
		{
			return eventSinkWindow;
		}

		void OnClick()
		{ 
			Rococo::Events::TEventArgs<ButtonClickContext> args;
			args.value.sourceWidget = this;
			publisher.Publish(args, evOnClick);
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(eventSinkWindow, isVisible ? SW_SHOW : SW_HIDE);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_COMMAND:
			{
				auto* hdr = reinterpret_cast<NMHDR*>(lParam);
				HWND hSrc = (HWND)lParam;
				if (hSrc == hbuttonWnd)
				{
					OnClick();
					return 0L;
				}
				break;
			}
			}
			return DefWindowProc(eventSinkWindow, msg, wParam, lParam);
		}

		IWidgetSet* Children() override
		{
			return nullptr;
		}
	};
}

namespace Rococo::SexyStudio
{
	IButtonWidget* CreateButton(Rococo::Events::IPublisher& publisher, IWidgetSet& widgets, cstr text, Rococo::Events::EventIdRef evOnClick)
	{
		return new Button(publisher, widgets, text, evOnClick);
	}
}
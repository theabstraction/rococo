#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include "resource.h"

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#include <rococo.events.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
{
	void SetupDWM(HWND hMainWnd)
	{
		MARGINS margins;

		margins.cxLeftWidth = 0;
		margins.cxRightWidth = 0;
		margins.cyBottomHeight = 0;
		margins.cyTopHeight = 0;

		HRESULT hr = DwmExtendFrameIntoClientArea(hMainWnd, &margins);
		if FAILED(hr)
		{
			Throw(hr, "DwmExtendFrameIntoClientArea failed");
		}
	}

	const DWORD ideExStyle = 0;
	const DWORD ideStyle = WS_OVERLAPPEDWINDOW;

	class Win32MainIDEWindow : public IIDEFrameSupervisor, private IWin32WindowMessageLoopHandler
	{
	public:
		Rococo::Events::IPublisher& publisher;
		Win32TopLevelWindow mainFrame;
		AutoFree<IWidgetSetSupervisor> children;
		AutoFree<IToolbar> frameBar;
		EventIdRef evClose = { 0 };
		EventIdRef evResize = { 0 };


		Win32MainIDEWindow(Rococo::Events::IPublisher& _publisher):
			publisher(_publisher),
			mainFrame(ideExStyle, ideStyle, *this)
		{
			children = CreateDefaultWidgetSet(mainFrame);
			frameBar = CreateToolbar(*children);
		}

		IToolbar& FrameBar() override
		{
			return *frameBar;
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_NCCALCSIZE:
				if (wParam == TRUE)
				{
					return 0L;
				}
				break;
			case WM_ACTIVATE:
				SetupDWM(mainFrame);
				break;
			case WM_CLOSE:
				if (evClose.name == nullptr)
				{
					PostQuitMessage(0);
				}
				else
				{
					Rococo::Events::TEventArgs<ButtonClickContext> args;
					args.value.sourceWidget = nullptr;
					publisher.Publish(args, evClose);
				}
				break;
			case WM_SIZE:
				if (evResize.name != nullptr)
				{
					Rococo::Events::TEventArgs<IWidgetSet*> args;
					args.value = children;
					publisher.Publish(args, evResize);
				}
				else
				{
					for (auto* child : *children)
					{
						child->Layout();
					}
				}
				return 0L;
			}
			return DefWindowProcA(mainFrame, msg, wParam, lParam);
		}

		IWindow& Window() override
		{
			return mainFrame;
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			if (isVisible)
			{
				SetupDWM(mainFrame);
				SetWindowPos(mainFrame, NULL, 0, 0, 800, 600, SWP_FRAMECHANGED);
			}
			ShowWindow(mainFrame, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet& Children() override
		{
			return *children;
		}

		void SetCloseEvent(const Rococo::Events::EventIdRef& evClose) override
		{
			this->evClose = evClose;
		}

		void SetResizeEvent(const Rococo::Events::EventIdRef& evResize) override
		{
			this->evResize = evResize;
		}
	};
}

namespace Rococo::SexyStudio
{
	IIDEFrameSupervisor* CreateMainIDEFrame(Rococo::Events::IPublisher& publisher)
	{
		return new Win32MainIDEWindow(publisher);
	}

	void UseDefaultFrameBarLayout(IToolbar& frameBar)
	{
		AnchorToParentLeft(frameBar.Layouts(), 0);
		AnchorToParentRight(frameBar.Layouts(), 0);
		AnchorToParentTop(frameBar.Layouts(), 0);
		ExpandBottomFromTop(frameBar.Layouts(), 27);
		frameBar.SetSpacing(10, 20);
	}

	void AddDefaultCloseButton(IPublisher& publisher, IToolbar& frameBar, Rococo::Events::EventIdRef evClicked)
	{
		auto* textButton = CreateButton(publisher, *frameBar.Children(), "X", evClicked);
		frameBar.SetManualLayout(textButton);

		AnchorToParentRight(textButton->Layouts(), 0);
		AnchorToParentTop(textButton->Layouts(), 4);
		ExpandLeftFromRight(textButton->Layouts(), 25);
		ExpandBottomFromTop(textButton->Layouts(), 25);
	}
}
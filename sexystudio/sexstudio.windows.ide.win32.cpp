#include "sexystudio.api.h"
#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include "resource.h"

#include <rococo.events.h>


using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
{
	const DWORD ideExStyle = 0;
	const DWORD ideStyle = WS_OVERLAPPEDWINDOW;

	class Win32MainIDEWindow : public IIDEFrameSupervisor, private IWin32WindowMessageLoopHandler
	{
	public:
		Rococo::Events::IPublisher& publisher;
		Win32TopLevelWindow mainFrame;
		AutoFree<IWidgetSetSupervisor> children;
		EventIdRef evClose = { 0 };
		EventIdRef evResize = { 0 };
		Brush bkBrush;

		Win32MainIDEWindow(Rococo::Events::IPublisher& _publisher) :
			publisher(_publisher),
			mainFrame(ideExStyle, ideStyle, *this),
			bkBrush { CreateSolidBrush(RGB(192, 192, 192)) }
		{
			children = CreateDefaultWidgetSet(mainFrame);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
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

		RECT borderThickness = { 0 };

		void SetVisible(bool isVisible) override
		{
			if (isVisible)
			{
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
		Widgets::AnchorToParentLeft(frameBar, 0);
		Widgets::AnchorToParentRight(frameBar, 0);
		Widgets::AnchorToParentTop(frameBar, 0);
		Widgets::ExpandBottomFromTop(frameBar, 48);
		frameBar.SetSpacing(10, 20);
	}

	void AddDefaultRHSFrameButtons(IPublisher& publisher, IToolbar& frameBar, Rococo::Events::EventIdRef evClose, Rococo::Events::EventIdRef evMax, Rococo::Events::EventIdRef evMin)
	{
		auto* closeButton = CreateButtonByResource(publisher, *frameBar.Children(), IDI_IDE_CLOSE, evClose);
		frameBar.SetManualLayout(closeButton);

		Vec2i spanClose = Widgets::GetSpan(*closeButton);

		Widgets::AnchorToParentRight(*closeButton, 4);
		Widgets::AnchorToParentTop(*closeButton, 4);
		Widgets::ExpandLeftFromRight(*closeButton, spanClose.x);
		Widgets::ExpandBottomFromTop(*closeButton, spanClose.y);

		auto* maxButton = CreateButtonByResource(publisher, *frameBar.Children(), IDI_IDE_MAXIMIZE, evMax);
		frameBar.SetManualLayout(maxButton);

		Vec2i spanMax = Widgets::GetSpan(*maxButton);

		Widgets::AnchorToParentRight(*maxButton, 4 + spanClose.x + 4);
		Widgets::AnchorToParentTop(*maxButton, 4);
		Widgets::ExpandLeftFromRight(*maxButton, spanMax.x);
		Widgets::ExpandBottomFromTop(*maxButton, spanMax.y);

		auto* minButton = CreateButtonByResource(publisher, *frameBar.Children(), IDI_IDE_MINIMIZE, evMin);
		frameBar.SetManualLayout(minButton);

		Vec2i spanMin = Widgets::GetSpan(*minButton);

		Widgets::AnchorToParentRight(*minButton, 4 + spanClose.x + 4 + spanMax.x + 4);
		Widgets::AnchorToParentTop(*minButton, 4);
		Widgets::ExpandLeftFromRight(*minButton, spanMin.x);
		Widgets::ExpandBottomFromTop(*minButton, spanMin.y);

		frameBar.Layout();
	}
}
#pragma once

#include <rococo.os.win32.h>
#include <rococo.window.h>

namespace Rococo
{
	namespace Events
	{
		class IPublisher;
		struct EventIdRef;
	}
}

namespace Rococo::SexyStudio
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	const char* GetChildClassName();

	struct ILayout;
	struct IWin32WindowMessageLoopHandler;
	struct ILayoutControl;
	struct IWindowMessageHandler;
	struct IToolbar;
	struct ILayoutSet;
	struct IWidgetSet;
	struct IGuiWidget;

	ROCOCOAPI ILayout
	{
		virtual void Layout(IWindow& parent, GuiRect& rect) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IWin32WindowMessageLoopHandler
	{
		virtual LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	ROCOCOAPI ILayoutControl
	{
		/// <summary>
		/// Adds a reference to a layout implementation that controls how the owner should be resized
		/// When the control is deleted the reference is Free'd
		/// </summary>
		/// <param name="preprocessor">the layout implemementation reference</param>
		virtual void AttachLayoutModifier(ILayout* preprocessor) = 0;
	};

	ROCOCOAPI IWindowMessageHandler: IWin32WindowMessageLoopHandler
	{
		virtual ILayoutControl& Layouts() = 0;
		virtual void Show() = 0;
		virtual void Free() = 0;
	};

	class Win32ChildWindow: public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler);
		~Win32ChildWindow();
		operator HWND() const override {return hWnd; }
	};

	class Win32TopLevelWindow : public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32TopLevelWindow(DWORD exStyle, DWORD style, IWin32WindowMessageLoopHandler& handler);
		~Win32TopLevelWindow();
		operator HWND() const override { return hWnd; }
	};

	void SetLastMessageError(IException& ex);
	void AssertNoMessageError();

	HINSTANCE GetMainInstance();

	ROCOCOAPI IGuiWidget
	{
		// reshape this control and its children according to the layout controls
		virtual void Layout() = 0;
		// get a reference to the layout control, to control how the widget is presented
		virtual ILayoutControl& Layouts() = 0;
		virtual Rococo::Windows::IWindow& Window() = 0;
		virtual void Free() = 0;
		virtual void SetVisible(bool isVisible) = 0;
		virtual IWidgetSet* Children() = 0;
	};

	void SetPosition(IWindow& window, const GuiRect& rect);
	Vec2i GetSpan(IWindow& window);

	ROCOCOAPI IWidgetSet
	{
		virtual void Add(IGuiWidget * widget) = 0;
		virtual Rococo::Windows::IWindow& Parent() = 0;
		virtual IGuiWidget** begin() = 0;
		virtual IGuiWidget** end() = 0;
	};

	ROCOCOAPI IToolbar: public IGuiWidget
	{
		virtual void SetManualLayout(IGuiWidget* widget) = 0;
		virtual void SetSpacing(int32 firstBorder, int32 widgetSpacing) = 0;
	};

	ROCOCOAPI IIDEFrame
	{
		virtual IWindow & Window() = 0;
		virtual void SetVisible(bool isVisible) = 0;
		virtual IToolbar& FrameBar() = 0;
		virtual IWidgetSet& Children() = 0;
	};

	ROCOCOAPI IIDEFrameSupervisor : IIDEFrame
	{
		virtual void Free() = 0;
		virtual void SetCloseEvent(const Rococo::Events::EventIdRef& evClose) = 0;
		virtual void SetResizeEvent(const Rococo::Events::EventIdRef& evResize) = 0;
	};

	IIDEFrameSupervisor* CreateMainIDEFrame(Rococo::Events::IPublisher& publisher);

	void UseDefaultFrameBarLayout(IToolbar& frameBar);

	// Adds a close button to the end of the frame bar
	void AddDefaultCloseButton(Rococo::Events::IPublisher& publisher, IToolbar& frameBar, Rococo::Events::EventIdRef evClicked);

	void InitStudioWindows(HINSTANCE hInstance, cstr iconLarge, cstr iconSmall);

	IToolbar* CreateToolbar(IWidgetSet& widgets);

	enum { WM_IDE_RESIZED = 0x8001 };

	void AnchorToParentLeft(ILayoutControl& layouts, int pixelBorder);
	void AnchorToParentRight(ILayoutControl& layouts, int pixelBorder);
	void AnchorToParentTop(ILayoutControl& layouts, int pixelBorder);
	void ExpandBottomFromTop(ILayoutControl& layouts, int pixels);
	void ExpandLeftFromRight(ILayoutControl& layouts, int pixels);

	ROCOCOAPI ILayoutSet
	{
		virtual void Add(ILayout* d) = 0;
		virtual void Free() = 0;
		virtual void Layout(Rococo::Windows::IWindow& window) = 0;
	};

	struct HWNDProxy: Rococo::Windows::IWindow
	{
		HWND hWnd;
		HWNDProxy(HWND _hWnd) : hWnd(_hWnd) {}
		operator HWND() const override { return hWnd;  }
	};

	ILayoutSet* CreateLayoutSet();

	ROCOCOAPI IButtonWidget : IGuiWidget
	{
	};

	IButtonWidget* CreateButton(Rococo::Events::IPublisher& publisher, IWidgetSet& widgets, cstr text, Rococo::Events::EventIdRef evClicked);

	// used as the template paramter for Rococo::Events::TEventArgs<T>;
	struct ButtonClickContext
	{
		IButtonWidget* sourceWidget;
	};

	ROCOCOAPI IWidgetSetSupervisor : IWidgetSet
	{
		virtual void Free() = 0;
	};

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent);
}

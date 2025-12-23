#pragma once
#include <rococo.types.h>
#include <rococo.eventargs.h>
#include <rococo.os.win32.global-ns.h>
#include <rococo.window.h>
#include <Uxtheme.h>
#include <windowsx.h>

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
	DECLARE_ROCOCO_INTERFACE IOSFont;
	DECLARE_ROCOCO_INTERFACE IGuiWidget;
	DECLARE_ROCOCO_INTERFACE IWidgetSet;
	DECLARE_ROCOCO_INTERFACE ISexyStudioEventHandler;

	struct WidgetContext
	{
		Events::IPublisher& publisher;
		IOSFont& fontSmallLabel;
	};

	struct WaitCursorSection
	{
		WaitCursorSection();
		~WaitCursorSection();
	};

	typedef int64 ID_TREE_ITEM;

	ROCOCO_INTERFACE ILayout
	{
		virtual void Layout(IGuiWidget & widget, GuiRect & rect) = 0;
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE ILayoutSet
	{
		virtual void Add(ILayout * d) = 0;
		virtual void Free() = 0;
		virtual void Layout(IGuiWidget& widget) = 0;
	};

	ROCOCO_INTERFACE IGuiWidget
	{
		/* Reshape this control and its children according to the layout controls.
		   parents should call this when they are resized
		 */
		virtual void Layout() = 0;

		// add a modifier - to modify the way this widget is layed out
		virtual void AddLayoutModifier(ILayout* l) = 0;

		// Release the memory associated with this widget, invalidating it.
		virtual void Free() = 0;

		// Modify visibility of the widget
		virtual void SetVisible(bool isVisible) = 0;

		// Specify a layout height, for parents that modify their children's layout
		virtual void SetDefaultHeight(int height) = 0;

		// return a layout height. If unknown the result is <= 0
		virtual int GetDefaultHeight() const = 0;

		// returns the set of children if it can possess children, otherwise returns nullptr
		virtual IWidgetSet* Children() = 0;

		// Get the OS or other implementation of this widget
		virtual Windows::IWindow& Window() = 0;

		operator Windows::IWindow& () { return Window(); }
	};


	namespace Widgets
	{
		void AnchorToParentLeft(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentRight(IGuiWidget& widget, int pixelBorder);
		void AnchorToParentTop(IGuiWidget& widget, int pixelBorder);
		void AnchorToParent(IGuiWidget& widget, int leftBorder, int topBorder, int rightBorder, int bottomBorder);
		void ExpandBottomFromTop(IGuiWidget& widget, int pixels);
		void ExpandLeftFromRight(IGuiWidget& widget, int pixels);

		void ExpandToFillParentSpace(Windows::IWindow& window);

		Vec2i GetParentSpan(Windows::IWindow& window);
		GuiRect GetScreenRect(Windows::IWindow& window);
		Vec2i GetSpan(Windows::IWindow& widget);
		GuiRect MapScreenToWindowRect(const GuiRect& rect, Windows::IWindow& window);
		void SetWidgetPosition(Windows::IWindow& widget, const GuiRect& rect);
		void Maximize(Windows::IWindow& window);
		void Minimize(Windows::IWindow& window);

		void SetSpan(Windows::IWindow& window, int32 dx, int32 dy);
		void SetText(Windows::IWindow& window, const char* text);
	}

	ROCOCO_INTERFACE IWidgetSet
	{
		// Add a widget to the widget set, when the set owner is done it will call Free() on the widget
		virtual void Add(IGuiWidget * widget) = 0;
		// the parent window to which this widget set belongs
		virtual Windows::IWindow& Parent() = 0;
		// IGuiWidget* iterator begin()
		virtual IGuiWidget** begin() = 0;
		// IGuiWidget* iterator end()
		virtual IGuiWidget** end() = 0;
		// IGuiWidget* iterator begin()
		virtual const IGuiWidget** begin() const = 0;
		// IGuiWidget* iterator end()
		virtual const IGuiWidget** end() const = 0;
		// Get the publisher associated with this widget set
		virtual WidgetContext& Context() = 0;
	};

	enum class EFolderIcon
	{
		FOLDER_CLOSED = 0,
		FOLDER_OPEN = 1
	};

	ROCOCO_INTERFACE IGuiTreeRenderer
	{
		virtual void RenderItem() = 0;
	};

	struct TreeItemInfo
	{
		ID_TREE_ITEM idItem;
	};

	ROCOCO_INTERFACE IPopupMenuBuilder
	{
		virtual void AppendMenuItem(uint16 id, cstr text) = 0;
	};

	ROCOCO_INTERFACE IPopupMenu : IPopupMenuBuilder
	{
		virtual void ClearPopupMenu() = 0;
		virtual void ShowPopupMenu(Vec2i pos) = 0;
	};

	ROCOCO_INTERFACE IGuiTree : IGuiWidget
	{
		virtual ID_TREE_ITEM AppendItem(ID_TREE_ITEM branch) = 0;
		virtual void Clear() = 0;
		virtual void Collapse() = 0;
		virtual void EnableExpansionIcons(bool enable) = 0;
		virtual void ExpandAt(ID_TREE_ITEM idItem) = 0;
		virtual void SetContext(ID_TREE_ITEM idItem, uint64 contextId) = 0;
		virtual void SetItemExpandedImage(ID_TREE_ITEM hItem, int imageIndex) = 0;
		virtual void SetItemText(ID_TREE_ITEM hItem, cstr text) = 0;
		virtual void SetItemImage(ID_TREE_ITEM hItem, int imageIndex) = 0;
		virtual void EnumerateChildren(ID_TREE_ITEM id, IEventCallback<TreeItemInfo>& cb) const = 0;
		virtual void GetText(char* buffer, size_t capacity, ID_TREE_ITEM id) = 0;

		// Define the list of image identifiers, each is an int32, example SetImageList(2, ID_FOLDER_CLOSED, ID_FOLDER_OPEN, ID_FILE)
		virtual void SetImageList(uint32 nItems, ...) = 0;
		virtual Windows::IWindow& TreeWindow() = 0;
		virtual IPopupMenu& PopupMenu() = 0;
	};

	ROCOCO_INTERFACE IGuiTreeEvents
	{
		virtual void OnItemContextClick(IGuiTree & tree, ID_TREE_ITEM hItem, Vec2i pos) = 0;
		virtual void OnCommand(uint16 id) = 0;
	};

	struct TreeStyle
	{
		bool hasCheckBoxes = false;
		bool hasButtons = false;
		bool hasLines = false;
	};

	IGuiTree* CreateTree(IWidgetSet& widgets, const TreeStyle& style, IGuiTreeEvents& eventHandler, IGuiTreeRenderer* customRenderer = nullptr);

	ROCOCO_INTERFACE IGuiWidgetEditor : IGuiWidget
	{
		virtual cstr Name() const = 0;
		virtual void SetName(cstr name) = 0;
	};

	ROCOCO_INTERFACE IAsciiStringEditor : IGuiWidgetEditor
	{
		virtual void Bind(char* buffer, size_t capacityBytes) = 0;
		virtual Windows::IWindow& OSEditor() = 0;
		virtual void SetCharacterUpdateEvent(Events::EventIdRef id) = 0;
		virtual void SetMouseMoveEvent(Events::EventIdRef id) = 0;
		virtual void SetText(cstr text) = 0;
		virtual void SetUpdateEvent(Events::EventIdRef id) = 0;
		virtual cstr Text() const = 0;
	};

	ROCOCO_INTERFACE IDropDownList : IGuiWidgetEditor
	{
		virtual Windows::IWindow & OSDropDown() = 0;
		virtual void AppendItem(cstr text) = 0;
		virtual void ClearItems() = 0;
	};

	ROCOCO_INTERFACE IListWidget : IGuiWidgetEditor
	{
		virtual Windows::IWindow & OSList() = 0;
		virtual void AppendItem(cstr text) = 0;
		virtual void ClearItems() = 0;
	};

	ROCOCO_INTERFACE IFloatingListWidget : IGuiWidget
	{
		virtual Windows::IWindow & OSList() = 0;
		virtual void AppendItem(cstr text) = 0;
		virtual void ClearItems() = 0;
		virtual void RenderWhileMouseInEditorOrList(Windows::IWindow& editorWindow) = 0;
		virtual void SetDoubleClickEvent(Events::EventIdRef id) = 0;
	};

	ROCOCO_INTERFACE IReportWidget : IGuiWidget
	{
		virtual Windows::IWindow & OSListView() = 0;

		virtual void AddColumn(cstr uniqueId, cstr header, int width) = 0;

		virtual void SetFont(int size, cstr name) = 0;

		// Sets the text at a particular column, If row does not refer to an existant row, a new row is appended
		// The return value is the actual row number used internally
		virtual int SetItem(cstr columnId, cstr text, int row, int imageIndex) = 0;

		// Clears all items, though leaves columns intact.
		virtual void ClearItems() = 0;

		virtual int GetImageIndex(int index, int subindex) = 0;
		virtual void SetImageIndex(int index, int subindex, int imageIndex) = 0;

		virtual int GetNumberOfRows() const = 0;
		virtual bool GetText(U8FilePath& text, int row, int column) = 0;
	};

	ROCOCO_INTERFACE IReportWidgetEvent
	{
		virtual void OnItemLeftClicked(int index, int subItem, IReportWidget & source) = 0;
		virtual void OnItemRightClicked(int index, int subItem, IReportWidget& source) = 0;
	};

	SEXYSTUDIO_API cstr FindDot(cstr s);

	IFloatingListWidget* CreateFloatingListWidget(Windows::IWindow& window, WidgetContext& wc);

	ROCOCO_INTERFACE IFilePathEditor : IGuiWidgetEditor
	{
		virtual void Bind(U8FilePath & path, uint32 maxChars) = 0;
		virtual void SetUpdateEvent(Events::EventIdRef id) = 0;
		virtual void UpdateText() = 0;
	};

	enum class EFilePathType
	{
		PING_PATHS,
		SYS_PATHS
	};

	ROCOCO_INTERFACE IVariableList : IGuiWidget
	{
		virtual IPingPathResolver& Resolver() = 0;
		virtual IAsciiStringEditor* AddAsciiEditor() = 0;
		virtual IDropDownList* AddDropDownList(bool addTextEditor) = 0;
		virtual IFilePathEditor* AddFilePathEditor(EFilePathType pathType) = 0;
		virtual IListWidget* AddListWidget() = 0;
		virtual IReportWidget* AddReportWidget(IReportWidgetEvent& eventHandler) = 0;

		// Gives number of pixels from LHS of the list to the editor column
		virtual int NameSpan() const = 0;
	};

	IVariableList* CreateVariableList(IWidgetSet& widgets, IPingPathResolver& resolver);

	ROCOCO_INTERFACE IToolbar : public IGuiWidget
	{
		// Tells the toolbar that the specified widget will manage its own layout
		// Otherwise the toolbar lays out the widget to the right of its predecessor
		virtual void SetManualLayout(IGuiWidget * widget) = 0;

	// The spacing between each widget in the toolbar
	virtual void SetSpacing(int32 firstBorder, int32 widgetSpacing) = 0;
	};

	ROCOCO_INTERFACE IDBProgress
	{
		virtual void SetProgress(float progressPercent, cstr bannerText) = 0;
	};

	ROCOCO_INTERFACE IIDEFrame : IDBProgress
	{
		virtual Windows::IWindow & Window() = 0;
		virtual void SetVisible(bool isVisible) = 0;
		virtual IWidgetSet& Children() = 0;
		operator Windows::IWindow& () { return Window(); };
		// Update child geometry. This is issued when the control is resized and also by calling SetVisible
		virtual void LayoutChildren() = 0;
		virtual ISexyStudioEventHandler& Events() = 0;
	};

	ROCOCO_INTERFACE IIDEFrameSupervisor : IIDEFrame
	{
		virtual void Free() = 0;
		virtual void SetCloseEvent(const Events::EventIdRef& evClose) = 0;
		virtual void SetResizeEvent(const Events::EventIdRef& evResize) = 0;
	};

	IIDEFrameSupervisor* CreateMainIDEFrame(WidgetContext& context, Windows::IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler);

	ROCOCO_INTERFACE IButtonWidget : IGuiWidget
	{
	};

	ILayoutSet* CreateLayoutSet();

	IButtonWidget* CreateButtonByResource(WidgetContext& context, IWidgetSet& widgets, int16 resourceId, Rococo::Events::EventIdRef evOnClick);

	struct ButtonClickContext
	{
		IButtonWidget* sourceWidget;
	};

	ROCOCO_INTERFACE IWidgetSetSupervisor : IWidgetSet
	{
		virtual void Free() = 0;
	};

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent, WidgetContext& context);

	ROCOCO_INTERFACE ISplitScreen : public IGuiWidget
	{
		virtual ISplitScreen * GetFirstHalf() = 0;
		virtual ISplitScreen* GetSecondHalf() = 0;
		virtual GuiRect GetRect() = 0;
		virtual void SplitIntoColumns(int32 firstSpan) = 0;
		virtual void SplitIntoRows(int32 firstSpan) = 0;
		virtual void Merge() = 0;
		virtual void SetBackgroundColour(RGBAb colour) = 0;
	};

	struct TooltipArgs : Rococo::Events::EventArgs
	{
		cstr text;
		Time::ticks hoverTime;
		bool useSingleLine;
		RGBAb textColour;
		RGBAb backColour;
		RGBAb borderColour;
	};

	ISplitScreen* CreateSplitScreen(IWidgetSet& widgets);

	ROCOCO_INTERFACE ITab
	{
		virtual int64 AddRef() = 0;
		virtual int64 Release() = 0;
		virtual cstr Name() const = 0;
		virtual cstr Tooltip() const = 0;
		virtual void SetName(cstr name) = 0;
		virtual void SetTooltip(cstr tooltip) = 0;
		virtual void Activate() = 0;
		virtual void Deactivate() = 0;
		virtual void Layout() = 0;
		virtual IWidgetSet& Children() = 0;
	};

	ROCOCO_INTERFACE ITabSplitter : IGuiWidget
	{
		virtual ITab & AddTab() = 0;
	};

	ITabSplitter* CreateTabSplitter(IWidgetSet& widgets);

	void AppendAncestorsToString(Windows::IWindow& window, Strings::StringBuilder& sb);
	void AppendAncestorsAndRectsToString(Windows::IWindow& window, Strings::StringBuilder& sb);
	void AppendDescendantsAndRectsToString(Windows::IWindow& window, Strings::StringBuilder& sb);

	const char* GetChildClassName();
	HFONT SetFont(int size, cstr name, HFONT oldFont, HWND hTarget);

	struct ILayout;
	struct IWin32WindowMessageLoopHandler;
	struct IWindowMessageHandler;
	struct IToolbar;
	struct ILayoutSet;
	struct IWidgetSet;
	struct IGuiWidget;

	ROCOCO_INTERFACE IOSFont
	{
		virtual operator HFONT () = 0;
	};

	class Font : public IOSFont
	{
		HFONT hFont;
	public:
		operator HFONT () override
		{
			return hFont;
		}

		Font(LOGFONTA logFont);
		~Font();
	};

	class Brush
	{
	private:
		HBRUSH hBrush = nullptr;

	public:
		operator HBRUSH() { return hBrush; }

		Brush& operator = (COLORREF color)
		{
			if (hBrush)
			{
				DeleteObject(hBrush);
			}

			hBrush = CreateSolidBrush(color);
			return *this;
		}

		~Brush()
		{
			if (hBrush) DeleteObject(hBrush);
		}
	};

	class Pen
	{
	private:
		HPEN hPen = nullptr;

	public:
		operator HPEN() { return hPen; }

		Pen& operator = (COLORREF color)
		{
			if (hPen)
			{
				DeleteObject(hPen);
			}

			hPen = CreatePen(PS_SOLID, 1, color);
			return *this;
		}

		~Pen()
		{
			if (hPen) DeleteObject(hPen);
		}
	};

	ROCOCO_INTERFACE IWin32WindowMessageLoopHandler
	{
		virtual LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	ROCOCO_INTERFACE IWindowMessageHandler: IWin32WindowMessageLoopHandler
	{
		virtual void Show() = 0;
		virtual void Free() = 0;
	};

	class Win32ChildWindow;
	class Win32PopupWindow;

	struct Win32WindowContext;

	class Win32ChildWindow: public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler, DWORD extraStyleFlags = 0);
		~Win32ChildWindow();
		operator HWND() const override {return hWnd; }
	};

	class Win32PopupWindow : public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32PopupWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler, DWORD extraStyleFlags = 0);
		~Win32PopupWindow();
		operator HWND() const override { return hWnd; }
	};

	class Win32TopLevelWindow : public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32TopLevelWindow(DWORD exStyle, DWORD style, IWin32WindowMessageLoopHandler& handler, IWindow& topLevelWindow);
		~Win32TopLevelWindow();
		operator HWND() const override { return hWnd; }
	};

	void SetLastMessageError(IException& ex);
	void AssertNoMessageError();

	HINSTANCE GetMainInstance();

	void SetPosition(Windows::IWindow& window, const GuiRect& rect);
	Vec2i GetSpan(Windows::IWindow& window);

	void InitStudioWindows(HINSTANCE hInstance, cstr iconLarge, cstr iconSmall);

	IToolbar* CreateToolbar(IWidgetSet& widgets);

	enum { WM_IDE_RESIZED = 0x8001 };

	struct HWNDProxy: Rococo::Windows::IWindow
	{
		HWND hWnd;
		HWNDProxy(HWND _hWnd = nullptr) : hWnd(_hWnd) {}
		operator HWND() const override { return hWnd;  }
	};

	ROCOCO_INTERFACE IWin32Painter
	{
		virtual void OnPaint(HDC dc) = 0;
	};

	void PaintDoubleBuffered(HWND hWnd, IWin32Painter& paint);
}

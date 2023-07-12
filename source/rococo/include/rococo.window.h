#ifndef Rococo_WINDOW_EX_H
#define Rococo_WINDOW_EX_H

#include <rococo.api.h>
#include <rococo.visitors.h>
#include <Rococo.ui.h>

#ifndef ROCOCO_WINDOWS_API
# define ROCOCO_WINDOWS_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	struct MenuCommand;

	namespace Windows
	{
		ROCOCO_INTERFACE IWindow
		{
		   virtual operator HWND () const = 0;
		};

		ROCOCO_API void PopulateStackView(HWND hStackView, Rococo::IException& ex);
		ROCOCO_API void SetStackViewColumns(HWND hStackView, const int columnWidths[5]);

		struct ExceptionDialogSpec
		{
			HINSTANCE dllInstance; // dll where the dialog template is defined. Ensure LoadLibraryA(TEXT("Riched20.dll")); is called if the dialog supports a rich editor box for the log view
			int widths[5]; // column widths for the stackview
			cstr dialogTemplate; // Typically (int) IDD_EXCEPTION_DIALOG from <rococo.win32.resources.h>
			uint16 stackViewId; // Typically (int) IDC_STACKVIEW from <rococo.win32.resources.h>
			uint16 logViewId; // Typically (int) IDC_LOGVIEW from <rococo.win32.resources.h>
			cstr title; // Dialog title
		};

		ROCOCO_WINDOWS_API void ShowExceptionDialog(const ExceptionDialogSpec& spec, HWND parent, IException& ex);

		ROCOCO_WINDOWS_API void SetControlFont(HWND hControlWindow); // Sets the font of the window to the default control font specified in InitRococoWindows
		ROCOCO_WINDOWS_API void SetTitleFont(HWND hTitleBar);  // Sets the font of the window to the default title font specified in InitRococoWindows

		ROCOCO_INTERFACE IWin32Menu
		{
			virtual operator HMENU () = 0;
			virtual IWin32Menu& AddPopup(cstr name) = 0; // Returns the popup with the given name, or creates a new one if it was not found
			virtual void AddString(cstr name, UINT_PTR id, cstr keyCmd = nullptr) = 0;
			virtual void Free() = 0;
		};

		ROCOCO_WINDOWS_API IWin32Menu* CreateMenu(bool contextMenu);

		ROCOCO_WINDOWS_API IWindow& NullParent();

		ROCOCO_INTERFACE ICommandTarget
		{
			virtual void OnAcceleratorCommand(DWORD id) = 0;
			virtual void OnMenuCommand(DWORD id) = 0;
		};

		typedef DWORD ControlId;

		ROCOCO_INTERFACE IControlCommandTarget
		{
			virtual LRESULT OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode) = 0;
		};

		enum RESIZE_TYPE : int32
		{
			RESIZE_TYPE_MAXHIDE = SIZE_MAXHIDE,
			RESIZE_TYPE_MAXSHOW = SIZE_MAXSHOW,
			RESIZE_TYPE_MAXIMIZED = SIZE_MAXIMIZED,
			RESIZE_TYPE_RESTORED = SIZE_RESTORED,
			RESIZE_TYPE_MINIMIZED = SIZE_MINIMIZED
		};

		enum SCROLL_BAR_MESSAGE : int32
		{
			SCROLL_BAR_MESSAGE_LEFT = SB_LEFT,
			SCROLL_BAR_MESSAGE_RIGHT = SB_RIGHT,
			SCROLL_BAR_MESSAGE_LINELEFT = SB_LINELEFT,
			SCROLL_BAR_MESSAGE_LINERIGHT = SB_LINERIGHT,
			SCROLL_BAR_MESSAGE_PAGELEFT = SB_PAGELEFT,
			SCROLL_BAR_MESSAGE_PAGERIGHT = SB_PAGERIGHT,
			SCROLL_BAR_MESSAGE_THUMBPOSITION = SB_THUMBPOSITION,
			SCROLL_BAR_MESSAGE_THUMBTRACK = SB_THUMBTRACK
		};

		enum MOUSE_BUTTON : int32
		{
			MOUSE_BUTTON_LEFT,
			MOUSE_BUTTON_RIGHT,
			MOUSE_BUTTON_MIDDLE
		};

		inline RECT& ToRECT(GuiRect& rect) { return reinterpret_cast<RECT&>(rect); }
		inline const RECT& ToRECT(const GuiRect& rect) { return reinterpret_cast<const RECT&>(rect); }
		inline GuiRect& FromRECT(RECT& rect) { return reinterpret_cast<GuiRect&>(rect); }
		inline const GuiRect& FromRECT(const RECT& rect) { return reinterpret_cast<const GuiRect&>(rect); }

		// InitRococoWindows should be called once in the DLL or EXE in which Rococo::Windows are created.
		// If titleFont is non null, then it will overwrite the default titlefont of Courier New 11pt for all control title bars
		// Use LoadImage or LoadIcon to generate the correct HICONs. HINSTANCE comes from the appropriate WinMain or DllMain argument
		// If controlFont is non null then it creates the default font used in Rococo::Window controls
		ROCOCO_WINDOWS_API void InitRococoWindows(HINSTANCE hInstance, HICON hLargeIcon, HICON hSmallIcon, const LOGFONTA* titleFont, const LOGFONTA* controlFont);

		ROCOCO_WINDOWS_API void ShowEditorError(HWND parent, cstr format, ...);

		ROCOCO_INTERFACE IItemRenderer
		{
			virtual void OnDrawItem(DRAWITEMSTRUCT& dis) = 0;
			virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis) = 0;
		};

		struct IListWindowSupervisor;

		ROCOCO_INTERFACE IListItemHandler : public IItemRenderer
		{
			virtual void OnItemSelectionChanged(IListWindowSupervisor& listWindow) = 0;
		};

		struct IWindowSupervisor;
		struct IParentWindowSupervisor;
		struct IDialogSupervisor;
		struct IModalControl;

		struct WindowConfig
		{
			DWORD exStyle;
			DWORD style;
			cstr windowName;
			int32 left;
			int32 top;
			int32 width;
			int32 height;
			HWND hWndParent;
			HMENU hMenu;
		};

		ROCOCO_WINDOWS_API void SetChildWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle);
		ROCOCO_WINDOWS_API void SetPopupWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);
		ROCOCO_WINDOWS_API void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& span, int32 showWindowCommand, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);
		ROCOCO_WINDOWS_API void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& topLeft, const Vec2i& span, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);

		ROCOCO_INTERFACE IWindowHandler
		{
			virtual void OnPretranslateMessage(MSG& msg) = 0;
			virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
		};

		class StandardWindowHandler : public IWindowHandler
		{
		private:
			COLORREF backgroundColour;

		public:
			ROCOCO_WINDOWS_API StandardWindowHandler();

			ROCOCO_WINDOWS_API virtual void SetBackgroundColour(COLORREF bkColour);
		protected:
			ROCOCO_WINDOWS_API virtual void OnDestroy(HWND hWnd);
			ROCOCO_WINDOWS_API virtual LRESULT OnInput(HWND hWnd, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API virtual LRESULT OnKeydown(HWND hWnd, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API virtual LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API virtual void OnClose(HWND hWnd);
			ROCOCO_WINDOWS_API LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API void OnEraseBackground(HWND hWnd, HDC dc);
			ROCOCO_WINDOWS_API virtual void OnPaint(HWND hWnd, PAINTSTRUCT& ps, HDC hdc);
			ROCOCO_WINDOWS_API virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type);
			ROCOCO_WINDOWS_API virtual void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO& info);
			ROCOCO_WINDOWS_API virtual LRESULT OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode); // If unhandled, call the base method
			ROCOCO_WINDOWS_API virtual void OnMenuCommand(HWND hWnd, DWORD id);
			ROCOCO_WINDOWS_API virtual void OnAcceleratorCommand(HWND hWnd, DWORD id);
			ROCOCO_WINDOWS_API virtual COLORREF GetBackgroundColour();
			ROCOCO_WINDOWS_API virtual LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam);
			ROCOCO_WINDOWS_API virtual void OnPretranslateMessage(MSG& msg);
		};

		typedef void(*FN_OnControlCommand)(void* context, HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode);
		typedef void(*FN_OnMenuCommand)(void* context, HWND hWnd, DWORD id);
		typedef void(*FN_OnAcceleratorCommand)(void* context, HWND hWnd, DWORD id);
		typedef void(*FN_OnClose)(void* context, HWND hWnd);
		typedef void(*FN_OnSize)(void* context, HWND hWnd, const Vec2i& span, RESIZE_TYPE type);
		typedef DWORD(*FN_OnIdle)(void* context);
		typedef void(*FN_OnPreTranslate)(void* context, MSG& msg);

		ROCOCO_INTERFACE IWiredWindowHandler : public IWindowHandler
		{
			virtual void Free() = 0;
			virtual DWORD OnIdle() = 0;
			virtual void RouteClose(void* context, FN_OnClose fn) = 0;
			virtual void RouteControlCommand(void* context, FN_OnControlCommand f) = 0;
			virtual void RouteIdle(void* context, FN_OnIdle f) = 0;
			virtual void RouteMenuCommand(void* context, FN_OnMenuCommand f) = 0;
			virtual void RouteAcceleratorCommand(void* context, FN_OnAcceleratorCommand f) = 0;
			virtual void RouteSize(void* context, FN_OnSize fn) = 0;
			virtual void RoutePreTranslate(void* context, FN_OnPreTranslate fn) = 0;
		};

		ROCOCO_WINDOWS_API IWiredWindowHandler* CreateWiredHandler();

		class WiredWindowHandler
		{
			AutoFree<IWiredWindowHandler> inner;
		public:
			WiredWindowHandler() : inner(CreateWiredHandler()) {}
			IWiredWindowHandler* operator -> () { return inner; }
			IWiredWindowHandler& operator * () { return *inner; }
			operator IWiredWindowHandler*() { return inner; }
		};

		ROCOCO_INTERFACE IModalControl
		{
			virtual void OnEnterModal() = 0; // Called when a BlockModal call begins
			virtual DWORD OnExitModal() = 0; // Called when a BlockModal call quits, and returns the exit code of the BlockModal call
			virtual bool IsRunning() = 0; // Returns true as long as the modal dialog is required to run
			virtual DWORD OnIdle() = 0; // Executes when no windows messages queued. Returns maximum timeout period (in milliseconds) before next idle processing
		};

		class ModalDialogHandler : private IModalControl
		{
		private:
			WiredWindowHandler wiredHandler;
			bool isRunning;
			DWORD exitCode;

			virtual bool IsRunning();
			virtual DWORD OnIdle();
			virtual void OnEnterModal();
			virtual DWORD OnExitModal();

		public:
			ROCOCO_WINDOWS_API ModalDialogHandler();

			ROCOCO_WINDOWS_API IDialogSupervisor* CreateDialogWindow(const WindowConfig& config);

			IModalControl& ModalControl() { return *this; }
			IWiredWindowHandler& Router() { return *wiredHandler; }

			ROCOCO_WINDOWS_API void TerminateDialog(DWORD exitCode); // Called by the host during a modal call to terminate the modal call and return the given exit code.

			ROCOCO_WINDOWS_API DWORD BlockModal(IDialogSupervisor& window, HWND hWndOwner); // Blocks the current thread until the modal dialog completes, and returns the exit code
		};

		ROCOCO_WINDOWS_API HWND CreateWindowIndirect(cstr className, const WindowConfig& c, IWindowHandler* handler);

		ROCOCO_WINDOWS_API bool ModalQuery(HWND hParentWnd, cstr caption, cstr question);

		ROCOCO_WINDOWS_API GuiRect ClientArea(HWND hWnd);
		ROCOCO_WINDOWS_API GuiRect WindowArea(HWND hWnd);

		ROCOCO_INTERFACE IWindowSupervisor : public IWindow
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
		};

		ROCOCO_INTERFACE IButton : public IWindowSupervisor
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
		};

		ROCOCO_INTERFACE ICheckbox : public IWindowSupervisor
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
			virtual Visitors::CheckState GetCheckState() const = 0;
			virtual void SetCheckState(Visitors::CheckState state) = 0;
		};

		ROCOCO_INTERFACE IRichEditorEvents
		{
			virtual LRESULT OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
			virtual void OnRightButtonUp(const Vec2i& clientPosition) = 0;
		};

		ROCOCO_INTERFACE IRichEditor : public IWindowSupervisor
		{
			virtual void AppendText(COLORREF foreground, COLORREF background, cstr text, size_t nChars = (size_t)-1) = 0;
			virtual HWND EditorHandle() const = 0;
			virtual void Hilight(const Vec2i& start, const Vec2i& end, RGBAb background, RGBAb foreground) = 0;
			virtual int32 LineCount() const = 0;
			virtual void ResetContent() = 0;
			virtual int32 GetFirstVisibleLine() const = 0;
			virtual void ScrollTo(int32 lineNumber) = 0;
			virtual void SetTooltip(cstr name, cstr text) = 0;
		};

		ROCOCO_WINDOWS_API bool OpenChooseFontBox(HWND hParent, LOGFONTA& output);
		ROCOCO_WINDOWS_API void SetDlgCtrlID(HWND hWnd, DWORD id);
		ROCOCO_WINDOWS_API void SetText(HWND hWnd, size_t capacity, cstr format, ...);

		ROCOCO_INTERFACE IParentWindowSupervisor : public IWindowSupervisor
		{
			virtual IWindowSupervisor* AddChild(const WindowConfig& childConfig, cstr className, ControlId id) = 0;
			virtual IParentWindowSupervisor* AddChild(const WindowConfig& childConfig, ControlId id, IWindowHandler* windowHandler) = 0;
		};

		ROCOCO_INTERFACE IDialogSupervisor : public IParentWindowSupervisor
		{
			virtual DWORD BlockModal(IModalControl& control, HWND ownerWindow, IWindowHandler* subHandler) = 0;
		};

		struct ITreeControlSupervisor;

		ROCOCO_INTERFACE ITreeControlHandler
		{
			virtual void OnItemSelected(int64 id, ITreeControlSupervisor& origin) = 0;
		};

		ROCOCO_INTERFACE IListViewEvents : public IItemRenderer
		{
			virtual void OnItemChanged(int index) = 0;
		};

		ROCOCO_INTERFACE ITreeControlSupervisor : public IWindowSupervisor
		{
			virtual Visitors::IUITree& Tree() = 0;
			virtual Visitors::CheckState GetCheckState(Visitors::TREE_NODE_ID id) const = 0;
			virtual HWND TreeHandle() const = 0;
		};

		ROCOCO_INTERFACE IListWindowSupervisor : public IWindowSupervisor
		{
			virtual int AddString(cstr data) = 0;
			virtual int GetCurrentSelection() = 0;
			virtual LRESULT GetItemData(int index) = 0;
			virtual bool GetString(int index, char* data, size_t capacity) = 0;
			virtual void ResetContent() = 0;
			virtual void SetCurrentSelection(int index) = 0;
			virtual HWND ListBoxHandle() const = 0;
		};

		ROCOCO_INTERFACE IListViewSupervisor : public IWindowSupervisor
		{
			virtual HWND ListViewHandle() const = 0;
			virtual Visitors::IUIList& UIList() = 0;
			virtual operator Visitors::IUIList& () = 0;
		};

		ROCOCO_INTERFACE IComboBoxSupervisor : public IWindowSupervisor
		{
			virtual int AddString(cstr text) = 0;
			virtual int FindString(cstr text) = 0;
			virtual bool GetString(int index, char* buffer, size_t capacity) = 0;
			virtual int GetCurrentSelection() = 0;
			virtual void SetCurrentSelection(int index) = 0;
		};

		ROCOCO_INTERFACE ITrackBarSupervisor : public IWindowSupervisor
		{
			virtual void SetRange(int mininma, int maxima) = 0;
			virtual void SetPageSize(int pageSize) = 0;
			virtual void SetPosition(int pos) = 0;
			virtual HWND TrackerHandle() const = 0;
		};

		ROCOCO_INTERFACE ITrackBarHandler
		{
			virtual void OnMove(int position) = 0;
		};

		ROCOCO_INTERFACE ITabControlEvents
		{
			virtual void OnSelectionChanged(int index) = 0;
			virtual void OnTabRightClicked(int index, const POINT& screenCursorPos) = 0;
		};

		ROCOCO_INTERFACE ITabControl : public IWindowSupervisor
		{
			virtual int AddTab(cstr data, cstr tooltip) = 0;
			virtual IParentWindowSupervisor& ClientSpace() = 0;
			virtual void SetClientSpaceBackgroundColour(COLORREF colour) = 0;
			virtual int GetCurrentSelection() = 0;
			virtual void ResetContent() = 0;
			virtual void SetCurrentSelection(int index) = 0;
			virtual bool GetTabName(int index, char* buffer, DWORD capacity) const = 0;
			virtual int TabCount() const = 0;
		};

		ROCOCO_ID(IDEPANE_ID, int32, -1);

		namespace IDE
		{
			ROCOCO_INTERFACE IIDENode : public IWindow
			{
			   virtual void Free() = 0;
			   virtual void SetFont(HFONT hFont) = 0;
			};

			ROCOCO_INTERFACE ISpatialManager : public IWindow
			{
			   virtual IIDENode* FindPane(IDEPANE_ID id) = 0;
			   virtual void Free() = 0;
			   virtual void NotifyMigration(IDEPANE_ID migratingId) = 0;
			   virtual void SetFontRecursive(HFONT hFont) = 0;
			   virtual void Save(const LOGFONTA& logFont, int32 version) = 0;
			};

			ROCOCO_INTERFACE IPaneDatabase
			{
			   virtual IDEPANE_ID GetMigratingId() = 0;
			   virtual void SetMigratingId(IDEPANE_ID) = 0;
			   virtual void NotifyMigration() = 0;
			   virtual void GetName(char name[256], IDEPANE_ID id) = 0;
			   virtual IIDENode* ConstructPane(IDEPANE_ID id, IParentWindowSupervisor& parent) = 0;
			};

			ROCOCO_INTERFACE IIDETextWindow : public IIDENode
			{
			   virtual void AddSegment(RGBAb colour, cstr segment, size_t length, RGBAb bkColor) = 0;
			   virtual IRichEditor& Editor() = 0;
			   virtual void AddContextMenuItem(cstr key, const uint8* command, size_t lenOfCommand) = 0;
			   virtual void SetEventCallback(IEventCallback<MenuCommand>* eventCallback) = 0;
			};

			ROCOCO_INTERFACE IIDETreeWindow : public IIDENode
			{
			   virtual ITreeControlSupervisor& GetTreeSupervisor() = 0;
			};

			ROCOCO_INTERFACE IIDEReportWindow : public IIDENode
			{
			   virtual IListViewSupervisor& GetListViewSupervisor() = 0;
			};

			ROCOCO_WINDOWS_API IIDETextWindow* CreateTextWindow(IWindow& parent);
			ROCOCO_WINDOWS_API IIDETreeWindow* CreateTreeView(IWindow& parent, ITreeControlHandler* handler);
			ROCOCO_WINDOWS_API IIDEReportWindow* CreateReportView(IWindow& parent, IListViewEvents& eventHandler);
			ROCOCO_WINDOWS_API ISpatialManager* LoadSpatialManager(IWindow& parent, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, LOGFONTA& logFont, cstr appName);
		}

		ROCOCO_WINDOWS_API IButton* AddPushButton(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API ICheckbox* AddCheckBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API IWindowSupervisor* AddLabel(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API IWindowSupervisor* AddEditor(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API ITreeControlSupervisor* AddTree(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITreeControlHandler& eventHandler, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API IListViewSupervisor* AddListView(IWindow& parent, const GuiRect& rect, cstr name, IListViewEvents& eventHandler, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		ROCOCO_WINDOWS_API IRichEditor* AddRichEditor(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, IRichEditorEvents& eventHandler, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API ITabControl* AddTabs(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITabControlEvents& eventHandler, DWORD style, DWORD styleEx = 0);
		ROCOCO_WINDOWS_API IListWindowSupervisor* AddListbox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, IListItemHandler& handler, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		ROCOCO_WINDOWS_API IComboBoxSupervisor* AddComboBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		ROCOCO_WINDOWS_API IDialogSupervisor* CreateDialogWindow(const WindowConfig& config, IWindowHandler* handler);
		ROCOCO_WINDOWS_API IParentWindowSupervisor* CreateChildWindow(const WindowConfig& config, IWindowHandler* handler);
		ROCOCO_WINDOWS_API ITrackBarSupervisor* AddTrackbar(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx, ITrackBarHandler& handler);
	} // Windows

	namespace Flow
	{
		struct IFlowGraph;
		struct IFlowGraphEditor;
		struct IFlowGraphEventHandler;
		struct INodeFactories;
		ROCOCO_WINDOWS_API IFlowGraphEditor* CreateFlowGraphEditor(HWND hWndParent, cstr title, const Vec2i& span, const Vec2i& position, IFlowGraphEventHandler& eventHandler, IFlowGraph& graph, INodeFactories& factories);
	}
} // Rococo

#endif // Rococo_WINDOW_EX_H

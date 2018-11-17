#ifndef Rococo_WINDOW_EX_H
#define Rococo_WINDOW_EX_H

#include <rococo.api.h>
#include <rococo.visitors.h>
#include <Rococo.ui.h>

namespace Rococo
{
	namespace Windows
	{
		ROCOCOAPI IWindow
		{
		   virtual operator HWND () const = 0;
		};

		void PopulateStackView(HWND hStackView, Rococo::IException& ex);
		void SetStackViewColumns(HWND hStackView, const int columnWidths[5]);

		struct ExceptionDialogSpec
		{
			HINSTANCE dllInstance; // dll where the dialog template is defined
			int widths[5]; // column widths for the stackview
			cstr dialogTemplate; // Typically (int) IDD_EXCEPTION_DIALOG from <rococo.win32.resources.h>
			uint16 stackViewId; // Typically (int) IDC_STACKVIEW from <rococo.win32.resources.h>
			uint16 logViewId; // Typically (int) IDC_LOGVIEW from <rococo.win32.resources.h>
			cstr title; // Dialog title
		};

		void ShowExceptionDialog(const ExceptionDialogSpec& spec, HWND parent, IException& ex);

		void SetControlFont(HWND hControlWindow); // Sets the font of the window to the default control font specified in InitRococoWindows
		void SetTitleFont(HWND hTitleBar);  // Sets the font of the window to the default title font specified in InitRococoWindows

		ROCOCOAPI IWin32Menu
		{
			virtual operator HMENU () = 0;
			virtual IWin32Menu& AddPopup(cstr name) = 0; // Returns the popup with the given name, or creates a new one if it was not found
			virtual void AddString(cstr name, UINT_PTR id, cstr keyCmd = nullptr) = 0;
			virtual void Free() = 0;
		};

		IWin32Menu* CreateMenu(bool contextMenu);

		IWindow& NullParent();

		ROCOCOAPI ICommandTarget
		{
			virtual void OnAcceleratorCommand(DWORD id) = 0;
			virtual void OnMenuCommand(DWORD id) = 0;
		};

		typedef DWORD ControlId;

		ROCOCOAPI IControlCommandTarget
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
		void InitRococoWindows(HINSTANCE hInstance, HICON hLargeIcon, HICON hSmallIcon, const LOGFONTA* titleFont, const LOGFONTA* controlFont);

		void ShowEditorError(HWND parent, cstr format, ...);

		ROCOCOAPI IItemRenderer
		{
			virtual void OnDrawItem(DRAWITEMSTRUCT& dis) = 0;
			virtual void OnMeasureItem(MEASUREITEMSTRUCT& mis) = 0;
		};

		struct IListWindowSupervisor;

		ROCOCOAPI IListItemHandler : public IItemRenderer
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

		void SetChildWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle);
		void SetPopupWindowConfig(WindowConfig& config, const GuiRect& rect, HWND hWndParent, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);
		void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& span, int32 showWindowCommand, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);
		void SetOverlappedWindowConfig(WindowConfig& config, const Vec2i& topLeft, const Vec2i& span, HWND hWndOwner, cstr name, DWORD style, DWORD exStyle, HMENU hPopupMenu = nullptr);

		ROCOCOAPI IWindowHandler
		{
		 virtual void OnPretranslateMessage(MSG& msg) = 0;
			virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;
		};

		class StandardWindowHandler : public IWindowHandler
		{
		private:
			COLORREF backgroundColour;

		public:
			StandardWindowHandler();

			virtual void SetBackgroundColour(COLORREF bkColour);
		protected:
			virtual void OnDestroy(HWND hWnd);
			virtual LRESULT OnInput(HWND hWnd, WPARAM wParam, LPARAM lParam);
			virtual LRESULT OnKeydown(HWND hWnd, WPARAM wParam, LPARAM lParam);
			virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			virtual LRESULT OnTimer(HWND hWnd, WPARAM wParam, LPARAM lParam);
			virtual void OnClose(HWND hWnd);
			virtual LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam);
			virtual void OnEraseBackground(HWND hWnd, HDC dc);
			virtual void OnPaint(HWND hWnd, PAINTSTRUCT& ps, HDC hdc);
			virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type);
			virtual void OnGetMinMaxInfo(HWND hWnd, MINMAXINFO& info);
			virtual LRESULT OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode); // If unhandled, call the base method
			virtual void OnMenuCommand(HWND hWnd, DWORD id);
			virtual void OnAcceleratorCommand(HWND hWnd, DWORD id);
			virtual COLORREF GetBackgroundColour();
			virtual LRESULT OnSetCursor(HWND hWnd, WPARAM wParam, LPARAM lParam);
			virtual void OnPretranslateMessage(MSG& msg);
		};

		typedef void(*FN_OnControlCommand)(void* context, HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode);
		typedef void(*FN_OnMenuCommand)(void* context, HWND hWnd, DWORD id);
		typedef void(*FN_OnAcceleratorCommand)(void* context, HWND hWnd, DWORD id);
		typedef void(*FN_OnClose)(void* context, HWND hWnd);
		typedef void(*FN_OnSize)(void* context, HWND hWnd, const Vec2i& span, RESIZE_TYPE type);
		typedef DWORD(*FN_OnIdle)(void* context);
		typedef void(*FN_OnPreTranslate)(void* context, MSG& msg);

		ROCOCOAPI IWiredWindowHandler : public IWindowHandler
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

		IWiredWindowHandler* CreateWiredHandler();

		class WiredWindowHandler
		{
			AutoFree<IWiredWindowHandler> inner;
		public:
			WiredWindowHandler() : inner(CreateWiredHandler()) {}
			IWiredWindowHandler* operator -> () { return inner; }
			IWiredWindowHandler& operator * () { return *inner; }
			operator IWiredWindowHandler*() { return inner; }
		};

		ROCOCOAPI IModalControl
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
			ModalDialogHandler();

			IDialogSupervisor* CreateDialogWindow(const WindowConfig& config);

			IModalControl& ModalControl() { return *this; }
			IWiredWindowHandler& Router() { return *wiredHandler; }

			void TerminateDialog(DWORD exitCode); // Called by the host during a modal call to terminate the modal call and return the given exit code.

			DWORD BlockModal(IDialogSupervisor& window, HWND hWndOwner); // Blocks the current thread until the modal dialog completes, and returns the exit code
		};

		HWND CreateWindowIndirect(cstr className, const WindowConfig& c, IWindowHandler* handler);

		bool ModalQuery(HWND hParentWnd, cstr caption, cstr question);

		GuiRect ClientArea(HWND hWnd);
		GuiRect WindowArea(HWND hWnd);

		ROCOCOAPI IWindowSupervisor : public IWindow
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
		};

		ROCOCOAPI IButton : public IWindowSupervisor
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
		};

		ROCOCOAPI ICheckbox : public IWindowSupervisor
		{
			virtual void Free() = 0;
			virtual IWindowHandler& Handler() = 0;
			virtual Visitors::CheckState GetCheckState() const = 0;
			virtual void SetCheckState(Visitors::CheckState state) = 0;
		};

		ROCOCOAPI IRichEditorEvents
		{
			virtual void OnRightButtonUp(const Vec2i& clientPosition) = 0;
		};

		ROCOCOAPI IRichEditor : public IWindowSupervisor
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

		bool OpenChooseFontBox(HWND hParent, LOGFONTA& output);
		void SetDlgCtrlID(HWND hWnd, DWORD id);
		void SetText(HWND hWnd, size_t capacity, cstr format, ...);

		ROCOCOAPI IParentWindowSupervisor : public IWindowSupervisor
		{
			virtual IWindowSupervisor* AddChild(const WindowConfig& childConfig, cstr className, ControlId id) = 0;
			virtual IParentWindowSupervisor* AddChild(const WindowConfig& childConfig, ControlId id, IWindowHandler* windowHandler) = 0;
		};

		ROCOCOAPI IDialogSupervisor : public IParentWindowSupervisor
		{
			virtual DWORD BlockModal(IModalControl& control, HWND ownerWindow, IWindowHandler* subHandler) = 0;
		};

		struct ITreeControlSupervisor;

		ROCOCOAPI ITreeControlHandler
		{
		 virtual void OnItemSelected(int64 id, ITreeControlSupervisor& origin) = 0;
		};

		ROCOCOAPI IListViewEvents : public IItemRenderer
		{
		};

		ROCOCOAPI ITreeControlSupervisor : public IWindowSupervisor
		{
			virtual Visitors::IUITree& Tree() = 0;
			virtual Visitors::CheckState GetCheckState(Visitors::TREE_NODE_ID id) const = 0;
		 virtual HWND TreeHandle() const = 0;
		};

		ROCOCOAPI IListWindowSupervisor : public IWindowSupervisor
		{
			virtual int AddString(cstr data) = 0;
			virtual int GetCurrentSelection() = 0;
			virtual LRESULT GetItemData(int index) = 0;
			virtual bool GetString(int index, char* data, size_t capacity) = 0;
			virtual void ResetContent() = 0;
			virtual void SetCurrentSelection(int index) = 0;
			virtual HWND ListBoxHandle() const = 0;
		};

		ROCOCOAPI IListViewSupervisor : public IWindowSupervisor
		{
			virtual HWND ListViewHandle() const = 0;
			virtual Visitors::IUIList& UIList() = 0;
			virtual operator Visitors::IUIList& () = 0;
		};

		ROCOCOAPI IComboBoxSupervisor : public IWindowSupervisor
		{
			virtual int AddString(cstr text) = 0;
			virtual int FindString(cstr text) = 0;
			virtual bool GetString(int index, char* buffer, size_t capacity) = 0;
			virtual int GetCurrentSelection() = 0;
			virtual void SetCurrentSelection(int index) = 0;
		};

		ROCOCOAPI ITrackBarSupervisor : public IWindowSupervisor
		{
			virtual void SetRange(int mininma, int maxima) = 0;
			virtual void SetPageSize(int pageSize) = 0;
			virtual void SetPosition(int pos) = 0;
			virtual HWND TrackerHandle() const = 0;
		};

		ROCOCOAPI ITrackBarHandler
		{
			virtual void OnMove(int position) = 0;
		};

		ROCOCOAPI ITabControlEvents
		{
			virtual void OnSelectionChanged(int index) = 0;
		 virtual void OnTabRightClicked(int index, const POINT& screenCursorPos) = 0;
		};

		ROCOCOAPI ITabControl : public IWindowSupervisor
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
			ROCOCOAPI IIDENode : public IWindow
			{
			   virtual void Free() = 0;
			   virtual void SetFont(HFONT hFont) = 0;
			};

			ROCOCOAPI ISpatialManager : public IWindow
			{
			   virtual IIDENode* FindPane(IDEPANE_ID id) = 0;
			   virtual void Free() = 0;
			   virtual void NotifyMigration(IDEPANE_ID migratingId) = 0;
			   virtual void SetFontRecursive(HFONT hFont) = 0;
			   virtual void Save(const LOGFONTA& logFont, int32 version) = 0;
			};

			ROCOCOAPI IPaneDatabase
			{
			   virtual IDEPANE_ID GetMigratingId() = 0;
			   virtual void SetMigratingId(IDEPANE_ID) = 0;
			   virtual void NotifyMigration() = 0;
			   virtual void GetName(char name[256], IDEPANE_ID id) = 0;
			   virtual IIDENode* ConstructPane(IDEPANE_ID id, IParentWindowSupervisor& parent) = 0;
			};

			ROCOCOAPI IIDETextWindow : public IIDENode
			{
			   virtual void AddSegment(RGBAb colour, cstr segment, size_t length, RGBAb bkColor) = 0;
			   virtual IRichEditor& Editor() = 0;
			};

			ROCOCOAPI IIDETreeWindow : public IIDENode
			{
			   virtual ITreeControlSupervisor& GetTreeSupervisor() = 0;
			};

			ROCOCOAPI IIDEReportWindow : public IIDENode
			{
			   virtual IListViewSupervisor& GetListViewSupervisor() = 0;
			};

			IIDETextWindow* CreateTextWindow(IWindow& parent);
			IIDETreeWindow* CreateTreeView(IWindow& parent, ITreeControlHandler* handler);
			IIDEReportWindow* CreateReportView(IWindow& parent);
			ISpatialManager* LoadSpatialManager(IWindow& parent, IPaneDatabase& database, const IDEPANE_ID* idArray, size_t nPanes, UINT versionId, LOGFONTA& logFont, cstr appName);
		}

		IButton* AddPushButton(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ICheckbox* AddCheckBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		IWindowSupervisor* AddLabel(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		IWindowSupervisor* AddEditor(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx = 0);
		ITreeControlSupervisor* AddTree(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITreeControlHandler& eventHandler, DWORD style, DWORD styleEx = 0);
		IListViewSupervisor* AddListView(IWindow& parent, const GuiRect& rect, cstr name, IListViewEvents& eventHandler, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		IRichEditor* AddRichEditor(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, IRichEditorEvents& eventHandler, DWORD style, DWORD styleEx = 0);
		ITabControl* AddTabs(IWindow& parent, const GuiRect& rect, cstr name, ControlId id, ITabControlEvents& eventHandler, DWORD style, DWORD styleEx = 0);
		IListWindowSupervisor* AddListbox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, IListItemHandler& handler, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		IComboBoxSupervisor* AddComboBox(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD containerStyle, DWORD containerStyleEx);
		IDialogSupervisor* CreateDialogWindow(const WindowConfig& config, IWindowHandler* handler);
		IParentWindowSupervisor* CreateChildWindow(const WindowConfig& config, IWindowHandler* handler);
		ITrackBarSupervisor* AddTrackbar(IParentWindowSupervisor& parent, const GuiRect& rect, cstr name, ControlId id, DWORD style, DWORD styleEx, ITrackBarHandler& handler);
	} // Windows

	namespace Flow
	{
		struct IFlowGraph;
		struct IFlowGraphEditor;
		struct IFlowGraphEventHandler;
		struct INodeFactories;
		IFlowGraphEditor* CreateFlowGraphEditor(HWND hWndParent, cstr title, const Vec2i& span, const Vec2i& position, IFlowGraphEventHandler& eventHandler, IFlowGraph& graph, INodeFactories& factories);
	}
} // Rococo

#endif // Rococo_WINDOW_EX_H

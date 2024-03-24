#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.api.h>
#include <rococo.strings.h>
#include <rococo.window.h>
#include <rococo.maths.h>

#include <commdlg.h>
#include <Windowsx.h>
#include <stdio.h>
#include <vector>

#include <rococo.variable.editor.h>

#include <CommCtrl.h>

namespace Rococo
{
   namespace Windows
   {
      void SetControlFont(HWND hControlWindow);
   }
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Strings;
	using namespace Rococo::Windows;

	struct VariantTextBuffer
	{
		DWORD capacity;
		char* text;
	};

	union UVariant
	{
		int int32Value;
		unsigned int uint32Value;
		float floatValue;
		double doubleValue;
		VariantTextBuffer textValue;
	};

	enum EVariantType
	{
		EVariantType_None,
		EVariantType_Bool,
		EVariantType_Int32,
		EVariantType_Uint32,
		EVariantType_Float,
		EVariantType_Double,
		EVariantType_String
	};

	struct Variant
	{
		UVariant value;
		EVariantType type;
	};

	void FormatWithVariant(char* desc, size_t capacity, const Variant& var)
	{
		StackStringBuilder sb(desc, capacity);

		switch (var.type)
		{
		case EVariantType_Int32:
			sb << var.value.int32Value;
			break;
		case EVariantType_Bool:
			sb << var.value.int32Value;
			break;
		case EVariantType_Uint32:
			sb << var.value.uint32Value;
			break;
		case EVariantType_Float:
			sb << var.value.floatValue;
			break;
		case EVariantType_Double:
			sb << var.value.doubleValue;
			break;
		case EVariantType_String:
			sb << var.value.textValue.text;
			break;
		default:
			sb << "? Unknown ?";
		}
	}

	struct VariableDesc
	{
		enum { NAME_CAPACITY = 64 };
		enum { TAB_NAME_CAPACITY = 256 };
		IRichEditor* ErrorMessage;
		IWindowSupervisor* StaticControl;
		IWindowSupervisor* EditControl;
		IComboBoxSupervisor* ComboControl;
		ICheckbox* CheckBox;
		IWindowSupervisor* SpecialButtonControl;
		Variant var;
		UVariant minimum;
		UVariant maximum;
		cstr filter;
		IStringValidator* validator;
		ISelection* selection;
		char name[NAME_CAPACITY];
		char tabName[TAB_NAME_CAPACITY];
	};

	void ParseInt32(VariableDesc& v, cstr s)
	{
		if (*s == 0)
		{
			v.var.value.int32Value = 0;
		}

		if (rlen(s) > 10)
		{
			if (s[0] == '-')
			{
				v.var.value = v.minimum;
			}
			else
			{
				v.var.value = v.maximum;
			}
		}
		else
		{
			_snscanf_s(s, 10, "%d", &v.var.value.int32Value);
		}

		if (v.var.value.int32Value < v.minimum.int32Value)
		{
			v.var.value = v.minimum;
		}
		else if (v.var.value.int32Value > v.maximum.int32Value)
		{
			v.var.value = v.maximum;
		}
	}

	void ParseVariable(VariableDesc& v, cstr s)
	{
		switch (v.var.type)
		{
		case EVariantType_Bool:
		case EVariantType_Int32:
			ParseInt32(v, s);
			break;
		case EVariantType_String:
		{
			StackStringBuilder sb(v.var.value.textValue.text, v.var.value.textValue.capacity);
			sb << s;
		}
		break;
		default:
			Throw(0, "Var type %d Not implemented for parsering.", v.var.type);
		}
	}

	static ATOM customAtom = 0;

	class TooltipPopupWindow : StandardWindowHandler
	{
		HWND hWnd = nullptr;
		
		TooltipPopupWindow()
		{

		}

		virtual ~TooltipPopupWindow()
		{

		}

		void PostConstruct(HWND hParentWnd)
		{
			hWnd = hParentWnd;
		}
	public:
		static TooltipPopupWindow* Create(HWND hParentWnd)
		{
			AutoFree<TooltipPopupWindow> popup = new TooltipPopupWindow();
			popup->PostConstruct(hParentWnd);
			return popup.Detach();
		}

		void Free()
		{
			delete this;
		}
	};

	enum
	{ 
		VariableEditEditorProcId = 42, 
		VariableEditButtonProcId = 43,
		WM_NAVIGATE_BY_TAB = WM_USER + 1,
	};

	HWND GetAncestorWithId(HWND hChildWnd, ControlId targetId)
	{
		HWND hWnd = hChildWnd;
		while (hWnd != nullptr)
		{
			auto id = GetWindowLongPtrA(hWnd, GWL_ID);
			if (targetId == id)
			{
				return hWnd;
			}

			hWnd = GetParent(hWnd);
		}

		return nullptr;
	}

	enum { NAV_RETURN = 2, NAV_FORWARD = 1, NAV_BACK = -1 };

	static LRESULT VariableEditEditorProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		UNUSED(dwRefData);

		if (uIdSubclass == VariableEditEditorProcId)
		{
			switch (uMsg)
			{
			case WM_KEYUP:
				if (wParam == VK_RETURN)
				{
					HWND hVariableEditorWnd = GetAncestorWithId(hWnd, 0);
					if (hVariableEditorWnd)
					{
						PostMessage(hVariableEditorWnd, WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), NAV_RETURN);
					}
					return 0L;
				}

				if (wParam == VK_TAB || wParam == VK_DOWN)
				{
					HWND hVariableEditorWnd = GetAncestorWithId(hWnd, 0);
					if (hVariableEditorWnd)
					{
						PostMessage(hVariableEditorWnd, WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), NAV_FORWARD);
					}
					return 0L;
				}

				if (wParam == VK_UP)
				{
					HWND hVariableEditorWnd = GetAncestorWithId(hWnd, 0);
					if (hVariableEditorWnd)
					{
						PostMessage(hVariableEditorWnd, WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), NAV_BACK);
					}
					return 0L;
				}
				return 0L;
			case WM_KEYDOWN:
			{
				if (wParam == VK_TAB || wParam == VK_RETURN)
				{
					return 0L;
				}
				break;
			}
			case WM_CHAR:
				if (wParam == VK_TAB || wParam == VK_RETURN)
				{
					return 0L;
				}
				break;
			}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	class VariableEditor : public IVariableEditor, private ITabControlEvents, private StandardWindowHandler, public IRichEditorEvents
	{
	private:
		int nextX;
		int nextY;
		Vec2i windowSpan;
		int labelWidth;
		Vec2i initialTopLeft{ 0,0 };

		typedef std::vector<VariableDesc> TVariables;
		TVariables variables;

		HWND hwndOwner;
		IDialogSupervisor* supervisor;
		IButton* okButton;
		IButton* cancelButton;
		ITabControl* tabControl;
		IParentWindowSupervisor* tab;

		ModalDialogHandler dlg;
		ControlId nextId;

		IVariableEditorEventHandler* eventHandler;

		LRESULT RichEditor_OnCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, IRichEditor& origin) override
		{
			UNUSED(origin);
			return DefWindowProcA(hWnd, uMsg, wParam, lParam);
		}

		void RichEditor_OnRightButtonUp(const Vec2i& clientPosition, IRichEditor& origin) override
		{
			UNUSED(clientPosition);
			UNUSED(origin);
		}

		void OnSelectionChanged(int index) override
		{
			char tabName[VariableDesc::TAB_NAME_CAPACITY];
			if (tabControl->GetTabName(index, tabName, VariableDesc::TAB_NAME_CAPACITY))
			{
				for (VariableDesc& v : variables)
				{
					UINT vis = strcmp(v.tabName, tabName) == 0 ? SW_SHOW : SW_HIDE;
					if (v.ComboControl) ShowWindow(*v.ComboControl, vis);
					if (v.EditControl) ShowWindow(*v.EditControl, vis);
					if (v.StaticControl) ShowWindow(*v.StaticControl, vis);
					if (v.SpecialButtonControl) ShowWindow(*v.SpecialButtonControl, vis);
					if (v.CheckBox) ShowWindow(*v.CheckBox, vis);
				}
			}
		}

		void SetHintError(cstr variableName, cstr message) override
		{
			for (VariableDesc& v : variables)
			{
				if (Eq(v.name, variableName))
				{
					if (!v.ErrorMessage)
					{
						GuiRect labelRect = WindowArea(*v.StaticControl);
						POINT topLeftEditor{ labelRect.right + LABEL_RIGHT_MARGIN, labelRect.top };
						ScreenToClient(*tab, &topLeftEditor);

						GuiRect errorRect{ topLeftEditor.x, topLeftEditor.y - 20,  ClientArea(*tab).right - 20, topLeftEditor.y };
						v.ErrorMessage = AddRichEditor(*tab, errorRect, "", (DWORD) - 1, *this, ES_READONLY | WS_VISIBLE);
					}

					ColourScheme scheme = GetDefaultLightScheme();
					v.ErrorMessage->SetColourSchemeRecursive(scheme);
			
					v.ErrorMessage->ResetContent();
					v.ErrorMessage->AppendText(RGB(255, 0, 0), RGB(scheme.backColour.red, scheme.backColour.green, scheme.backColour.blue), message);

					return;
				}
			}
		}

		void OnPretranslateMessage(MSG& msg) override
		{
			switch (msg.message)
			{
			case WM_KEYUP:
				if (msg.wParam == VK_TAB)
				{
					HWND hFocusWnd = GetFocus();
					if (hFocusWnd == *okButton)
					{
						SetFocus(*cancelButton);
						msg.message = WM_NULL;
					}
					else if (hFocusWnd == *cancelButton)
					{
						HWND firstVisibleEditor = GetNextVisibleEditor(0, nullptr);
						SetFocus(firstVisibleEditor ? firstVisibleEditor : *okButton);
						msg.message = WM_NULL;
					}
				}
				else if (msg.wParam == VK_RETURN)
				{
					HWND hFocusWnd = GetFocus();
					if (hFocusWnd == *okButton)
					{
						if (TryCompleteDialogElseReportErrors())
						{
							dlg.TerminateDialog(IDOK);
						}
						msg.message = WM_NULL;
					}
					else if (hFocusWnd == *cancelButton)
					{
						dlg.TerminateDialog(IDCANCEL);
						msg.message = WM_NULL;
					}
				}
			}
		}

		void OnTabRightClicked(int index, const POINT& screenPos) override
		{
			UNUSED(index);
			UNUSED(screenPos);
		}

		void OpenFilenameEditor(VariableDesc& v)
		{
			OPENFILENAMEA spec = { 0 };
			spec.lStructSize = sizeof(spec);
			spec.hwndOwner = *supervisor;
			spec.lpstrFilter = v.filter;
			spec.nFilterIndex = 1;

			spec.lpstrFile = v.var.value.textValue.text;
			spec.nMaxFile = v.var.value.textValue.capacity;

			char title[256];
			SecureFormat(title, sizeof(title), "%s: Select file name", v.name);
			spec.lpstrTitle = title;

			spec.Flags = 0;

			char currentDirectory[_MAX_PATH];
			GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

			if (GetOpenFileNameA(&spec))
			{
				SetWindowTextA(*v.EditControl, spec.lpstrFile);
			}

			SetCurrentDirectoryA(currentDirectory);
		}

		void OnMenuCommand(HWND, DWORD id) override
		{
			for (VariableDesc& v : variables)
			{
				if (v.SpecialButtonControl != nullptr && GetDlgCtrlID(*v.SpecialButtonControl) == (int)id)
				{
					if (v.var.type == EVariantType_String)
					{
						OpenFilenameEditor(v);
					}
					else if (v.var.type == EVariantType_None)
					{
						eventHandler->OnButtonClicked(v.name, *this);
					}

					return;
				}
				else if (v.CheckBox != nullptr && GetDlgCtrlID(*v.CheckBox) == (int)id)
				{
					eventHandler->OnButtonClicked(v.name, *this);
				}
			}
		}

		bool TryCompleteDialogElseReportErrors()
		{
			if (!IsWindowEnabled(*okButton))
			{
				return false;
			}

			for (VariableDesc& v : variables)
			{
				if (v.validator)
				{
					char* text = (char*)_malloca(sizeof(char) * v.var.value.textValue.capacity);
					GetWindowTextA(*v.EditControl, text, v.var.value.textValue.capacity);
					bool success = v.validator->ValidateAndReportErrors(text, v.name);
					_freea(text);
					if (!success)
					{
						return false;
					}
				}
			}

			return true;
		}

		LRESULT OnControlCommand(HWND, DWORD notificationCode, ControlId id, HWND hControlCode)
		{
			switch (notificationCode)
			{
			case EN_CHANGE:
				for (VariableDesc& v : variables)
				{
					if (v.EditControl && *v.EditControl == hControlCode)
					{
						OnChangeVariable(v);
						break;
					}
				}

				return 0L;
			case EN_UPDATE:
				PostMessage(hControlCode, EM_SCROLLCARET, 0, 0);
				return 0L;
			}

			if (notificationCode == BN_CLICKED)
			{
				switch (id)
				{
				case IDOK:
					if (!TryCompleteDialogElseReportErrors())
					{
						return 0L;
					}
					break;
				default:
					break;
				}

				dlg.TerminateDialog(id);
			}

			return 0L;
		}

		void OnClose(HWND)
		{
			dlg.TerminateDialog(IDCANCEL);
		}

		int GetIndexOfVariableEditor(HWND hWnd) const
		{
			if (hWnd == nullptr)
			{
				return -1;
			}

			for (size_t i = 0; i < variables.size(); ++i)
			{
				auto& v = variables[i];
				if (v.CheckBox && *v.CheckBox == hWnd)
				{
					return (int) i;
				}

				if (v.EditControl && *v.EditControl == hWnd)
				{
					return (int)i;
				}

				if (v.ComboControl && *v.ComboControl == hWnd)
				{
					return (int)i;
				}

				if (v.SpecialButtonControl && *v.SpecialButtonControl == hWnd)
				{
					return (int)i;
				}
			}

			return -1;
		}

		HWND GetNextVisibleEditor(int startIndex, HWND hExcludeThisHandle)
		{
			for (size_t i = startIndex; i < variables.size(); ++i)
			{
				auto& v = variables[i];
				if (IsWindowVisible(*v.StaticControl))
				{
					if (v.EditControl && IsWindowVisible(*v.EditControl) && hExcludeThisHandle != *v.EditControl)
					{
						return *v.EditControl;
					}

					if (v.CheckBox && IsWindowVisible(*v.CheckBox) && hExcludeThisHandle != *v.CheckBox)
					{
						return *v.CheckBox;
					}

					if (v.ComboControl && IsWindowVisible(*v.ComboControl) && hExcludeThisHandle != *v.ComboControl)
					{
						return *v.ComboControl;
					}

					if (v.SpecialButtonControl && IsWindowVisible(*v.SpecialButtonControl) && hExcludeThisHandle != *v.SpecialButtonControl)
					{
						return *v.SpecialButtonControl;
					}
				}
			}

			return nullptr;
		}

		LRESULT OnMessage(HWND hWnd, UINT msg, WPARAM, LPARAM id, OUT bool& wasHandled)
		{
			switch (msg)
			{
			case WM_NAVIGATE_BY_TAB:
				wasHandled = true;

				{
					HWND hFocus = GetFocus();
					
					int focusIndex = GetIndexOfVariableEditor(hWnd);
					if (focusIndex == -1)
					{
						if (id == NAV_RETURN)
						{
							if (TryCompleteDialogElseReportErrors())
							{
								dlg.TerminateDialog(IDOK);
							}
						}
						else
						{
							SetFocus(*okButton);
						}

						return 0L;
					}

					HWND nextVisibleEditor = GetNextVisibleEditor(focusIndex, hFocus);
					SetFocus(nextVisibleEditor ? nextVisibleEditor : *okButton);	
				}
				return 0L;
			}

			wasHandled = false;
			return 0L;
		}

		void OnSize(HWND, const Vec2i&, RESIZE_TYPE)
		{
			Resize();
		}

		enum { OK_AND_CANCEL_HEIGHT = 24 };
		enum { OK_AND_CANCEL_BOTTOM_MARGIN = 12 };
		enum { STATIC_LEFT_MARGIN = 10, CONTROL_HEIGHT = 20, CONTROL_VERTICAL_SEPARATION = 12, LABEL_RIGHT_MARGIN = 12, EDITOR_RIGHT_MARGIN = 24, COMBO_HEIGHT = 60 };

		void Resize()
		{
			int captionHeight = GetSystemMetrics(SM_CYSMCAPTION);

			Vec2i newSpan{ windowSpan.x, (3 + (int32)variables.size()) * (CONTROL_HEIGHT + CONTROL_VERTICAL_SEPARATION) + captionHeight};

			if (tabControl)
			{
				newSpan.y += 60;
			}

			MoveWindow(*supervisor, initialTopLeft.x, initialTopLeft.y, newSpan.x, newSpan.y, TRUE);

			GuiRect clientArea = ClientArea(*supervisor);
			Vec2i clientSpan = Span(clientArea);
			Vec2i centre = Centre(clientArea);

			int buttonsBottom = clientArea.bottom - OK_AND_CANCEL_BOTTOM_MARGIN;
			int buttonsTop = buttonsBottom - OK_AND_CANCEL_HEIGHT;
			
			int buttonWidth = 80;

			MoveWindow(*okButton, centre.x - buttonWidth - 20, buttonsTop, buttonWidth, OK_AND_CANCEL_HEIGHT, TRUE);
			MoveWindow(*cancelButton, centre.x + buttonWidth + 20, buttonsTop, buttonWidth, OK_AND_CANCEL_HEIGHT, TRUE);
			
			int32 tabHeight = clientSpan.y - (OK_AND_CANCEL_HEIGHT + OK_AND_CANCEL_BOTTOM_MARGIN) - captionHeight;

			if (tabControl)
			{
				MoveWindow(*tabControl, 0, 0, clientSpan.x, tabHeight, TRUE);
				clientArea = ClientArea(tabControl->ClientSpace());
				clientSpan = Span(clientArea);
			}
			else if (tab)
			{
				MoveWindow(*tab, 0, 0, clientSpan.x, tabHeight, TRUE);
			}

			int variableTop = CONTROL_HEIGHT + CONTROL_VERTICAL_SEPARATION;

			int editorLeft = STATIC_LEFT_MARGIN + labelWidth + LABEL_RIGHT_MARGIN;
			int editorWidth = clientSpan.x - (editorLeft + EDITOR_RIGHT_MARGIN);

			for (VariableDesc& v : variables)
			{			
				int editStringWidth = editorWidth;

				if (v.SpecialButtonControl)
				{
					if (v.EditControl)
					{
						// Shrink the editor to make room for the special button on the right
						editStringWidth -= CONTROL_HEIGHT;
						MoveWindow(*v.SpecialButtonControl, editorLeft + editStringWidth, variableTop, CONTROL_HEIGHT, variableTop + CONTROL_HEIGHT, TRUE);
					}
					else
					{
						MoveWindow(*v.SpecialButtonControl, editorLeft, variableTop, editorWidth, variableTop + COMBO_HEIGHT, TRUE);
					}
				}

				if (v.ComboControl) MoveWindow(*v.ComboControl, editorLeft, variableTop, editorWidth, variableTop + COMBO_HEIGHT, TRUE);
				if (v.EditControl) MoveWindow(*v.EditControl, editorLeft, variableTop, editStringWidth, variableTop + CONTROL_HEIGHT, TRUE);
				if (v.StaticControl) MoveWindow(*v.StaticControl, STATIC_LEFT_MARGIN, variableTop, labelWidth, variableTop + CONTROL_HEIGHT, TRUE);	
				if (v.CheckBox) MoveWindow(*v.CheckBox, editorLeft, variableTop, editorWidth, variableTop + CONTROL_HEIGHT, TRUE);

				variableTop += CONTROL_VERTICAL_SEPARATION + CONTROL_HEIGHT;
			}
		}

		VariableEditor(const Vec2i& span, int _labelWidth, HWND _hwndOwner, IVariableEditorEventHandler* _eventHandler) :
			hwndOwner(_hwndOwner),
			supervisor(nullptr),
			okButton(nullptr),
			cancelButton(nullptr),
			nextId(IDSPECIAL),
			windowSpan(span),
			labelWidth(_labelWidth),
			eventHandler(_eventHandler),
			tab(nullptr)
		{
			nextX = 10;
			nextY = 28;
		}

		void OnModal() override
		{
			HWND hTarget = GetNextVisibleEditor(0, NULL);
			SetFocus(hTarget);

			if (eventHandler)
			{
				eventHandler->OnModal(*this);
			}
		}

		void SetEnabled(bool _isEnabled, cstr controlName)
		{
			BOOL isEnabled = _isEnabled ? TRUE : FALSE;
			if (controlName == (cstr)IDOK)
			{
				EnableWindow(*okButton, isEnabled);
				SendMessage(*okButton, BM_SETDONTCLICK, isEnabled, 0);
				return;
			}

			if (controlName == (cstr) IDCANCEL)
			{
				EnableWindow(*cancelButton, isEnabled);
				SendMessage(*okButton, BM_SETDONTCLICK, isEnabled, 0);
				return;
			}

			for (auto& v : variables)
			{
				if (Eq(v.name, controlName))
				{
					if (v.StaticControl) EnableWindow(*v.StaticControl, isEnabled);
					if (v.CheckBox) EnableWindow(*v.CheckBox, isEnabled);
					if (v.ComboControl) EnableWindow(*v.ComboControl, isEnabled);
					if (v.EditControl) EnableWindow(*v.EditControl, isEnabled);
					if (v.SpecialButtonControl) EnableWindow(*v.SpecialButtonControl, isEnabled);
					return;
				}
			}
		}

		void Construct(cstr appQueryName, cstr defaultTab, cstr defaultTooltip, const Vec2i* topLeft)
		{
			if (topLeft != nullptr)
			{
				initialTopLeft = *topLeft;
			}
			else
			{
				POINT pos;
				initialTopLeft = GetCursorPos(&pos) ? Vec2i {pos.x, pos.y} : Vec2i{ 0,0 };
			}

			WindowConfig config;
			Vec2i nullSpan{ 0,0 };
			Windows::SetOverlappedWindowConfig(config, nullSpan, SW_HIDE, hwndOwner, appQueryName, WS_OVERLAPPED | WS_SYSMENU, 0);
			supervisor = dlg.CreateDialogWindow(config);

			if (defaultTab)
			{
				tabControl = Windows::AddTabs(*supervisor, GuiRect(0, 0, 0, 0), "", (ControlId) - 1, *this, WS_CLIPSIBLINGS | TCS_TOOLTIPS, 0);
				tabControl->AddTab(defaultTab, defaultTooltip);
			}
			else
			{
				tabControl = nullptr;
			}
			
			IParentWindowSupervisor& tabParent = defaultTab ? tabControl->ClientSpace() : *supervisor;

			bool debugTabs = false;

			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, ClientArea(tabParent), tabParent, "", WS_VISIBLE | WS_CHILD, debugTabs ? WS_EX_CLIENTEDGE : 0);
			tab = tabParent.AddChild(childConfig, (ControlId) -1, this);
			SetBackgroundColour(GetSysColor(COLOR_3DFACE));

			okButton = Windows::AddPushButton(*supervisor, GuiRect(0, 0, 0, 0), "&OK", IDOK, WS_VISIBLE | BS_DEFPUSHBUTTON, 0);
			cancelButton = Windows::AddPushButton(*supervisor, GuiRect(0, 0, 0, 0), "&Cancel", IDCANCEL, WS_VISIBLE | BS_PUSHBUTTON, 0);

			struct ANON
			{
				static void OnControlCommand(void* context, HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
				{
					VariableEditor* This = (VariableEditor*)context;
					This->OnControlCommand(hWnd, notificationCode, id, hControlCode);
				}

				static void OnClose(void* context, HWND hWnd)
				{
					VariableEditor* This = (VariableEditor*)context;
					This->OnClose(hWnd);
				}

				static void OnSize(void* context, HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
				{
					VariableEditor* This = (VariableEditor*)context;
					This->OnSize(hWnd, span, type);
				}

				static void OnPreTranslate(void* context, MSG& msg)
				{
					VariableEditor* This = (VariableEditor*)context;
					This->OnPretranslateMessage(msg);
				}

				static LRESULT OnMessage(void* context, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, OUT bool& wasHandled)
				{
					VariableEditor* This = (VariableEditor*)context;
					return This->OnMessage(hWnd, msg, wParam, lParam, OUT wasHandled);
				}

				static void OnModal(void* context)
				{
					VariableEditor* This = (VariableEditor*)context;
					return This->OnModal();
				}
			};

			dlg.Router().RouteControlCommand(this, ANON::OnControlCommand);
			dlg.Router().RouteClose(this, ANON::OnClose);
			dlg.Router().RouteSize(this, ANON::OnSize);
			dlg.Router().RoutePreTranslate(this, ANON::OnPreTranslate);
			dlg.Router().RouteMessage(this, ANON::OnMessage);
			dlg.Router().RouteOnModal(this, ANON::OnModal);
		}
	public:
		static VariableEditor* Create(HWND hwndOwner, const Vec2i& span, int labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft)
		{
			VariableEditor* editor = new VariableEditor(span, labelWidth, hwndOwner, eventHandler);
			editor->Construct(appQueryName, defaultTab, defaultTooltip, topLeft);
			return editor;
		}
	private:

		~VariableEditor()
		{
			supervisor->Free();
		}

		bool ValidateAndReportErrors()
		{
			for (VariableDesc& v : variables)
			{
				if (v.validator != nullptr)
				{
					if (v.EditControl == nullptr)
					{
						Throw(0, "Validated control missing an edit box");
					}

					char* liveBuffer = (char*)alloca(sizeof(char)* v.var.value.textValue.capacity + 2);
					GetWindowTextA(*v.EditControl, liveBuffer, v.var.value.textValue.capacity);
					if (!v.validator->ValidateAndReportErrors(liveBuffer, v.name))
					{
						return false;
					}
				}
			}

			return true;
		}

		GuiRect GetDefaultLabelRect()
		{
			return GuiRect(nextX, nextY + 2, nextX + labelWidth, nextY + 20);
		}

		GuiRect GetDefaultEditRect()
		{
			GuiRect clientrect = ClientArea(*supervisor);
			return  GuiRect(nextX + labelWidth + 10, nextY, clientrect.right - 10, nextY + 22);
		}

		void AddToolTip(HWND hwndTool, cstr pszText)
		{
			UNUSED(hwndTool);
			UNUSED(pszText);
		}

		void AddBooleanEditor(cstr variableName, bool state) override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			if (tabControl)
			{
				tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);
			}
			else
			{
				v.tabName[0] = 0;
			}

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, (ControlId) -1, WS_VISIBLE | SS_RIGHT, 0);
			v.CheckBox = AddCheckBox(*tab, GetDefaultEditRect(), "", nextId++, BS_AUTOCHECKBOX | BS_TOP | WS_VISIBLE | BS_LEFT, 0);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.value.int32Value = state ? 1 : 0;
			v.var.type = EVariantType_Bool;

			variables.push_back(v);

			nextY += 22;
		}

		void AddIntegerEditor(cstr variableName, cstr variableDesc, int minimum, int maximum, int defaultValue) override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			if (tabControl)
			{
				tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);
			}
			else
			{
				v.tabName[0] = 0;
			}

			char editor[256];
			SecureFormat(editor, sizeof(editor), "Edit_%s", variableName);

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, (ControlId) -1, WS_VISIBLE | SS_RIGHT, 0);
			v.EditControl = AddEditor(*tab, GetDefaultEditRect(), editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);

			SetWindowSubclass(*v.EditControl, VariableEditEditorProc, VariableEditEditorProcId, 0);

			AddToolTip(*v.StaticControl, variableDesc);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.value.int32Value = defaultValue;
			v.minimum.int32Value = minimum;
			v.maximum.int32Value = maximum;
			v.var.type = EVariantType_Int32;

			variables.push_back(v);

			nextY += 22;
		}

		void AddPushButton(cstr variableName, cstr variableDesc) override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			if (eventHandler == nullptr)
			{
				Throw(0, "The variable editor was not constructed with an event handler and thus cannot deliver push button events");
			}

			VariableDesc v = { 0 };
			if (tabControl)
			{
				tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);
			}
			else
			{
				v.tabName[0] = 0;
			}

			GuiRect labelRect = GetDefaultLabelRect();

			GuiRect buttonRect = GetDefaultEditRect();

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableDesc, (ControlId) -1, WS_VISIBLE | SS_RIGHT, 0);
			v.SpecialButtonControl = Windows::AddPushButton(*tab, buttonRect, variableDesc, nextId++, WS_VISIBLE | BS_PUSHBUTTON, WS_EX_CLIENTEDGE);
			AddToolTip(*v.StaticControl, variableDesc);

			//   SetControlFont(*v.StaticControl);
			//   SetControlFont(*v.SpecialButtonControl);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.type = EVariantType_None;

			variables.push_back(v);

			nextY = buttonRect.bottom + 2;
		}

		void AddSelection(cstr variableName, cstr variableDesc, char* buffer, uint32 capacityIncludingNullCharacter, ISelection& selection, IStringValidator* validator)  override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			if (tabControl)
			{
				tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);
			}
			else
			{
				v.tabName[0] = 0;
			}

			char editor[256];
			SecureFormat(editor, sizeof(editor), "Edit_%s", variableName);

			GuiRect labelRect = GetDefaultLabelRect();
			GuiRect comboRect = GetDefaultEditRect();
			comboRect.bottom += 60;

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, (ControlId) -1, WS_VISIBLE | SS_RIGHT, 0);
			v.ComboControl = AddComboBox(*tab, comboRect, editor, nextId++, WS_VISIBLE | CBS_SIMPLE | CBS_HASSTRINGS | CBS_DISABLENOSCROLL, 0, WS_EX_STATICEDGE);
			AddToolTip(*v.StaticControl, variableDesc);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.value.textValue.capacity = capacityIncludingNullCharacter;
			v.var.value.textValue.text = buffer;
			v.var.type = EVariantType_String;
			v.validator = validator;
			v.selection = &selection;

			variables.push_back(v);

			nextY = comboRect.bottom + 2;
		}

		void AddTab(cstr tabName, cstr tabToolTip) override
		{
			tabControl->AddTab(tabName, tabToolTip);
			nextY = 28;
		}

		void AddStringEditor(cstr variableName, cstr variableDesc, char* buffer, uint32 capacity, IStringValidator* validator) override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			if (tabControl)
			{
				tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);
			}
			else
			{
				v.tabName[0] = 0;
			}

			char editor[256];
			SecureFormat(editor, sizeof(editor), "Edit_%s", variableName);

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, (ControlId) -1, WS_VISIBLE | SS_SIMPLE, 0);
			v.EditControl = AddEditor(*tab, GetDefaultEditRect(), editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);
			AddToolTip(*v.StaticControl, variableDesc);

			SetWindowSubclass(*v.EditControl, VariableEditEditorProc, VariableEditEditorProcId, 0);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.value.textValue.capacity = capacity;
			v.var.value.textValue.text = buffer;
			v.var.type = EVariantType_String;
			v.validator = validator;

			variables.push_back(v);

			nextY += 22;
		}

		void AddFilenameEditor(cstr variableName, cstr variableDesc, char* buffer, uint32 capacity, cstr filter, IStringValidator* validator = nullptr) override
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, "Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);

			char editor[256];
			SecureFormat(editor, sizeof(editor), "Edit_%s", variableName);

			GuiRect editRect = GetDefaultEditRect();
			editRect.right -= 40;

			GuiRect buttonRect = GetDefaultEditRect();
			buttonRect.left = buttonRect.right - 36;

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, (ControlId) -1, WS_VISIBLE | SS_RIGHT, 0);
			v.EditControl = AddEditor(*tab, editRect, editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);
			SetWindowSubclass(*v.EditControl, VariableEditEditorProc, VariableEditEditorProcId, 0);
			v.SpecialButtonControl = Windows::AddPushButton(*tab, buttonRect, "...", nextId++, WS_VISIBLE | BS_PUSHBUTTON, WS_EX_CLIENTEDGE);
			AddToolTip(*v.StaticControl, variableDesc);

			StackStringBuilder sb(v.name, v.NAME_CAPACITY);
			sb << variableName;

			v.var.value.textValue.capacity = capacity;
			v.var.value.textValue.text = buffer;
			v.var.type = EVariantType_String;
			v.validator = validator;
			v.filter = filter;

			variables.push_back(v);

			nextY += 22;
		}

		enum { IDSPECIAL = 10000 };

		void PrepareModal()
		{
			Resize();

			for (VariableDesc& v : variables)
			{
				SetWindowTextA(*v.StaticControl, v.name);

				if (tabControl)
				{
					int currentTabIndex = tabControl->GetCurrentSelection();
					OnSelectionChanged(currentTabIndex);
				}

				char desc[256];
				FormatWithVariant(desc, 256, v.var);

				if (v.var.type == EVariantType_Int32)
				{
					SendMessage(*v.EditControl, EM_SETLIMITTEXT, 11, 0);
				}
				else if (v.var.type == EVariantType_Bool)
				{
					v.CheckBox->SetCheckState(v.var.value.int32Value != 0 ? Visitors::CheckState_Ticked : Visitors::CheckState_Clear);
				}
				else if (v.var.type == EVariantType_String)
				{
					if (v.EditControl != nullptr)
					{
						SendMessage(*v.EditControl, EM_SETLIMITTEXT, v.var.value.textValue.capacity - 1, 0);
					}
				}

				if (v.EditControl != nullptr)
				{
					SetWindowTextA(*v.EditControl, desc);
				}

				if (v.ComboControl != nullptr)
				{
					for (size_t i = 0; i < v.selection->Count(); i++)
					{
						v.ComboControl->AddString(v.selection->GetElement(i));
					}

					if (v.var.value.textValue.text[0] != 0)
					{
						int index = v.ComboControl->FindString(v.var.value.textValue.text);
						if (index >= 0)
						{
							v.ComboControl->SetCurrentSelection(index);
						}
					}
					else
					{
						int index = v.ComboControl->FindString(v.selection->GetDefaultItem());
						if (index >= 0)
						{
							v.ComboControl->SetCurrentSelection(index);
						}
					}
				}
			}
		}

		std::vector<char> stringSpace;

		void SyncWithInputBuffers()
		{
			for (VariableDesc& v : variables)
			{
				if (v.EditControl != nullptr)
				{
					int len = GetWindowTextLengthA(*v.EditControl);
					{
						stringSpace.resize(len + 2);
						GetWindowTextA(*v.EditControl, stringSpace.data(), len + 1);
						ParseVariable(v, stringSpace.data());
					}
				}
				else if (v.ComboControl != nullptr)
				{
					int index = v.ComboControl->GetCurrentSelection();
					v.ComboControl->GetString(index, v.var.value.textValue.text, v.var.value.textValue.capacity);
				}
				else if (v.CheckBox != nullptr)
				{
					v.var.value.int32Value = v.CheckBox->GetCheckState() == Visitors::CheckState_Ticked ? 1 : 0;
				}
			}
		}

		bool IsModalDialogChoiceYes() override
		{
			PrepareModal();

			DWORD exitCode = dlg.BlockModal(*supervisor, hwndOwner);

			SyncWithInputBuffers();

			return exitCode == IDOK;
		}

		void OnChangeInt32Variable(VariableDesc& v)
		{
			char buffer[16];
			GetWindowTextA(*v.EditControl, buffer, 16);

			char* s = buffer;

			if (s[0] == '+' || s[0] == '-') s++;
			else if (s[0] == 0) return;

			for (; *s != 0; ++s)
			{
				if (*s < '0' || *s > '9')
				{
					*s = 0;
					SetWindowTextA(*v.EditControl, buffer);
					SendMessage(*v.EditControl, EM_SETSEL, s - buffer, s - buffer);
					break;
				}
			}
		}

		void OnChangeVariable(VariableDesc& v)
		{
			switch (v.var.type)
			{
			case EVariantType_Int32:
				OnChangeInt32Variable(v);
				break;
			case EVariantType_String:
				// No validation/correction necessary
				break;
			}
		}

		int GetInteger(cstr variableName) override
		{
			for (const VariableDesc& v : variables)
			{
				if (strcmp(variableName, v.name) == 0)
				{
					if (v.var.type == EVariantType_Int32)
					{
						return v.var.value.int32Value;
					}
					else
					{
						Throw(0, "VariableEditor::GetInteger('%s'): variable is not an Int32", variableName);
					}
				}
			}

			Throw(0, "VariableEditor::GetInteger('%s'). Item not found", variableName);
		}

		bool GetBoolean(cstr variableName) override
		{
			for (const VariableDesc& v : variables)
			{
				if (strcmp(variableName, v.name) == 0)
				{
					if (v.CheckBox)
					{
						return v.CheckBox->GetCheckState() == Visitors::CheckState::CheckState_Ticked;
					}
					else if (v.var.type == EVariantType_Bool)
					{
						return v.var.value.int32Value == 1;
					}
					else
					{
						Throw(0, "VariableEditor::GetBoolean('%s'): variable is not a Bool", variableName);
					}
				}
			}

			Throw(0, "VariableEditor::GetInteger('%s'). Item not found", variableName);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	ROCOCO_API_EXPORT IVariableEditor* CreateVariableEditor(Windows::IWindow& parent, const Vec2i& span, int labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft)
	{
		return VariableEditor::Create(parent, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
	}
}
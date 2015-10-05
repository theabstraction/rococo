#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <rococo.target.h>
#include <Windows.h>

#undef min
#undef max

#include <rococo.types.h>
#include <rococo.window.h>
#include <rococo.maths.h>

#include <commdlg.h>
#include <Windowsx.h>
#include <wchar.h>
#include <vector>

#include <rococo.variable.editor.h>

#include <CommCtrl.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	struct VariantTextBuffer
	{
		DWORD capacity;
		wchar_t* text;
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

	void FormatWithVariant(wchar_t* desc, size_t capacity, const Variant& var)
	{
		switch (var.type)
		{
		case EVariantType_Int32:
			SecureFormat(desc, capacity, L"%d", var.value.int32Value);
			break;
		case EVariantType_Uint32:
			SecureFormat(desc, capacity, L"%u", var.value.uint32Value);
			break;
		case EVariantType_Float:
			SecureFormat(desc, capacity, L"%f", var.value.floatValue);
			break;
		case EVariantType_Double:
			SecureFormat(desc, capacity, L"%lf", var.value.doubleValue);
			break;
		case EVariantType_String:
			SafeCopy(desc, capacity, var.value.textValue.text, _TRUNCATE);
			break;
		default:
			SafeFormat(desc, capacity, capacity, L"? Unknown ?");
		}
	}

	struct VariableDesc
	{
		enum { NAME_CAPACITY = 64 };
		enum { TAB_NAME_CAPACITY = 256 };
		IWindowSupervisor* StaticControl;
		IWindowSupervisor* EditControl;
		IComboBoxSupervisor* ComboControl;
		IWindowSupervisor* SpecialButtonControl;
		Variant var;
		UVariant minimum;
		UVariant maximum;
		const wchar_t* filter;
		IStringValidator* validator;
		ISelection* selection;
		wchar_t name[NAME_CAPACITY];
		wchar_t tabName[TAB_NAME_CAPACITY];
	};

	void ParseInt32(VariableDesc& v, const wchar_t* s)
	{
		if (*s == 0)
		{
			v.var.value.int32Value = 0;
		}

		if (wcslen(s) > 10)
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
			_snwscanf_s(s, 10, L"%d", &v.var.value.int32Value);
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

	void ParseVariable(VariableDesc& v, const wchar_t* s)
	{
		switch (v.var.type)
		{
		case EVariantType_Int32:
			ParseInt32(v, s);
			break;
		case EVariantType_String:
			SecureCopy(v.var.value.textValue.text, v.var.value.textValue.capacity, s);
			break;
		default:
			Throw(0, L"Var type %d Not implemented for parsering.", v.var.type);
		}
	}

	class VariableEditor : public IVariableEditor, private ITabControlEvents, private StandardWindowHandler
	{
	private:
		int nextX;
		int nextY;
		Vec2i windowSpan;
		int labelWidth;

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

		virtual void OnSelectionChanged(int index)
		{
			wchar_t tabName[VariableDesc::TAB_NAME_CAPACITY];
			if (tabControl->GetTabName(index, tabName, VariableDesc::TAB_NAME_CAPACITY))
			{
				for (VariableDesc& v : variables)
				{
					UINT vis = wcscmp(v.tabName, tabName) == 0 ? SW_SHOW : SW_HIDE;
					if (v.ComboControl) ShowWindow(*v.ComboControl, vis);
					if (v.EditControl) ShowWindow(*v.EditControl, vis);
					if (v.StaticControl) ShowWindow(*v.StaticControl, vis);
					if (v.SpecialButtonControl) ShowWindow(*v.SpecialButtonControl, vis);
				}
			}
		}

		void OpenFilenameEditor(VariableDesc& v)
		{
			OPENFILENAME spec = { 0 };
			spec.lStructSize = sizeof(spec);
			spec.hwndOwner = *supervisor;
			spec.lpstrFilter = v.filter;
			spec.nFilterIndex = 1;

			spec.lpstrFile = v.var.value.textValue.text;
			spec.nMaxFile = v.var.value.textValue.capacity;

			wchar_t title[256];
			SecureFormat(title, L"%s: Select file name", v.name);
			spec.lpstrTitle = title;

			spec.Flags = 0;

			wchar_t currentDirectory[_MAX_PATH];
			GetCurrentDirectory(_MAX_PATH, currentDirectory);

			if (GetOpenFileNameW(&spec))
			{
				SetWindowText(*v.EditControl, spec.lpstrFile);
			}

			SetCurrentDirectory(currentDirectory);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			for (VariableDesc& v : variables)
			{
				if (v.SpecialButtonControl != nullptr && GetDlgCtrlID(*v.SpecialButtonControl) == id)
				{
					if (v.var.type == EVariantType_String)
					{
						OpenFilenameEditor(v);
					}
					else if (v.var.type == EVariantType_None)
					{
						eventHandler->OnButtonClicked(v.name);
					}
					
					return;
				}
			}
		}

		LRESULT OnControlCommand(HWND hWnd, DWORD notificationCode, ControlId id, HWND hControlCode)
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
					for (VariableDesc& v : variables)
					{
						if (v.validator)
						{
							wchar_t* text = (wchar_t*)_malloca(sizeof(wchar_t)* v.var.value.textValue.capacity);
							GetWindowText(*v.EditControl, text, v.var.value.textValue.capacity);
							if (!v.validator->ValidateAndReportErrors(text))
							{
								return 0L;
							}
							_freea(text);
						}
					}
					break;
				default:
					break;
				}	

				dlg.TerminateDialog(id);
			}

			return 0L;
		}

		void OnClose(HWND hWnd)
		{
			dlg.TerminateDialog(IDCANCEL);
		}

		void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			Resize();
		}
		
		enum { CONTROL_BUTTON_LINE = 30 };

		void Resize()
		{		
			GuiRect rect = ClientArea(*supervisor);
			if (tabControl) MoveWindow(*tabControl, 0, 0, rect.right, rect.bottom - CONTROL_BUTTON_LINE, TRUE);
			if (tab) MoveWindow(*tab, 0, 0, rect.right, rect.bottom - CONTROL_BUTTON_LINE, TRUE);
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

		void Construct(LPCWSTR appQueryName, LPCWSTR defaultTab, LPCWSTR defaultTooltip, const POINT* topLeft)
		{
			WindowConfig config;
			Windows::SetOverlappedWindowConfig(config, windowSpan, SW_HIDE, hwndOwner, appQueryName, WS_OVERLAPPED | WS_SYSMENU, 0);
			supervisor = dlg.CreateDialogWindow(config);

			if (defaultTab)
			{
				tabControl = Windows::AddTabs(*supervisor, GuiRect(1, 1, 1, 1), L"", -1, *this, WS_CLIPSIBLINGS | TCS_TOOLTIPS, 0);
				tabControl->AddTab(defaultTab, defaultTooltip);
			}
			else
			{
				tabControl = nullptr;
			}

			if (topLeft) MoveWindow(*supervisor, topLeft->x, topLeft->y, windowSpan.x, windowSpan.y, TRUE);

			IParentWindowSupervisor& tabParent = defaultTab ? tabControl->ClientSpace() : *supervisor;

			WindowConfig childConfig;
			Windows::SetChildWindowConfig(childConfig, ClientArea(tabParent), tabParent, L"", WS_VISIBLE | WS_CHILD, 0);
			tab = tabParent.AddChild(childConfig, -1, this);
			SetBackgroundColour(GetSysColor(COLOR_3DFACE));

			GuiRect clientArea = ClientArea(*supervisor);

			Vec2i centre = Centre(clientArea);

			int buttonsTop = clientArea.bottom - CONTROL_BUTTON_LINE + 4;
			int buttonsBottom = buttonsTop + 24;

			int buttonWidth = 80;

			okButton = Windows::AddPushButton(*supervisor, GuiRect(centre.x - buttonWidth - 20, buttonsTop, centre.x - 20, buttonsBottom), L"&OK", IDOK, WS_VISIBLE | BS_DEFPUSHBUTTON, WS_EX_CLIENTEDGE);
			cancelButton = Windows::AddPushButton(*supervisor, GuiRect(centre.x + 20, buttonsTop, centre.x + buttonWidth + 20, buttonsBottom), L"&Cancel", IDCANCEL, WS_VISIBLE | BS_PUSHBUTTON, WS_EX_CLIENTEDGE);

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
			};

			dlg.Router().RouteControlCommand(this, ANON::OnControlCommand);
			dlg.Router().RouteClose(this, ANON::OnClose);
			dlg.Router().RouteSize(this, ANON::OnSize);

			GuiRect windowArea = WindowArea(*supervisor);
			POINT pos;
			GetCursorPos(&pos);
			MoveWindow(*supervisor, pos.x - (Width(windowArea) >> 1), pos.y, Width(windowArea), Height(windowArea), TRUE);

			Resize();
		}
	public:
		static VariableEditor* Create(HWND hwndOwner, const Vec2i& span, int labelWidth, LPCWSTR appQueryName, LPCWSTR defaultTab, LPCWSTR defaultTooltip, IVariableEditorEventHandler* eventHandler, const POINT* topLeft)
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
						Throw(0, L"Validated control missing an edit box");
					}
					{
						wchar_t* liveBuffer = (wchar_t*)alloca(sizeof(wchar_t)* v.var.value.textValue.capacity + 2);
						GetWindowText(*v.EditControl, liveBuffer, v.var.value.textValue.capacity);
						if (!v.validator->ValidateAndReportErrors(liveBuffer))
						{
							return false;
						}
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

		virtual void AddIntegerEditor(LPCWSTR variableName, LPCWSTR variableDesc, int minimum, int maximum, int defaultValue)
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, L"Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			tabControl->GetTabName(tabControl->TabCount()-1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);

			wchar_t editor[256];
			SecureFormat(editor, L"Edit_%s", variableName);

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, -1, WS_VISIBLE | SS_RIGHT, 0);
			v.EditControl = AddEditor(*tab, GetDefaultEditRect(), editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);

			SecureCopy(v.name, variableName);
			v.var.value.int32Value = defaultValue;
			v.minimum.int32Value = minimum;
			v.maximum.int32Value = maximum;
			v.var.type = EVariantType_Int32;

			variables.push_back(v);

			nextY += 22;
		}

		virtual void AddPushButton(LPCWSTR variableName, LPCWSTR variableDesc)
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, L"Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			if (eventHandler == nullptr)
			{
				Throw(0, L"The variable editor was not constructed with an event handler and thus cannot deliver push button events");
			}

			VariableDesc v = { 0 };
			tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);

			GuiRect labelRect = GetDefaultLabelRect();

			GuiRect buttonRect = labelRect;
			buttonRect.left = labelRect.right + 20;
			buttonRect.right = ClientArea(*supervisor).right - 20;
			
			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableDesc, -1, WS_VISIBLE | SS_RIGHT, 0);
			v.SpecialButtonControl = Windows::AddPushButton(*tab, buttonRect, L"Edit Script", nextId++, WS_VISIBLE | BS_PUSHBUTTON, WS_EX_CLIENTEDGE);

			SecureCopy(v.name, variableName);

			v.var.type = EVariantType_None;

			variables.push_back(v);

			nextY = buttonRect.bottom + 2;
		}

		virtual void AddSelection(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacityIncludingNullCharacter, ISelection& selection, IStringValidator* validator)
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, L"Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);

			wchar_t editor[256];
			SecureFormat(editor, L"Edit_%s", variableName);

			GuiRect labelRect = GetDefaultLabelRect();
			GuiRect comboRect = GetDefaultEditRect();
			comboRect.bottom += 60;

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, -1, WS_VISIBLE | SS_RIGHT, 0);
			v.ComboControl = AddComboBox(*tab, comboRect, editor, nextId++, WS_VISIBLE | CBS_SIMPLE | CBS_HASSTRINGS | CBS_DISABLENOSCROLL, 0, WS_EX_STATICEDGE);

			SecureCopy(v.name, variableName);

			v.var.value.textValue.capacity = capacityIncludingNullCharacter;
			v.var.value.textValue.text = buffer;
			v.var.type = EVariantType_String;
			v.validator = validator;
			v.selection = &selection;

			variables.push_back(v);

			nextY = comboRect.bottom + 2;
		}

		virtual void AddTab(LPCWSTR tabName, LPCWSTR tabToolTip)
		{
			tabControl->AddTab(tabName, tabToolTip);
			nextY = 28;
		}

		virtual void AddStringEditor(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacity, IStringValidator* validator)
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, L"Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
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

			wchar_t editor[256];
			SecureFormat(editor, L"Edit_%s", variableName);

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, -1, WS_VISIBLE | SS_RIGHT, 0);
			v.EditControl = AddEditor(*tab, GetDefaultEditRect(), editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);

			SecureCopy(v.name, variableName);

			v.var.value.textValue.capacity = capacity;
			v.var.value.textValue.text = buffer;
			v.var.type = EVariantType_String;
			v.validator = validator;

			variables.push_back(v);

			nextY += 22;
		}

		virtual void AddFilenameEditor(LPCWSTR variableName, LPCWSTR variableDesc, wchar_t* buffer, DWORD capacity, const wchar_t* filter, IStringValidator* validator = nullptr)
		{
			if (nextY > windowSpan.y - 30)
			{
				Throw(0, L"Too many editors added to variable editor.\nCannot add editor for: %s\n", variableName);
			}

			VariableDesc v = { 0 };
			tabControl->GetTabName(tabControl->TabCount() - 1, v.tabName, VariableDesc::TAB_NAME_CAPACITY);

			wchar_t editor[256];
			SecureFormat(editor, L"Edit_%s", variableName);

			GuiRect editRect = GetDefaultEditRect();
			editRect.right -= 40;

			GuiRect buttonRect = GetDefaultEditRect();
			buttonRect.left = buttonRect.right - 36;

			v.StaticControl = AddLabel(*tab, GetDefaultLabelRect(), variableName, -1, WS_VISIBLE | SS_RIGHT, 0);
			v.EditControl = AddEditor(*tab, editRect, editor, nextId++, WS_VISIBLE, WS_EX_CLIENTEDGE);
			v.SpecialButtonControl = Windows::AddPushButton(*tab, buttonRect, L"...", nextId++, WS_VISIBLE | BS_PUSHBUTTON, WS_EX_CLIENTEDGE);

			SecureCopy(v.name, variableName);

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
			for (const VariableDesc& v : variables)
			{
				SetWindowTextW(*v.StaticControl, v.name);

				if (tabControl)
				{
					int currentTabIndex = tabControl->GetCurrentSelection();
					OnSelectionChanged(currentTabIndex);
				}

				wchar_t desc[256];
				FormatWithVariant(desc, 256, v.var);

				if (v.var.type == EVariantType_Int32)
				{
					SendMessage(*v.EditControl, EM_SETLIMITTEXT, 11, 0);
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
					SetWindowText(*v.EditControl, desc);
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

		void SyncWithInputBuffers()
		{
			for (VariableDesc& v : variables)
			{
				if (v.EditControl != nullptr)
				{
					int len = GetWindowTextLength(*v.EditControl);
					{
						wchar_t *value = (wchar_t*)alloca(2 * len + 2);
						GetWindowText(*v.EditControl, value, len + 1);
						ParseVariable(v, value);
					}
				}
				else if (v.ComboControl != nullptr)
				{
					int index = v.ComboControl->GetCurrentSelection();
					v.ComboControl->GetString(index, v.var.value.textValue.text, v.var.value.textValue.capacity);
				}
			}
		}

		virtual bool IsModalDialogChoiceYes()
		{
			PrepareModal();
			
			DWORD exitCode = dlg.BlockModal(*supervisor, hwndOwner);

			SyncWithInputBuffers();

			return exitCode == IDOK;
		}

		void OnChangeInt32Variable(VariableDesc& v)
		{
			wchar_t buffer[16];
			GetWindowText(*v.EditControl, buffer, 16);

			wchar_t* s = buffer;

			if (s[0] == '+' || s[0] == '-') s++;
			else if (s[0] == 0) return;

			for (; *s != 0; ++s)
			{
				if (*s < '0' || *s > '9')
				{
					*s = 0;
					SetWindowText(*v.EditControl, buffer);
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

		virtual int GetInteger(LPCWSTR variableName)
		{
			for(const VariableDesc& v: variables)
			{
				if (wcscmp(variableName, v.name) == 0)
				{
					if (v.var.type == EVariantType_Int32)
					{
						return v.var.value.int32Value;
					}
					else
					{
						Throw(0, L"VariableEditor::GetInteger('%s'): variable is not an Int32", variableName);
					}
				}
			}

			Throw(0, L"VariableEditor::GetInteger('%s'). Item not found", variableName);
			return 0;
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IVariableEditor* CreateVariableEditor(HWND hwndOwner, const Vec2i& span, int labelWidth, LPCWSTR appQueryName, LPCWSTR defaultTab, LPCWSTR defaultTooltip, IVariableEditorEventHandler* eventHandler, const POINT* topLeft)
	{
		return VariableEditor::Create(hwndOwner, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
	}
}
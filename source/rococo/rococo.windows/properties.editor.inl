#include <rococo.editors.h>
#include <rococo.properties.h>
#include <rococo.hashtable.h>
#include <rococo.validators.h>

using namespace Rococo::Reflection;
using namespace Rococo::Editors;
using namespace Rococo::Validators;

namespace Rococo::Windows::Internal
{
	ROCOCO_INTERFACE IPropertySupervisor : Rococo::Reflection::IPropertyEditor
	{
		virtual void AdvanceSelection() = 0;
		virtual void Free() = 0;
		virtual void OnButtonClicked() = 0;
		virtual void OnEditorChanged() = 0;
		virtual void OnEditorLostKeyboardFocus() = 0;
		virtual bool TryTakeFocus() = 0;
		virtual GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId editorId) = 0;
		virtual GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) = 0;
	};

	struct VisualStyle
	{
		int minSpan = 200;
		int labelSpan = 100;
		int defaultPadding = 2;
		int rightPadding = 2;
		int rowHeight = 20;
	};

	void AddLabel(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, int yOffset, UI::SysWidgetId id)
	{
		Rococo::Windows::AddLabel(panel, GuiRect{ style.defaultPadding, yOffset, style.labelSpan - style.defaultPadding, yOffset + style.rowHeight }, text, id.value, WS_VISIBLE | SS_LEFTNOWORDWRAP, 0);
	}

	enum { NAV_TARGET_CLASS_ID = 42, ABEDIT_BTN_CLASS_ID = 9000 };

	static LRESULT AbEditEditorProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		UNUSED(dwRefData);

		if (uIdSubclass == NAV_TARGET_CLASS_ID)
		{
			switch (uMsg)
			{
			case WM_KEYUP:
				if (wParam == VK_TAB || wParam == VK_RETURN || wParam == VK_DOWN)
				{
					PostMessage(GetParent(hWnd), WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), 1);
					return 0L;
				}

				if (wParam == VK_UP)
				{
					PostMessage(GetParent(hWnd), WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), -1);
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

	static LRESULT AbEditCheckBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		UNUSED(dwRefData);

		if (uIdSubclass == ABEDIT_BTN_CLASS_ID)
		{
			switch (uMsg)
			{
			case WM_KEYUP:
				if (wParam == VK_TAB || wParam == VK_DOWN || wParam == VK_RETURN)
				{
					PostMessage(GetParent(hWnd), WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), 1);
					return 0L;
				}

				if (wParam == VK_UP)
				{
					PostMessage(GetParent(hWnd), WM_NAVIGATE_BY_TAB, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), -1);
					return 0L;
				}

				if (wParam == VK_SPACE)
				{
					PostMessage(GetParent(hWnd), WM_ADVANCE_SELECTION, (LPARAM)GetWindowLongPtrA(hWnd, GWLP_ID), 0);
					return 0L;
				}
			}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	Rococo::Windows::IWindowSupervisor* AddEditor(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, size_t maxCharacters, int yOffset, UI::SysWidgetId id)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		auto* editor = Rococo::Windows::AddEditor(panel, GuiRect{ style.labelSpan, yOffset, containerRect.right, yOffset + style.rowHeight }, text, id.value, WS_VISIBLE, 0);

		SetWindowSubclass(*editor, AbEditEditorProc, NAV_TARGET_CLASS_ID, 0);

		size_t trueMaxLen = max(strlen(text), maxCharacters);
		if (trueMaxLen == 0) trueMaxLen = 32768;
		SendMessageA(*editor, EM_SETLIMITTEXT, trueMaxLen, 0);
		return editor;
	}

	Rococo::Windows::IWin32SuperComboBox* AddOptionsList(ISuperListSpec& spec, const VisualStyle& style, IParentWindowSupervisor& panel, cstr currentOptionText, int yOffset, size_t maxCharacters, UI::SysWidgetId id)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		auto* editor = Rococo::Windows::AddSuperComboBox(panel, spec, GuiRect{ style.labelSpan, yOffset, containerRect.right, yOffset + style.rowHeight }, currentOptionText, id.value, WS_VISIBLE, 0);

		size_t trueMaxLen = max(strlen(currentOptionText), maxCharacters);
		if (trueMaxLen == 0) trueMaxLen = 32768;
		SendMessageA(*editor, EM_SETLIMITTEXT, trueMaxLen, 0);
		return editor;
	}

	HWND AddCheckbox(const VisualStyle& style, IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId id, IOwnerDrawItem* item)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		HWND checkbox = CreateWindowExA(0, "BUTTON", "", WS_CHILD | BS_OWNERDRAW, style.labelSpan, yOffset, containerRect.right, style.rowHeight, panel, (HMENU)id.value, GetModuleHandle(NULL), NULL);
		if (!checkbox)
		{
			Throw(GetLastError(), "Error creating AbEditor checkbox");
		}

		SetWindowLongPtrA(checkbox, GWLP_USERDATA, (LONG_PTR)item);
		SetWindowSubclass(checkbox, AbEditCheckBoxProc, ABEDIT_BTN_CLASS_ID, 0);
		ShowWindow(checkbox, SW_SHOW);

		return checkbox;
	}

	Rococo::Windows::IWindowSupervisor* AddImmutableEditor(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, int yOffset, UI::SysWidgetId id)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		auto* editor = Rococo::Windows::AddEditor(panel, GuiRect{ style.labelSpan, yOffset, containerRect.right, yOffset + style.rowHeight }, text, id.value, WS_VISIBLE | ES_READONLY, 0);
		return editor;
	}

	GuiRect GetEditorRect(const VisualStyle& style, IParentWindowSupervisor& panel, int yOffset)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		GuiRect totalRect{ style.defaultPadding, yOffset, containerRect.right - style.rightPadding, yOffset + style.rowHeight };
		return totalRect;
	}

	GuiRect LayoutSimpleEditor(IParentWindowSupervisor& panel, const VisualStyle& style, UI::SysWidgetId labelId, HWND hEditor, int yOffset)
	{
		HWND hLabel = GetDlgItem(panel, labelId.value);

		RECT rect;
		GetClientRect(panel, &rect);

		MoveWindow(hLabel, 0, yOffset, style.labelSpan - style.defaultPadding, style.rowHeight, TRUE);
		MoveWindow(hEditor, style.labelSpan, yOffset, rect.right - style.labelSpan, style.rowHeight, TRUE);

		return GetEditorRect(style, panel, yOffset);
	}

	bool TryGetEditorString(IWindowSupervisor* editor, REF HString& value)
	{
		if (!editor)
		{
			return false;
		}

		LRESULT nChars = SendMessage(*editor, WM_GETTEXTLENGTH, 0, 0);
		if (nChars < (LONG)1_megabytes && nChars >= 0)
		{
			std::vector<char> buf;
			buf.resize(nChars + 1);

			LRESULT nCharsCopied = SendMessage(*editor, WM_GETTEXT, buf.size(), (LPARAM)buf.data());
			if (nCharsCopied > 0)
			{
				value = buf.data();
				return true;
			}
		}

		return false;
	}

	struct StringProperty : IPropertySupervisor
	{
		size_t capacity;
		HString id;
		HString initialString;
		HString displayName;
		VisualStyle style;
		bool isDirty = false;
		IPropertyUIEvents& events;
		IWindowSupervisor* editor{ nullptr };
		UI::SysWidgetId labelId{ 0 };

		enum { HARD_CAP = 32767 };

		StringProperty(PropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int _capacity) :
			id(stub.propertyIdentifier), initialString(value), capacity(HARD_CAP), displayName(stub.displayName), events(stub.eventHandler)
		{
			if (_capacity > 0 && _capacity < HARD_CAP)
			{
				capacity = (size_t)_capacity;
			}

			if (stub.displayName == nullptr || *stub.displayName == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			if (stub.propertyIdentifier == nullptr || *stub.propertyIdentifier == 0)
			{
				Throw(0, "%s: '%s' blank id", __FUNCTION__, stub.displayName);
			}
		}

		void Free() override
		{
			delete this;
		}

		void AdvanceSelection() override
		{

		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId editorId)
		{
			this->labelId = labelId;
			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddEditor(style, panel, initialString.c_str(), capacity, yOffset, editorId);
			return GetEditorRect(style, panel, yOffset);
		}

		GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) override
		{
			return LayoutSimpleEditor(panel, style, labelId, *editor, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(UI::SysWidgetId id) const
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		UI::SysWidgetId ControlId() const
		{
			if (!editor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return Internal::TryGetEditorString(editor, REF value);
		}

		void OnButtonClicked() override
		{

		}

		void OnEditorChanged() override
		{
			isDirty = true;
		}

		void OnEditorLostKeyboardFocus() override
		{
			events.OnPropertyEditorLostFocus(*this);

			if (*editor) InvalidateRect(*editor, NULL, TRUE);
		}

		bool IsDirty() const override
		{
			return isDirty;
		}

		bool TryTakeFocus() override
		{
			if (!editor)
			{
				return false;
			}

			SetFocus(*editor);
			InvalidateRect(*editor, NULL, TRUE);
			return true;
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			UNUSED(sizeOfData);
			cstr text = (cstr)data;
			if (*editor) SetWindowTextA(*editor, text);
		}
	};

	struct StringConstant : IPropertySupervisor
	{
		HString propertyId;
		HString initialString;
		HString displayName;
		VisualStyle style;
		bool isDirty = false;

		UI::SysWidgetId labelId{ 0 };

		IWindowSupervisor* editor{ nullptr };

		enum { HARD_CAP = 32767 };

		StringConstant(cstr _propertyId, cstr _displayName, cstr _initialString) :
			propertyId(_propertyId), displayName(_displayName), initialString(_initialString)
		{
			if (_propertyId == nullptr || *_propertyId == 0)
			{
				Throw(0, "%s: blank property Id", __FUNCTION__);
			}

			if (_displayName == nullptr || *_displayName == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			if (_initialString == nullptr || *_initialString == 0)
			{
				Throw(0, "%s: '%s' blank initial string", __FUNCTION__, _displayName);
			}
		}

		void AdvanceSelection() override
		{

		}

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId constantId) override
		{
			this->labelId = labelId;

			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddImmutableEditor(style, panel, initialString.c_str(), yOffset, constantId);
			return GetEditorRect(style, panel, yOffset);
		}

		GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) override
		{
			return LayoutSimpleEditor(panel, style, labelId, *editor, yOffset);
		}

		cstr Id() const override
		{
			return propertyId;
		}

		bool IsForControl(UI::SysWidgetId id) const override
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		UI::SysWidgetId ControlId() const override
		{
			if (!editor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value) override
		{
			return Internal::TryGetEditorString(editor, REF value);
		}

		bool TryTakeFocus() override
		{
			return false;
		}

		void OnEditorChanged() override
		{
			isDirty = true;
		}

		void OnButtonClicked() override
		{

		}

		void OnEditorLostKeyboardFocus() override
		{
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			UNUSED(sizeOfData);
			cstr text = (cstr)data;
			if (*editor) SetWindowTextA(*editor, text);
		}

		bool IsDirty() const override
		{
			return isDirty;
		}
	};

	static bool IsDigit(char c)
	{
		return c >= '0' && c <= '9';
	}

	template<class VALUE_TYPE>
	struct EditorFilter
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			UNUSED(nChars);
			UNUSED(targetBuffer);
			return false;
		}
	};

	template<>
	struct EditorFilter<int32>
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			char* writeCursor = targetBuffer;
			const char* readCursor = targetBuffer;
			const char* endChar = targetBuffer + nChars;

			if (*targetBuffer == '-' || *targetBuffer == '+')
			{
				readCursor++;
				writeCursor++;
			}

			for (; readCursor < endChar; readCursor++)
			{
				if (!IsDigit(*readCursor))
				{
					// Illegal character -> skip to next
					continue;
				}
				else
				{
					if (writeCursor != readCursor)
					{
						*writeCursor = *readCursor;
					}

					writeCursor++;
				}
			}

			if (readCursor != writeCursor)
			{
				*writeCursor = 0;
				return true;
			}

			return false;
		}
	};

	template<>
	struct EditorFilter<int64>
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			EditorFilter<int32> subFilter;
			return subFilter.RemoveIllegalCharacters(targetBuffer, nChars);
		}
	};

	template<>
	struct EditorFilter<uint32>
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			char* writeCursor = targetBuffer;
			const char* endChar = targetBuffer + nChars;
			const char* readCursor = targetBuffer;

			for (; readCursor < endChar; readCursor++)
			{
				if (!IsDigit(*readCursor))
				{
					// Illegal character -> skip to next
					*readCursor++;
				}
				else
				{
					if (writeCursor != readCursor)
					{
						*writeCursor = *readCursor;
					}

					writeCursor++;
				}
			}

			if (readCursor != writeCursor)
			{
				*writeCursor = 0;
				return true;
			}

			return false;
		}
	};

	template<>
	struct EditorFilter<uint64>
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			EditorFilter<uint32> subFilter;
			return subFilter.RemoveIllegalCharacters(targetBuffer, nChars);
		}
	};

	template<>
	struct EditorFilter<double>
	{
		bool IsLegal(char c, REF int& dotCount)
		{
			if (dotCount == 0 && c == '.')
			{
				dotCount++;
				return true;
			}

			return IsDigit(c);
		}

		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			char* writeCursor = targetBuffer;
			const char* readCursor = targetBuffer;
			const char* endChar = targetBuffer + nChars;
			int dotCount = 0;

			if (*targetBuffer == '-' || *targetBuffer == '+')
			{
				readCursor++;
				writeCursor++;
			}

			for (; readCursor < endChar; readCursor++)
			{
				if (!IsLegal(*readCursor, REF dotCount))
				{
					// Illegal character -> skip to next
				}
				else
				{
					if (writeCursor != readCursor)
					{
						*writeCursor = *readCursor;
					}

					writeCursor++;
				}
			}

			if (readCursor != writeCursor)
			{
				*writeCursor = 0;
				return true;
			}

			return false;
		}
	};

	template<>
	struct EditorFilter<float>
	{
		bool RemoveIllegalCharacters(char* targetBuffer, size_t nChars)
		{
			EditorFilter<double> subFilter;
			return subFilter.RemoveIllegalCharacters(targetBuffer, nChars);
		}
	};

	template<class VALUE_TYPE>
	struct PrimitiveProperty : IPropertySupervisor
	{
		HString id;
		VALUE_TYPE initialValue;
		HString displayName;
		VisualStyle style;
		bool isDirty = false;

		IWindowSupervisor* editor{ nullptr };

		IValueValidator<VALUE_TYPE>& validator;
		IValueFormatter<VALUE_TYPE>& formatter;
		IPropertyUIEvents& events;

		UI::SysWidgetId labelId{ 0 };

		PrimitiveProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<VALUE_TYPE>& marshaller) :
			id(stub.propertyIdentifier), initialValue(marshaller.value), displayName(stub.displayName), validator(marshaller.validator), formatter(marshaller.formatter), events(stub.eventHandler)
		{
			if (displayName.length() == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			validator.ThrowIfBad(initialValue, EValidationPurpose::Construction);

			if (id.length() == 0)
			{
				Throw(0, "%s: '%s' blank id", __FUNCTION__, stub.displayName);
			}
		}

		void AdvanceSelection() override
		{

		}

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId editorId) override
		{
			enum { MAX_PRIMITIVE_LEN = 24 };

			char initialText[MAX_PRIMITIVE_LEN];
			formatter.Format(initialText, MAX_PRIMITIVE_LEN, initialValue);

			this->labelId = labelId;

			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddEditor(style, panel, initialText, MAX_PRIMITIVE_LEN, yOffset, editorId);
			return GetEditorRect(style, panel, yOffset);
		}

		GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) override
		{
			return LayoutSimpleEditor(panel, style, labelId, *editor, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(UI::SysWidgetId id) const override
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		UI::SysWidgetId ControlId() const override
		{
			if (!editor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return Internal::TryGetEditorString(editor, REF value);
		}

		bool TryTakeFocus() override
		{
			if (!editor)
			{
				return false;
			}

			SetFocus(*editor);
			LRESULT nChars = SendMessage(*editor, WM_GETTEXTLENGTH, 0, 0);
			SendMessage(*editor, EM_SETSEL, nChars, nChars);

			InvalidateRect(*editor, NULL, TRUE);
			UpdateWindow(*editor);

			return true;
		}

		void OnButtonClicked() override
		{

		}

		std::vector<char> textBuffer;

		void OnEditorChanged() override
		{
			if (!editor) return;

			isDirty = true;

			LRESULT nChars = SendMessage(*editor, WM_GETTEXTLENGTH, 0, 0);
			if (nChars <= 0)
			{
				return;
			}

			textBuffer.resize(nChars + 1);

			LRESULT nCharsCopied = SendMessageA(*editor, WM_GETTEXT, textBuffer.size(), (LPARAM)textBuffer.data());

			DWORD start, end;
			SendMessage(*editor, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

			EditorFilter<VALUE_TYPE> filter;
			if (!filter.RemoveIllegalCharacters(textBuffer.data(), (size_t)nCharsCopied))
			{
				return;
			}

			// If a bad key was removed from the middle of the string then the cursor selection would have advanced by one. 
			// We need to retract it by 1 in this case, otherwise bad keys serve as cursor forward, which looks odd
			if (start == end && end > 0 && end < strlen(textBuffer.data()))
			{
				end--;
				start--;
			}

			SendMessageA(*editor, WM_SETTEXT, 0, (LPARAM)textBuffer.data());
			SendMessage(*editor, EM_SETSEL, end, end);
		}

		void OnEditorLostKeyboardFocus() override
		{
			events.OnPropertyEditorLostFocus(*this);

			if (*editor) InvalidateRect(*editor, NULL, TRUE);
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			if (sizeOfData != sizeof(VALUE_TYPE))
			{
				Throw(0, "%s: bad size match", __FUNCTION__);
			}

			if (editor)
			{
				VALUE_TYPE value = *(const VALUE_TYPE*)data;

				char text[64];
				formatter.Format(text, sizeof text, value);
				SendMessageA(*editor, WM_SETTEXT, 0, (LPARAM)text);
			}
		}

		bool IsDirty() const override
		{
			return isDirty;
		}
	};

	struct StdVectorPopulateDelegate : IStringPopulator
	{
		std::vector<char>& dest;

		StdVectorPopulateDelegate(std::vector<char>& _dest) : dest(_dest)
		{

		}

		void Populate(cstr text) override
		{
			CopyString(dest.data(), dest.size(), text);
		}
	};

	struct OptionVectorProperty : IPropertySupervisor, ISuperListSpec, ISuperListEvents
	{
		AutoFree<IEnumVectorSupervisor> enumVector;
		HString id;
		HString displayName;
		VisualStyle style;
		IWin32SuperComboBox* selectedOptionEditor{ nullptr };
		HString selectedText;
		int stringCapacity = 0;
		std::vector<char> keyBuffer;
		std::vector<char> descBuffer;

		IPropertyUIEvents& events;
		UI::SysWidgetId labelId{ 0 };

		OptionVectorProperty(PropertyMarshallingStub& stub, IEnumVectorSupervisor* newOwnedEnumVector, REF OptionRef& opt, size_t _stringCapacity) :
			enumVector(newOwnedEnumVector), displayName(stub.displayName), id(stub.propertyIdentifier), selectedText(opt.value), events(stub.eventHandler)
		{
			if (!enumVector)
			{
				Throw(0, "%s: No enum vector", __FUNCTION__);
			}

			if (_stringCapacity > 0x7FFFULL)
			{
				Throw(0, "%s: <%s> <%s> String capacity exceeded maximum of 32767 characters", __FUNCTION__, stub.propertyIdentifier, stub.displayName);
			}

			if (_stringCapacity < 2)
			{
				Throw(0, "%s: <%s> <%s> String capacity needs to be 2 or more characters", __FUNCTION__, stub.propertyIdentifier, stub.displayName);
			}

			keyBuffer.resize(_stringCapacity);
			descBuffer.resize(_stringCapacity);
		}

		void AdvanceSelection() override
		{

		}

		// Editors::ISuperListSpec method
		ISuperListEvents& EventHandler() override
		{
			return *this;
		}

		// Editors::ISuperListEvents method
		void OnDoubleClickAtSelection(size_t index) override
		{
			Select(index);
		}

		// Editors::ISuperListEvents method
		void OnReturnAtSelection(size_t index) override
		{
			Select(index);
		}

		void Select(size_t index)
		{
			if (index < enumVector->Count())
			{
				StdVectorPopulateDelegate delegateToKey(keyBuffer);
				enumVector->GetEnumName(index, delegateToKey);

				selectedText = keyBuffer.data();

				selectedOptionEditor->ComboBuilder().SetSelection(selectedText);
				selectedOptionEditor->ComboBuilder().HidePopup();
			}
		}

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId editorId)
		{
			this->labelId = labelId;

			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (selectedOptionEditor)
			{
				Throw(0, "%s: unexpected non-null selectedOptionEditor for %s", __FUNCTION__, displayName.c_str());
			}

			selectedOptionEditor = Internal::AddOptionsList(*this, style, panel, selectedText, yOffset, stringCapacity, editorId);

			auto& builder = selectedOptionEditor->ListBuilder();

			builder.AddColumnWithMaxWidth("Option");

			for (size_t i = 0; i < enumVector->Count(); ++i)
			{
				StdVectorPopulateDelegate delegateToKey(keyBuffer);
				enumVector->GetEnumName(i, delegateToKey);

				StdVectorPopulateDelegate delegateToDesc(descBuffer);
				enumVector->GetEnumDescription(i, delegateToDesc);

				// Column zero is keyed for the enum name, but also displays the key name
				builder.AddKeyValue(keyBuffer.data(), keyBuffer.data());
			}

			return GetEditorRect(style, panel, yOffset);
		}

		GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) override
		{
			return LayoutSimpleEditor(panel, style, labelId, *selectedOptionEditor, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(UI::SysWidgetId id) const override
		{
			return selectedOptionEditor && GetWindowLongPtrA(*selectedOptionEditor, GWLP_ID) == id.value;
		}

		UI::SysWidgetId ControlId() const override
		{
			if (!selectedOptionEditor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*selectedOptionEditor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return Internal::TryGetEditorString(selectedOptionEditor, REF value);
		}

		void OnButtonClicked() override
		{
		}

		void OnEditorChanged() override
		{
		}

		void OnEditorLostKeyboardFocus() override
		{
			if (*selectedOptionEditor) InvalidateRect(*selectedOptionEditor, NULL, TRUE);
		}

		bool TryTakeFocus() override
		{
			if (!selectedOptionEditor)
			{
				return false;
			}

			SetFocus(*selectedOptionEditor);
			InvalidateRect(*selectedOptionEditor, NULL, TRUE);

			return true;
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			UNUSED(data);
			UNUSED(sizeOfData);
		}

		bool IsDirty() const override
		{
			return false;
		}
	};

	struct BooleanProperty : IPropertySupervisor, IOwnerDrawItem
	{
		HString id;
		bool initialValue;
		HString displayName;
		VisualStyle style;
		bool isDirty = false;

		HWND checkbox = nullptr;

		IValueValidator<bool>& validator;
		IPropertyUIEvents& events;

		UI::SysWidgetId labelId{ 0 };

		BooleanProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller) :
			id(stub.propertyIdentifier), initialValue(marshaller.value), displayName(stub.displayName), validator(marshaller.validator), events(stub.eventHandler)
		{
			if (displayName.length() == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			validator.ThrowIfBad(initialValue, EValidationPurpose::Construction);

			if (id.length() == 0)
			{
				Throw(0, "%s: '%s' blank id", __FUNCTION__, stub.displayName);
			}
		}

		void AdvanceSelection() override
		{
			initialValue = !initialValue;
			OnEditorChanged();
			events.OnBooleanButtonChanged(*this);
		}

		void Free() override
		{
			delete this;
		}

		void DrawTick(HDC dc, const RECT& buttonRect)
		{
			HPEN hPen = CreatePen(PS_SOLID, 4, RGB(0, 192, 0));
			HPEN hOldPen = (HPEN)SelectObject(dc, hPen);

			// The downslash
			MoveToEx(dc, buttonRect.left + 2, (buttonRect.bottom + buttonRect.top) / 2 - 2, NULL);
			LineTo(dc, (buttonRect.left + buttonRect.right) / 2, buttonRect.bottom - 2);

			// The upslash
			LineTo(dc, buttonRect.right + 6, buttonRect.top - 2);

			SelectObject(dc, hOldPen);
			DeleteObject(hPen);
		}

		void DrawTickRect(HDC dc, const RECT& buttonRect)
		{
			HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
			HPEN hOldPen = (HPEN)SelectObject(dc, hPen);

			MoveToEx(dc, buttonRect.left, buttonRect.bottom, NULL);
			LineTo(dc, buttonRect.right, buttonRect.bottom);
			LineTo(dc, buttonRect.right, buttonRect.top);
			LineTo(dc, buttonRect.left, buttonRect.top);
			LineTo(dc, buttonRect.left, buttonRect.bottom);

			SelectObject(dc, hOldPen);

			DeleteObject(hPen);
		}

		void GetTickRect(OUT RECT& rect)
		{
			RECT r;
			if (!GetClientRect(checkbox, &r))
			{
				rect = { -1, -1, -1, -1 };
				return;
			}

			int height = r.bottom;

			rect.left = r.left + 3;
			rect.top = r.top + 3;
			rect.bottom = r.bottom - 3;
			rect.right = rect.left + height - 6;
		}

		void DrawItem(DRAWITEMSTRUCT& d) override
		{
			RECT r;
			GetClientRect(checkbox, &r);

			bool isFocus = GetFocus() == checkbox;
			HBRUSH hBrush = CreateSolidBrush(isFocus ? RGB(255, 224, 224) : RGB(255, 255, 255));
			FillRect(d.hDC, &r, hBrush);

			RECT buttonRect;
			GetTickRect(OUT buttonRect);

			DrawTickRect(d.hDC, buttonRect);

			if (initialValue)
			{
				DrawTick(d.hDC, buttonRect);
			}

			DeleteObject(hBrush);
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, UI::SysWidgetId labelId, UI::SysWidgetId editorId)
		{
			enum { MAX_PRIMITIVE_LEN = 24 };

			this->labelId = labelId;
			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (checkbox)
			{
				Throw(0, "%s: unexpected non-null checkbox for %s", __FUNCTION__, displayName.c_str());
			}

			checkbox = Internal::AddCheckbox(style, panel, yOffset, editorId, static_cast<IOwnerDrawItem*>(this));
			return GetEditorRect(style, panel, yOffset);
		}

		GuiRect Layout(IParentWindowSupervisor& panel, int yOffset) override
		{
			return LayoutSimpleEditor(panel, style, labelId, checkbox, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(UI::SysWidgetId id) const override
		{
			return checkbox && GetWindowLongPtrA(checkbox, GWLP_ID) == id.value;
		}

		UI::SysWidgetId ControlId() const override
		{
			if (!checkbox)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(checkbox, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			if (!checkbox)
			{
				return false;
			}

			value = initialValue ? "true" : "false";

			return true;
		}

		bool TryTakeFocus() override
		{
			if (!checkbox)
			{
				return false;
			}

			if (checkbox) SetFocus(checkbox);
			if (checkbox) InvalidateRect(checkbox, NULL, TRUE);
			return true;
		}

		void OnButtonClicked() override
		{
			POINT pos;
			if (!GetCursorPos(&pos))
			{
				return;
			}

			if (!ScreenToClient(checkbox, &pos))
			{
				return;
			}

			RECT buttonRect;
			GetTickRect(OUT buttonRect);

			if (pos.x > buttonRect.left && pos.x < buttonRect.right && pos.y > buttonRect.top && pos.y < buttonRect.bottom)
			{
				AdvanceSelection();
			}
		}

		void OnEditorChanged() override
		{
			if (!checkbox) return;
			isDirty = true;
			InvalidateRect(checkbox, NULL, TRUE);
		}

		void OnEditorLostKeyboardFocus() override
		{
			events.OnPropertyEditorLostFocus(*this);
			if (checkbox) InvalidateRect(checkbox, NULL, TRUE);
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			if (sizeOfData != sizeof(bool))
			{
				Throw(0, "%s: bad size match", __FUNCTION__);
			}

			if (checkbox)
			{
				initialValue = *(const bool*)data;
				InvalidateRect(checkbox, NULL, TRUE);
			}
		}

		bool IsDirty() const override
		{
			return isDirty;
		}
	};

	struct PropertyEditorEnsemble;

	struct PropertyBuilder : IPropertyVisitor
	{
		PropertyEditorEnsemble& container;

		PropertyBuilder(PropertyEditorEnsemble& _container) : container(_container)
		{

		}

		bool IsWritingToReferences() const override
		{
			return false;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(PropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)  override;
	};

	struct PropertyRefresher : IPropertyVisitor
	{
		PropertyEditorEnsemble& container;

		// The lifetime of the property refresher is withing the lifetime of the propertyId pointer
		cstr onlyThisPropertyId;

		PropertyRefresher(PropertyEditorEnsemble& _container, cstr _propertyId) : container(_container), onlyThisPropertyId(_propertyId)
		{

		}

		bool IsWritingToReferences() const override
		{
			return false;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(PropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc) override;
	};

	struct PropertyEventRouting : IPropertyVisitor
	{
		PropertyEditorEnsemble& container;
		cstr id;

		PropertyEventRouting(PropertyEditorEnsemble& _container, cstr _id) : container(_container), id(_id)
		{

		}

		bool IsWritingToReferences() const override
		{
			return true;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(PropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc) override;
	};

	struct AssertiveNullEventHandler : Rococo::Reflection::IPropertyUIEvents
	{
		void OnPropertyEditorLostFocus(IPropertyEditor& property) override
		{
			Throw(0, "%s: property %s incorrectly raised lost focus event", __FUNCTION__, property.Id());
		}

		void OnBooleanButtonChanged(IPropertyEditor& property) override
		{
			Throw(0, "%s: property %s incorrectly raised button changed event", __FUNCTION__, property.Id());
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			UNUSED(agent);
			Throw(0, "%s: property %s incorrectly raised dependency variable change. Check to see if the agent raised such an event when visitor.IsWritingToReferences() was false", __FUNCTION__, propertyId);
		}
	};

	struct PropertyEditorEnsemble : IUIPropertiesEditorSupervisor
	{
		IParentWindowSupervisor& panelArea;

		std::vector<IPropertySupervisor*> properties;
		stringmap< IPropertySupervisor*> identifierToProperty;

		PropertyBuilder builder;

		UI::SysWidgetId nextId{ 0 };

		PropertyEditorEnsemble(IParentWindowSupervisor& _panelArea) : panelArea(_panelArea), builder(*this)
		{
		}

		~PropertyEditorEnsemble()
		{
			Clear();
		}

		void LayouVertically() override
		{
			int lastY = 2;

			for (auto* p : properties)
			{
				GuiRect rect = p->Layout(panelArea, lastY);
				lastY = rect.bottom + 2;
			}

			InvalidateRect(panelArea, NULL, TRUE);
		}

		UI::SysWidgetId Next()
		{
			nextId.value++;
			return nextId;
		}

		void Clear()
		{
			nextId = { 0 };

			for (auto* p : properties)
			{
				p->Free();
			}

			properties.clear();
			identifierToProperty.clear();
		}

		void Free() override
		{
			delete this;
		}

		IPropertySupervisor* FindControlById(UI::SysWidgetId id)
		{
			for (auto* p : properties)
			{
				if (p->IsForControl(id))
				{
					return p;
				}
			}

			return nullptr;
		}

		IPropertyEditor* FindPropertyById(cstr propertyIdentifier)
		{
			for (auto* p : properties)
			{
				if (Eq(p->Id(), propertyIdentifier))
				{
					return p;
				}
			}

			return nullptr;
		}

		void NavigateByTabFrom(UI::SysWidgetId id, int delta)
		{
			if (delta > 0)
			{
				for (size_t i = 0; i < properties.size() - 1; i++)
				{
					auto& currentFocus = properties[i];
					if (currentFocus->IsForControl(id))
					{
						i++;
						while (i < properties.size())
						{
							if (properties[i]->TryTakeFocus())
							{
								return;
							}

							i++;
						}
						return;
					}
				}

				// If the final property is the target then begin enumerating new targets from slot zero
				if (properties.size() > 0 && properties[properties.size() - 1]->IsForControl(id))
				{
					for (size_t i = 0; i < properties.size(); i++)
					{
						if (properties[i]->TryTakeFocus())
						{
							return;
						}
					}
				}
			}
			else
			{
				for (size_t i = 1; i < properties.size(); i++)
				{
					auto& currentFocus = properties[i];
					if (currentFocus->IsForControl(id))
					{
						i--;
						while (i >= 0)
						{
							if (properties[i]->TryTakeFocus())
							{
								return;
							}

							i--;
						}
						return;
					}
				}

				// If the final property is the target then begin enumerating new targets from slot zero
				if (properties.size() > 0 && properties[0]->IsForControl(id))
				{
					for (size_t i = properties.size() - 1; i >= 1; i--)
					{
						if (properties[i]->TryTakeFocus())
						{
							return;
						}
					}
				}
			}
		}

		void AdvanceSelection(UI::SysWidgetId id)
		{
			for (auto* p : properties)
			{
				if (p->IsForControl(id))
				{
					p->AdvanceSelection();
					return;
				}
			}
		}

		void OnEditorChanged(UI::SysWidgetId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnEditorChanged();
		}

		void OnButtonClicked(UI::SysWidgetId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnButtonClicked();
		}

		void OnEditorLostKeyboardFocus(UI::SysWidgetId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnEditorLostKeyboardFocus();
		}

		void Populate()
		{
			int lastY = 2;

			for (auto* p : properties)
			{
				GuiRect rect = p->AddToPanel(panelArea, lastY, Next(), Next());
				lastY = rect.bottom + 2;

				if (!identifierToProperty.insert(p->Id(), p).second)
				{
					Throw(0, "Property with id [%s] had a duplicate. Property identifiers must be unique", p->Id());
				}
			}
		}

		void BuildEditorsForProperties(IPropertyVenue& venue) override
		{
			Clear();
			venue.VisitVenue(builder);
			Populate();
		}

		void UpdateFromVisuals(IPropertyEditor& p, IPropertyVenue& venue) override
		{
			PropertyEventRouting eventRouting(*this, p.Id());
			venue.VisitVenue(eventRouting);
		}

		bool TryGetEditorString(cstr propertyIdentifier, REF HString& value) override
		{
			auto i = identifierToProperty.find(propertyIdentifier);
			if (i == identifierToProperty.end())
			{
				return false;
			}

			return i->second->TryGetEditorString(REF value);
		}

		void Refresh(cstr onlyThisPropertyId, IEstateAgent& agent) override
		{
			PropertyRefresher refresher(*this, onlyThisPropertyId);
			AssertiveNullEventHandler throwOnEvent;

			// Enumerate through the agent's properties and refreshes those that match the id supplied here
			agent.AcceptVisit(refresher, throwOnEvent);
		}
	};

	void PropertyBuilder::VisitHeader(cstr propertyId, cstr displayName, cstr displayText)
	{
		container.properties.push_back(new StringConstant(propertyId, displayName, displayText));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
	{
		container.properties.push_back(new StringProperty(stub, REF value, capacity));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<int32>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<uint32>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<int64>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<uint64>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<float>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<double>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller)
	{
		container.properties.push_back(new BooleanProperty(stub, marshaller));
	}

	void PropertyBuilder::VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
	{
		UNUSED(stub);
		UNUSED(value);
		UNUSED(stringCapacity);
		UNUSED(enumDesc);

		AutoFree<IEnumVectorSupervisor> enumVector = enumDesc.CreateEnumList();
		if (enumVector)
		{
			container.properties.push_back(new OptionVectorProperty(stub, enumVector, REF value, stringCapacity));
			enumVector.Detach();
		}
	}

	void PropertyRefresher::VisitHeader(cstr propertyId, cstr displayName, cstr displayText)
	{
		UNUSED(displayName);
		IPropertyEditor* p = Eq(propertyId, this->onlyThisPropertyId) ? container.FindPropertyById(propertyId) : nullptr;
		if (p) p->UpdateWidget((const void*)displayText, strlen(displayText));
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
	{
		UNUSED(value);
		UNUSED(capacity);
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(value.c_str(), value.length());
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller)
	{
		IPropertyEditor* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
	{
		UNUSED(stub);
		UNUSED(value);
		UNUSED(stringCapacity);
		UNUSED(enumDesc);
	}

	void PropertyEventRouting::VisitHeader(cstr id, cstr displayName, cstr displayText)
	{
		UNUSED(id);
		UNUSED(displayName);
		UNUSED(displayText);
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
	{
		UNUSED(stub);
		UNUSED(capacity);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = newValue;
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int32>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = atoi(newValue);
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<int64>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = atoll(newValue);
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<float>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = (float)atof(newValue);
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<double>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = atof(newValue);
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<bool>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = Eq(newValue, "true");
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint32>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				uint32 newUint32;
				if (1 == sscanf_s(newValue.c_str(), "%u", &newUint32))
				{
					marshaller.value = newUint32;
				}
			}
		}
	}

	void PropertyEventRouting::VisitProperty(PropertyMarshallingStub& stub, PrimitiveMarshaller<uint64>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				uint64 newUint64;
				if (1 == sscanf_s(newValue.c_str(), "%llu", &newUint64))
				{
					marshaller.value = newUint64;
				}
			}
		}
	}

	void PropertyEventRouting::VisitOption(PropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
	{
		UNUSED(stub);
		UNUSED(value);
		UNUSED(stringCapacity);
		UNUSED(enumDesc);
	}
}

namespace Rococo::Windows
{
	ROCOCO_WINDOWS_API Editors::IUIPropertiesEditorSupervisor* CreatePropertiesEditor(Rococo::Windows::IParentWindowSupervisor& panelArea)
	{
		return new Internal::PropertyEditorEnsemble(panelArea);
	}
}
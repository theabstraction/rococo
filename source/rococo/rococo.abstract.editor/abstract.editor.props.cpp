#include <rococo.abstract.editor.win32.h>
#include <rococo.window.h>
#include <vector>
#include <rococo.strings.h>
#include <rococo.validators.h>
#include <rococo.hashtable.h>
#include <rococo.editors.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::Abedit;
using namespace Rococo::Validators;

namespace ANON
{
	ROCOCO_INTERFACE IPropertySupervisor : Rococo::Abedit::IProperty
	{
		virtual void Free() = 0;
		virtual void OnButtonClicked() = 0;
		virtual void OnEditorChanged() = 0;
		virtual void OnEditorLostKeyboardFocus() = 0;
		virtual GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId) = 0;
	};

	struct VisualStyle
	{
		int minSpan = 200;
		int labelSpan = 100;
		int defaultPadding = 2;
		int rightPadding = 2;
		int rowHeight = 20;
	};

	void AddLabel(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, int yOffset, ControlPropertyId id)
	{
		Rococo::Windows::AddLabel(panel, GuiRect{ style.defaultPadding, yOffset, style.labelSpan - style.defaultPadding, yOffset + style.rowHeight }, text, id.value, WS_VISIBLE, 0);
	}

	Rococo::Windows::IWindowSupervisor* AddEditor(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, size_t maxCharacters, int yOffset, ControlPropertyId id)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		auto* editor = Rococo::Windows::AddEditor(panel, GuiRect{ style.labelSpan, yOffset, containerRect.right, yOffset + style.rowHeight }, text, id.value, WS_VISIBLE, 0);

		size_t trueMaxLen = max(strlen(text), maxCharacters);
		if (trueMaxLen == 0) trueMaxLen = 32768;
		SendMessageA(*editor, EM_SETLIMITTEXT, trueMaxLen, 0);
		return editor;
	}

	Rococo::Windows::IWin32SuperComboBox* AddOptionsList(Editors::ISuperListSpec& spec, const VisualStyle& style, IParentWindowSupervisor& panel, cstr currentOptionText, int yOffset, size_t maxCharacters, ControlPropertyId id)
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

	Rococo::Windows::IWindowSupervisor* AddCheckbox(const VisualStyle& style, IParentWindowSupervisor& panel, bool value, int yOffset, ControlPropertyId id)
	{
		RECT containerRect;
		GetClientRect(panel, &containerRect);

		if (containerRect.right < style.minSpan) containerRect.right = style.minSpan;

		auto* checkbox = Rococo::Windows::AddCheckBox(panel, GuiRect{ style.labelSpan, yOffset, containerRect.right, yOffset + style.rowHeight }, "", id.value, WS_VISIBLE | BS_CHECKBOX, 0);
		if (checkbox) SendMessage(*checkbox, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
		return checkbox;
	}

	Rococo::Windows::IWindowSupervisor* AddImmutableEditor(const VisualStyle& style, IParentWindowSupervisor& panel, cstr text, int yOffset, ControlPropertyId id)
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

	bool TryGetEditorString(IWindowSupervisor* editor, REF HString& value)
	{
		if (!editor)
		{
			return false;
		}

		LRESULT nChars = SendMessage(*editor, WM_GETTEXTLENGTH, 0, 0);
		if (nChars < (LONG) 1_megabytes && nChars >= 0)
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

		enum { HARD_CAP = 32767 };

		StringProperty(UIPropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int _capacity):
			id(stub.propertyIdentifier), initialString(value), capacity(HARD_CAP), displayName(stub.displayName), events(stub.eventHandler)
		{
			if (_capacity > 0 && _capacity < HARD_CAP)
			{
				capacity = (size_t) _capacity;
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

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId)
		{
			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddEditor(style, panel, initialString.c_str(), capacity, yOffset, editorId);
			return GetEditorRect(style, panel, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(ControlPropertyId id) const
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		ControlPropertyId ControlId() const
		{
			if (!editor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(editor, REF value);
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
		}

		bool IsDirty() const override
		{
			return isDirty;
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

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId constantId) override
		{
			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddImmutableEditor(style, panel, initialString.c_str(), yOffset, constantId);
			return GetEditorRect(style, panel, yOffset);
		}

		cstr Id() const override
		{
			return propertyId;
		}

		bool IsForControl(ControlPropertyId id) const override
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		ControlPropertyId ControlId() const override
		{
			if (!editor)
			{
				return {0};
			}

			return { (uint16) GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value) override
		{
			return ANON::TryGetEditorString(editor, REF value);
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

		PrimitiveProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<VALUE_TYPE>&  marshaller) :
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

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId)
		{
			enum {MAX_PRIMITIVE_LEN = 24};

			char initialText[MAX_PRIMITIVE_LEN];
			formatter.Format(initialText, MAX_PRIMITIVE_LEN, initialValue);

			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (editor)
			{
				Throw(0, "%s: unexpected non-null editor for %s", __FUNCTION__, displayName.c_str());
			}

			editor = AddEditor(style, panel, initialText, MAX_PRIMITIVE_LEN, yOffset, editorId);
			return GetEditorRect(style, panel, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(ControlPropertyId id) const
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		ControlPropertyId ControlId() const override
		{
			if (!editor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*editor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(editor, REF value);
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
			SendMessage(*editor, EM_GETSEL, (WPARAM) &start, (LPARAM) &end);

			EditorFilter<VALUE_TYPE> filter;
			if (!filter.RemoveIllegalCharacters(textBuffer.data(), (size_t)nCharsCopied))
			{
				return;
			}

			// If a bad key was removed from the middle of the string then the cursor selection would have advanced by one. 
			// We need to retract it by 1 in this case, otherwise bad keys serve as cursor forward, which looks odd
			if (start == end  && end > 0 && end < strlen(textBuffer.data()))
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

	struct StdVectorPopulateDelegate: IStringPopulator
	{
		std::vector<char>& dest;

		StdVectorPopulateDelegate(std::vector<char>& _dest): dest(_dest)
		{

		}

		void Populate(cstr text) override
		{
			CopyString(dest.data(), dest.size(), text);
		}
	};

	struct OptionVectorProperty : IPropertySupervisor, Editors::ISuperListSpec, Editors::ISuperListEvents
	{
		AutoFree<IEnumVectorSupervisor> enumVector;
		HString id;
		HString displayName;
		VisualStyle style;
		IWin32SuperComboBox* selectedOptionEditor { nullptr };
		HString selectedText;
		int stringCapacity = 0;
		std::vector<char> keyBuffer;
		std::vector<char> descBuffer;

		IPropertyUIEvents& events;

		OptionVectorProperty(UIPropertyMarshallingStub& stub, IEnumVectorSupervisor* newOwnedEnumVector, REF OptionRef& opt, size_t _stringCapacity):
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

		// Editors::ISuperListSpec method
		Editors::ISuperListEvents& EventHandler() override
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

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId)
		{
			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (selectedOptionEditor)
			{
				Throw(0, "%s: unexpected non-null selectedOptionEditor for %s", __FUNCTION__, displayName.c_str());
			}

			selectedOptionEditor = ANON::AddOptionsList(*this, style, panel, selectedText, yOffset, stringCapacity, editorId);

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

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(ControlPropertyId id) const
		{
			return selectedOptionEditor && GetWindowLongPtrA(*selectedOptionEditor, GWLP_ID) == id.value;
		}

		ControlPropertyId ControlId() const override
		{
			if (!selectedOptionEditor)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*selectedOptionEditor, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(selectedOptionEditor, REF value);
		}

		void OnButtonClicked() override
		{
		}

		void OnEditorChanged() override
		{
		}

		void OnEditorLostKeyboardFocus() override
		{
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

	struct BooleanProperty : IPropertySupervisor
	{
		HString id;
		bool initialValue;
		HString displayName;
		VisualStyle style;
		bool isDirty = false;

		IWindowSupervisor* checkbox = nullptr;

		IValueValidator<bool>& validator;
		IPropertyUIEvents& events;

		BooleanProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller) :
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

		void Free() override
		{
			delete this;
		}

		GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId)
		{
			enum { MAX_PRIMITIVE_LEN = 24 };

			AddLabel(style, panel, displayName.c_str(), yOffset, labelId);

			if (checkbox)
			{
				Throw(0, "%s: unexpected non-null checkbox for %s", __FUNCTION__, displayName.c_str());
			}

			GuiRect editorRect{  };
			checkbox = ANON::AddCheckbox(style, panel, initialValue, yOffset, editorId);
			return GetEditorRect(style, panel, yOffset);
		}

		cstr Id() const override
		{
			return id;
		}

		bool IsForControl(ControlPropertyId id) const
		{
			return checkbox && GetWindowLongPtrA(*checkbox, GWLP_ID) == id.value;
		}

		ControlPropertyId ControlId() const override
		{
			if (!checkbox)
			{
				return { 0 };
			}

			return { (uint16)GetWindowLongPtrA(*checkbox, GWLP_ID) };
		}

		bool TryGetEditorString(REF HString& value)
		{
			if (!checkbox)
			{
				return false;
			}

			LRESULT result = SendMessageA(*checkbox, BM_GETSTATE, 0, 0);
			value = (result & BST_CHECKED) != 0 ? "true" : "false";
			
			return true;
			
		}

		void OnButtonClicked() override
		{
			LRESULT result = SendMessageA(*checkbox, BM_GETSTATE, 0, 0);
			bool isChecked = (result & BST_CHECKED);
			SendMessageA(*checkbox, BM_SETCHECK, isChecked ? BST_UNCHECKED : BST_CHECKED, 0);
			OnEditorChanged();
			events.OnBooleanButtonChanged(*this);
		}

		void OnEditorChanged() override
		{
			if (!checkbox) return;
			isDirty = true;
		}

		void OnEditorLostKeyboardFocus() override
		{
			events.OnPropertyEditorLostFocus(*this);
		}

		void UpdateWidget(const void* data, size_t sizeOfData) override
		{
			if (sizeOfData != sizeof(bool))
			{
				Throw(0, "%s: bad size match", __FUNCTION__);
			}

			if (checkbox)
			{
				bool value = *(const bool*)data;

				SendMessageA(*checkbox, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
			}
		}

		bool IsDirty() const override
		{
			return isDirty;
		}
	};

	struct Properties;

	struct PropertyBuilder : IPropertyVisitor
	{
		Properties& container;

		PropertyBuilder(Properties& _container): container(_container)
		{

		}

		bool IsWritingToReferences() const override
		{
			return false;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)  override;
	};

	struct PropertyRefresher : IPropertyVisitor
	{
		Properties& container;

		// The lifetime of the property refresher is withing the lifetime of the propertyId pointer
		cstr onlyThisPropertyId;

		PropertyRefresher(Properties& _container, cstr _propertyId) : container(_container), onlyThisPropertyId(_propertyId)
		{

		}

		bool IsWritingToReferences() const override
		{
			return false;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc) override;
	};

	struct PropertyEventRouting : IPropertyVisitor
	{
		Properties& container;
		cstr id;

		PropertyEventRouting(Properties& _container, cstr _id) : container(_container), id(_id)
		{

		}

		bool IsWritingToReferences() const override
		{
			return true;
		}

		void VisitHeader(cstr propertyId, cstr displayName, cstr displayText) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, Rococo::Strings::HString& value, int capacity) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller) override;
		void VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller) override;
		void VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc) override;
	};

	struct AssertiveNullEventHandler : Rococo::Abedit::IPropertyUIEvents
	{
		void OnPropertyEditorLostFocus(IProperty& property) override
		{
			Throw(0, "%s: property %s incorrectly raised lost focus event", __FUNCTION__, property.Id());
		}

		void OnBooleanButtonChanged(IProperty& property) override
		{
			Throw(0, "%s: property %s incorrectly raised button changed event", __FUNCTION__, property.Id());
		}

		void OnDependentVariableChanged(cstr propertyId, IEstateAgent& agent) override
		{
			UNUSED(agent);
			Throw(0, "%s: property %s incorrectly raised dependency variable change. Check to see if the agent raised such an event when visitor.IsWritingToReferences() was false", __FUNCTION__, propertyId);
		}
	};

	struct Properties : IUIPropertiesSupervisor
	{
		IParentWindowSupervisor& panelArea;

		std::vector<IPropertySupervisor*> properties;
		stringmap< IPropertySupervisor*> identifierToProperty;

		PropertyBuilder builder;

		ControlPropertyId nextId{ 0 };

		Properties(IParentWindowSupervisor& _panelArea) : panelArea(_panelArea), builder(*this)
		{
		}

		~Properties()
		{
			Clear();
		}

		ControlPropertyId Next()
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

		IPropertySupervisor* FindControlById(ControlPropertyId id)
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

		IProperty* FindPropertyById(cstr propertyIdentifier)
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

		void OnEditorChanged(ControlPropertyId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnEditorChanged();
		}

		void OnButtonClicked(ControlPropertyId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnButtonClicked();
		}

		void OnEditorLostKeyboardFocus(ControlPropertyId id) override
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

		void UpdateFromVisuals(IProperty& p, IPropertyVenue& venue) override
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

		void Refresh(cstr onlyThisPropertyId, Rococo::Abedit::IEstateAgent& agent) override
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

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
	{
		container.properties.push_back(new StringProperty(stub, REF value, capacity));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<int32>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<uint32>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<int64>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<uint64>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<float>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller)
	{
		container.properties.push_back(new PrimitiveProperty<double>(stub, marshaller));
	}

	void PropertyBuilder::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller)
	{
		container.properties.push_back(new BooleanProperty(stub, marshaller));
	}

	void PropertyBuilder::VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
	{
		UNUSED(stub);
		UNUSED(value);
		UNUSED(stringCapacity);
		UNUSED(enumDesc);

		AutoFree<IEnumVectorSupervisor> enumVector = enumDesc.CreateEnumList();
		if (enumVector)
		{		
			container.properties.push_back(new OptionVectorProperty(stub, enumVector, REF value, stringCapacity));
			enumVector.Release();
		}
	}

	void PropertyRefresher::VisitHeader(cstr propertyId, cstr displayName, cstr displayText)
	{
		UNUSED(displayName);
		IProperty* p = Eq(propertyId, this->onlyThisPropertyId) ? container.FindPropertyById(propertyId) : nullptr;
		if (p) p->UpdateWidget((const void*) displayText, strlen(displayText));
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
	{
		UNUSED(value);
		UNUSED(capacity);
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(value.c_str(), value.length());
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller)
	{
		IProperty* p = Eq(stub.propertyIdentifier, this->onlyThisPropertyId) ? container.FindPropertyById(stub.propertyIdentifier) : nullptr;
		if (p) p->UpdateWidget(&marshaller.value, sizeof marshaller.value);
	}

	void PropertyRefresher::VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, REF Rococo::Strings::HString& value, int capacity)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int32>& marshaller)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<int64>& marshaller)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<float>& marshaller)
	{
		UNUSED(stub);

		if (Eq(this->id, stub.propertyIdentifier))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				marshaller.value = (float) atof(newValue);
			}
		}
	}

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<double>& marshaller)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<bool>& marshaller)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint32>& marshaller)
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

	void PropertyEventRouting::VisitProperty(UIPropertyMarshallingStub& stub, UIPrimitiveMarshaller<uint64>& marshaller)
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

	void PropertyEventRouting::VisitOption(UIPropertyMarshallingStub& stub, REF OptionRef& value, int stringCapacity, IEnumDescriptor& enumDesc)
	{
		UNUSED(stub);
		UNUSED(value);
		UNUSED(stringCapacity);
		UNUSED(enumDesc);
	}
}

namespace Rococo::Abedit::Internal
{
	IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& panelArea)
	{
		return new ANON::Properties(panelArea);
	}
}
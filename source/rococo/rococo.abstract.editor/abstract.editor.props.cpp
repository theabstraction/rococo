#include <rococo.abstract.editor.win32.h>
#include <rococo.window.h>
#include <vector>
#include <rococo.strings.h>
#include <rococo.validators.h>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Windows;
using namespace Rococo::Abedit;
using namespace Rococo::Validators;

namespace ANON
{
	struct IProperty
	{
		// Represents the property in the panel and returns the containing rectangle in the panel co-ordinates
		// The yOffset parameter determines the y co-ordinate of the top left of the panel
		virtual GuiRect AddToPanel(IParentWindowSupervisor& panel, int yOffset, ControlPropertyId labelId, ControlPropertyId editorId) = 0;

		virtual cstr Id() const = 0;

		// Windows Control Id for the editor
		virtual bool IsForControl(ControlPropertyId id) const = 0;

		virtual bool TryGetEditorString(REF HString& value) = 0;
	};

	struct IPropertySupervisor : IProperty
	{
		virtual void Free() = 0;
		virtual void OnEditorChanged() = 0;
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

		IWindowSupervisor* editor{ nullptr };

		enum { HARD_CAP = 32767 };

		StringProperty(cstr _id, int _capacity, cstr _displayName, cstr _initialString):
			id(_id), initialString(_initialString), capacity(HARD_CAP), displayName(_displayName)
		{
			if (_capacity > 0 && _capacity < HARD_CAP)
			{
				capacity = (size_t) _capacity;
			}

			if (_displayName == nullptr || *_displayName == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			if (_initialString == nullptr || *_initialString == 0)
			{
				Throw(0, "%s: '%s' blank initial string", __FUNCTION__, _displayName);
			}

			if (_id == nullptr || *_id == 0)
			{
				Throw(0, "%s: '%s' blank id", __FUNCTION__, _displayName);
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

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(editor, REF value);
		}

		void OnEditorChanged() override
		{
			UNUSED(id);
		}
	};

	struct StringConstant : IPropertySupervisor
	{
		HString initialString;
		HString displayName;
		VisualStyle style;

		IWindowSupervisor* editor{ nullptr };

		enum { HARD_CAP = 32767 };

		StringConstant(cstr _displayName, cstr _initialString) :
			displayName(_displayName), initialString(_initialString)
		{
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
			return "#StringConstant";
		}

		bool IsForControl(ControlPropertyId id) const
		{
			return editor && GetWindowLongPtrA(*editor, GWLP_ID) == id.value;
		}

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(editor, REF value);
		}

		void OnEditorChanged() override
		{
			
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

		IWindowSupervisor* editor{ nullptr };

		IValueValidator<VALUE_TYPE>& validator;
		IValueFormatter<VALUE_TYPE>& formatter;

		PrimitiveProperty(cstr _id, cstr _displayName, VALUE_TYPE _initialValue, IValueValidator<VALUE_TYPE>& _validator, IValueFormatter<VALUE_TYPE>& _formatter) :
			id(_id), initialValue(_initialValue), displayName(_displayName), validator(_validator), formatter(_formatter)
		{
			if (_displayName == nullptr || *_displayName == 0)
			{
				Throw(0, "%s: blank display name", __FUNCTION__);
			}

			validator.ThrowIfBad(_initialValue, EValidationPurpose::Construction);

			if (_id == nullptr || *_id == 0)
			{
				Throw(0, "%s: '%s' blank id", __FUNCTION__, _displayName);
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

		bool TryGetEditorString(REF HString& value)
		{
			return ANON::TryGetEditorString(editor, REF value);
		}

		void OnEditorChanged() override
		{
			if (!editor) return;

			LRESULT nChars = SendMessage(*editor, WM_GETTEXTLENGTH, 0, 0);
			if (nChars <= 0)
			{
				return;
			}

			std::vector<char> text;
			text.resize(nChars + 1);

			LRESULT nCharsCopied = SendMessageA(*editor, WM_GETTEXT, text.size(), (LPARAM) text.data());

			DWORD start, end;
			SendMessage(*editor, EM_GETSEL, (WPARAM) & start, (LPARAM) & end);

			EditorFilter<VALUE_TYPE> filter;
			if (!filter.RemoveIllegalCharacters(text.data(), (size_t)nCharsCopied))
			{
				return;
			}

			SendMessageA(*editor, WM_SETTEXT, 0, (LPARAM) text.data());
			SendMessage(*editor, EM_SETSEL, end, end);
		}
	};

	struct Properties;

	struct PropertyBuilder : IPropertySerializer
	{
		Properties& container;

		PropertyBuilder(Properties& _container): container(_container)
		{

		}

		void AddHeader(cstr displayName, cstr displayText) override;
		void Target(cstr id, cstr displayName, Rococo::Strings::HString& value, int capacity) override;
		void Target(cstr id, cstr displayName, int32& value, IValueValidator<int32>& validator, IValueFormatter<int32>& formatter) override;
		void Target(cstr id, cstr displayName, int64& value, IValueValidator<int64>& validator, IValueFormatter<int64>& formatter) override;
		void Target(cstr id, cstr displayName, float& value, IValueValidator<float>& validator, IValueFormatter<float>& formatter) override;
		void Target(cstr id, cstr displayName, double& value, IValueValidator<double>& validator, IValueFormatter<double>& formatter) override;
		void Target(cstr id, cstr displayName, bool& value, IValueValidator<bool>& validator, IValueFormatter<bool>& formatter) override;
		void Target(cstr id, cstr displayName, uint32& value, IValueValidator<uint32>& validator, IValueFormatter<uint32>& formatter) override;
		void Target(cstr id, cstr displayName, uint64& value, IValueValidator<uint64>& validator, IValueFormatter<uint64>& formatter) override;
	};

	struct PropertyEventRouting : IPropertySerializer
	{
		Properties& container;
		cstr id;

		PropertyEventRouting(Properties& _container, cstr _id) : container(_container), id(_id)
		{

		}

		void AddHeader(cstr displayName, cstr displayText) override;
		void Target(cstr id, cstr displayName, Rococo::Strings::HString& value, int capacity) override;
		void Target(cstr id, cstr displayName, int32& value, IValueValidator<int32>& validator, IValueFormatter<int32>& formatter) override;
		void Target(cstr id, cstr displayName, int64& value, IValueValidator<int64>& validator, IValueFormatter<int64>& formatter) override;
		void Target(cstr id, cstr displayName, float& value, IValueValidator<float>& validator, IValueFormatter<float>& formatter) override;
		void Target(cstr id, cstr displayName, double& value, IValueValidator<double>& validator, IValueFormatter<double>& formatter) override;
		void Target(cstr id, cstr displayName, bool& value, IValueValidator<bool>& validator, IValueFormatter<bool>& formatter) override;
		void Target(cstr id, cstr displayName, uint32& value, IValueValidator<uint32>& validator, IValueFormatter<uint32>& formatter) override;
		void Target(cstr id, cstr displayName, uint64& value, IValueValidator<uint64>& validator, IValueFormatter<uint64>& formatter) override;
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

		void OnEditorChanged(ControlPropertyId id) override
		{
			auto* p = FindControlById(id);
			if (!p) return;

			p->OnEditorChanged();
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

		void Build(IPropertyManager& manager) override
		{
			Clear();
			manager.SerializeProperties(builder);
			Populate();
		}

		void UpdateFromVisuals(ControlPropertyId id, IPropertyManager& manager) override
		{
			for (auto* p : properties)
			{
				if (p->IsForControl(id))
				{
					PropertyEventRouting eventRouting(*this, p->Id());
					manager.SerializeProperties(eventRouting);
				}
			}
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
	};

	void PropertyBuilder::AddHeader(cstr displayName, cstr displayText)
	{
		container.properties.push_back(new StringConstant(displayName, displayText));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, Rococo::Strings::HString& value, int capacity)
	{
		container.properties.push_back(new StringProperty(id, capacity, displayName, value));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, int32& value, IValueValidator<int32>& validator, IValueFormatter<int32>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<int32>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, int64& value, IValueValidator<int64>& validator, IValueFormatter<int64>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<int64>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, float& value, IValueValidator<float>& validator, IValueFormatter<float>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<float>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, double& value, IValueValidator<double>& validator, IValueFormatter<double>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<double>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, bool& value, IValueValidator<bool>& validator, IValueFormatter<bool>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<bool>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, uint32& value, IValueValidator<uint32>& validator, IValueFormatter<uint32>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<uint32>(id, displayName, value, validator, formatter));
	}

	void PropertyBuilder::Target(cstr id, cstr displayName, uint64& value, IValueValidator<uint64>& validator, IValueFormatter<uint64>& formatter)
	{
		container.properties.push_back(new PrimitiveProperty<uint64>(id, displayName, value, validator, formatter));
	}

	void PropertyEventRouting::AddHeader(cstr displayName, cstr displayText)
	{
		UNUSED(displayName);
		UNUSED(displayText);
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, Rococo::Strings::HString& value, int capacity)
	{
		UNUSED(capacity);
		UNUSED(displayName);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = newValue;
			}			
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, int32& value, IValueValidator<int32>& validator, IValueFormatter<int32>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = atoi(newValue);
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, int64& value, IValueValidator<int64>& validator, IValueFormatter<int64>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = atoll(newValue);
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, float& value, IValueValidator<float>& validator, IValueFormatter<float>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = (float) atof(newValue);
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, double& value, IValueValidator<double>& validator, IValueFormatter<double>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = atof(newValue);
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, bool& value, IValueValidator<bool>& validator, IValueFormatter<bool>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				value = Eq(newValue, "true");
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, uint32& value, IValueValidator<uint32>& validator, IValueFormatter<uint32>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				uint32 newUint32;
				if (1 == sscanf_s(newValue.c_str(), "%u", &newUint32))
				{
					value = newUint32;
				}
			}
		}
	}

	void PropertyEventRouting::Target(cstr id, cstr displayName, uint64& value, IValueValidator<uint64>& validator, IValueFormatter<uint64>& formatter)
	{
		UNUSED(displayName);
		UNUSED(validator);
		UNUSED(formatter);

		if (Eq(this->id, id))
		{
			HString newValue;
			if (container.TryGetEditorString(id, OUT newValue))
			{
				uint64 newUint64;
				if (1 == sscanf_s(newValue.c_str(), "%llu", &newUint64))
				{
					value = newUint64;
				}
			}
		}
	}
}

namespace Rococo::Abedit::Internal
{
	IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& panelArea)
	{
		return new ANON::Properties(panelArea);
	}
}
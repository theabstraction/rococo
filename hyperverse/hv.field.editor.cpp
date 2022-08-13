#include "hv.h"
#include <vector>
#include <rococo.strings.h>
#include <rococo.ui.h> // For IKeyboardSink
#include <rococo.fonts.hq.h>

using namespace HV;

namespace
{
	enum { TOTAL_INTERNAL_VBORDER_SPAN = 8 };
	ROCOCOAPI IField
	{
		virtual bool OnKeyboardEvent(const KeyboardEvent & key) = 0;
		virtual void MakeActive() = 0;

		// When a field is read we format a string buffer. At this point we should also update the internal fields and validate
		virtual void FormatValue(char* outputBuffer, size_t capacity) = 0;
		virtual cstr Name() const = 0;
		virtual const GuiRect& Rect() const = 0;
		virtual void SetRect(const GuiRect& rect) = 0;
		virtual void Render(IGuiRenderContext& grc, bool isActive) = 0;
		virtual void Free() = 0;
	};

	void SetBufferWithString(const char* str, std::vector<char>& buffer)
	{
		buffer.clear();
		buffer.resize(buffer.capacity());

		SafeFormat(buffer.data(), buffer.size(), "%s", str);
	}

	Vec2 RenderName(IGuiRenderContext& grc, ID_FONT idFont, cstr name, const GuiRect& lineRect)
	{
		return Graphics::RenderHQText_LeftAligned_VCentre(grc, idFont, lineRect, name, RGBAb(255, 255, 255, 255));
	}

	void RenderEditorBackground(IGuiRenderContext& grc, const GuiRect& editorRect, bool isActive)
	{
		int32 edge_alpha = isActive ? 255 : 200;
		RGBAb edge1(224, 224, 224, edge_alpha);
		RGBAb edge2(255, 255, 255, edge_alpha);

		int32 bk_alpha = isActive ? 255 : 64;

		GuiRect borderRect = Expand(editorRect, 4);
		Graphics::DrawRectangle(grc, borderRect, RGBAb(0, 0, 0, bk_alpha), RGBAb(0, 0, 0, bk_alpha));
		Graphics::DrawBorderAround(grc, borderRect, { 1,1 }, edge1, edge2);
	}

	GuiRect RenderNameAndEditorBackground(IGuiRenderContext& grc, ID_FONT idFont, cstr name, const GuiRect& lineRect, bool isActive)
	{
		Vec2 nameSpan = RenderName(grc, idFont, name, lineRect);

		GuiRect editorRect
		{
			lineRect.left + (int32) nameSpan.x + (Height(lineRect) >> 1),
			lineRect.top + (TOTAL_INTERNAL_VBORDER_SPAN >> 1),
			lineRect.right - 4,
			lineRect.bottom - (TOTAL_INTERNAL_VBORDER_SPAN >> 1)
		};

		RenderEditorBackground(grc, editorRect, isActive);

		return editorRect;
	}

	void RenderEditorForeground(IGuiRenderContext& grc, ID_FONT idFont, const GuiRect& editorRect, int32 width, bool isActive, cstr editBuffer, int32 caretPos)
	{
		GuiRect textRect
		{
			editorRect.left + width,
			editorRect.top,
			editorRect.right,
			editorRect.bottom
		};

		int fontIndex = 0;

		RGBAb white(255, 255, 255, 255);

		if (isActive)
		{
			Graphics::RenderHQText_LeftAligned_VCentre_WithCaret(grc, idFont, textRect, editBuffer, white, caretPos);
		}
		else
		{
			Graphics::RenderHQText_LeftAligned_VCentre(grc, idFont, textRect, editBuffer, white);
		}
	}

	enum { NAME_WIDTH = 100 };

	struct UnboundedInt32Field : IField
	{
		IKeyboardSupervisor& keyboard;
		HString name;
		GuiRect rect;

		int32 value;	
		
		int32 caretPos = -1;
		std::vector<char> editBuffer;

		bool preferHex;

		bool showHex;

		ID_FONT idFont;

		UnboundedInt32Field(ID_FONT _idFont, IKeyboardSupervisor& _keyboard, cstr _name, int32 _value, bool _preferHex):
			keyboard(_keyboard), name(_name), value(_value), editBuffer(12), preferHex(_preferHex), idFont(_idFont)
		{
			char buf[16];

			if (preferHex)
			{
				SafeFormat(buf, sizeof buf, "%d", _value);
			}
			else
			{
				SafeFormat(buf, sizeof buf, "%X", _value);
			}

			SetBufferWithString(buf, editBuffer);
			caretPos = (int32) strlen(buf);

			showHex = preferHex;
		}

		void ChangeHexMode()
		{
			if (showHex)
			{
				// Convert to decimal
				int hexvalue;
				if (0 == sscanf_s(editBuffer.data(), "%8x", &hexvalue))
				{
					hexvalue = 0;
				}

				SafeFormat(editBuffer.data(), editBuffer.capacity(), "%d", hexvalue);
			}
			else
			{
				// Convert to hex
				int decvalue;
				if (0 == sscanf_s(editBuffer.data(), "%10d", &decvalue))
				{
					decvalue = 0;
				}

				SafeFormat(editBuffer.data(), editBuffer.capacity(), "%X", decvalue);
			}

			showHex = !showHex;
			caretPos = (int32)strlen(editBuffer.data());
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (key.unicode > 32 && key.unicode < 127)
			{
				char c = key.unicode;
				if (!showHex && c == '-' && editBuffer.data()[0] == 0)
				{

				}
				else if (key.IsUp() && (c == 'x' || c == 'X'))
				{
					ChangeHexMode();
					return true;
				}
				else if (IsNumeric(c))
				{
					
				}
				else if (showHex && c >= 'A' && c <= 'F')
				{

				}
				else if (showHex && c >= 'a' && c <= 'f')
				{

				}
				else
				{
					return true;
				}
			}

			size_t capacity = showHex ? 9 : 12;
			keyboard.AppendKeyboardInputToEditBuffer(caretPos, editBuffer.data(), capacity, key);
			return true;
		}

		cstr Name() const override
		{
			return name;
		}

		void MakeActive() override
		{
			caretPos = (int32) strlen(editBuffer.data());
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void FormatValue(char* outputBuffer, size_t capacity) override
		{
			if (showHex)
			{
				ChangeHexMode();
			}
			value = (int)atoi(editBuffer.data());
			SafeFormat(outputBuffer, capacity, "%d", value);
			SetBufferWithString(outputBuffer, editBuffer);
			caretPos = (int32)strlen(outputBuffer);
		}

		void Render(IGuiRenderContext& grc, bool isActive)  override
		{
			GuiRect editorRect = RenderNameAndEditorBackground(grc, idFont, name, rect, isActive);

			if (showHex)
			{
				Vec2 span0x = Graphics::RenderHQText_LeftAligned_VCentre(grc, idFont, editorRect, "0x", RGBAb(255, 255, 0, 255));
				RenderEditorForeground(grc, idFont, editorRect, 1 + (int32) span0x.x, isActive, editBuffer.data(), caretPos);
			}
			else
			{
				RenderEditorForeground(grc, idFont, editorRect, 0, isActive, editBuffer.data(), caretPos);
			}
		}

		void Free() override
		{
			delete this;
		}
	};

	struct BoundedInt32Field : IField
	{
		HString name;
		GuiRect rect;

		int32 value;
		int32 minValue;
		int32 maxValue;
		int32 caretPos = -1;
		std::vector<char> editBuffer;
		IKeyboardSupervisor& keyboard;
		ID_FONT idFont;

		BoundedInt32Field(ID_FONT _idFont, IKeyboardSupervisor& _keyboard, cstr _name, int32 _value, int32 _minValue, int32 _maxValue) :
			keyboard(_keyboard), name(_name), value(_value), minValue(_minValue), maxValue(_maxValue),
			editBuffer(10), idFont(_idFont)
		{
			char buf[64];
			SafeFormat(buf, sizeof buf, "%d", _value);
			SetBufferWithString(buf, editBuffer);
			caretPos = (int32)strlen(buf);
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (key.unicode > 32 && key.unicode < 127)
			{
				char c = key.unicode;
				if (c == '-' && editBuffer.data()[0] == 0)
				{

				}
				else if (!IsNumeric(c))
				{
					return false;
				}
			}
			keyboard.AppendKeyboardInputToEditBuffer(caretPos, editBuffer.data(), editBuffer.capacity(), key);
			return true;
		}

		cstr Name() const override
		{
			return name;
		}

		void MakeActive() override
		{
			caretPos = (int32)strlen(editBuffer.data());
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void FormatValue(char* outputBuffer, size_t capacity)
		{
			int32 newValue = (int)atoi(editBuffer.data());
			value = clamp(newValue, minValue, maxValue);
			SafeFormat(outputBuffer, capacity, "%d", value);
			SetBufferWithString(outputBuffer, editBuffer);
			caretPos = (int32)strlen(outputBuffer);
		}

		void Render(IGuiRenderContext& grc, bool isActive)  override
		{
			RenderNameAndEditorBackground(grc, idFont, name, rect, isActive);
			RenderEditorForeground(grc, idFont, rect, NAME_WIDTH, isActive, editBuffer.data(), caretPos);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct StringField : IField
	{
		HString name;
		GuiRect rect;
		int caretPos = 0;
		IKeyboardSupervisor& keyboard;
		bool isVarName;

		std::vector<char> editBuffer;

		ID_FONT idFont;

		StringField(ID_FONT _idFont, IKeyboardSupervisor& _keyboard, size_t _capacity, cstr value, cstr _name, bool _isVarName) :
			keyboard(_keyboard), editBuffer(_capacity), name(_name), isVarName(_isVarName), idFont(_idFont)
		{
			SetBufferWithString(value, editBuffer);
			caretPos = (int32)strlen(value);
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (isVarName)
			{
				int code = key.unicode;
				if (code >= 32 && code < 127)
				{
					if (IsAlphaNumeric(code))
					{

					}
					else
					{
						switch (code)
						{
						case '_':
						case '.':
						case '-':
							break;
						default:
							return true;
						}
					}
				}
			}
			
			keyboard.AppendKeyboardInputToEditBuffer(caretPos, editBuffer.data(), editBuffer.capacity(), key);
			return true;
		}

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void MakeActive() override
		{
			caretPos = (int32) strlen(editBuffer.data());
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void FormatValue(char* outputBuffer, size_t capacity)
		{
			SafeFormat(outputBuffer, capacity, "%s", editBuffer.data());
		}

		void Render(IGuiRenderContext& grc, bool isActive)  override
		{
			GuiRect editorRect = RenderNameAndEditorBackground(grc, idFont, name, rect, isActive);
			RenderEditorForeground(grc, idFont, editorRect, 0, isActive, editBuffer.data(), caretPos);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct FloatField : IField
	{
		IKeyboardSupervisor& keyboard;
		HString name;
		GuiRect rect;
		int caretPos = -1;
		std::vector<char> editBuffer;

		float value;
		float minValue;
		float maxValue;

		ID_FONT idFont;

		FloatField(ID_FONT _idFont, IKeyboardSupervisor& _keyboard, cstr _name, float _value, float _minValue, float _maxValue)
			: keyboard(_keyboard), name(_name), value(_value), minValue(_minValue), maxValue(_maxValue),
			editBuffer(11), idFont(_idFont)
		{
			char buf[64];
			SafeFormat(buf, sizeof buf, "%g", _value);
			SetBufferWithString(buf, editBuffer);
			caretPos = (int32)strlen(buf);
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (key.unicode > 32 && key.unicode < 127)
			{
				char c = key.unicode;
				if (!IsNumeric(c) && c != '.' && c != '-')
				{
					return false;
				}
			}
			keyboard.AppendKeyboardInputToEditBuffer(caretPos, editBuffer.data(), editBuffer.capacity(), key);
			return true;
		}

		void MakeActive() override
		{
			caretPos = (int32)strlen(editBuffer.data());
		}

		cstr Name() const override
		{
			return name;
		}

		const GuiRect& Rect() const  override
		{
			return rect;
		}

		void SetRect(const GuiRect& _rect)
		{
			rect = _rect;
		}

		void FormatValue(char* outputBuffer, size_t capacity)
		{
			float newValue = (float) atof(editBuffer.data());
			value = clamp(newValue, minValue, maxValue);
			SafeFormat(outputBuffer, capacity, "%g", value);
			SetBufferWithString(outputBuffer, editBuffer);
			caretPos = (int32) strlen(outputBuffer);
		}

		void Render(IGuiRenderContext& grc, bool isActive)  override
		{
			GuiRect editorRect = RenderNameAndEditorBackground(grc, idFont, name, rect, isActive);
			RenderEditorForeground(grc, idFont, editorRect, 0, isActive, editBuffer.data(), caretPos);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct FieldEditor : IFieldEditor, IUIElement, IKeyboardSink
	{
		std::vector<IField*> fields;
		int32 activeIndex = -1;

		FieldEditorContext context;
		FieldEditor(FieldEditorContext& _context): 
			context(_context)
		{

		}

		~FieldEditor()
		{
			Deactivate();
			Clear();
		}

		void Deactivate() override
		{
			activeIndex = -1;
			context.gui.DetachKeyboardSink(this);
		}

		void Free() override
		{
			delete this;
		}

		IUIElement& UIElement() override { return *this; }

		void AddInt32FieldUnbounded(cstr name, int32 value, bool preferHex) override
		{
			auto* field = new UnboundedInt32Field(context.idFont, context.keyboard, name, value, preferHex);
			fields.push_back(field);
		}

		void AddInt32FieldBounded(cstr name, int32 value, int32 minValue, int32 maxValue) override
		{
			auto* field = new BoundedInt32Field(context.idFont, context.keyboard, name, value, minValue, maxValue);
			fields.push_back(field);
		}

		void AddStringField(cstr name, cstr value, size_t capacity, bool isVariableName) override
		{
			auto* field = new StringField(context.idFont, context.keyboard, capacity, value, name, isVariableName);
			auto* f = static_cast<IField*>(field);
			fields.push_back(f);
		}

		void AddFloat32FieldBounded(cstr name, float value, float minValue, float maxValue) override
		{
			auto* field = new FloatField(context.idFont, context.keyboard, name, value, minValue, maxValue);
			fields.push_back(field);
		}

		void Clear() override
		{
			for (auto* i : fields)
			{
				i->Free();
			}

			fields.clear();

			activeIndex = -1;
		}

		void NotifyUpdate()
		{
			if (activeIndex >= 0)
			{
				char text[260];
				fields[activeIndex]->FormatValue(text, sizeof text);
				context.onActiveChange.OnActiveIndexChanged(activeIndex, text);
			}
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			if (activeIndex >= 0)
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					NotifyUpdate();
					Deactivate();
					return true;
				}
				return fields[activeIndex]->OnKeyboardEvent(key);
			}
			return false;
		}

		void GetActiveValueAsString(char* buffer, size_t capacity) override
		{

		}

		void OnRawMouseEvent(const MouseEvent& ev) override
		{

		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
		{

		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
		{
			for (int i = 0; i < fields.size(); ++i)
			{
				if (IsPointInRect(cursorPos, fields[i]->Rect()))
				{
					if (activeIndex != i)
					{
						activeIndex = i;
						fields[i]->MakeActive();
						context.gui.AttachKeyboardSink(this);
					}
					return;
				}
			}

			NotifyUpdate();
			Deactivate();
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
		{

		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			GuiRect lineRect = absRect;

			int32 lineHeight = grc.Gui().GetFontMetrics(context.idFont).height;

			lineRect.right -= (lineHeight >> 1);

			int32 vBorderSpan = (lineHeight >> 1);

			for (int i = 0; i < fields.size(); ++i)
			{
				lineRect.top += vBorderSpan;
				lineRect.bottom = lineRect.top + lineHeight + TOTAL_INTERNAL_VBORDER_SPAN;

				auto& f = *fields[i];

				f.SetRect(lineRect);
				f.Render(grc, i == activeIndex);

				lineRect.top = lineRect.bottom + 1;
			}
		}
	};
}

namespace HV
{
	IFieldEditor* CreateFieldEditor(FieldEditorContext& context)
	{
		return new FieldEditor(context);
	}
}
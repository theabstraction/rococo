#include <rococo.gui.retained.h>
#include <rococo.maths.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GREditBox : IGRWidgetEditBox, IGREditorMicromanager
	{
		IGRPanel& panel;
		std::vector<char> text;
		GRFontId fontId = GRFontId::MENU_FONT;
		GRAlignmentFlags alignment;
		Vec2i spacing{ 0,0 };
		int32 caretPos = 0;

		GREditBox(IGRPanel& owningPanel, int32 capacity) : panel(owningPanel)
		{
			text.reserve(capacity);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			panel.Focus();
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void AddToCaretPos(int32 delta) override
		{
			caretPos = clamp(caretPos + delta, 0, (int32) (text.size() - 1));
		}

		void AppendChar(char c) override
		{
			if (text.size() < text.capacity())
			{
				if (caretPos >= text.size() - 1)
				{
					text.back() = c;
					text.push_back(0);
				}
				else
				{
					auto i = text.begin();
					std::advance(i, caretPos);
					text.insert(i, c);
				}

				caretPos++;
			}
		}

		void Backspace() override
		{
			if (caretPos > 0 && text.size() > 1)
			{
				if (caretPos >= text.size() - 1)
				{
					text.pop_back();
					text.back() = 0;
				}
				else
				{
					auto i = text.begin();
					std::advance(i, caretPos-1);
					text.erase(i);
				}

				caretPos--;			
			}
		}

		void Return() override
		{
			panel.Root().GR().SetFocus(-1);
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();
			bool isHovered = g.IsHovered(panel);
			bool isFocused = panel.Id() == panel.Root().GR().GetFocusId();

			if (isFocused)
			{
				rect.left += 1;
				rect.top += 1;
				rect.right -= 1;
				rect.bottom -= 1;
				ESchemeColourSurface surface = isHovered ? ESchemeColourSurface::FOCUSED_EDITOR_HOVERED : ESchemeColourSurface::FOCUSED_EDITOR;
				RGBAb defaultColour = isHovered ? RGBAb(255, 255, 255, 255) : RGBAb(225, 225, 225, 225);
				g.DrawRect(rect, panel.GetColour(surface, defaultColour));
			}

			if (!isFocused)
			{
				if (text.size() > 0)
				{
					g.DrawText(fontId, rect, alignment, spacing, { text.data(), (int32)text.size() - 1 }, panel.GetColour(isHovered ? ESchemeColourSurface::TEXT_HOVERED : ESchemeColourSurface::TEXT));
				}
			}
			else
			{
				RGBAb textColour = panel.GetColour(isHovered ? ESchemeColourSurface::EDIT_TEXT_HOVERED : ESchemeColourSurface::EDIT_TEXT);
				g.DrawEditableText(fontId, rect, alignment, spacing, { text.data(), (int32)text.size() - 1 }, caretPos, textColour);
			}
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			panel.Root().Custodian().TranslateToEditor(keyEvent, *this);
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 10,10 };
		}

		IGRWidgetEditBox& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) override
		{
			this->alignment = alignment;
			this->spacing = spacing;
			return *this;
		}

		IGRWidgetEditBox& SetFont(GRFontId fontId) override
		{
			this->fontId = fontId;
			return *this;
		}

		void SetText(cstr argText) override
		{
			if (argText == nullptr)
			{
				argText = "";
			}

			size_t len = strlen(argText);

			size_t newSize = min(len + 1, text.capacity());
			text.resize(newSize);

			strncpy_s(text.data(), text.capacity(), argText, _TRUNCATE);

			caretPos = (int32) len;
		}

		size_t GetCapacity() const override
		{
			return (int32) text.capacity();
		}

		bool isReadOnly = false;

		int32 GetTextAndLength(char* buffer, int32 capacity) const override
		{
			if (buffer != nullptr && capacity > 0)
			{
				strncpy_s(buffer, text.capacity(), text.data(), _TRUNCATE);
			}

			return (int32) text.size();
		}

		IGRWidgetEditBox& SetReadOnly(bool isReadOnly) override
		{
			this->isReadOnly = isReadOnly;
			return *this;
		}
	};

	struct GREditBoxFactory : IGRWidgetFactory
	{
		int32 capacity;

		GREditBoxFactory(int32 _capacity): capacity(_capacity)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GREditBox(panel, capacity);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetEditBox& CreateEditBox(IGRWidget& parent, int32 capacity)
	{
		if (capacity <= 2)
		{
			parent.Panel().Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Capacity should be >= 2");
		}
		else
		{
			capacity = (int32) GRPaths::MAX_FULL_PATH_LENGTH;
		}

		if (capacity > 1024_megabytes)
		{
			parent.Panel().Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Capacity should be <= 1 gigabyte");
			capacity = (int32) 1024_megabytes;
		}

		GRANON::GREditBoxFactory factory(capacity);

		auto& gr = parent.Panel().Root().GR();
		auto& editor = static_cast<IGRWidgetEditBox&>(gr.AddWidget(parent.Panel(), factory));
		return editor;
	}
}
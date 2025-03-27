#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <vector>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GREditBox : IGRWidgetEditBox, IGREditorMicromanager, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		std::vector<char> text;
		GRFontId fontId = GRFontId::MENU_FONT;
		GRAlignmentFlags alignment;
		Vec2i spacing{ 0,0 };
		int32 caretPos = 0;
		int32 updateLock = 0;
		IGREditFilter* filter;

		struct UpdateLock
		{
			GREditBox* This;

			UpdateLock(GREditBox* _This) : This(_This)
			{
				This->updateLock++;
			}

			~UpdateLock()
			{
				This->updateLock--;
			}
		};

		GREditBox(IGRPanel& owningPanel, IGREditFilter* _filter, int32 capacity) : panel(owningPanel), filter(_filter)
		{
			text.reserve(capacity);
			owningPanel.SetMinimalSpan({ 10, 10 });
			owningPanel.Add(EGRPanelFlags::AcceptsFocus);
		}

		void Free() override
		{
			delete this;
		}

		int32 CaretPos() const override
		{
			return caretPos;
		}

		void OnUpdate()
		{
			if (updateLock > 0)
			{
				// Prevent recursion
				return;
			}

			if (filter) filter->OnUpdate(*this, *this);

			UpdateLock lock(this);

			GRWidgetEvent_EditorUpdated we;
			we.eventType = EGRWidgetEventType::EDITOR_UPDATED;
			we.iMetaData = iMetaData;
			we.sMetaData = sMetaData.c_str();
			we.panelId = panel.Id();	
			we.editor = this;
			we.manager = this;
			we.caretPos = caretPos;
			we.clickPosition = { 0,0 };

			RouteEventToHandler(panel, we);
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			UNUSED(panelDimensions);
		}

		bool preppingSelect = false;

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				preppingSelect = true;
				return EGREventRouting::Terminate;
			}
			else if (ce.click.LeftButtonUp && preppingSelect)
			{
				if (panel.Root().GR().GetFocusId() == panel.Id())
				{
					panel.Root().GR().SetFocus(-1);
				}
				else
				{
					panel.Focus();
				}
				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{
			preppingSelect = false;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void AddToCaretPos(int32 delta) override
		{
			caretPos = clamp(caretPos + delta, 0, (int32) (text.size() - 1));
		}

		void AppendCharAtCaret(char c) override
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

				OnUpdate();
			}
		}

		void BackspaceAtCaret() override
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

				OnUpdate();
			}
		}

		void DeleteAtCaret() override
		{
			if (text.size() > 1)
			{
				if (caretPos >= text.size() - 1)
				{
					return;
				}
				else
				{
					auto i = text.begin();
					std::advance(i, caretPos);
					text.erase(i);

					OnUpdate();
				}
			}
		}

		void Return() override
		{
			panel.Root().GR().SetFocus(-1);
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			GRRenderState rs(false, g.IsHovered(panel), panel.Id() == panel.Root().GR().GetFocusId());

			RGBAb editorColour = panel.GetColour(EGRSchemeColourSurface::EDITOR, rs, RGBAb(0, 0, 0, 225));

			if (rs.value.bitValues.focused)
			{
				GuiRect innerRect = rect;
				innerRect.left += 1;
				innerRect.top += 1;
				innerRect.right -= 1;
				innerRect.bottom -= 1;
				g.DrawRect(innerRect, editorColour);
				g.DrawRectEdge(innerRect, panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs, RGBAb(0, 0, 0, 225)), panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs, RGBAb(0, 0, 0, 225)));
			}

			if (!rs.value.bitValues.focused)
			{
				if (text.size() > 0)
				{
					g.DrawText(fontId, rect, rect, alignment, spacing, { text.data(), (int32)text.size() - 1 }, panel.GetColour(EGRSchemeColourSurface::TEXT, rs));
				}
			}
			else
			{
				RGBAb textColour = panel.GetColour(EGRSchemeColourSurface::EDIT_TEXT, rs);

				CaretSpec caret;
				caret.IsInserting = true;
				caret.CaretPos = caretPos;
				caret.BlinksPerSecond = 2;
				caret.CaretColour1 = textColour;
				caret.CaretColour2 = editorColour;
				g.DrawEditableText(fontId, rect, alignment, spacing, { text.data(), (int32)text.size() - 1 }, textColour, caret);
			}
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			panel.Root().Custodian().TranslateToEditor(keyEvent, *this);
			return EGREventRouting::NextHandler;
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

		int64 iMetaData = 0;
		std::string sMetaData;

		IGRWidgetEditBox& SetMetaData(const GRControlMetaData& metaData)
		{
			if (updateLock > 0)
			{
				panel.Root().Custodian().RaiseError(panel.GetAssociatedSExpression(), EGRErrorCode::RecursionLocked, __FUNCTION__, "It is forbidden to set meta data of an edit box in the context of an update to the edit box");
				return *this;
			}

			iMetaData = metaData.intData;
			sMetaData = metaData.stringData ? metaData.stringData : std::string();
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

			OnUpdate();
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
				strncpy_s(buffer, capacity, text.data(), _TRUNCATE);
			}

			return (int32) text.size();
		}

		IGRWidgetEditBox& SetReadOnly(bool isReadOnly) override
		{
			this->isReadOnly = isReadOnly;
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetEditBox>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GREditBox";
		}
	};

	struct GREditBoxFactory : IGRWidgetFactory
	{
		IGREditFilter* filter;
		int32 capacity;

		GREditBoxFactory(IGREditFilter* _filter, int32 _capacity): filter(_filter), capacity(_capacity)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GREditBox(panel, filter, capacity);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetEditBox::InterfaceId()
	{
		return "IGRWidgetEditBox";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetEditBox& CreateEditBox(IGRWidget& parent, IGREditFilter* filter, int32 capacity)
	{
		if (capacity <= 2)
		{
			parent.Panel().Root().Custodian().RaiseError(parent.Panel().GetAssociatedSExpression(), EGRErrorCode::InvalidArg, __FUNCTION__, "Capacity should be >= 2");
		}
		else
		{
			capacity = (int32) EGRPaths::MAX_FULL_PATH_LENGTH;
		}

		if (capacity > 1024_megabytes)
		{
			parent.Panel().Root().Custodian().RaiseError(parent.Panel().GetAssociatedSExpression(), EGRErrorCode::InvalidArg, __FUNCTION__, "Capacity should be <= 1 gigabyte");
			capacity = (int32) 1024_megabytes;
		}

		GRANON::GREditBoxFactory factory(filter, capacity);

		auto& gr = parent.Panel().Root().GR();
		auto* editor = Cast<IGRWidgetEditBox>(gr.AddWidget(parent.Panel(), factory));
		return *editor;
	}

	ROCOCO_GUI_RETAINED_API IGREditFilter& GetF32Filter()
	{
		struct F32Filter : IGREditFilter
		{
			std::vector<char> scratchBuffer;

			F32Filter()
			{
				scratchBuffer.reserve(12);
			}

			void Free() override
			{

			}

			void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager)
			{
				scratchBuffer.clear();

				char buffer[12];
				int32 len = editor.GetTextAndLength(buffer, sizeof buffer);

				int32 originalLength = len;

				if (len >= 12)
				{
					len = 11;
					buffer[11] = 0;
				}

				int32 caretPos = manager.CaretPos();

				bool hasPoint = false;

				if (len > 0)
				{
					switch (buffer[0])
					{
					case '-':
					case '+':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						scratchBuffer.push_back(buffer[0]);
						break;
					case '.':
						hasPoint = true;
						scratchBuffer.push_back('.');
						break;
					}
				}

				for (int32 i = 1; i < len; ++i)
				{
					char c = buffer[i];
					if (c >= '0' && c <= '9')
					{
						scratchBuffer.push_back(buffer[i]);
					}
					else if (!hasPoint && c == '.')
					{
						scratchBuffer.push_back('.');
						hasPoint = true;
					}
				}

				scratchBuffer.push_back(0);

				int32 newLength = (int32)scratchBuffer.size();

				int32 lengthDelta = originalLength - newLength;

				if (lengthDelta != 0)
				{
					// This sets the caret pos to the null char
					editor.SetText(scratchBuffer.data());

					// Now the caret position is set to zero
					manager.AddToCaretPos(-10000);

					manager.AddToCaretPos(caretPos - lengthDelta);
				}
			}
		};

		static F32Filter s_F32Editor;
		return s_F32Editor;
	}

	ROCOCO_GUI_RETAINED_API IGREditFilter& GetF64Filter()
	{
		struct F64Filter : IGREditFilter
		{
			std::vector<char> scratchBuffer;

			F64Filter()
			{
				scratchBuffer.reserve(24);
			}

			void Free() override
			{

			}

			void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager)
			{
				scratchBuffer.clear();

				char buffer[24];
				int32 len = editor.GetTextAndLength(buffer, sizeof buffer);

				int32 originalLength = len;

				if (len >= 24)
				{
					len = 23;
					buffer[23] = 0;
				}

				int32 caretPos = manager.CaretPos();

				bool hasPoint = false;

				if (len > 0)
				{
					switch (buffer[0])
					{
					case '-':
					case '+':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						scratchBuffer.push_back(buffer[0]);
						break;
					case '.':
						hasPoint = true;
						scratchBuffer.push_back('.');
						break;
					}
				}

				for (int32 i = 1; i < len; ++i)
				{
					char c = buffer[i];
					if (c >= '0' && c <= '9')
					{
						scratchBuffer.push_back(buffer[i]);
					}
					else if (!hasPoint && c == '.')
					{
						scratchBuffer.push_back('.');
						hasPoint = true;
					}
				}

				scratchBuffer.push_back(0);

				int32 newLength = (int32)scratchBuffer.size();

				int32 lengthDelta = originalLength - newLength;

				if (lengthDelta != 0)
				{
					// This sets the caret pos to the null char
					editor.SetText(scratchBuffer.data());

					// Now the caret position is set to zero
					manager.AddToCaretPos(-10000);

					manager.AddToCaretPos(caretPos - lengthDelta);
				}
			}
		};

		static F64Filter s_F64Editor;
		return s_F64Editor;
	}

	ROCOCO_GUI_RETAINED_API IGREditFilter& GetI32Filter()
	{
		struct I32Filter : IGREditFilter
		{
			std::vector<char> scratchBuffer;

			I32Filter()
			{
				scratchBuffer.reserve(12);
			}

			void Free() override
			{

			}

			void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager)
			{
				scratchBuffer.clear();

				char buffer[12];
				int32 len = editor.GetTextAndLength(buffer, sizeof buffer);

				int32 originalLength = len;

				if (len >= 12)
				{
					len = 11;
					buffer[11] = 0;
				}

				if (len > 0)
				{
					switch (buffer[0])
					{
					case '-':
					case '+':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						scratchBuffer.push_back(buffer[0]);
						break;
					}
				}

				for (int32 i = 1; i < len; ++i)
				{
					char c = buffer[i];
					if (c >= '0' && c <= '9')
					{
						scratchBuffer.push_back(buffer[i]);
					}
				}

				scratchBuffer.push_back(0);

				int32 caretPos = manager.CaretPos();

				int32 newLength = (int32)scratchBuffer.size();

				int32 lengthDelta = originalLength - newLength;

				if (lengthDelta != 0)
				{
					// This sets the caret pos to the null char
					editor.SetText(scratchBuffer.data());

					// Now the caret position is set to zero
					manager.AddToCaretPos(-10000);

					manager.AddToCaretPos(caretPos - lengthDelta);
				}
			}
		};
		
		static I32Filter s_I32Editor;
		return s_I32Editor;
	}

	ROCOCO_GUI_RETAINED_API IGREditFilter& GetI64Filter()
	{
		struct I64Filter : IGREditFilter
		{
			std::vector<char> scratchBuffer;

			I64Filter()
			{
				scratchBuffer.reserve(24);
			}

			void Free() override
			{

			}

			void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager)
			{
				scratchBuffer.clear();

				char buffer[24];
				int32 len = editor.GetTextAndLength(buffer, sizeof buffer);

				int32 originalLength = len;

				if (len >= 24)
				{
					len = 23;
					buffer[23] = 0;
				}

				if (len > 0)
				{
					switch (buffer[0])
					{
					case '-':
					case '+':
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						scratchBuffer.push_back(buffer[0]);
						break;
					}
				}

				for (int32 i = 1; i < len; ++i)
				{
					char c = buffer[i];
					if (c >= '0' && c <= '9')
					{
						scratchBuffer.push_back(buffer[i]);
					}
				}

				scratchBuffer.push_back(0);

				int32 caretPos = manager.CaretPos();

				int32 newLength = (int32)scratchBuffer.size();

				int32 lengthDelta = originalLength - newLength;

				if (lengthDelta != 0)
				{
					// This sets the caret pos to the null char
					editor.SetText(scratchBuffer.data());

					// Now the caret position is set to zero
					manager.AddToCaretPos(-10000);

					manager.AddToCaretPos(caretPos - lengthDelta);
				}
			}
		};

		static I64Filter s_I64Editor;
		return s_I64Editor;
	}

	ROCOCO_GUI_RETAINED_API IGREditFilter& GetUnsignedFilter()
	{
		struct UnsignedFilter : IGREditFilter
		{
			std::vector<char> scratchBuffer;

			UnsignedFilter()
			{
				scratchBuffer.reserve(24);
			}

			void Free() override
			{

			}

			void OnUpdate(IGRWidgetEditBox& editor, IGREditorMicromanager& manager)
			{
				scratchBuffer.clear();

				char buffer[24];
				int32 len = editor.GetTextAndLength(buffer, sizeof buffer);

				int32 originalLength = len;

				if (len >= 24)
				{
					len = 23;
					buffer[23] = 0;
				}

				for (int32 i = 0; i < len; ++i)
				{
					char c = buffer[i];
					if (c >= '0' && c <= '9')
					{
						scratchBuffer.push_back(buffer[i]);
					}
				}

				scratchBuffer.push_back(0);

				int32 caretPos = manager.CaretPos();

				int32 newLength = (int32)scratchBuffer.size();

				int32 lengthDelta = originalLength - newLength;

				if (lengthDelta != 0)
				{
					// This sets the caret pos to the null char
					editor.SetText(scratchBuffer.data());

					// Now the caret position is set to zero
					manager.AddToCaretPos(-10000);

					manager.AddToCaretPos(caretPos - lengthDelta);
				}
			}
		};

		static UnsignedFilter s_UnsignedFilter;
		return s_UnsignedFilter;
	}
}
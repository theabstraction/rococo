#include <rococo.mplat.h>
#include <vector>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdarg.h>
#include <rococo.ui.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Events;

   struct DebugLine
   {
      enum size_t { KEY_LEN = 64, VALUE_LEN = 128 };
      rchar key[KEY_LEN];
      rchar value[VALUE_LEN];
	  bool isSelectable = false;
   };

   class MathsVisitor : public IMathsVisitorSupervisor, public IEventCallback<ScrollEvent>
   {
	   AutoFree<IScrollbar> scrollbar;
	   std::vector<DebugLine> lines;
	   GuiRect scrollRect;
	   bool queueMouseUp = false;

	   void Name(DebugLine& line, VisitorName name)
	   {
		   StackStringBuilder sb(line.key, sizeof(line.key));
		   sb << name;
	   }

	   void AddBlankLine()
	   {
		   DebugLine line;
		   *line.key = 0;
		   *line.value = 0;
		   lines.push_back(line);
	   }

	   void FillNameWithSpaces(DebugLine& line, size_t count)
	   {
		   size_t len = min(count, size_t(DebugLine::KEY_LEN - 1));
		   for (size_t i = 0; i < len; ++i)
		   {
			   line.key[i] = L' ';
		   }

		   line.key[len] = 0;
	   }

   public:
	   MathsVisitor(IUtilitiies& utilities, IKeyboardSupervisor& keyboard):
		   scrollbar(utilities.CreateScrollbar(keyboard, true))
	   {

	   }

	   virtual void Show(VisitorName name, const Matrix4x4& m)
	   {
		   DebugLine line[4];
		   Name(line[0], name);

		   size_t len = rlen(line[0].key);
		   for (int i : {1, 2, 3})
		   {
			   FillNameWithSpaces(line[i], len);
		   }

		   const Vec4* row = &m.row0;

		   for (int i : {0, 1, 2, 3})
		   {
			   auto& r = row[i];
			   SafeFormat(line[i].value, 128, "(%+12.4f %+12.4f %+12.4f %+12.4f    )", r.x, r.y, r.z, r.w);
			   lines.push_back(line[i]);
		   }

		   AddBlankLine();
	   }

	   virtual void ShowRow(VisitorName name, const float* vector, const size_t nComponents)
	   {
		   DebugLine line;
		   Name(line, name);

		   StackStringBuilder sb(line.value, sizeof(line.value));
		   sb << "(";

		   for (size_t i = 0; i < nComponents; ++i)
		   {
			   size_t capacity = sizeof(line.value) - sb.Length();
			   if (capacity < 12)
			   {
				   sb << "...";
				   break;
			   }

			   sb.AppendFormat("%+12.4f", vector[i]);
		   }

		   sb << ")";

		   lines.push_back(line);
		   AddBlankLine();
	   }

	   virtual void ShowColumn(VisitorName name, const float* vector, const size_t nComponents)
	   {
		   DebugLine line;
		   Name(line, name);

		   size_t len = rlen(line.key);

		   SafeFormat(line.value, 128, "( %+12.4f    )", vector[0]);

		   lines.push_back(line);

		   for (size_t i = 1; i < nComponents; ++i)
		   {
			   DebugLine line;
			   FillNameWithSpaces(line, len);

			   SafeFormat(line.value, 128, "( %+12.4f    )", vector[i]);
			   lines.push_back(line);
		   }
	   }

	   virtual void Show(VisitorName name, const float value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%+12.4f", value);
		   lines.push_back(line);
	   }

	   virtual void ShowDecimal(VisitorName name, const int32 value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%8d", value);
		   lines.push_back(line);;
	   }

	   virtual void ShowHex(VisitorName name, const int32 value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "0x%.8X", value);
		   lines.push_back(line);
	   }

	   virtual void ShowBool(VisitorName name, const bool value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%s", value ? "true" : "false");
		   lines.push_back(line);
	   }

	   virtual void ShowDecimal(VisitorName name, const int64 value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%lld", value);
		   lines.push_back(line);
	   }

	   virtual void ShowHex(VisitorName name, const int64 value)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%.8x:%.8x", (int32)(value >> 32), (int32)(value & 0xFFFFFFFF));
		   lines.push_back(line);
	   }

	   virtual void ShowPointer(VisitorName name, const void* ptr)
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "0x%p", ptr);
		   lines.push_back(line);
	   }

	   virtual void ShowString(VisitorName name, cstr format, ...)
	   {
		   DebugLine line;
		   Name(line, name);

		   va_list args;
		   va_start(args, format);

		   SafeVFormat(line.value, line.VALUE_LEN, format, args);

		   lines.push_back(line);
	   }

	   virtual void ShowSelectableString(VisitorName name, cstr format, ...)
	   {
		   DebugLine line;
		   Name(line, name);

		   va_list args;
		   va_start(args, format);

		   SafeVFormat(line.value, line.VALUE_LEN, format, args);

		   line.isSelectable = true;

		   lines.push_back(line);
	   }

	   virtual void Clear()
	   {
		   lines.clear();
	   }

	   virtual void Free()
	   {
		   delete this;
	   }

	   void OnEvent(Events::ScrollEvent& se)
	   {
		   if (se.fromScrollbar)
		   {
			   pos = se.logicalValue;
		   }
	   }

	   bool AppendKeyboardEvent(const KeyboardEvent& key) override
	   {
		   ScrollEvent se(""_event);
		   if (scrollbar->AppendEvent(key, se))
		   {
			   pos = se.logicalValue;
			   return true;
		   }

		   return false;
	   }

	   void AppendMouseEvent(const MouseEvent& ev) override
	   {
		   ScrollEvent se(""_event);
		   if (scrollbar->AppendEvent(ev, TopLeft(scrollRect), se))
		   {
			   pos = se.logicalValue;
		   }

		   if (ev.HasFlag(MouseEvent::LUp))
		   {
			   queueMouseUp = true;
		   }
	   }

	   void RenderScrollbar(IGuiRenderContext& gc, const GuiRect& absRect)
	   {
		   Modality modality;
		   modality.isModal = true;
		   modality.isTop = true;
		   modality.isUnderModal = false;

		   RGBAb hiCol(192, 192, 192);
		   RGBAb loCol(160, 160, 160);
		   RGBAb hiEdg(255, 255, 255);
		   RGBAb loEdg(224, 224, 224);

		   scrollbar->Render(gc, absRect, modality, hiCol, loCol, hiEdg, loEdg, *this, ""_event);
	   }

	   int32 knownHeight = 0;
	   int32 pos = 0;

	   std::string selectedKey;
	   std::string selectedValue;

	   void CancelSelect()
	   {
		   selectedKey.clear();
		   selectedValue.clear();
	   }

	   cstr SelectedKey() const override
	   {
		   return selectedKey.empty() ? nullptr : selectedKey.c_str();
	   }

	   cstr SelectedValue() const override
	   {
		   return selectedValue.empty() ? nullptr : selectedValue.c_str();
	   }

	   void RenderStringList(IGuiRenderContext& gc, const GuiRect& absRect, int padding)
	   {
		   GuiMetrics metrics;
		   gc.Renderer().GetGuiMetrics(metrics);

		   Rococo::Graphics::StackSpaceGraphics ssg;

		   RGBAb keyColour(192, 192, 192);
		   RGBAb valueColour(255, 255, 255);

		   int fontIndex = 9;

		   int32 keyMaxWidth = 0;

		   GuiRect screenRect{ absRect };

		   int32 totalHeight = 0;
		   int32 lastRowHeight = 0;

		   for (auto& line : lines)
		   {
			   auto& job = Rococo::Graphics::CreateLeftAlignedText(ssg, screenRect, 0, 0, fontIndex, line.key, keyColour);
			   auto span = gc.EvalSpan({ 0,0 }, job);
			   keyMaxWidth = max(keyMaxWidth, span.x);
			   totalHeight += span.y;
			   lastRowHeight = span.y;
		   }

		   if (totalHeight != knownHeight)
		   {
			   knownHeight = totalHeight;
			   pos = 0;

			   ScrollEvent se(""_event);
			   se.logicalMaxValue = totalHeight;
			   se.logicalMinValue = 0;
			   se.logicalPageSize = Height(absRect);
			   se.logicalValue = 0;
			   se.rowSize = lastRowHeight;
			   se.fromScrollbar = false;

			   if (se.logicalPageSize > totalHeight)
			   {
				   se.logicalPageSize = totalHeight;
			   }

			   scrollbar->SetScroller(se);
		   }

		   keyMaxWidth += padding;

		   GuiRect keyRect = { absRect.left, absRect.top, absRect.left + keyMaxWidth, absRect.bottom };
		   GuiRect valueRect{ absRect.left + keyMaxWidth, absRect.top, absRect.right, absRect.bottom };

		   keyRect.top -= pos;
		   valueRect.top -= pos;

		   for (auto& line : lines)
		   {
			   int32 dy = 0;
			   if (*line.key != 0 || *line.value != 0)
			   {
				   auto& job = Rococo::Graphics::CreateLeftAlignedText(ssg, keyRect, 0, 0, fontIndex, line.key, keyColour);
				   auto span = gc.EvalSpan({ 0,0 }, job);

				   if (keyRect.top >= absRect.top)
				   {
					   RGBAb backColour = RGBAb(0, 0, 0, 192);

					   GuiRect keyback{ keyRect.left, keyRect.top, keyRect.right, keyRect.top + span.y };
					  
					   if (line.isSelectable)
					   {
						   if (IsPointInRect(metrics.cursorPosition, keyback))
						   {
							   backColour = RGBAb(32, 16, 16, 255);
						   }

						   if (!selectedKey.empty() && Eq(line.key, selectedKey.c_str()))
						   {
							   if (!selectedValue.empty() && Eq(line.value, selectedValue.c_str()))
							   {
								   backColour = RGBAb(64, 64, 64, 255);
							   }
						   }
					   }

					   Rococo::Graphics::DrawRectangle(gc, keyback, backColour, backColour);

					   gc.RenderText(TopLeft(keyRect), job);

					   auto& job2 = Rococo::Graphics::CreateLeftAlignedText(ssg, valueRect, 0, 0, fontIndex, line.value, valueColour);
					   auto span2 = gc.EvalSpan({ 0,0 }, job);
					  
					   GuiRect valueback{ valueRect.left, valueRect.top, valueRect.left + span2.x + 4, valueRect.top + span2.y };

					   if (line.isSelectable)
					   {
						   if (IsPointInRect(metrics.cursorPosition, keyback) || IsPointInRect(metrics.cursorPosition, valueback))
						   {
							   backColour = RGBAb(32, 16, 16, 255);
						   }

						   if (!selectedKey.empty() && Eq(line.key, selectedKey.c_str()))
						   {
							   if (!selectedValue.empty() && Eq(line.value, selectedValue.c_str()))
							   {
								   backColour = RGBAb(64, 64, 64, 255);
							   }
						   }
					   }
					   Rococo::Graphics::DrawRectangle(gc, valueback, backColour, backColour);
					   
					   if (queueMouseUp && IsPointInRect(metrics.cursorPosition, valueback))
					   {
						   queueMouseUp = false;

						   if (line.isSelectable)
						   {
							   selectedKey = line.key;
							   selectedValue = line.value;
						   }
						   else
						   {
							   selectedKey.clear();
							   selectedValue.clear();
						   }
					   }

					   gc.RenderText(TopLeft(valueRect), job);
					   dy = max(span2.y, span.y);
				   }
				   else
				   {
					   dy = span.y;
				   }
			   }
			   else
			   {
				   dy = 13;
			   }

			   keyRect.top += dy;
			   valueRect.top += dy;

			   if (keyRect.top >= keyRect.bottom)
				   break;
		   }
	   }

	   virtual void Render(IGuiRenderContext& gc, const GuiRect& absRect, int padding)
	   {
		   scrollRect = GuiRect{ absRect.right - 24, absRect.top, absRect.right, absRect.bottom };
		   RenderScrollbar(gc, scrollRect);

		   GuiRect strListRect{ absRect.left, absRect.top, scrollRect.left, absRect.bottom };
		   RenderStringList(gc, strListRect, padding);
	   }
   };
}

namespace Rococo
{
   IMathsVisitorSupervisor* CreateMathsVisitor(IUtilitiies& utilities, IKeyboardSupervisor& keyboard)
   {
      return new MathsVisitor(utilities, keyboard);
   }
}
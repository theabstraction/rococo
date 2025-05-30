#include <rococo.mplat.h>
#include <vector>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdarg.h>
#include <rococo.ui.h>
#include <rococo.fonts.hq.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Strings;
using namespace Rococo::Graphics;

namespace
{
   auto nullEvent = ""_event;

   struct DebugLine
   {
      enum size_t { KEY_LEN = 64, VALUE_LEN = 128 };
	  EventIdRef selectEvent = nullEvent;
      char key[KEY_LEN];
      char value[VALUE_LEN];
	  bool isSelectable = false;
   };

   class MathsVisitor : public IMathsVisitorSupervisor, public IEventCallback<ScrollEvent>
   {
	   AutoFree<GUI::IScrollbar> scrollbar;
	   std::vector<DebugLine> lines;
	   GuiRect scrollRect;
	   bool queueMouseUp = false;
	   IUtilities& utilities;
	   Events::IPublisher& publisher;
	   ID_FONT idDebuggerFont;

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
	   MathsVisitor(IUtilities& _utilities, IPublisher& _publisher) :
		   utilities(_utilities),
		   scrollbar(_utilities.CreateScrollbar(true)),
		   publisher(_publisher)
		   
	   {
	   }

	   void Show(VisitorName name, const Matrix4x4& m) override
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

	   void ShowRow(VisitorName name, const float* vector, const size_t nComponents) override
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

	   void ShowColumn(VisitorName name, const float* vector, const size_t nComponents) override
	   {
		   DebugLine line;
		   Name(line, name);

		   size_t len = rlen(line.key);

		   SafeFormat(line.value, 128, "( %+12.4f    )", vector[0]);

		   lines.push_back(line);

		   for (size_t i = 1; i < nComponents; ++i)
		   {
			   DebugLine line2;
			   FillNameWithSpaces(line2, len);

			   SafeFormat(line2.value, 128, "( %+12.4f    )", vector[i]);
			   lines.push_back(line2);
		   }
	   }

	   void Show(VisitorName name, const float value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%+12.4f", value);
		   lines.push_back(line);
	   }

	   void ShowDecimal(VisitorName name, const int32 value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%8d", value);
		   lines.push_back(line);;
	   }

	   void ShowDecimal(VisitorName name, const uint64 value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%llu", value);
		   lines.push_back(line);;
	   }

	   void ShowHex(VisitorName name, const int32 value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "0x%.8X", value);
		   lines.push_back(line);
	   }

	   void ShowBool(VisitorName name, const bool value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%s", value ? "true" : "false");
		   lines.push_back(line);
	   }

	   void ShowDecimal(VisitorName name, const int64 value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%lld", value);
		   lines.push_back(line);
	   }

	   void ShowHex(VisitorName name, const int64 value) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "%.8x:%.8x", (int32)(value >> 32), (int32)(value & 0xFFFFFFFF));
		   lines.push_back(line);
	   }

	   void ShowPointer(VisitorName name, const void* ptr) override
	   {
		   DebugLine line;
		   Name(line, name);
		   SafeFormat(line.value, line.VALUE_LEN, "0x%p", ptr);
		   lines.push_back(line);
	   }

	   void ShowString(VisitorName name, cstr format, ...) override
	   {
		   DebugLine line;
		   Name(line, name);

		   va_list args;
		   va_start(args, format);

		   SafeVFormat(line.value, line.VALUE_LEN, format, args);

		   lines.push_back(line);
	   }

	   void ShowSelectableString(cstr eventName, VisitorName name, cstr format, ...) override
	   {
		   DebugLine line{ EventIdRef { eventName, 0} };
		   Name(line, name);

		   va_list args;
		   va_start(args, format);

		   SafeVFormat(line.value, line.VALUE_LEN, format, args);

		   line.isSelectable = true;

		   lines.push_back(line);
	   }

	   void Clear() override
	   {
		   lines.clear();
	   }

	   void Free() override
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
		   ScrollEvent se;
		   if (scrollbar->AppendEvent(key, se))
		   {
			   pos = se.logicalValue;
			   return true;
		   }

		   return false;
	   }

	   void SelectAtPos(Vec2i pos) override
	   {
		   UNUSED(pos);
		   queueMouseUp = true;
	   }

	   void AppendMouseEvent(const MouseEvent& ev) override
	   {
		   ScrollEvent se;
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
		   RGBAb hiSli(224, 224, 224);
		   RGBAb loSli(192, 192, 192);

		   scrollbar->Render(
			   gc,
			   absRect,
			   modality,
			   hiCol,
			   loCol, 
			   hiSli,
			   loSli,
			   hiEdg,
			   loEdg,
			   *this, 
			   ""_event
		   );
	   }

	   int32 knownHeight = 0;
	   int32 pos = 0;

	   size_t selectedLine = (size_t) - 1;

	   void CancelSelect()
	   {
		   selectedLine = (size_t) -1;
	   }

	   void RenderStringList(IGuiRenderContext& gc, const GuiRect& absRect, int padding)
	   {
		   GuiMetrics metrics;
		   gc.Renderer().GetGuiMetrics(metrics);

		   RGBAb keyColour(192, 192, 192);
		   RGBAb valueColour(255, 255, 255);

		   int32 keyMaxWidth = 0;

		   GuiRect screenRect{ absRect };

		   int32 totalHeight = 0;
		   int32 lastRowHeight = 0;
		   
		   GuiRect lineRect{ screenRect };

		   auto fm = gc.Resources().GetFontMetrics(idDebuggerFont);

		   lineRect.bottom = lineRect.top + fm.imgHeight;

		   for (auto& line : lines)
		   {
			   RGBAb transparent(0, 0, 0, 0);
			   Vec2 span = Rococo::Graphics::RenderHQText_LeftAligned_VCentre(gc, idDebuggerFont, lineRect, line.key, transparent);
			   keyMaxWidth = max(keyMaxWidth, (int32) span.x);
			   totalHeight += fm.imgHeight;
			   lastRowHeight = fm.imgHeight;
		   }

		   if (totalHeight != knownHeight)
		   {
			   knownHeight = totalHeight;
			   pos = 0;

			   ScrollEvent se;
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

			   scrollbar->SetScrollState(se);
		   }

		   keyMaxWidth += padding;

		   GuiRect keyRect = { absRect.left, absRect.top, absRect.left + keyMaxWidth, 0 };
		   GuiRect valueRect{ absRect.left + keyMaxWidth, absRect.top, absRect.right, 0 };

		   keyRect.top -= pos;
		   valueRect.top -= pos;

		   keyRect.bottom = keyRect.top + fm.height;
		   valueRect.bottom = valueRect.top + fm.height;

		   size_t lineIndex = 0;

		   for (auto& line : lines)
		   {
			   int32 dy = 0;
			   if (*line.key != 0 || *line.value != 0)
			   {
				   if (keyRect.top >= absRect.top)
				   {
					   RGBAb backColour = RGBAb(0, 0, 0, 255);

					   if (line.isSelectable)
					   {
						   if (IsPointInRect(metrics.cursorPosition, lineRect))
						   {
							   backColour = RGBAb(96, 16, 16, 255);
						   }

						   if (selectedLine == lineIndex)
						   {
							   backColour = RGBAb(128, 32, 32, 255);
						   }
					   }

					   GuiRect backRect = lineRect;
					   backRect.bottom = min(backRect.bottom, absRect.bottom);

					   if (lineRect.top < absRect.bottom)
					   {
						   Rococo::Graphics::DrawRectangle(gc, backRect, backColour, backColour);
					   }

					   Rococo::Graphics::RenderHQText_LeftAligned_VCentre(gc, idDebuggerFont, keyRect, line.key, keyColour);
					   Rococo::Graphics::RenderHQText_LeftAligned_VCentre(gc, idDebuggerFont, valueRect, line.value, valueColour);

					   if (queueMouseUp && IsPointInRect(metrics.cursorPosition, lineRect))
					   {
						   queueMouseUp = false;

						   if (line.isSelectable)
						   {
							   selectedLine = lineIndex;
							   if (*line.selectEvent.name != 0)
							   {
								   VisitorItemClickedEvent clicked;
								   clicked.key = line.key;
								   clicked.value = line.value;
								   publisher.Publish(clicked, line.selectEvent);
							   }
						   }
						   else
						   {
							   CancelSelect();
						   }
					   }
				   }
			   }

			   dy = fm.height;

			   keyRect.top += dy;
			   valueRect.top += dy;

			   keyRect.bottom += dy;
			   valueRect.bottom += dy;

			   lineRect.top += dy;
			   lineRect.bottom += dy;

			   lineIndex++;

			   if (keyRect.top >= absRect.bottom)
				   break;
		   }
	   }

	   void Render(IGuiRenderContext& gc, const GuiRect& absRect, int padding) override
	   {
		   if (!idDebuggerFont)
		   {
			   idDebuggerFont = ID_FONT{ static_cast<int>(Graphics::HQFont::DebuggerFont) };
		   }

		   scrollRect = GuiRect{ absRect.right - 24, absRect.top, absRect.right, absRect.bottom };
		   RenderScrollbar(gc, scrollRect);

		   GuiRect strListRect{ absRect.left + 4, absRect.top, scrollRect.left - 8, absRect.bottom };
		   gc.FlushLayer();
		   gc.SetScissorRect(strListRect);
		   RenderStringList(gc, strListRect, padding);
		   gc.ClearScissorRect();
		   gc.FlushLayer();
	   } 
   };
}

namespace Rococo
{
   IMathsVisitorSupervisor* CreateMathsVisitor(IUtilities& utilities, IPublisher& publisher)
   {
      return new MathsVisitor(utilities, publisher);
   }
}
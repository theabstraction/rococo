#include "hv.h"
#include <vector>
#include <rococo.strings.h>
#include <stdarg.h>

namespace
{
   using namespace HV;

   struct DebugLine
   {
      enum size_t { KEY_LEN = 64, VALUE_LEN = 128 };
      wchar_t key[KEY_LEN];
      wchar_t value[VALUE_LEN];
   };

   class MathsVisitor: public IMathsVisitorSupervisor, public IUIOverlay
   {
      std::vector<DebugLine> lines;

      void Name(DebugLine& line, VisitorName name)
      {
         SafeCopy(line.key, name, _TRUNCATE);
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
         size_t len = min(count, size_t (DebugLine::KEY_LEN - 1));
         for (size_t i = 0; i < len; ++i)
         {
            line.key[i] = L' ';
         }

         line.key[len] = 0;
      }
   public:
      MathsVisitor()
      {

      }

      virtual void Show(VisitorName name, const Matrix4x4& m)
      {
         DebugLine line[4];
         Name(line[0], name);

         size_t len = wcslen(line[0].key);
         for (int i : {1, 2, 3})
         {
            FillNameWithSpaces(line[i], len);
         }

         const Vec4* row = &m.row0;

         for (int i : {0, 1, 2, 3})
         {
            auto& r = row[i];
            SafeFormat(line[i].value, _TRUNCATE, L"(%+12.4f %+12.4f %+12.4f %+12.4f    )", r.x, r.y, r.z, r.w);
            lines.push_back(line[i]);
         }

         AddBlankLine();
      }

      virtual void ShowRow(VisitorName name, const float* vector, const size_t nComponents)
      {
         DebugLine line;
         Name(line, name);
       
         SafeCopy(line.value, L"(", _TRUNCATE);
         
         size_t index = 1;
         for (size_t i = 0; i < nComponents; ++i)
         {
            size_t capacity = DebugLine::VALUE_LEN - index;
            if (capacity < 12)
            {
               SafeFormat(line.value + index, capacity, _TRUNCATE, L"...");
               break;
            }

            int nChars = SafeFormat(line.value + index, capacity, _TRUNCATE, L"%+12.4f", vector[i]);
            if (nChars > 0)
            {
               index += nChars;
            }
         }

         SafeFormat(line.value + index, DebugLine::VALUE_LEN - index, _TRUNCATE, L")");

         lines.push_back(line);
         AddBlankLine();
      }

      virtual void ShowColumn(VisitorName name, const float* vector, const size_t nComponents)
      {
         DebugLine line;
         Name(line, name);

         size_t len = wcslen(line.key);

         SafeFormat(line.value, _TRUNCATE, L"( %+12.4f    )", vector[0]);

         lines.push_back(line);

         for (size_t i = 1; i < nComponents; ++i)
         {
            DebugLine line;
            FillNameWithSpaces(line, len);

            SafeFormat(line.value, _TRUNCATE, L"( %+12.4f    )", vector[i]);
            lines.push_back(line);
         }
      }

      virtual void Show(VisitorName name, const float value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%+12.4f", value);
         lines.push_back(line);
      }

      virtual void ShowDecimal(VisitorName name, const int32 value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%8d", value);
         lines.push_back(line);;
      }

      virtual void ShowHex(VisitorName name, const int32 value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"0x%.8X", value);
         lines.push_back(line);
      }

      virtual void ShowBool(VisitorName name, const bool value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%S", value ? "true" : "false");
         lines.push_back(line);
      }

      virtual void ShowDecimal(VisitorName name, const int64 value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%lld", value);
         lines.push_back(line);
      }

      virtual void ShowHex(VisitorName name, const int64 value)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%.8x:%.8x", (int32)(value >> 32), (int32) (value & 0xFFFFFFFF));
         lines.push_back(line);
      }

      virtual void ShowPointer(VisitorName name, const void* ptr)
      {
         DebugLine line;
         Name(line, name);
         SafeFormat(line.value, _TRUNCATE, L"%p", ptr);
         lines.push_back(line);
      }

      virtual void ShowString(VisitorName name, const wchar_t* format, ...)
      {
         DebugLine line;
         Name(line, name);

         va_list args;
         va_start(args, format);

         SafeFormat(line.value, _TRUNCATE, format, args);

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

      virtual IUIOverlay& Overlay()
      {
         return *this;
      }

      virtual void Render(IGuiRenderContext& gc)
      {
         GuiMetrics metrics;
         gc.Renderer().GetGuiMetrics(metrics);

         Rococo::Graphics::StackSpaceGraphics ssg;

         RGBAb keyColour(192, 192, 192);
         RGBAb valueColour(255, 255, 255);

         int fontIndex = 9;

         int32 keyMaxWidth = 0;

         GuiRect screenRect{ 0, 0,  metrics.screenSpan.x, metrics.screenSpan.y };

         for (auto& line : lines)
         {
            auto& job = Rococo::Graphics::CreateLeftAlignedText(ssg, screenRect, 300, 350, fontIndex, line.key, keyColour);
            auto span = gc.EvalSpan({ 0,0 }, job);
            keyMaxWidth = max(keyMaxWidth, span.x);
         }

         GuiRect keyRect{ 0, 0, keyMaxWidth, metrics.screenSpan.y };
         GuiRect valueRect{ keyMaxWidth, 0, metrics.screenSpan.x, metrics.screenSpan.y };

         for (auto& line : lines)
         {
            int32 dy = 0;
            if (*line.key != 0 || *line.value != 0)
            {
               auto& job = Rococo::Graphics::CreateLeftAlignedText(ssg, keyRect, 0, 0, fontIndex, line.key, keyColour);
               auto span = gc.EvalSpan({ 0,0 }, job);

               GuiRect keyback { keyRect.left, keyRect.top, keyRect.right, keyRect.top + span.y };
               Rococo::Graphics::DrawRectangle(gc, keyback, RGBAb(0, 0, 0, 192), RGBAb(0, 0, 0, 192));

               gc.RenderText(TopLeft(keyRect), job);

               auto& job2 = Rococo::Graphics::CreateLeftAlignedText(ssg, valueRect, 100, 150, fontIndex, line.value, valueColour);
               auto span2 = gc.EvalSpan({ 0,0 }, job);
               GuiRect valueback{ valueRect.left, valueRect.top, valueRect.left + span2.x + 4, valueRect.top + span2.y };
               Rococo::Graphics::DrawRectangle(gc, valueback, RGBAb(0, 0, 0, 192), RGBAb(0, 0, 0, 192));

               gc.RenderText(TopLeft(valueRect), job);

               dy = max(span2.y, span.y);
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
   };
}

namespace HV
{
   IMathsVisitorSupervisor* CreateMathsVisitor()
   {
      return new MathsVisitor();
   }
}
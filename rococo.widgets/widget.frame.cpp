#include <rococo.api.h>
#include <rococo.renderer.h>

namespace Rococo
{
   namespace Widgets
   {
      struct WidgetFrame
      {
         RGBAb bkColour1;
         RGBAb bkColour2;
         RGBAb borderColour1;
         RGBAb borderColour2;
         GuiRect borderWidths;
      };

      struct WidgetFrameSet
      {
         WidgetFrame frames[2]; // normal, lit
      };

      struct Label
      {
         int32 fontIndex;
         RGBAb fontColour;
      };

      struct LabelSet
      {
         Label labels[2]; // normal, lit
         wchar_t text[256];
      };

      void Draw3DFrame(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, bool isLit)
      {
         auto& frame = frameSet.frames[isLit ? 1 : 0];

         GuiRect innerRect = rect;
         innerRect.left += frame.borderWidths.left;
         innerRect.top += frame.borderWidths.top;
         innerRect.right -= frame.borderWidths.right;
         innerRect.bottom -= frame.borderWidths.bottom;

         Graphics::DrawRectangle(rc, innerRect, frame.bkColour1, frame.bkColour2);
     
         GuiRect topRect{ rect.left, rect.top, rect.right, rect.top + frame.borderWidths.top};
         Graphics::DrawRectangle(rc, topRect, frame.borderColour1, frame.borderColour1);

         GuiRect bottomRect{ rect.left, rect.bottom, rect.right, rect.bottom - frame.borderWidths.bottom };
         Graphics::DrawRectangle(rc, bottomRect, frame.borderColour2, frame.borderColour2);

         GuiRect leftRect{ rect.left, rect.top, rect.left + frame.borderWidths.left, rect.bottom };
         Graphics::DrawRectangle(rc, leftRect, frame.borderColour1, frame.borderColour1);

         GuiRect rightRect{ rect.right - frame.borderWidths.right, rect.top, rect.right, rect.bottom };
         Graphics::DrawRectangle(rc, rightRect, frame.borderColour2, frame.borderColour2);
      }

      void DrawCenteredLabel(IGuiRenderContext& rc, const GuiRect& rect, struct LabelSet& labelSet, bool isLit)
      {
         auto& label = labelSet.labels[isLit ? 1 : 0];
         Graphics::RenderCentredText(rc, labelSet.text, label.fontColour, label.fontIndex, Centre(rect));
      }

      void Draw3DButton(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, struct LabelSet& labelSet, bool isLit)
      {
         Draw3DFrame(rc, rect, frameSet, isLit);
         DrawCenteredLabel(rc, rect, labelSet, isLit);
      }
   }
}
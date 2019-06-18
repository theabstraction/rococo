#include <rococo.api.h>

namespace Rococo
{
   namespace Events
   {
      struct Event;
      class IPublisher;
   }

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
         char text[256];
      };

      void Draw3DFrame(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, bool isLit);
      void DrawCenteredLabel(IGuiRenderContext& rc, const GuiRect& rect, struct LabelSet& labelSet, bool isLit);
      void Draw3DButton(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, struct LabelSet& labelSet, bool isLit);

      ROCOCOAPI IStatusBar
      {
         virtual void Free() = 0;
         virtual void Render(IGuiRenderContext& gc, const GuiRect& rect) = 0;
      };

      IStatusBar* CreateStatusBar(Events::IPublisher& publisher);

      void SetStatus(cstr statustext, Events::IPublisher& publisher);
   }
}
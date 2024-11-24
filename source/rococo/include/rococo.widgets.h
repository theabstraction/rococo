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

      void Draw3DFrame(Graphics::IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, bool isLit);
      void DrawCenteredLabel(Graphics::IGuiRenderContext& rc, const GuiRect& rect, struct LabelSet& labelSet, bool isLit);
      void Draw3DButton(Graphics::IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, struct LabelSet& labelSet, bool isLit);

      ROCOCO_INTERFACE IStatusBar
      {
         virtual void Free() = 0;
         virtual void Render(Graphics::IGuiRenderContext& gc, const GuiRect& rect) = 0;
      };

      IStatusBar* CreateStatusBar(Events::IPublisher& publisher);

      void SetStatus(cstr statustext, Events::IPublisher& publisher);
   }
}
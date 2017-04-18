#include <rococo.api.h>

namespace Rococo
{
   namespace Events
   {
      struct Event;
      struct IPublisher;
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
         wchar_t text[256];
      };

      void Draw3DFrame(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, bool isLit);
      void DrawCenteredLabel(IGuiRenderContext& rc, const GuiRect& rect, struct LabelSet& labelSet, bool isLit);
      void Draw3DButton(IGuiRenderContext& rc, const GuiRect& rect, struct WidgetFrameSet& frameSet, struct LabelSet& labelSet, bool isLit);

      ROCOCOAPI IUITarget
      {
         virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) = 0;
         virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown) = 0;
         virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown) = 0;
         virtual void GetRect(GuiRect& rect) const = 0;
      };

      void RouteEventToUI(Events::Event& ev, Vec2i cursorPos, Widgets::IUITarget& ui);

      ROCOCOAPI IWindowTree : public IUITarget
      {
         virtual void AddTarget(IUITarget* target, int32 zOrder) = 0;
         virtual void Clear() = 0;
         virtual void Free() = 0;
         virtual void SetRect(const GuiRect& rect) = 0;
      };

      IWindowTree* CreateWindowTree();

      ROCOCOAPI IToolbar: public IUITarget
      {
         virtual void AddButton(const wchar_t* name, Rococo::Events::EventId id, const wchar_t* buttonTextureResource) = 0;
         virtual GuiRect Render(IGuiRenderContext& rc, bool horizontal, int buttonBorder, RGBAb backColour, RGBAb highlightBorder) = 0;
         virtual void SetPosition(Vec2i pos) = 0;
         virtual void SetToggleOn(const wchar_t* name) = 0;
         virtual void SetToggleOff(const wchar_t* name) = 0;
         virtual void SetToggleColours(RGBAb colour, RGBAb borderColour) = 0;
         virtual void Free() = 0;
      };

      IToolbar* CreateToolbar(Rococo::Events::IPublisher& publisher, IRenderer& renderer);

      ROCOCOAPI IStatusBar
      {
         virtual void Free() = 0;
         virtual void Render(IGuiRenderContext& gc, const GuiRect& rect) = 0;
      };

      IStatusBar* CreateStatusBar(Events::IPublisher& publisher);

      void SetStatus(const wchar_t* statustext, Events::IPublisher& publisher);
   }
}
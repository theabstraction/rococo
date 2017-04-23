#include "dystopia.h"

#include "rococo.renderer.h"
#include "rococo.ui.h"
#include "rococo.fonts.h"
#include "rococo.strings.h"

#include <unordered_map>
#include <algorithm>

#include <stdarg.h>

namespace
{
   using namespace Dystopia;
   using namespace Dystopia::UI;
   using namespace Rococo;

   enum EWidgetType
   {
      EWidgetType_Frame,
      EWidgetType_Button
   };

   struct BaseWidget
   {
      struct BackgroundColours
      {
         BackgroundColours() :
            bk1{ 0x00,0x20,000,0xFF },
            bk2{ 0x20,0x00,0x00,0xFF },
            border1{ 0xDF,0xDF,0xDF,0xFF },
            border2{ 0xFF,0xFF,0xFF,0xFF },
            borderWidths{ 1,1 }
         {

         }

         RGBAb bk1;
         RGBAb bk2;
         RGBAb border1;
         RGBAb border2;
         Vec2i borderWidths;
      };

      BaseWidget(ID_WIDGET _id) :
         span{ 32,32 },
         posInContainer{ 0,0 },
         id(_id)
      {

      }

      virtual EWidgetType Type() const = 0;
      virtual void Render(IGuiRenderContext& gc, const Vec2i& containerAbsPos) const = 0;
      virtual void Free() = 0;

      Vec2i posInContainer;
      Vec2i span;
      BackgroundColours normalBk;
      BackgroundColours hilightBk;
      ID_WIDGET id;
   };

   void RenderBaseWidgetBackground(IGuiRenderContext& gc, const BaseWidget& widget, const GuiRect& rect)
   {
      GuiMetrics metrics;
      gc.Renderer().GetGuiMetrics(metrics);

      auto& scheme = IsPointInRect(metrics.cursorPosition, rect) ? widget.hilightBk : widget.normalBk;

      Graphics::DrawRectangle(gc, rect, scheme.bk1, scheme.bk2);
      Graphics::DrawBorderAround(gc, rect, scheme.borderWidths, scheme.border1, scheme.border2);
   }

   struct WidgetButton : public BaseWidget
   {
      struct FontDef
      {
         FontDef() : fontIndex(2), colour{ 0xFF,0xFF,0xFF,0xFF } {}
         int32 fontIndex;
         RGBAb colour;
      } normalFont, hiFont;

      rchar text[32];
      ID_UI_EVENT_TYPE commandId;
      boolean32 fireWhenDown;
      boolean32 fireWhenUp;

      WidgetButton(ID_WIDGET _id) :
         BaseWidget(_id),
         text{ 0 },
         commandId(0),
         fireWhenDown(false),
         fireWhenUp(false)
      {
      }

      virtual EWidgetType Type() const
      {
         return EWidgetType_Button;
      }

      virtual void Render(IGuiRenderContext& gc, const Vec2i& absContainerPos) const
      {
         Vec2i absWidgetPos = absContainerPos + posInContainer;
         GuiRect rect{ absWidgetPos.x, absWidgetPos.y, span.x + absWidgetPos.x, span.y + absWidgetPos.y };
         RenderBaseWidgetBackground(gc, *this, rect);

         GuiMetrics metrics;
         gc.Renderer().GetGuiMetrics(metrics);

         const FontDef& font = IsPointInRect(metrics.cursorPosition, rect) ? hiFont : normalFont;

         Graphics::RenderCentredText(gc, text, font.colour, font.fontIndex, Centre(rect));
      }

      virtual void Free()
      {
         delete this;
      }
   };

   struct WidgetFrame : public BaseWidget
   {
      Vec2i border;
      std::vector<ID_WIDGET> children;

      WidgetFrame(ID_WIDGET _id) :
         BaseWidget(_id)
      {
      }

      virtual EWidgetType Type() const
      {
         return EWidgetType_Frame;
      }

      virtual void Render(IGuiRenderContext& gc, const Vec2i& absContainerPos) const
      {
         Vec2i absWidgetPos = absContainerPos + posInContainer;
         GuiRect rect{ absWidgetPos.x, absWidgetPos.y, span.x + absWidgetPos.x, span.y + absWidgetPos.y };
         RenderBaseWidgetBackground(gc, *this, rect);
      }

      virtual void Free()
      {
         delete this;
      }
   };

   typedef std::unordered_map<ID_WIDGET, BaseWidget*, ID_WIDGET> TWidgetSet;

   WidgetFrame& GetFrame(ID_WIDGET id, TWidgetSet& set, cstr debugSrc)
   {
      auto i = set.find(id);
      if (i == set.end())
      {
         Throw(0, L"Could not find widget with Id %d in %s", id, debugSrc);
      }

      if (i->second->Type() != EWidgetType_Frame)
      {
         Throw(0, L"Widget %d is not a frame", id);
      }

      return reinterpret_cast<WidgetFrame&>(*i->second);
   }

   void Clear(TWidgetSet& set)
   {
      for (auto i : set)
      {
         i.second->Free();
      }

      set.clear();
   }

   void AssertUnique(TWidgetSet& set, ID_WIDGET id, cstr debugName)
   {
      auto i = set.find(id);
      if (i != set.end())
      {
         Throw(0, L"Cannot add widget %d to %s: a widget with this id already exists. Ids must be unique.", id, debugName);
      }
   }

   class GuiBuilder : public UI::IUIBuilder, public IUIBuilderSupervisor
   {
      std::unordered_map<std::string, TWidgetSet*> panelSet;
      TWidgetSet* currentPanel;
      std::string currentPanelName;
   public:
      GuiBuilder()
      {
         BuildPanel(L"default");
         panelSet.insert(std::make_pair(currentPanelName, currentPanel));
      }

      ~GuiBuilder()
      {
         for (auto i : panelSet)
         {
            TWidgetSet* panel = i.second;
            Clear(*panel);
            delete panel;
         }
      }

      virtual void Free()
      {
         delete this;
      }

      virtual UI::IUIBuilder& Builder()
      {
         return *this;
      }

      void BuildPanel(cstr name)
      {
         auto i = panelSet.find(name);
         if (i != panelSet.end())
         {
            currentPanelName = i->first;
         }
         else
         {
            i = panelSet.insert(std::make_pair(name, new TWidgetSet)).first;
            currentPanelName = name;  
         }

         Clear(*i->second);

         auto* baseFrame = new WidgetFrame(ID_WIDGET(1));
         baseFrame->span = lastScreenSpan;

         i->second->insert(std::make_pair(ID_WIDGET(1), baseFrame));
         currentPanel = i->second;
      }

      virtual void RebuildPanel(const fstring& panelName)
      {
         BuildPanel(panelName);
      }

      WidgetFrame& GetTopmostFrame()
      {
         auto i = currentPanel->find(ID_WIDGET(1));
         BaseWidget& widget = *i->second;
         return reinterpret_cast<WidgetFrame&>(widget);
      }

      virtual void AddButton(ID_WIDGET id, Vec2i& span, const fstring& text)
      {
         AssertUnique(*currentPanel, id, currentPanelName.c_str());

         auto* b = new WidgetButton(id);
         currentPanel->insert(std::make_pair(id, b));

         b->span.x = min(span.x, 1024);
         b->span.x = max(4, span.x);

         b->span.y = min(span.y, 512);
         b->span.y = max(4, span.y);
         
         SafeCopy(b->text, text, _TRUNCATE);

         auto& f = GetTopmostFrame();
         
         if (f.children.empty())
         {
            // Button inherits frame colours
            b->hilightBk = f.hilightBk;
            b->normalBk = f.normalBk;  
         }
         else
         {
            // Button inherits sibling's colours
            auto siblingId = f.children[f.children.size() - 1];
            auto& sibling = *(*currentPanel)[siblingId];
            b->hilightBk = sibling.hilightBk;
            b->normalBk = sibling.normalBk;

            if (sibling.Type() == EWidgetType_Button)
            {
               auto& sibbutton = reinterpret_cast<WidgetButton&>(sibling);
               b->hiFont = sibbutton.hiFont;
               b->normalFont = sibbutton.normalFont;
            }
         }

         f.children.push_back(id);

         int32 border = 10;

         // Let's have a guess
         int32 buttonsPerRow = max(1, f.span.x / (b->span.x + border));

         int32 rowIndex = (int32) (f.children.size()) / buttonsPerRow;

         int32 y = rowIndex * (border + b->span.y) + border;
         int32 x = ((f.children.size()-1) % buttonsPerRow) * (b->span.x + border) + border;;

         b->posInContainer = Vec2i{ x, y };
      }

      virtual void AddFrame(ID_WIDGET id, ID_WIDGET frameId, Vec2i& span)
      {
         AssertUnique(*currentPanel, id, currentPanelName.c_str());
         currentPanel->insert(std::make_pair(id, new WidgetFrame(id)));
      }

      virtual void AddWidgetToFrame(ID_WIDGET frameId, ID_WIDGET id)
      {
         WidgetFrame& frame = GetFrame(frameId, *currentPanel, currentPanelName.c_str());
         if (frame.children.end() != std::find(frame.children.begin(), frame.children.end(), id))
         {
            Throw(0, L"Widget %d is already framed by frame %d in %s", id, frameId, currentPanelName.c_str());
         }

         for (auto i : *currentPanel)
         {
            BaseWidget* widget = i.second;
            if (widget->Type() == EWidgetType_Frame)
            {
               WidgetFrame& f = reinterpret_cast<WidgetFrame&>(*widget);
               auto j = std::remove(f.children.begin(), f.children.end(), id);
               f.children.erase(j, f.children.end());
            }
         }

         frame.children.push_back(id);
      }

      virtual void HCentreChildren(ID_WIDGET frameId)
      {
         WidgetFrame& frame = GetFrame(frameId, *currentPanel, currentPanelName.c_str());

         int32 halfX = frame.span.x >> 1;

         int32 childSpan = 0;

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            childSpan += child.second->span.x;
         }

         if (!frame.children.empty())
         {
            childSpan += int32(frame.children.size() - 1) * frame.border.x;
         }

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            childSpan += child.second->span.x;
         }

         int32 x = halfX - (childSpan >> 1);

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            child.second->posInContainer.x = x;
            x += child.second->span.x;
            x += frame.border.x;
         }
      }

      virtual void VCentreChildren(ID_WIDGET frameId)
      {
         WidgetFrame& frame = GetFrame(frameId, *currentPanel, currentPanelName.c_str());

         int32 halfY = frame.span.y >> 1;

         int32 childSpan = 0;

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            childSpan += child.second->span.y;
         }

         if (!frame.children.empty())
         {
            childSpan += int32(frame.children.size() - 1) * frame.border.y;
         }

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            childSpan += child.second->span.y;
         }

         int32 y = halfY - (childSpan >> 1);

         for (auto i : frame.children)
         {
            auto& child = *currentPanel->find(i);
            child.second->posInContainer.y = y;
            y += child.second->span.y;
            y += frame.border.y;
         }
      }

      GuiRect GetRectRelativeToParent(const BaseWidget& w)
      {
         return GuiRect(w.posInContainer.x, w.posInContainer.y, w.posInContainer.x + w.span.x, w.posInContainer.y + w.span.y);
      }

      GuiRect GetSmallestRectContainingChildren(const WidgetFrame& parent)
      {
         if (!parent.children.empty())
         {
            GuiRect containingRect(0x80000000, 0x8000000, 0x7FFFFFFF, 0x7FFFFFFF);
            for (auto& id : parent.children)
            {
               auto* child = currentPanel->find(id)->second;
               auto rect = GetRectRelativeToParent(*child);

               if (rect.left > containingRect.left)      containingRect.left = rect.left;
               if (rect.top > containingRect.top)        containingRect.top = rect.top;
               if (rect.right < containingRect.right)    containingRect.right = rect.right;
               if (rect.bottom < containingRect.bottom)  containingRect.bottom = rect.bottom;
            }

            return containingRect;
         }
         else
         {
            return GuiRect(-1, -1, -1, -1);
         }
      }

      virtual void ShrinkWrap(ID_WIDGET frameId)
      {
         WidgetFrame& frame = GetFrame(frameId, *currentPanel, currentPanelName.c_str());
         if (!frame.children.empty())
         {
            GuiRect rect = GetSmallestRectContainingChildren(frame);
            rect.left -= frame.border.x;
            rect.right += frame.border.x;
            rect.top -= frame.border.y;
            rect.bottom += frame.border.y;

            int32 dx = 0;
            if (rect.left > 0)
            {
               dx = -rect.left;
               frame.posInContainer.x += rect.left;
            }

            if (rect.right < frame.span.x)
            {
               dx -= frame.span.x - rect.right;
               frame.span.x += dx;
            }

            int32 dy = 0;
            if (rect.top > 0)
            {
               dy = -rect.top;
               frame.posInContainer.y += rect.top;
            }

            if (rect.bottom < frame.span.y)
            {
               dy -= frame.span.y - rect.bottom;
               frame.span.y += dy;
            }
         }
      }

      virtual void ExpandToFit(ID_WIDGET frameId)
      {
         WidgetFrame& frame = GetFrame(frameId, *currentPanel, currentPanelName.c_str());
         if (!frame.children.empty())
         {
            GuiRect rect = GetSmallestRectContainingChildren(frame);
            rect.left -= frame.border.x;
            rect.right += frame.border.x;
            rect.top -= frame.border.y;
            rect.bottom += frame.border.y;

            int32 dx = 0;
            if (rect.left < 0)
            {
               dx = rect.left;
               frame.posInContainer.x -= rect.left;
            }

            if (rect.right > frame.span.x)
            {
               dx += rect.right - frame.span.x;
               frame.span.x += dx;
            }

            int32 dy = 0;
            if (rect.top < 0)
            {
               dy = -rect.top;
               frame.posInContainer.y += rect.top;
            }

            if (rect.bottom > frame.span.y)
            {
               dy += rect.bottom - frame.span.y;
               frame.span.y += dy;
            }
         }
      }

      virtual void SetBorder(ID_WIDGET id, EWidgetState state, Vec2i& dxdy, RGBAb c1, RGBAb c2)
      {
         auto i = currentPanel->find(id);
         if (i != currentPanel->end())
         {
            BaseWidget::BackgroundColours &bk = state == EWidgetState_HasFocus ? i->second->hilightBk : i->second->normalBk;
            bk.border1 = c1;
            bk.border2 = c2;
            bk.borderWidths = dxdy;
         }
         else
         {
            Throw(0, L"Could not find widget %d in %s", id, currentPanelName.c_str());
         }
      }

      virtual void SetBackcolours(ID_WIDGET id, EWidgetState state, RGBAb b1, RGBAb b2)
      {
         auto i = currentPanel->find(id);
         if (i != currentPanel->end())
         {
            BaseWidget::BackgroundColours &bk = state == EWidgetState_HasFocus ? i->second->hilightBk : i->second->normalBk;
            bk.bk1 = b1;
            bk.bk2 = b2;
         }
         else
         {
            Throw(0, L"Could not find widget %d in %s", id, currentPanelName.c_str());
         }
      }

      virtual void SetButtonPulse(ID_WIDGET id, ID_UI_EVENT_TYPE commandId, boolean32 fireWhenDown, boolean32 fireWhenUp)
      {
         auto& i = currentPanel->find(id);
         if (i != currentPanel->end())
         {
            auto& w = *i->second;
            if (w.Type() != EWidgetType_Button)
            {
               Throw(0, L"Cannot set button pulse, widget %d is not a button", id);
            }

            WidgetButton& button = reinterpret_cast<WidgetButton&>(w);

            button.commandId = commandId;
            button.fireWhenDown = fireWhenDown;
            button.fireWhenUp = fireWhenUp;
         }
         else
         {
            Throw(0, L"Could not find widget %d in %s", id, currentPanelName.c_str());
         }
      }

      virtual void SetFont(ID_WIDGET id, EWidgetState state, int32 fontId, RGBAb fontColour)
      {
         auto& i = currentPanel->find(id);
         if (i != currentPanel->end())
         {
            auto& w = *i->second;
            if (w.Type() != EWidgetType_Button)
            {
               Throw(0, L"Cannot set font, widget %d does not have a font", id);
            }

            WidgetButton& button = reinterpret_cast<WidgetButton&>(w);
            WidgetButton::FontDef& font = state == EWidgetState_HasFocus ? button.hiFont : button.normalFont;
            font.colour = fontColour;
            font.fontIndex = fontId;
         }
         else
         {
            Throw(0, L"Could not find widget %d in %s", id, currentPanelName.c_str());
         }
      }

      virtual void Move(ID_WIDGET id, Vec2i& positionInContainer)
      {
         auto& i = currentPanel->find(id);
         if (i != currentPanel->end())
         {
            i->second->posInContainer = positionInContainer;
         }
         else
         {
            Throw(0, L"Could not find widget %d in %s", id, currentPanelName.c_str());
         }
      }

      void RenderError(IGuiRenderContext& gc, const GuiRect& rect, cstr format, ...)
      {
         va_list args;
         va_start(args, format);

         rchar errmsg[256];
         SafeVFormat(errmsg, _TRUNCATE, format, args);

         Graphics::DrawRectangle(gc, rect, RGBAb(64, 64, 64, 128), RGBAb(64, 64, 64, 128));
         Graphics::RenderCentredText(gc, errmsg, RGBAb(255, 255, 255), 2, Centre(rect));
      }

      void RecurseAndRenderHierarchy(IGuiRenderContext& gc, TWidgetSet& panel, const BaseWidget& widget, const GuiRect& absOutput)
      {
         auto topLeft = TopLeft(absOutput);
         widget.Render(gc, topLeft);

         if (widget.Type() == EWidgetType_Frame)
         {
            auto& frame = reinterpret_cast<const WidgetFrame&>(widget);

            for (auto id : frame.children)
            {
               auto& child = *panel[id];
               Vec2i pos = topLeft + child.posInContainer;
               GuiRect childRect{ pos.x, pos.y, pos.x + child.span.x, pos.y + child.span.y };
               RecurseAndRenderHierarchy(gc, panel, child, childRect);
            }
         }
      }

      Vec2i lastScreenSpan;
      virtual void Resize(Vec2i span)
      {
         lastScreenSpan = span;
      }

      virtual void RenderHierarchy(IGuiRenderContext& gc, cstr panelName)
      {
         GuiMetrics metrics;
         gc.Renderer().GetGuiMetrics(metrics);

         lastScreenSpan = metrics.screenSpan;

         GuiRect baseFrameRect{ 0,0, metrics.screenSpan.x, metrics.screenSpan.y };
         GuiRect errorRect = Expand(baseFrameRect, -10);

         auto p = panelSet.find(panelName);
         if (p == panelSet.end())
         {     
            RenderError(gc, errorRect, L"Error: cannot find pane '%s'", panelName);
            return;
         }

         auto& panel = *p->second;
         
         auto itWidget = panel.find(ID_WIDGET{ 1 });
         if (itWidget == panel.end())
         {
            RenderError(gc, errorRect, L"Error: expecting widget 1 to exist in '%s'", panelName);
            return;
         }

         itWidget->second->span = metrics.screenSpan;
         itWidget->second->posInContainer = { 0,0 };

         RecurseAndRenderHierarchy(gc, panel, *itWidget->second, baseFrameRect);
      }
   };
}

namespace Dystopia
{
   namespace UI
   {
      IUIBuilderSupervisor* CreateScriptableUIBuilder()
      {
         return new GuiBuilder();
      }
   }
}
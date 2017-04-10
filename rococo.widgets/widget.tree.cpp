#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.widgets.h>

#include <vector>
#include <algorithm>

using namespace Rococo;
using namespace Rococo::Widgets;
using namespace Rococo::Events;

namespace
{
   struct UITargetBind
   {
      Widgets::IUITarget* target;
      int32 zOrder;
   };

   class WindowTree : public IWindowTree
   {
      std::vector<UITargetBind> targets;
      GuiRect rect;

      IUITarget* GetTargetAtPoint(Vec2i p)
      {
         for (auto& t : targets)
         {
            GuiRect rect;
            t.target->GetRect(rect);

            if (IsPointInRect(p, rect))
            {
               return t.target;
            }
         }

         return nullptr;
      }
   public:
      void AddTarget(IUITarget* target, int32 zOrder) override
      {
         targets.push_back({ target,zOrder });

         std::sort(targets.begin(), targets.end(),
            [](const UITargetBind& a, const UITargetBind& b)
         {
            return b.zOrder < a.zOrder;
         }
         );
      }

      void Clear() override
      {
         targets.clear();
      }

      void GetRect(GuiRect& rect) const override
      {
         rect = this->rect;
      }

      void SetRect(const GuiRect& rect) override
      {
         this->rect = rect;
      }

      void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
      {
         IUITarget* t = GetTargetAtPoint(cursorPos);
         if (t) t->OnMouseMove(cursorPos, delta, dWheel);
      }

      void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
      {
         IUITarget* t = GetTargetAtPoint(cursorPos);
         if (t) t->OnMouseLClick(cursorPos, clickedDown);
      }

      void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
      {
         IUITarget* t = GetTargetAtPoint(cursorPos);
         if (t) t->OnMouseRClick(cursorPos, clickedDown);
      }

      void Free() override
      {
         delete this;
      }
   };
} // anon

namespace Rococo
{
   namespace Widgets
   {
      IWindowTree* CreateWindowTree()
      {
         return new WindowTree();
      }

      void RouteEventToUI(Event& ev, Vec2i cursorPos, Widgets::IUITarget& ui)
      {
         if (ev == Input::OnMouseMoveRelative)
         {
            auto& mc = As<Input::OnMouseMoveRelativeEvent>(ev);
            ui.OnMouseMove(cursorPos, { mc.dx, mc.dy }, mc.dz);
         }
         else if (ev == Input::OnMouseChanged)
         {
            auto& mc = As <Input::OnMouseChangedEvent>(ev);
            if ((mc.flags & Input::MouseFlags_LDown))
            {
               ui.OnMouseLClick(cursorPos, true);
            }
            else if ((mc.flags & Input::MouseFlags_LUp))
            {
               ui.OnMouseLClick(cursorPos, false);
            }
            else if ((mc.flags & Input::MouseFlags_RDown))
            {
               ui.OnMouseRClick(cursorPos, true);
            }
            else if ((mc.flags & Events::Input::MouseFlags_RUp))
            {
               ui.OnMouseRClick(cursorPos, false);
            }
         }
      }
   }
}

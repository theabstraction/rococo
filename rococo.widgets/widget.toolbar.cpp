#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.widgets.h>
#include <rococo.strings.h>
#include <rococo.textures.h>
#include <vector>

using namespace Rococo; 
using namespace Rococo::Widgets;
using namespace Rococo::Events;

namespace
{
   struct Button
   {
      EventId id;
      wchar_t name[32];
      wchar_t resource[128];
      Textures::BitmapLocation bitmap;
      GuiRect renderLocation;

      Button(EventId _id, const wchar_t* name, const wchar_t* resource) : id(_id)
      {
         SafeCopy(this->name, name, _TRUNCATE);
         SafeCopy(this->resource, resource, _TRUNCATE);
      }
   };

   class Toolbar : public IToolbar
   {
      IPublisher& publisher; 
      IRenderer& renderer;
      Vec2i pos{ 0, 0 };
      std::vector<Button*> buttons;
      GuiRect rect;
      Button* focus{ nullptr };
   public:
      Toolbar(IPublisher& _publisher, IRenderer& _renderer):
         publisher(_publisher), renderer(_renderer)
      {
      }

      ~Toolbar()
      {
         for (auto* b : buttons)
         {
            delete b;
         }
      }

      void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
      {
         for (auto* b : buttons)
         {
            if (IsPointInRect(cursorPos, b->renderLocation))
            {
               if (focus != b)
               {
                  focus = b;
                  SetStatus(focus->name, publisher);
               }
               return;
            }
         }

         if (focus)
         {
            focus = nullptr;
            SetStatus(L"", publisher);
         }
      }

      void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
      {
         for (auto* b : buttons)
         {
            if (IsPointInRect(cursorPos, b->renderLocation))
            {      
               publisher.Publish(Event(b->id));
               return;
            }
         }
      }

      void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
      {

      }

      void GetRect(GuiRect& rect) const  override
      {
         rect = this->rect;
      }


      virtual void AddButton(const wchar_t* name, EventId id, const wchar_t* buttonTextureResource)
      {
         auto* b = new Button(id, name, buttonTextureResource);
         buttons.push_back(b);
      }

      GuiRect TileHorizontally(int buttonBorder)
      {
         int32 height = -1;

         int x = pos.x;

         for (auto* b : buttons)
         {
            Vec2i span = Span(b->bitmap.txUV);
            if (height < 0)
            {
               height = span.y;
            }
            else
            {
               if (height != span.y)
               {
                  Throw(0, L"Cannot tile toolbar horizontally. %s was not %d pixels high", b->name, span.y);
               }
            }

            b->renderLocation.left = x + buttonBorder;
            b->renderLocation.right = x + span.x + buttonBorder;
            b->renderLocation.top = pos.y + buttonBorder;
            b->renderLocation.bottom = pos.y + span.y + buttonBorder;

            x += span.x + buttonBorder;
         }

         return GuiRect(pos.x, pos.y, x + buttonBorder, pos.y + height + 2 * buttonBorder);
      }

      GuiRect TileVertically(int buttonBorder)
      {
         int32 width = -1;

         int y = pos.y;

         for (auto* b : buttons)
         {
            Vec2i span = Span(b->bitmap.txUV);
            if (width < 0)
            {
               width = span.x;
            }
            else
            {
               if (width != span.x)
               {
                  Throw(0, L"Cannot tile toolbar vertically. %s was not %d pixels wide", b->name, span.x);
               }
            }

            b->renderLocation.left = pos.x;
            b->renderLocation.right = pos.x + span.x;
            b->renderLocation.top = y;
            b->renderLocation.bottom = y + span.y;

            y += span.y + buttonBorder;
         }

         return GuiRect(pos.x, pos.y, pos.x + width, y);
      }

      virtual GuiRect Render(IGuiRenderContext& rc, bool horizontal, int buttonBorder, RGBAb backColour, RGBAb highlightBorder)
      {
         for (auto* b : buttons)
         {
            if (!renderer.SpriteBuilder().TryGetBitmapLocation(b->resource, b->bitmap))
            {
               Throw(0, L"Cannot add button '%s'\n - failed to load texture resource '%s'", b->name, b->resource);
            }
         }


         if (horizontal)
         {
            rect = TileHorizontally(buttonBorder);
         }
         else
         {
            rect = TileVertically(buttonBorder);
         }

         if (backColour.alpha != 0)
         {
            Graphics::DrawRectangle(rc, rect, backColour, backColour);
         }

         GuiMetrics metrics;
         rc.Renderer().GetGuiMetrics(metrics);

         for (auto* b : buttons)
         {
            if (highlightBorder.alpha != 0 && IsPointInRect(metrics.cursorPosition, b->renderLocation))
            {
               Graphics::DrawBorderAround(rc, b->renderLocation, { 1,1 }, highlightBorder, highlightBorder);
            }
            Graphics::DrawSprite(TopLeft(b->renderLocation), b->bitmap, rc, true);
         }

         return rect;
      }

      virtual void SetPosition(Vec2i pos)
      {
         this->pos = pos;
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Widgets
   {
      IToolbar* CreateToolbar(IPublisher& publisher, IRenderer& renderer)
      {
         return new Toolbar(publisher, renderer);
      }
   }
}
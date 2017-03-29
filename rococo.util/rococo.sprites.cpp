#include "rococo.api.h"
#include "rococo.textures.h"
#include "rococo.renderer.h"

using namespace Rococo;
using namespace Textures;

namespace Rococo
{
   namespace Graphics
   {
      void DrawSprite(const Vec2i& topLeftBitmap, const BitmapLocation& location, IGuiRenderContext& gc, bool alphaBlend)
      {
         Vec2 topLeft, bottomRight;
         topLeft.x = (float)topLeftBitmap.x;
         topLeft.y = (float)topLeftBitmap.y;
         bottomRight.x = (float) (topLeftBitmap.x + location.txUV.right - location.txUV.left);
         bottomRight.y = (float) (topLeftBitmap.y + location.txUV.bottom - location.txUV.top);

         // [0 1]
         // [2 3]
         GuiVertex quad[4] = { 0 };
         quad[0].x = quad[2].x = topLeft.x;
         quad[1].x = quad[3].x = bottomRight.x;
         quad[0].y = quad[1].y = topLeft.y;
         quad[2].y = quad[3].y = bottomRight.y;

         quad[0].u = quad[2].u = (float) location.txUV.left;
         quad[1].u = quad[3].u = (float) location.txUV.right;
         quad[0].v = quad[1].v = (float) location.txUV.top;
         quad[2].v = quad[3].v = (float) location.txUV.bottom;

         quad[0].textureIndex = quad[1].textureIndex = quad[2].textureIndex = quad[3].textureIndex = (float)location.textureIndex;

         gc.AddSpriteTriangle(alphaBlend, quad);
         gc.AddSpriteTriangle(alphaBlend, quad+1);
      }
   } // Sprites
} // Rococo
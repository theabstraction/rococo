#include "hv.h"

namespace HV
{
   namespace GraphicsEx
   {
      void DrawPointer(IGuiRenderContext& grc, Vec2i pos, Degrees heading, RGBAb shadowColour, RGBAb bodyColour)
      {
         Vec3 tri[6] =
         {
            { 18,  0,  0 },
            { -18, -10, 0 },
            { -18, +10, 0 },
            { 16,   0, 0 },
            { -16, -8,  0 },
            { -16, +8,  0 }
         };

         Matrix4x4 T = Matrix4x4::Translate({ (float)pos.x, (float)pos.y, 0 });

         float theta = heading - Degrees{ 90.0f };

         Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ theta });
         Matrix4x4 R = T * Rz;

         Vec3 triP[6];
         TransformPositions(tri, 6, R, triP);

         GuiVertex shadow[3];
		 for (int i = 0; i < 3; ++i)
		 {
			 shadow[i] = GuiVertex{ {0, 0}, {{0,0}, 0}, { 1, 0, 0, 0 }, shadowColour };
			 shadow[i].pos = { triP[i].x, triP[i].y };
		 }

         grc.AddTriangle(shadow);

         GuiVertex light[3];
         for (int i = 0; i < 3; ++i)
         {
            light[i] = GuiVertex{ { 0, 0 },{ { 0,0 }, 0 },{ 1, 0, 0, 0 }, bodyColour};
            light[i].pos = { triP[i + 3].x, triP[i + 3].y };
         }

         grc.AddTriangle(light);
      }

   }
}
#include "hv.h"
#include "hv.events.h"
#include <rococo.strings.h>
#include <vector>
#include <deque>
#include <rococo.maths.h>

namespace
{
   using namespace HV;
   using namespace Rococo;

   struct GuiTriangle
   {
      GuiVertex A;
      GuiVertex B;
      GuiVertex C;
   };

   void SetGroundVertex(GuiVertex& v, Vec2 p, float uvScale, Vec2 uvOffset)
   {
      v.x = p.x;
      v.y = p.y;
      v.u = uvScale * p.x + uvOffset.x;
      v.y = uvScale * p.y + uvOffset.y;
      v.colour = RGBAb(0, 0, 0, 255);
      v.fontBlend = 0;
      v.saturation = 1.0f;
      v.textureIndex = 1;
   }

   void EarClipTesselator(std::vector<GuiTriangle>& output, std::deque<Vec2> mesh, float uvScale, Vec2 uvOffset)
   {
      if (mesh.size() < 3)
      {
         Throw(0, L"Cannot tesselate vector array with fewer than 3 elements");
      }

      while (!mesh.empty())
      {
         auto i = mesh.begin();
         for (; i != mesh.end(); ++i)
         {
            auto j = i;
            Vec2 a = *j++; if (j == mesh.end()) j = mesh.begin();
            Vec2 b = *j++; if (j == mesh.end()) j = mesh.begin();
            Vec2 c = *j++;

            Vec2 ab = b - a;
            Vec2 bc = c - a;

            float k = Cross(ab, bc);
            if (k < 0) // clockwise, we have an ear
            {
               GuiTriangle t;
               SetGroundVertex(t.A, a, uvScale, uvOffset);
               SetGroundVertex(t.B, b, uvScale, uvOffset);
               SetGroundVertex(t.C, c, uvScale, uvOffset);
               output.push_back(t);

               j = i;
               j++;
               if (j == mesh.end()) j = mesh.begin();
               mesh.erase(j);

               if (mesh.size() < 4)
               {
                  mesh.clear();
                  return;
               }

               i = mesh.begin();
               break;
            }
         }

         if (i == mesh.end())
         {
            Throw(0, L"Could not tesselate mesh. Iterated through all mesh and did not find ears");
         }
      }
   }

   class Sector
   {
      std::vector<ObjectVertex> wallVertices;
      std::vector<ObjectVertex> floorVertices;
      std::vector<ObjectVertex> ceilingVertices;
      float uvScale{ 1.0f };

   public:
      std::vector<GuiTriangle> guiView;

      Sector(const Vec2* positionArray, size_t nVertices, float z0, float z1)
      {
         if (nVertices < 4 || positionArray[0] != positionArray[nVertices - 1])
         {
            Throw(0, L"Cannot construct Sector. Bad position array");
         }

         Vec3 p0 = { positionArray[0].x, positionArray[0].y, z0 };
         Vec3 p1 = { positionArray[0].x, positionArray[0].y, z1 };

         Vec3 q0 = { positionArray[1].x, positionArray[1].y, z0 };
         Vec3 q1 = { positionArray[1].x, positionArray[1].y, z1 };

         Vec3 qp = q0 - p0;
         Vec3 ph = (p0 + p1) * 0.5f;
         Vec3 qh = (q0 + q1) * 0.5f;
         Vec3 centre = (ph + qh) * 0.5f;

         Vec3 normal = Cross(qp, { 0, 0, 1 });

         int32 countDirection;
         int32 startIndex;
         int32 endIndex;

         IntersectCounts counts = CountLineIntersects({ centre.x, centre.y }, { normal.x, normal.y }, positionArray + 1, nVertices - 1);

         if ((counts.forwardCount % 2) == 1)
         {
            if ((counts.backwardCount % 2) == 1)
            {
               Throw(0, L"Error computing mesh from sector perimeter");
            }

            countDirection = 1;
            startIndex = 0;
            endIndex = (int32) nVertices;
         }
         else
         {
            countDirection = -1;
            startIndex = (int32) nVertices-1;
            endIndex = -1;
         }

         std::deque<Vec2> perimeter;
         for (int32 i = startIndex; i != endIndex; i += countDirection)
         {
            perimeter.push_back(positionArray[i]);
         }

         EarClipTesselator(guiView, perimeter, uvScale, { 0,0 });
      }
   };

   class Sectors
   {
      const Metres defaultFloorLevel{ 200.0f };
      const Metres defaultRoomHeight{ 3.0f };
   public:
      std::vector<Sector*> sectors;

      void AddSector(const Vec2* positionArray, size_t nVertices)
      {
         sectors.push_back(new Sector(positionArray, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight));
      }
   };

   class WorldMap
   {
   private:
      int32 gridlinePixelWidth{ 8 };
      Metres gridlineMetricWidth{ 2.0f };
      Vec2 gridCentre{ 0, 0 }; // Always uses integral co-ordinates
      bool isGrabbed{ false };
      Vec2i grabPosition;
      Vec2 grabbedCentre;
      Vec2i pixelOffset{ 0, 0 };
      Vec2i grabbedOffset{ 0,0 };
      GuiMetrics metrics { 0 };    
   public:
      Sectors sectors;

      Vec2 GetWorldPosition(Vec2i screenPosition)
      {
         Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };
         Vec2i cursorOffset = { screenPosition.x - centre.x , centre.y - screenPosition.y };
         Vec2i pixelDelta = cursorOffset - pixelOffset;

         Vec2 centreOffset =  Vec2{ pixelDelta.x / (float) gridlinePixelWidth, pixelDelta.y / (float) gridlinePixelWidth };
         return gridCentre + centreOffset * gridlineMetricWidth;
      }

      Vec2i GetScreenPosition(Vec2 worldPosition)
      {
         Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };

         Vec2 pixelDelta = (worldPosition - gridCentre) * (float) gridlinePixelWidth / gridlineMetricWidth;
         return{ ((int32)(pixelDelta.x)) + centre.x + pixelOffset.x, - (  ((int32)(pixelDelta.y)) - centre.y + pixelOffset.y )};
      }

      void DrawGridLines(IGuiRenderContext& grc)
      {
         Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
         int32 nGridLinesX = (metrics.screenSpan.x / gridlinePixelWidth);
         int32 nGridLinesY = (metrics.screenSpan.y / gridlinePixelWidth);
         int32 farLeft = centre.x - gridlinePixelWidth * (nGridLinesX >> 1) - gridlinePixelWidth;
         int32 farRight = centre.x + gridlinePixelWidth * (nGridLinesX >> 1) + gridlinePixelWidth;
         int32 farUp = centre.y - gridlinePixelWidth *  (nGridLinesY >> 1) - gridlinePixelWidth;
         int32 farDown = centre.y + gridlinePixelWidth *  (nGridLinesY >> 1) + gridlinePixelWidth;

         for (int j = farUp; j <= farDown; j += gridlinePixelWidth)
         {
            auto y = j - pixelOffset.y;
            Rococo::Graphics::DrawLine(grc, 1, { 0, y }, { metrics.screenSpan.x, y }, RGBAb(192, 192, 192, 255));
         }

         for (int i = farLeft; i <= farRight; i += gridlinePixelWidth)
         {
            auto x = i + pixelOffset.x;
            Rococo::Graphics::DrawLine(grc, 1, { x, 0 }, { x, metrics.screenSpan.y }, RGBAb(192, 192, 192, 255));
         }
      }

      void RenderTopGui(IGuiRenderContext& grc)
      {
         Vec2 worldCursor = GetWorldPosition(metrics.cursorPosition);

         wchar_t originText[24];
         SafeFormat(originText, _TRUNCATE, L"(%4.4f,%4.4f)", worldCursor.x, worldCursor.y);

         Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
         Rococo::Graphics::DrawRectangle(grc, { centre.x - 70,0,centre.x + 70, 20 }, RGBAb(64, 64, 64, 224), RGBAb(64, 64, 64, 224));
         Rococo::Graphics::RenderCentredText(grc, originText, RGBAb(255, 255, 255), 9, { metrics.screenSpan.x >> 1, 8 });
      }

      void Render(IGuiRenderContext& grc)
      {
         grc.Renderer().GetGuiMetrics(metrics);

         if (isGrabbed)
         {
            Vec2i delta = metrics.cursorPosition - grabPosition;
            // If we drag centre to upperleft, delta.x and delta.y -ve
            // But we are moving map centre in +ve world direction and -ve y direction

            int32 nCellsXDelta = -(delta.x + grabbedOffset.x) / gridlinePixelWidth;
            int32 nCellsYDelta = (delta.y + grabbedOffset.y) / gridlinePixelWidth;

            gridCentre.x = grabbedCentre.x + gridlineMetricWidth * (float)nCellsXDelta;
            gridCentre.y = grabbedCentre.y + gridlineMetricWidth * (float)nCellsYDelta;

            pixelOffset.x = (delta.x + grabbedOffset.x) % gridlinePixelWidth;
            pixelOffset.y = (-delta.y - grabbedOffset.y) % gridlinePixelWidth;
         }

         Rococo::Graphics::DrawRectangle(grc, { 0,0,metrics.screenSpan.x, metrics.screenSpan.y }, RGBAb(0, 0, 0, 224), RGBAb(0, 0, 0, 224));

         DrawGridLines(grc);

         for (auto sector : sectors.sectors)
         {
            for (auto& triangle : sector->guiView)
            {
               GuiTriangle screenMappedTriangle = triangle;
               Vec2i p0 = GetScreenPosition(Vec2{ triangle.A.x, triangle.A.y });
               screenMappedTriangle.A.x = (float) p0.x;
               screenMappedTriangle.A.y = (float) p0.y;

               Vec2i p1 = GetScreenPosition(Vec2{ triangle.B.x, triangle.B.y });
               screenMappedTriangle.B.x = (float)p1.x;
               screenMappedTriangle.B.y = (float)p1.y;

               Vec2i p2 = GetScreenPosition(Vec2{ triangle.C.x, triangle.C.y });
               screenMappedTriangle.C.x = (float)p2.x;
               screenMappedTriangle.C.y = (float)p2.y;

               grc.AddTriangle(&screenMappedTriangle.A);
            }
         }
      }

      void ZoomIn(int32 degrees)
      {
         pixelOffset = { 0,0 };

         for (int i = 0; i < degrees; ++i)
         {
            if (gridlinePixelWidth == 256)
            {
               return;
            }
            else if (gridlinePixelWidth < 24)
            {
               gridlinePixelWidth += 2;
            }
            else
            {
               gridlinePixelWidth += 8;
            }
         }
      }

      void ZoomOut(int32 degrees)
      {
         pixelOffset = { 0,0 };

         for (int i = 0; i < degrees; ++i)
         {
            if (gridlinePixelWidth == 2)
            {
               return;
            }
            else if (gridlinePixelWidth < 24)
            {
               gridlinePixelWidth -= 2;
            }
            else
            {
               gridlinePixelWidth -= 8;
            }
         }
      }

      Vec2 SnapToGrid(Vec2 worldPosition)
      {
         float x = gridlineMetricWidth * roundf(worldPosition.x / gridlineMetricWidth);
         float y = gridlineMetricWidth * roundf(worldPosition.y / gridlineMetricWidth);
         return{ x, y };
      }

      void GrabAtCursor()
      {
         isGrabbed = true;
         grabPosition = metrics.cursorPosition;
         grabbedCentre = gridCentre;
         grabbedOffset = { pixelOffset.x, -pixelOffset.y };
      }

      void ReleaseGrab()
      {
         isGrabbed = false;
      }
   };

   ROCOCOAPI IUITarget
   {
      virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) = 0;
      virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown) = 0;
      virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown) = 0;
   };

   ROCOCOAPI IEditMode: public IUITarget
   {
      virtual void Render(IGuiRenderContext& grc) = 0; 
   };

   class EditMode_SectorBuilder: private IEditMode
   {
      bool isLineBuilding{ false };
      std::vector<Vec2> lineList;
      WorldMap& map;

      void Render(IGuiRenderContext& grc) override
      {
         GuiMetrics metrics;
         grc.Renderer().GetGuiMetrics(metrics);

         for (int i = 1; i < lineList.size(); ++i)
         {
            Vec2i start = map.GetScreenPosition(lineList[i - 1]);
            Vec2i end = map.GetScreenPosition(lineList[i]);
            Rococo::Graphics::DrawLine(grc, 2, start, end, RGBAb(255, 255, 0));
         }

         if (isLineBuilding && !lineList.empty())
         {
            Vec2i start = map.GetScreenPosition(lineList[lineList.size() - 1]);
            Rococo::Graphics::DrawLine(grc, 2, start, metrics.cursorPosition, RGBAb(255, 255, 0));
         }
      }

      void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
      {
         if (dWheel < 0)
         {
            map.ZoomIn(-dWheel);
         }
         else if (dWheel > 0)
         {
            map.ZoomOut(dWheel);
         }
      }

      void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
      {
         if (clickedDown)
         {
            map.GrabAtCursor();
            isLineBuilding = false;
         }
         else
         {
            map.ReleaseGrab();
         }
      }

      void DestroyCrossedLines()
      {
         if (lineList.size() > 2)
         {
            Vec2 lastVertex = lineList[lineList.size()-1];
            Vec2 startOfLastLine = lineList[lineList.size() - 2];

            for (size_t i = 1; i < lineList.size() - 2; ++i)
            {
               Vec2 a = lineList[i - 1];
               Vec2 b = lineList[i];

               float t, u;
               if (GetLineIntersect(a, b, startOfLastLine, lastVertex, t, u))
               {
                  if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
                  {
                     lineList.clear();
                     return;
                  }
               }
               else if (DoParallelLinesIntersect(a, b, startOfLastLine, lastVertex))
               {
                  lineList.clear();
                  return;
               }
            }
         }
      }

      void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
      {
         if (clickedDown)
         {
            Vec2 wp = map.SnapToGrid(map.GetWorldPosition(cursorPos));
            lineList.push_back(wp);
            isLineBuilding = true;

            if (lineList.size() > 1)
            {
               if (lineList[0] == wp)
               {
                  // Sector closed
                  isLineBuilding = false;

                  if (lineList.size() == 2)
                  {
                     // Forbid naked line segments
                     lineList.clear();
                  }

                  if (!lineList.empty())
                  {
                     map.sectors.AddSector(&lineList[0], lineList.size());
                  }

                  return;
               }

               DestroyCrossedLines();
            }
         }
      }
   public:
      EditMode_SectorBuilder(WorldMap& _map) : map(_map) 
      {
      }
      IEditMode& Mode() { return *this; }
   };

   void RouteEventToUI(Event& ev, Vec2i cursorPos, IUITarget& ui)
   {
      if (ev == HV::Events::Input::OnMouseMoveRelative)
      {
         auto& mc = As<HV::Events::Input::OnMouseMoveRelativeEvent>(ev);
         ui.OnMouseMove(cursorPos, { mc.dx, mc.dy }, mc.dz);
      }
      else if (ev == HV::Events::Input::OnMouseChanged)
      {
         auto& mc = As <HV::Events::Input::OnMouseChangedEvent>(ev);
         if ((mc.flags & HV::Events::Input::MouseFlags_LDown))
         {
            ui.OnMouseLClick(cursorPos, true);
         }
         else if ((mc.flags & HV::Events::Input::MouseFlags_LUp))
         {
            ui.OnMouseLClick(cursorPos, false);
         }
         else if ((mc.flags & HV::Events::Input::MouseFlags_RDown))
         {
            ui.OnMouseRClick(cursorPos, true);
         }
         else if ((mc.flags & HV::Events::Input::MouseFlags_RUp))
         {
            ui.OnMouseRClick(cursorPos, false);
         }
      }
   }

   class Editor : public IEditor, public IUIOverlay, public IObserver
   {
      IPublisher& publisher;
      WorldMap map;
      EditMode_SectorBuilder editMode_Sectors;
      IEditMode* editMode;
      GuiMetrics metrics;

      void Render(IGuiRenderContext& grc) override
      {
         grc.Renderer().GetGuiMetrics(metrics);
         map.Render(grc);

         editMode->Render(grc);

         map.RenderTopGui(grc);
      }

      IUIOverlay& Overlay() override
      {
         return *this;
      }

      void Free() override
      {
         delete this;
      }

      void OnEvent(Event& ev) override
      {
         RouteEventToUI(ev, metrics.cursorPosition, *editMode);
      }
   public:
      Editor(IPublisher& _publisher) : publisher(_publisher), editMode_Sectors(map), editMode(&editMode_Sectors.Mode())
      {
         publisher.Attach(this);
      }
   };
}

namespace HV
{
   IEditor* CreateEditor(IPublisher& publisher)
   {
      return new Editor(publisher);
   }
}
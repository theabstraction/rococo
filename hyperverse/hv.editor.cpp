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

   struct Triangle2d
   {
      Vec2 A;
      Vec2 B;
      Vec2 C;
   };

   void ValidateArray(const Vec2* positionArray, size_t nVertices)
   {
       // N.B we use modular arithmetic to connect final vertex with beginning
      // So we do not duplicate end vertex in the array with the beginning
      if (nVertices < 3 || positionArray[0] == positionArray[nVertices - 1])
      {
         Throw(0, L"IsClockwiseSequential: Cannot evaluate chirality. Bad position array");
      }

      if (nVertices > 256)
      {
         Throw(0, L"IsClockwiseSequential: Too many elements in mesh. Maximum is 256. Simplify");
      }
   }

   // If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
   template<class T>
   T GetRingElement(size_t index, const T* array, size_t capacity)
   {
      return array[index % capacity];
   }

   bool IsOdd(int32 i)
   {
      return (i % 2) == 1;
   }

   bool DoesLineIntersectSegments(Vec2 origin, Vec2 normal, const Vec2* positionArray, size_t nVertices)
   {
      IntersectCounts counts = { 0 };
      for (size_t i = 0; i < nVertices; ++i)
      {
         Vec2 q0 = GetRingElement(i, positionArray, nVertices);
         Vec2 q1 = GetRingElement(i + 1, positionArray, nVertices);

         Vec2 specimen[2];
         specimen[0] = q0;
         specimen[1] = q1;

         if (q0 != origin && q1 != origin)
         {
            auto count = CountLineIntersects(origin, normal, specimen, 2, 0.001f);
            counts.forwardCount += count.forwardCount;
            counts.backwardCount += count.backwardCount;
            counts.edgeCases += count.edgeCases;
         }
      }

      return counts.forwardCount > 0;
   }

   // Determine whether forward iteration of a perimeter array is clockwise
   bool IsClockwiseSequential(const Vec2* positionArray, size_t nVertices)
   {
      for (size_t j = 0; j < nVertices; ++j)
      {
         Vec2 p0 = GetRingElement(j, positionArray, nVertices);
         Vec2 p1 = GetRingElement(j+1, positionArray, nVertices); 
         
         Vec2 centre = (p0 + p1) * 0.5f;
         Vec2 dp = p1 - p0;
         Vec3 normal = Cross({ dp.x, dp.y, 0 }, { 0, 0, 1 });

         // Take a segment from the perimeter and generate its normal
         // Count the number of timest that normal crosses the perimeter

         IntersectCounts counts = { 0 };
         for (size_t i = j+1; i < nVertices+j; ++i)
         {
            Vec2 q0 = GetRingElement(i, positionArray, nVertices);
            Vec2 q1 = GetRingElement(i + 1, positionArray, nVertices);
           
            Vec2 specimen[2];
            specimen[0] = q0;
            specimen[1] = q1;
            auto count = CountLineIntersects(centre, { normal.x, normal.y }, specimen, 2, 0.001f);
            counts.forwardCount += count.forwardCount;
            counts.backwardCount += count.backwardCount;
            counts.edgeCases += count.edgeCases;
         }

         // If its odd, then the segment is facing inside the enclosed region
         if (counts.edgeCases == 0)
         {
            if (IsOdd(counts.forwardCount))
            {
               if (IsOdd(counts.backwardCount))
               {
                  Throw(0, L"IsClockwiseSequential: Unexpected - forwardcount and backward count both odd!");
               }

               return true;
            }
            else
            {
               // Iteration needs to go backwardds to go clockwise aroun the perimeter
               if (!IsOdd(counts.backwardCount))
               {
                  Throw(0, L"IsClockwiseSequential: Unexpected - forwardcount and backward count both even!");
               }

               return false;
            }
         }
      }

      Throw(0, L"IsClockwiseSequential: Every calculation to evaluate chirality was an edge case");
      return false;
   }

   bool IsInternal(const Triangle2d& t, Vec2 p)
   {
      float Fa = Cross(t.B - t.A, p - t.B);
      float Fb = Cross(t.C - t.B, p - t.C);
      float Fc = Cross(t.A - t.C, p - t.A);

      if (Fa < 0 && Fb < 0 && Fc < 0)
      {
         return true;
      }

      return false;
   }

   size_t CountInternalPoints(const Triangle2d& t, const Vec2* points, size_t nPoints)
   {
      size_t count = 0;
      for (size_t i = 0; i < nPoints; ++i)
      {
         auto p = points[i];
         if (IsInternal(t, p))
         {
            count++;
         }
      }

      return count;
   }

   ROCOCOAPI I2dMeshBuilder
   {
      virtual void Append(const Triangle2d& t) = 0;
   };

   void EarClipTesselator(I2dMeshBuilder& tb, std::deque<Vec2>& perimeter)
   {
      if (perimeter.size() < 3)
      {
         Throw(0, L"Cannot tesselate vector array with fewer than 3 elements");
      }

      while (!perimeter.empty())
      {
         auto i = perimeter.begin();
         for (; i != perimeter.end(); ++i)
         {
            auto j = i;
            Triangle2d t;
            t.A = *j++; if (j == perimeter.end()) j = perimeter.begin();
            t.B = *j++; if (j == perimeter.end()) j = perimeter.begin();
            t.C = *j++;

            Vec2 ab = t.B - t.A;
            Vec2 bc = t.C - t.B;

            float k = Cross(ab, bc);
            if (k <= 0) // clockwise, we have an ear
            {
               // If we have an ear, we can eliminate b
               // For the case ab and bc are parallel k = 0, but since they share a point b, are part of the same line, so we can eliminate b

               Vec2* stackArray = (Vec2*)alloca(sizeof(Vec2) * perimeter.size());
               int index = 0;
               for (auto v : perimeter)
               {
                  stackArray[index++] = v;
               }

               if (!DoesLineIntersectSegments(t.A, t.C - t.A, stackArray, perimeter.size()) && CountInternalPoints(t, stackArray, perimeter.size()) == 0)
               {
                  tb.Append(t);

                  j = i;
                  j++;
                  if (j == perimeter.end()) j = perimeter.begin();
                  perimeter.erase(j);

                  if (perimeter.size() < 3)
                  {
                     perimeter.clear();
                     return;
                  }

                  i = perimeter.begin();
                  break;
               }
            }
            else
            {
               // Concave, not an ear
            }
         }

         if (i == perimeter.end())
         {
            Throw(0, L"Could not tesselate mesh. Iterated through all mesh and did not find ears");
         }
      }
   }

   struct ObjectVertexBuffer
   {
      const HV::Graphics::Vertex* v;
      const size_t VertexCount;
   };

   uint32 nextSectorId = 1;

   class Sector: private I2dMeshBuilder
   {
      std::vector<HV::Graphics::Vertex> wallVertices;
      std::vector<HV::Graphics::Vertex> floorVertices;
      std::vector<HV::Graphics::Vertex> ceilingVertices;
      float uvScale{ 0.2f };
      Vec2 uvOffset{ 0,0 };
      float z0;
      float z1;
      wchar_t name[32];
      uint32 id;
      ID_ENTITY floorId;
      ID_ENTITY ceilingId;
      ID_ENTITY wallId;

      virtual void Append(const Triangle2d& t)
      {       
         Vec3 up{ 0, 0, 1 };
         HV::Graphics::Vertex a, b, c;
         a.position = { t.A.x, t.A.y, z0 };
         b.position = { t.B.x, t.B.y, z0 };
         c.position = { t.C.x, t.C.y, z0 };
         a.normal = b.normal = c.normal = up;
         a.emissiveColour = b.emissiveColour = c.emissiveColour = RGBAb(0, 0, 0, 0);
         a.diffuseColour = b.diffuseColour = c.diffuseColour = RGBAb(255, 255, 255, 0);
         a.uv = uvScale * Vec2{ t.A.x,t.A.y } + uvOffset;
         b.uv = uvScale * Vec2{ t.B.x,t.B.y } + uvOffset;
         c.uv = uvScale * Vec2{ t.C.x,t.C.y } + uvOffset;
         floorVertices.push_back(a);
         floorVertices.push_back(b);
         floorVertices.push_back(c);
         a.normal = b.normal = c.normal = -up;
         a.position.z = b.position.z = c.position.z = z1;
         ceilingVertices.push_back(b);
         ceilingVertices.push_back(a);
         ceilingVertices.push_back(c);
      }

      void BuildWalls(const Vec2* positionArray, size_t nVertices, bool forwardIsClockwise, HV::Entities::IInstancesSupervisor& instances)
      {
         float u = 0;

         for (size_t i = 0; i != nVertices; i++)
         {
            Vec2 p = GetRingElement(i, positionArray, nVertices);
            Vec2 q = GetRingElement(i + 1, positionArray, nVertices);

            if (!forwardIsClockwise)
            {
               std::swap(p, q);
            }

            Vec3 up{ 0, 0, 1 };
            Vec3 P0 = { p.x, p.y, z0 };
            Vec3 Q0 = { q.x, q.y, z0 };
            Vec3 P1 = { p.x, p.y, z1 };
            Vec3 Q1 = { q.x, q.y, z1 };

            Vec3 delta = Q0 - P0;

            float segmentLength = round(Length(delta));

            Vec3 normal = Cross(delta, up);

            HV::Graphics::Vertex PV0, PV1, QV0, QV1;

            PV0.position = P0;
            PV1.position = P1;
            QV0.position = Q0;
            QV1.position = Q1;

            PV0.normal = PV1.normal = QV0.normal = QV1.normal = normal;
            PV0.emissiveColour = PV1.emissiveColour = QV0.emissiveColour = QV1.emissiveColour = RGBAb(0, 0, 0, 0);
            PV0.diffuseColour = PV1.diffuseColour = QV0.diffuseColour = QV1.diffuseColour = RGBAb(255, 255, 255, 0);

            PV0.uv.y = QV0.uv.y = uvScale * z0;
            PV1.uv.y = QV1.uv.y = uvScale * z1;
            PV0.uv.x = PV1.uv.x = uvScale * u;
            QV0.uv.x = QV1.uv.x = uvScale * (u + segmentLength);

            u += segmentLength;

            wallVertices.push_back(PV0);
            wallVertices.push_back(PV1);
            wallVertices.push_back(QV0);
            wallVertices.push_back(PV1);
            wallVertices.push_back(QV1);
            wallVertices.push_back(QV0);
         }

         SafeFormat(name, _TRUNCATE, L"sector.walls.%u", id);
         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (size_t i = 0; i < wallVertices.size();)
         {
            const HV::Graphics::Vertex& a = wallVertices[i++];
            const HV::Graphics::Vertex& b = wallVertices[i++];
            const HV::Graphics::Vertex& c = wallVertices[i++];
            mb.AddTriangle(a, b, c);
         }

         mb.End();

         wallId = instances.AddBody(to_fstring(name), L"!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
      }
   public:
      Sector(): id(nextSectorId++)
      {

      }

      ObjectVertexBuffer FloorVertices() const
      {
         return{ &floorVertices[0], floorVertices.size() };
      }

      void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1, HV::Entities::IInstancesSupervisor& instances)
      {
         this->z0 = z0;
         this->z1 = z1;

         ValidateArray(positionArray, nVertices);

         std::deque<Vec2> perimeter;

         bool forwardIsClockwise;
         if (IsClockwiseSequential(positionArray, nVertices))
         {
            forwardIsClockwise = true;
            for (size_t i = 0; i != nVertices; i++)
            {
               perimeter.push_back(positionArray[i]);
            }
         }
         else
         {
            forwardIsClockwise = false;
            for (int32 i = (int32)nVertices - 1; i >= 0; i--)
            {
               perimeter.push_back(positionArray[i]);
            }
         }

         BuildWalls(positionArray, nVertices, forwardIsClockwise, instances);

         EarClipTesselator(*this, perimeter);

         SafeFormat(name, _TRUNCATE, L"sector.floor.%u", id);

         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (size_t i = 0; i < floorVertices.size();)
         {
            const HV::Graphics::Vertex& a = floorVertices[i++];
            const HV::Graphics::Vertex& b = floorVertices[i++];
            const HV::Graphics::Vertex& c = floorVertices[i++];
            mb.AddTriangle(a, b, c);
         }

         mb.End();

         floorId = instances.AddBody(to_fstring(name), L"!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());

         SafeFormat(name, _TRUNCATE, L"sector.ceiling.%u", id);

         mb.Begin(to_fstring(name));

         for (size_t i = 0; i < ceilingVertices.size();)
         {
            const HV::Graphics::Vertex& a = ceilingVertices[i++];
            const HV::Graphics::Vertex& b = ceilingVertices[i++];
            const HV::Graphics::Vertex& c = ceilingVertices[i++];
            mb.AddTriangle(a, b, c);
         }

         mb.End();

         ceilingId = instances.AddBody(to_fstring(name), L"!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
      }
   };

   class Sectors
   {
      HV::Entities::IInstancesSupervisor&  instances;
      const Metres defaultFloorLevel{ 0.0f };
      const Metres defaultRoomHeight{ 4.0f };
   public:
      Sectors(HV::Entities::IInstancesSupervisor& _instances) : instances(_instances) {}
      std::vector<Sector*> sectors;

      void AddSector(const Vec2* positionArray, size_t nVertices)
      {
         auto* s = new Sector();
         try
         {
            s->Build(positionArray, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight, instances);
            sectors.push_back(s);
         }
         catch (IException& ex)
         {
            delete s;
            ShowErrorBox(Windows::NoParent(), ex, L"Algorithmic error creating sector. Try something simpler");

#ifdef _DEBUG
            if (QueryYesNo(Windows::NoParent(), L"Try again?"))
            {
               TripDebugger();
               OS::PrintDebug("\n\n\n // Troublesome perimeter: \n");
               OS::PrintDebug("const Vec2 perimeter[%d] = { ", nVertices);
               for (const Vec2* p = positionArray; p < positionArray + nVertices; p++)
               {
                  OS::PrintDebug("{%f,%f},", p->x, p->y);
               }
               OS::PrintDebug("};\n\n\n");

               AddSector(positionArray, nVertices);
            }
#endif
         }
      }
   };

   class WorldMap
   {
   private:
      HV::Entities::IInstancesSupervisor& instances;
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

      WorldMap(HV::Entities::IInstancesSupervisor& _instances) : instances(_instances), sectors(_instances)
      {
         
      }

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

         Vec2i centreOffseti = GetScreenPosition(Vec2{0, 0});
         Vec2 centreOffset{ (float)centreOffseti.x, (float)centreOffseti.y };

         const float scale = gridlinePixelWidth / gridlineMetricWidth;

         for (auto sector : sectors.sectors)
         {
            ObjectVertexBuffer vertices = sector->FloorVertices();
            for(size_t i = 0; i < vertices.VertexCount; i += 3 )
            {
               GuiVertex v[3];
               for (int j = 0; j < 3; ++j)
               {
                  Vec2 worldPos{ vertices.v[i + j].position.x, vertices.v[i + j].position.y };
                  auto pos = GetScreenPosition(worldPos);
                  v[j].x = (float) pos.x;
                  v[j].y = (float) pos.y;
                  v[j].u = vertices.v[i + j].uv.x;
                  v[j].v = vertices.v[i + j].uv.y;
                  v[j].colour = RGBAb(192, 192, 192, 192);
                  v[j].fontBlend = 0;
                  v[j].saturation = 1;
                  v[j].textureIndex = 1;
               }

               grc.AddTriangle(v);
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
            isLineBuilding = true;

            if (!lineList.empty() && lineList[0] == wp)
            {
               // Sector closed
               isLineBuilding = false;

               if (lineList.size() >= 3)
               {
                  map.sectors.AddSector(&lineList[0], lineList.size());      
               }
                  
               lineList.clear();
               return;
            }
            else
            {
               lineList.push_back(wp);
            }
            DestroyCrossedLines();
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
      HV::Entities::IInstancesSupervisor& instances;
      WorldMap map;
      EditMode_SectorBuilder editMode_Sectors;
      IEditMode* editMode;
      GuiMetrics metrics;
      bool isActive{ false };

      void Render(IGuiRenderContext& grc) override
      {
         if (!isActive) return;

         grc.Renderer().GetGuiMetrics(metrics);
         map.Render(grc);

         editMode->Render(grc);

         map.RenderTopGui(grc);
      }

      IUIOverlay& Overlay() override
      {
         return *this;
      }

      virtual void Activate(bool isActive)
      {
         this->isActive = isActive;
      }

      void Free() override
      {
         delete this;
      }

      void OnEvent(Event& ev) override
      {
         if (isActive) RouteEventToUI(ev, metrics.cursorPosition, *editMode);
      }
   public:
      Editor(IPublisher& _publisher, HV::Entities::IInstancesSupervisor& _instances) :
         publisher(_publisher),
         instances(_instances),
         map(_instances),
         editMode_Sectors(map), 
         editMode(&editMode_Sectors.Mode())
      {
         publisher.Attach(this);
      }

      ~Editor()
      {
         publisher.Detach(this);
      }
   };
}

namespace HV
{
   IEditor* CreateEditor(IPublisher& publisher, HV::Entities::IInstancesSupervisor& instances)
   {
      return new Editor(publisher, instances);
   }
}
#include "hv.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <deque>

#include <rococo.rings.inl>

#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.variable.editor.h>

namespace
{
   using namespace Rococo;
   using namespace HV;

   void ValidateArray(const Vec2* positionArray, size_t nVertices)
   {
      // N.B we use modular arithmetic to connect final vertex with beginning
      // So we do not duplicate end vertex in the array with the beginning
      if (nVertices < 3 || positionArray[0] == positionArray[nVertices - 1])
      {
         Throw(0, "ValidateArray: Bad position array");
      }

      if (nVertices > 256)
      {
         Throw(0, "ValidateArray: Too many elements in mesh. Maximum is 256. Simplify");
      }
   }

   // If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
   template<class T>
   T GetRingElement(size_t index, const T* array, size_t capacity)
   {
      return array[index % capacity];
   }

   uint32 nextSectorId = 1;

   struct VertexTriangle
   {
      HV::Graphics::Vertex a;
      HV::Graphics::Vertex b;
      HV::Graphics::Vertex c;
   };

   class Sector : public ISector
   {   
      Entities::IInstancesSupervisor& instances;
      ISectors& co_sectors;

      // 2-D map co-ordinates of sector perimeter
      std::vector<Vec2> floorPerimeter;

      // Indices into floor perimeter for each section of solid wall
      std::vector<Segment> wallSegments;

      // Triangle list for the physics and graphics meshes
      std::vector<VertexTriangle> wallTriangles;
      std::vector<VertexTriangle> floorTriangles;
      std::vector<VertexTriangle> ceilingTriangles;
     
      float uvScale{ 0.2f };
      Vec2 uvOffset{ 0,0 };

      float z0; // Floor height
      float z1; // Ceiling height (> floor height)
      
      uint32 id;

      // Once instance for each major mesh of the sector
      ID_ENTITY floorId;
      ID_ENTITY ceilingId;
      ID_ENTITY wallId;
      
      rchar name[32];
      
      Segment GetSegment(Vec2 p, Vec2 q) override
      {
         int32 index_p = GetPerimeterIndex(p);
         int32 index_q = GetPerimeterIndex(q);

         if (index_p >= 0 && index_q >= 0)
         {
            if (((index_p + 1) % floorPerimeter.size()) == index_q)
            {
               return{ index_p, index_q };
            }
            if (((index_q + 1) % floorPerimeter.size()) == index_p)
            {
               return{ index_q, index_p };
            }
         }

         return{ -1,-1 };
      }

      void RebuildWalls()
      {
         rchar name[32];
         SafeFormat(name, _TRUNCATE, "sector.walls.%u", id);
         auto& mb = instances.MeshBuilder();
         mb.Clear();
         mb.Begin(to_fstring(name));

         for (auto& t: wallTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End();

         wallId = instances.AddBody(to_fstring(name), "!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
      }

      void RemoveWallSegment(Segment segment)
      {
         for (auto i = wallSegments.begin(); i != wallSegments.end(); ++i)
         {
            if (i->perimeterIndexStart == segment.perimeterIndexStart && i->perimeterIndexEnd == segment.perimeterIndexEnd)
            {
               wallSegments.erase(i);
               RaiseWallsFromSegments();
               break;
            }
         }  
      }

      void BuildWalls(IRing<Vec2>& perimeter)
      {
         for (size_t i = 0; i < perimeter.ElementCount(); i++)
         {
            Vec2 p = perimeter[i];
            Vec2 q = perimeter[i + 1];

            bool deleteSection = false;

            for (auto* s : co_sectors)
            {
               if (s != static_cast<ISector*>(this))
               {
                  Segment segment = s->GetSegment(q, p);
                  if (segment.perimeterIndexStart >= 0)
                  {
                     deleteSection = true;
                     s->RemoveWallSegment(segment);
                     break;
                  }
               }
            }

            if (deleteSection) continue;

            wallSegments.push_back({ (int32) i, (int32) (i + 1) % (int32)perimeter.ElementCount() });
         }

         RaiseWallsFromSegments();
      }

      void RaiseWallsFromSegments()
      {
         float u = 0;

         wallTriangles.clear();

         for(auto segment: wallSegments)
         {
            Vec2 p = floorPerimeter[segment.perimeterIndexStart];
            Vec2 q = floorPerimeter[segment.perimeterIndexEnd];

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

            VertexTriangle t0;
            t0.a = PV0;
            t0.b = PV1;
            t0.c = QV0;

            VertexTriangle t1;
            t1.a = PV1;
            t1.b = QV1;
            t1.c = QV0;


            wallTriangles.push_back(t0);
            wallTriangles.push_back(t1);
         }

         RebuildWalls();
      }
   public:
      Sector(Entities::IInstancesSupervisor& _instances, ISectors& _co_sectors) :
         instances(_instances),
         id(nextSectorId++),
         co_sectors(_co_sectors)
      {

      }

      virtual ObjectVertexBuffer FloorVertices() const
      {
         return{ &floorTriangles[0].a, 3 * floorTriangles.size() };
      }

      RGBAb GetGuiColour(float intensity) const override
      { 
         intensity = max(0.0f, intensity);
         intensity = min(1.0f, intensity);

         uint32 rgb[3] = { 0 };

         Random::RandomMT mt;
         Random::Seed(mt, id + 1);
         uint32 index = mt() % 3;
         rgb[index] = mt() % 256;

         uint32 nextIndex = index + 1 + mt() % 2;

         rgb[nextIndex % 3] = 255 - rgb[index];

         return RGBAb(
            (uint32)(rgb[0] * intensity), 
            (uint32)(rgb[1] * intensity),
            (uint32)(rgb[2] * intensity), 
            (uint32)(224.0f * intensity));
      }

      const Vec2* WallVertices(size_t& nVertices) const override
      {
         nVertices = floorPerimeter.size();
         return floorPerimeter.empty() ? nullptr : &floorPerimeter[0];
      }

      int32 GetFloorTriangleIndexContainingPoint(Vec2 p) override
      {
         for (int32 i = 0; i < floorTriangles.size(); ++i)
         {
            auto& T = floorTriangles[i];

            Triangle2d t{ Flatten(T.a.position), Flatten(T.b.position), Flatten(T.c.position) };
            if (t.IsInternalOrOnEdge(p))
            {
               return i;
            }
         }

         return -1;
      }

      int32 GetPerimeterIndex(Vec2 a) override
      {
         for (int32 i = 0; i < floorPerimeter.size(); ++i)
         {
            if (floorPerimeter[i] == a)
            {
               return i;
            }
         }

         return -1;
      }

      bool DoesLineCrossSector(Vec2 a, Vec2 b) override
      {
         size_t nVertices = floorPerimeter.size();
         for (size_t i = 0; i <= nVertices; ++i)
         {
            auto c = GetRingElement(i, &floorPerimeter[0], nVertices);
            auto d = GetRingElement(i + 1, &floorPerimeter[0], nVertices);

            float t, u;
            if (GetLineIntersect(a, b, c, d, t, u))
            {
               if (u > 0 && u < 1 && t > 0 && t < 1)
               {
                  return true;
               }
            }
         }

         return false;
      }

      void RebuildFloors()
      {
         rchar name[32];
         SafeFormat(name, _TRUNCATE, "sector.floor.%u", id);

         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (auto& t : floorTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End();

         floorId = instances.AddBody(to_fstring(name), "!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
      }

      void RebuildCeiling()
      {
         rchar name[32];
         SafeFormat(name, _TRUNCATE, "sector.ceiling.%u", id);

         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (auto& t : ceilingTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End();

         ceilingId = instances.AddBody(to_fstring(name), "!textures/walls/metal1.jpg"_fstring, Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
      }

      void Rebuild()
      {
         BuildWalls(Ring<Vec2>(&floorPerimeter[0], floorPerimeter.size()));

         size_t len = sizeof(Vec2) * floorPerimeter.size();
         Vec2* tempArray = (Vec2*)alloca(len);

         for (size_t i = 0; i != floorPerimeter.size(); i++)
         {
            tempArray[i] = floorPerimeter[i];
         }

         RingManipulator<Vec2> ring(tempArray, floorPerimeter.size());

         struct ANON: I2dMeshBuilder
         {
            float z0;
            float z1;
            float uvScale;
            Vec2 uvOffset;
            std::vector<VertexTriangle>* floorTriangles;
            std::vector<VertexTriangle>* ceilingTriangles;

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
               a.uv = uvScale * Vec2{ t.A.x,t.A.y } +uvOffset;
               b.uv = uvScale * Vec2{ t.B.x,t.B.y } +uvOffset;
               c.uv = uvScale * Vec2{ t.C.x,t.C.y } +uvOffset;
               VertexTriangle T;
               T.a = a;
               T.b = b;
               T.c = c;
               floorTriangles->push_back(T);
               a.normal = b.normal = c.normal = -up;
               a.position.z = b.position.z = c.position.z = z1;
               T.a = b;
               T.b = a;
               T.c = c;
               ceilingTriangles->push_back(T);
            }
         } builder;

         floorTriangles.clear();
         ceilingTriangles.clear();

         builder.z0 = z0;
         builder.z1 = z1;
         builder.uvOffset = uvOffset;
         builder.uvScale = uvScale;
         builder.floorTriangles = &floorTriangles;
         builder.ceilingTriangles = &ceilingTriangles;

         TesselateByEarClip(builder, ring);

         RebuildFloors();
         RebuildCeiling();
      }

      void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) override
      {
         this->z0 = z0;
         this->z1 = z1;

         ValidateArray(positionArray, nVertices);

         Ring<Vec2> ring_of_unknown_sense(positionArray, nVertices);
 
         if (IsClockwiseSequential(ring_of_unknown_sense))
         {
            for (size_t i = 0; i != nVertices; i++)
            {
               floorPerimeter.push_back(positionArray[i]);
            }
         }
         else
         {
            for (int32 i = (int32)nVertices - 1; i >= 0; i--)
            {
               floorPerimeter.push_back(positionArray[i]);
            }
         }

         Rebuild();
      }

      void Free() override
      {
         delete this;
      }

      void InvokeSectorDialog(Rococo::Windows::IWindow& parent) override
      {
         rchar title[32];
         SafeFormat(title, _TRUNCATE, "Sector %u", id);
         AutoFree<IVariableEditor> editor = CreateVariableEditor(parent, { 640, 400 }, 120, title, "Floor and Ceiling", "Edit floor and ceiling parameters");
         editor->AddIntegerEditor("Altitiude", "Altitiude - centimetres", 0, 100000, (int32)( z0 * 100.0f));
         editor->AddIntegerEditor("Height", "Height - centimetres", 250, 100000, (int32)( (z1 - z0) * 100.0f));
         if (editor->IsModalDialogChoiceYes())
         {
            int Z0 = editor->GetInteger("Altitiude");
            int Z1 = editor->GetInteger("Height");

            Z0 = min(10000, Z0);
            Z0 = max(0, Z0);

            Z1 = min(100000, Z1);
            Z1 = max(250, Z1);

            z0 = (float)Z0 / 100;
            z1 = z0 + (float)Z1 / 100;

            Rebuild();
         }
      }
   };
}

namespace HV
{
   ISector* CreateSector(Entities::IInstancesSupervisor& instances, ISectors& co_sectors)
   {
      return new Sector(instances, co_sectors);
   }
}
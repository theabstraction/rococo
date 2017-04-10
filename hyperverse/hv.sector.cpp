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

   uint32 nextSectorId = 1;

   class Sector : private I2dMeshBuilder, public ISector
   {
      std::vector<HV::Graphics::Vertex> wallVertices;
      std::vector<HV::Graphics::Vertex> floorVertices;
      std::vector<HV::Graphics::Vertex> ceilingVertices;
      std::vector<Vec2> floorPerimeter;

      Entities::IInstancesSupervisor& instances;
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
         a.uv = uvScale * Vec2{ t.A.x,t.A.y } +uvOffset;
         b.uv = uvScale * Vec2{ t.B.x,t.B.y } +uvOffset;
         c.uv = uvScale * Vec2{ t.C.x,t.C.y } +uvOffset;
         floorVertices.push_back(a);
         floorVertices.push_back(b);
         floorVertices.push_back(c);
         a.normal = b.normal = c.normal = -up;
         a.position.z = b.position.z = c.position.z = z1;
         ceilingVertices.push_back(b);
         ceilingVertices.push_back(a);
         ceilingVertices.push_back(c);
      }

      int32 GetSegment(Vec2 p, Vec2 q) override
      {
         int32 index_p = GetPerimeterIndex(p);
         int32 index_q = GetPerimeterIndex(q);

         if (index_p >= 0 && index_q >= 0)
         {
            if ((index_p + 1 % floorPerimeter.size()) == index_q)
            {
               return index_p;
            }
         }

         return -1;
      }

      void RemoveWallSegment(size_t index)
      {
         Vec2 a = floorPerimeter[index];
         Vec2 b = GetRingElement(index + 1, &floorPerimeter[0], floorPerimeter.size());

         for (size_t i = 0; i < wallVertices.size(); i += 6)
         {
            auto& P = wallVertices[i];
            auto& Q = wallVertices[i + 2];

            Vec2 p = Flatten(P.position);
            Vec2 q = Flatten(Q.position);

            if (a == p && b == q)
            {
               for (size_t j = i + 6; j < wallVertices.size(); ++j)
               {
                  wallVertices[j - 6] = wallVertices[j];
               }

               wallVertices.pop_back();
               wallVertices.pop_back();
               wallVertices.pop_back();
               wallVertices.pop_back();
               wallVertices.pop_back();
               wallVertices.pop_back();
               break;
            }
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
      }

      void BuildWalls(IRing<Vec2>& perimeter, ISectors& otherSectors)
      {
         float u = 0;

         wallVertices.clear();

         for (size_t i = 0; i < perimeter.ElementCount(); i++)
         {
            Vec2 p = perimeter[i];
            Vec2 q = perimeter[i+1];

            bool deleteSection = false;

            for (auto* s : otherSectors)
            {
               if (s != static_cast<ISector*>( this ))
               {
                  int32 index = s->GetSegment(q, p);
                  if (index >= 0)
                  {
                     deleteSection = true;
                     s->RemoveWallSegment(index);
                     break;
                  }
                  else
                  {
                     index = s->GetSegment(q, p);
                     if (index >= 0)
                     {
                        deleteSection = true;
                        s->RemoveWallSegment(index);
                        break;
                     }
                  }
               }
            }

            if (deleteSection) continue;

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
      Sector(Entities::IInstancesSupervisor& _instances) :
         instances(_instances),
         id(nextSectorId++)
      {

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
         for (int32 i = 0; i < floorVertices.size(); i += 3)
         {
            Vec2 a = Flatten(floorVertices[i].position);
            Vec2 b = Flatten(floorVertices[i + 1].position);
            Vec2 c = Flatten(floorVertices[i + 2].position);

            Triangle2d t;
            t.A = a;
            t.B = b;
            t.C = c;

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

      ObjectVertexBuffer FloorVertices() const override
      {
         return{ &floorVertices[0], floorVertices.size() };
      }

      void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1, ISectors& otherSectors) override
      {
         this->z0 = z0;
         this->z1 = z1;

         ValidateArray(positionArray, nVertices);

         Ring<Vec2> ring_no_chirality(positionArray, nVertices);

         size_t len = sizeof(Vec2) * nVertices;
         Vec2* stackArray = (Vec2*)alloca(len);
         
         bool forwardIsClockwise;
         if (IsClockwiseSequential(ring_no_chirality))
         {
            forwardIsClockwise = true;
            for (size_t i = 0; i != nVertices; i++)
            {
               stackArray[i] = positionArray[i];
               floorPerimeter.push_back(positionArray[i]);
            }
         }
         else
         {
            forwardIsClockwise = false;
            for (int32 i = (int32)nVertices - 1; i >= 0; i--)
            {
               stackArray[i] = positionArray[(nVertices-1)-i];
               floorPerimeter.push_back(positionArray[i]);
            }
         }

         BuildWalls(Ring<Vec2>(stackArray,nVertices), otherSectors);

         RingManipulator<Vec2> ring(stackArray, nVertices);
         TesselateByEarClip(*this, ring);

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

      void Free() override
      {
         delete this;
      }

      void InvokeSectorDialog() override
      {
         wchar_t title[32];
         SafeFormat(title, _TRUNCATE, L"Sector %u", id);

         HWND hMain = nullptr;
         AutoFree<IVariableEditor> editor = CreateVariableEditor(hMain, { 640, 400 }, 120, title, L"Floor and Ceiling", L"Edit floor and ceiling parameters");
         editor->AddIntegerEditor(L"Altitiude", L"Altitiude - centimetres", 0, 100000, 0);
         editor->AddIntegerEditor(L"Height", L"Height - centimetres", 0, 100000, 0);
         if (editor->IsModalDialogChoiceYes())
         {
         }
      }
   };
}

namespace HV
{
   ISector* CreateSector(Entities::IInstancesSupervisor& instances)
   {
      return new Sector(instances);
   }
}
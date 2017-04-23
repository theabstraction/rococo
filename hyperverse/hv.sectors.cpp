#include "hv.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <deque>

namespace
{
   using namespace Rococo;
   using namespace HV;

   // If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
   template<class T>
   T GetRingElement(size_t index, const T* array, size_t capacity)
   {
      return array[index % capacity];
   }

   class Sectors: public ISectors
   {
      HV::Entities::IInstancesSupervisor&  instances;
      const Metres defaultFloorLevel{ 0.0f };
      const Metres defaultRoomHeight{ 4.0f };
      std::vector<ISector*> sectors;
   public:
      Sectors(HV::Entities::IInstancesSupervisor& _instances) : instances(_instances)
      {
      }

      ~Sectors()
      {
         for (auto* s : sectors)
         {
            s->Free();
         }
      }

      ISector** begin() { return (sectors.empty() ? nullptr : &sectors[0]); }
      ISector** end() { return (sectors.empty() ? nullptr : &sectors[0] + sectors.size()); }

      void AddSector(const Vec2* positionArray, size_t nVertices) override
      {
         auto* s = CreateSector(instances, *this);
         try
         {
            s->Build(positionArray, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight);
            sectors.push_back(s);
         }
         catch (IException& ex)
         {
            s->Free();
            ShowErrorBox(Windows::NoParent(), ex, "Algorithmic error creating sector. Try something simpler");

#ifdef _DEBUG
            if (QueryYesNo(Windows::NoParent(), "Try again?"))
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

      void Free() override
      {
         delete this;
      }

      ISector* GetFirstSectorCrossingLine(Vec2 a, Vec2 b) override
      {
         for (auto* s : sectors)
         {
            if (s->DoesLineCrossSector(a, b))
            {
               return s;
            }
         }

         return nullptr;
      }

      SectorAndSegment GetFirstSectorWithPoint(Vec2 a)  override
      {
         for (auto* s : sectors)
         {
            int32 seg = s->GetPerimeterIndex(a);
            if (seg >= 0)
            {
               return{ s, seg };
            }
         }

         return{ nullptr, -1 };
      }

      ISector* GetFirstSectorContainingPoint(Vec2 a)  override
      {
         for (auto* s : sectors)
         {
            int32 i = s->GetFloorTriangleIndexContainingPoint(a);
            if (i >= 0)
            {
               return s;
            }
         }

         return nullptr;
      }
   };
}

namespace HV
{
   ISectors* CreateSectors(HV::Entities::IInstancesSupervisor& instances)
   {
      return new Sectors(instances);
   }
}
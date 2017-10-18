#include "hv.h"
#include "rococo.mplat.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <algorithm>

namespace
{
   using namespace Rococo;
   using namespace HV;

   enum LineClassify
   {
	   LineClassify_Left = 1,
	   LineClassify_Right = 2,
	   LineClassify_OnLine = 3,
   };

   LineClassify ClassifyPtAgainstPlane(Vec2 a, Vec2 b, Vec2 p)
   {
	   Vec2 ab = b - a;
	   Vec2 ap = p - a;

	   float f = Cross(ab, ap);
	   if (f > 0)
	   {
		   return LineClassify_Left;
	   }
	   else if (f < 0)
	   {
		   return LineClassify_Right;
	   }
	   else
	   {
		   return LineClassify_OnLine;
	   }
   }

   struct Frustum2d
   {
	   Frustum2d(cr_vec3 _eye, cr_vec3 _direction, Radians halfFov, Metres nearPlane, Metres farPlane)
	   {
		   eye = { _eye.x, _eye.y };
		   direction = Normalize(Vec2{ _direction.x, _direction.y });

		   Vec2 farplaneDirection = Normalize(Vec2{ direction.y, -direction.x });

		   Metres halfFarLength{ farPlane * tan(halfFov) };
		   Metres halfNearLength{ nearPlane * tan(halfFov) };

		   farLeft = eye + direction * farPlane - farplaneDirection * halfFarLength;
		   farRight = eye + direction * farPlane + farplaneDirection * halfFarLength;

		   nearLeft = eye + direction * nearPlane - farplaneDirection * halfNearLength;
		   nearRight = eye + direction * nearPlane + farplaneDirection * halfNearLength;
	   }

	   Vec2 eye;
	   Vec2 direction;

	   Vec2 nearLeft;
	   Vec2 nearRight;
	   Vec2 farLeft;
	   Vec2 farRight;

	   bool CanSeeThrough(Vec2 a, Vec2 b, Vec2& leftPoint, Vec2& rightPoint) const
	   {
		   LineClassify Cab = ClassifyPtAgainstPlane(eye, b, a);

		   if (Cab == LineClassify_OnLine) return false;

		   if (Cab == LineClassify_Right)
		   {
			   // The gap is in the frustum, but we are looking at it from behind, so cannot see through it
			   return false;
		   }

		   auto CLa = ClassifyPtAgainstPlane(nearLeft, farLeft, a);
		   auto CLb = ClassifyPtAgainstPlane(nearLeft, farLeft, b);

		   leftPoint = farLeft;
		   rightPoint = farRight;

		   if (CLa != LineClassify_Right && CLb != LineClassify_Right)
		   {
			   // both segments to the left of the frustum or sit on the left edge
			   return false;
		   }

		   auto CRa = ClassifyPtAgainstPlane(nearRight, farRight, a);
		   auto CRb = ClassifyPtAgainstPlane(nearRight, farRight, b);

		   if (CRa != LineClassify_Left && CRb != LineClassify_Left)
		   {
			   // both segments to the right of the view frustum or sit on the right edge
			   return false;
		   }

		   auto CFa = ClassifyPtAgainstPlane(farLeft, farRight, a);
		   auto CFb = ClassifyPtAgainstPlane(farLeft, farRight, b);

		   if (CFa != LineClassify_Right && CFb != LineClassify_Right)
		   {
			   // both segments outside the view frustum far plane
			   return false;
		   }

		   // N.B we do not clip against the eye, because even if the segment is between eye and the near plane
		   // Then what lies beyond the plane is visible and needs to be rendered
		   if (CLa == LineClassify_Right && CRa == LineClassify_Left)
		   {
			   // a is somewhere in the middle zone
			   if (eye != a)
			   {
				   leftPoint = a;
			   }
		   }

		   if (CLb == LineClassify_Right && CRb == LineClassify_Left)
		   {
			   // a is somewhere in the middle zone
			   if (eye != b)
			   {
				   rightPoint = b;
			   }
		   }

		   return true;
	   }
   };

   // If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
   template<class T>
   T GetRingElement(size_t index, const T* array, size_t capacity)
   {
      return array[index % capacity];
   }

   class Sectors: public ISectors
   {
      const Metres defaultFloorLevel{ 0.0f };
      const Metres defaultRoomHeight{ 4.0f };
      std::vector<ISector*> sectors;
      Platform& platform;
   public:
      Sectors(Platform& _platform) :
         platform(_platform)
      {
      }

      ~Sectors()
      {
         for (auto* s : sectors)
         {
            s->Free();
         }
      }

	  int64 iterationFrame = 0x81000000000;

	  void RecurseVisibilityScanOnGaps(Frustum2d& f, ISector& current, IEventCallback<VisibleSector>& cb, size_t& count)
	  {
		  count++;

		  current.SetIterationFrame(iterationFrame);

		  cb.OnEvent(VisibleSector{ current });

		  size_t numberOfGaps;
		  auto* gaps = current.Gaps(numberOfGaps);
		  for (size_t i = 0; i < numberOfGaps; ++i)
		  {
			  auto* gap = gaps++;

			  if (gap->other->IterationFrame() != iterationFrame)
			  {
				  Vec2 leftPoint, rightPoint;
				  if (f.CanSeeThrough(gap->a, gap->b, leftPoint, rightPoint))
				  {
					  RecurseVisibilityScanOnGaps(f, *gap->other, cb, count);
				  }
			  }
		  }
	  }

	  size_t ForEverySectorVisibleAt(cr_vec3 eye, cr_vec3 direction, IEventCallback<VisibleSector>& cb)
	  {
		  iterationFrame++;

		  Frustum2d f(eye, direction, 45_degrees, 0.15_metres, 1000_metres);

		  size_t count = 0;
		  ISector* home = GetFirstSectorContainingPoint(f.eye);
		  if (home)
		  {
			  RecurseVisibilityScanOnGaps(f, *home, cb, count);
		  }

		  return count;
	  }

      ISector** begin() { return (sectors.empty() ? nullptr : &sectors[0]); }
      ISector** end() { return (sectors.empty() ? nullptr : &sectors[0] + sectors.size()); }

      void AddSector(const SectorPalette& palette, const Vec2* positionArray, size_t nVertices) override
      {
         auto* s = CreateSector(platform, *this);
         s->SetPalette(palette);

         try
         {
            s->Build(positionArray, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight);
            sectors.push_back(s);
         }
         catch (IException& ex)
         {
            s->Free();
            platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, "Algorithmic error creating sector. Try something simpler");

#ifdef _DEBUG
            if (platform.utilities.QueryYesNo(platform, platform.renderer.Window(), "Try again?"))
            {
               OS::TripDebugger();
               OS::PrintDebug("\n\n\n // Troublesome perimeter: \n");
               OS::PrintDebug("const Vec2 perimeter[%d] = { ", nVertices);
               for (const Vec2* p = positionArray; p < positionArray + nVertices; p++)
               {
                  OS::PrintDebug("{%f,%f},", p->x, p->y);
               }
               OS::PrintDebug("};\n\n\n");

               AddSector(palette,  positionArray, nVertices);
            }
#endif
         }
      }

      void Delete(ISector* sector) override
      {
         struct
         {
            ISector* a;

            bool operator()(ISector* b) const
            {
               return a == b;
            }
         } match{ sector };
         sectors.erase(std::remove_if(sectors.begin(), sectors.end(), match), sectors.end());

         for (auto& s : sectors)
         {
            s->Rebuild();
         }

         sector->Free();
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

	  void OnSectorScriptChanged(const FileModifiedArgs& args) override
	  {
		  for (auto i : sectors)
		  {
			  i->OnSectorScriptChanged(args);
		  }
	  }
   };
}

namespace HV
{
   ISectors* CreateSectors(Platform& platform)
   {
      return new Sectors(platform);
   }
}
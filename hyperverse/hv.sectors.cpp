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

   Vec3 GetNormal(const Gap& gap)
   {
	   Vec3 top = { gap.b.x - gap.a.x, gap.a.y - gap.a.y, 0 };
	   return Vec3 { top.y, -top.x, 0 };
   }

   // Determine if we can look through a gap to see the sector beyond
   // If we are in the home sector, then don't worry too much if the gap is slightly behind the near plane
   // As we may be on the edge of the home sector, and our near plane just over the gap into the next sector
   bool CanSeeThrough(const Matrix4x4& camera, cr_vec3 forward, const Gap& gap, Vec2& leftPoint, Vec2& rightPoint, bool fromHome)
   {
	   Vec4 qGap[4] =
	   {
		   { gap.a.x, gap.a.y, gap.z1, 1 },
		   { gap.b.x, gap.b.y, gap.z1, 1 },
		   { gap.b.x, gap.b.y, gap.z0, 1 },
		   { gap.a.x, gap.a.y, gap.z0, 1 }
	   };

	   Vec4 qScreen[4];

	   for (int i = 0; i < 4; ++i)
	   {
		   qScreen[i] = camera * qGap[i];
	   }

	   Vec3 qScreenV[4];
	   for (int i = 0; i < 4; ++i)
	   {
		   qScreenV[i] = ((const Vec3&) qScreen[i]) * (1.0f / qScreen[i].w);
	   }

	   Vec3 ab1 = qScreenV[1] - qScreenV[0];
	   Vec3 bc1 = qScreenV[2] - qScreenV[1];
	   float tri1Normal = Cross(Flatten(ab1), Flatten(bc1));

	   Vec3 ab2 = qScreenV[3] - qScreenV[2];
	   Vec3 bc2 = qScreenV[0] - qScreenV[3];
	   float tri2Normal = Cross(Flatten(ab2), Flatten(bc2));

	   const Vec3* v = qScreenV;

	   int j = 0;

	   for (int i = 0; i < 4; i++)
	   {
		   if (qScreen[i].w < 0)
		   {
			   j--;
		   }
		   else if (qScreen[i].w > 0)
		   {
			   j++;
		   }
	   }

	   if (j == 4 || j == -4)
	   {
		   if (tri1Normal > 0 && tri2Normal > 0)
		   {
			   // Edge case -> the eye may be in one sector, and the near plane just over the gap
			   // This flips triangles, but we are still looking into the sector.
			   // If this is so our view direction opposes the normal  of the gap

			   if (fromHome)
			   {
				   if (Dot(forward, GetNormal(gap)))
				   {
					   return true;
				   }
			   }

			   return false;
		   }

		   if (v[0].x <= -1 && v[1].x <= -1 && v[2].x <= -1 && v[3].x <= -1)
		   {
			   // Everything to left of screen, so gap is not visible
			   return false;
		   }

		   if (v[0].x >= 1 && v[1].x >= 1 && v[2].x >= 1 && v[3].x >= 1)
		   {
			   // Everything to right of screen, so gap is not visible
			   return false;
		   }

		   if (v[0].y >= 1 && v[1].y >= 1 && v[2].y >= 1 && v[3].y >= 1)
		   {
			   // Everything to top of screen, so gap is not visible
			   return false;
		   }

		   if (v[0].y <= -1 && v[1].y <= -1 && v[2].y <= -1 && v[3].y <= -1)
		   {
			   // Everything to bottom of screen, so gap is not visible
			   return false;
		   }
	   }

	   return true;
   }

   class Sectors: public ISectors, public ISectorBuilder
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
		  Clear();
      }

	  void Clear()
	  {
		  for (auto* s : sectors)
		  {
			  s->Free();
		  }

		  sectors.clear();
		  vertices.clear();
	  }

	  virtual void SaveAsFunction(StringBuilder& sb)
	  {
		  sb.AppendFormat("(using HV)\n\n");

		  sb.AppendFormat("(function AddSectorsToLevel -> :\n");
		  sb.AppendFormat("\t(ISectors sectors (Sectors))\n\n");
		  sb.AppendFormat("\t(sectors.Clear)\n\n");

		  uint32 index = 0;
		  for (auto s : sectors)
		  {
			  sb.AppendFormat("\t(AddSector%u sectors)%s\n", index++, index == 0 ? " // entrance" : "");
		  }

		  sb.AppendFormat(")\n\n");

		  index = 0;
		  for (auto s : sectors)
		  {
			  sb.AppendFormat("(function AddSector%u (ISectors sectors) -> :\n", index++);

			  int z0 = (int) (s->Z0() * 100);
			  int z1 = (int) (s->Z1() * 100);
			  sb.AppendFormat("\t(Int64 flags = %llu) // 0x%llX", s->Flags(), s->Flags());

			  if (s->Flags() == 0)
			  {
				  sb.AppendFormat(" None");
			  }

			  if (s->IsFlagged(SectorFlag_Occlude_Players))
			  {
				  sb.AppendFormat(" Occlude_Players");
			  }

			  if (s->IsFlagged(SectorFlag_Occlude_Friends))
			  {
				  sb.AppendFormat(" Occlude_Friends");
			  }

			  if (s->IsFlagged(SectorFlag_Occlude_Enemies))
			  {
				  sb.AppendFormat(" Occlude_Enemies");
			  }

			  if (s->IsFlagged(SectorFlag_Has_Door))
			  {
				  sb.AppendFormat(" Has_Door");
			  }

			  sb.AppendFormat("\n\n");

			  size_t nVertices;
			  auto* v = s->WallVertices(nVertices);

			  for (size_t i = 0; i < nVertices; i++)
			  {
				  sb.AppendFormat("\t(sectors.AddVertex %f %f)\n", v[i].x, v[i].y);
			  }

			  sb.AppendFormat("\n\t(sectors.Create %d %d flags wall floor ceiling)\n", z0, z1 - z0);
			  sb.AppendFormat(")\n\n");
		  }
	  }

	  void ResetConfig()
	  {

	  }

	  int64 iterationFrame = 0x81000000000;

	  void CallbackOnce(ISector& sector, IEventCallback<VisibleSector>& cb, int64 iterationFrame)
	  {
		  if (sector.IterationFrame() != iterationFrame)
		  {
			  sector.SetIterationFrame(iterationFrame);
			  cb.OnEvent(VisibleSector{ sector });
		  }
	  }

	  bool IsInLineOfSight(const Gap& gap, cr_vec3 eye)
	  {
		  Vec2 qGap[2] =
		  {
			  gap.a, 
			  gap.b
		  };

		  for (auto k : lineOfSight)
		  {
			  if ((k->a == gap.a && k->b == gap.b) || (k->a == gap.b && k->b == gap.a))
			  {
				  // Prevent any gap being counted twice in any direction
				  return false;
			  }
		  }

		  for (auto k : lineOfSight)
		  {
			  // Use k to generate a cone and check bounds of gap fits in cone
			  
			  Vec3 displacement = k->bounds.centre - eye;
			  if (LengthSq(displacement) > Sq(k->bounds.radius + 0.1f))
			  {
				  float d = Length(k->bounds.centre - eye);
				  Radians coneAngle{ asinf(k->bounds.radius / d) };
				  if (IsOutsideCone(eye, Normalize(displacement), coneAngle, gap.bounds))
				  {
					  return false;
				  }
			  }
		  }

		  for (auto k : lineOfSight)
		  {
			  int j = 0;
			  for (int i = 0; i < 2; ++i)
			  {
				  auto l = ClassifyPtAgainstPlane(k->a, k->b, qGap[i]);
				  if (l == LineClassify_Left)
				  {
					  j++;
				  }
				  else if (l == LineClassify_Right)
				  {
					  j--;
				  }
			  }	  

			  auto l = ClassifyPtAgainstPlane(k->a, k->b, Flatten(eye));

			  if (j == 2)
			  {
				  return l == LineClassify_Right;
			  }
			  else if (j == -2)
			  {
				  return l == LineClassify_Left;
			  }
		  }

		  return true;
	  }

	  void RecurseVisibilityScanOnGapsBy(const Matrix4x4& cameraMatrix, cr_vec3 eye, cr_vec3 forward, ISector& current, IEventCallback<VisibleSector>& cb, size_t& count, bool fromHome)
	  {
		  count++;

		  size_t numberOfGaps;
		  auto* gaps = current.Gaps(numberOfGaps);
		  for (size_t i = 0; i < numberOfGaps; ++i)
		  {
			  auto* gap = gaps++;

			  Vec2 a1, b1;
			  if (CanSeeThrough(cameraMatrix, forward, *gap, a1, b1, fromHome))
			  {
				  if (IsInLineOfSight(*gap, eye))
				  {
					  lineOfSight.push_back(gap);
					  CallbackOnce(*gap->other, cb, iterationFrame);
					  RecurseVisibilityScanOnGapsBy(cameraMatrix, eye, forward, *gap->other, cb, count, false);
					  lineOfSight.pop_back();
				  }
			  }
		  }
	  }

	  std::vector<const Gap*> lineOfSight; // Quick, nasty but effective BSP test

	  size_t ForEverySectorVisibleBy(const Matrix4x4& cameraMatrix, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector>& cb)
	  {
		  iterationFrame++;

		  lineOfSight.clear();

		  size_t count = 0;
		  ISector* home = GetFirstSectorContainingPoint({ eye.x, eye.y });
		  if (home)
		  {
			  CallbackOnce(*home, cb, iterationFrame);
			  RecurseVisibilityScanOnGapsBy(cameraMatrix, eye, forward, *home, cb, count, true);
		  }

		  return count;
	  }

      ISector** begin() { return (sectors.empty() ? nullptr : &sectors[0]); }
      ISector** end() { return (sectors.empty() ? nullptr : &sectors[0] + sectors.size()); }

	  std::vector<ISector*> dirty;

	  void AddDirty(ISector* dirtySector) override
	  {
		  dirty.push_back(dirtySector);
	  }

	  void RebuildDirtySectors(int64 iterationFrame)
	  {
		  for (auto& s : dirty)
		  {
			  s->Rebuild(iterationFrame);
		  }

		  for (auto& s : dirty)
		  {
			  if (s->IsCorridor())
			  {
				  size_t count;
				  auto* g = s->Gaps(count);
				  g[0].other->Rebuild(iterationFrame);
				  g[1].other->Rebuild(iterationFrame);
			  }
		  }

		  dirty.clear();
	  }

	  std::vector<Vec2> vertices;

	  void AddVertex(float x, float y) override
	  {
		  vertices.push_back(Vec2{ x, y });
	  }

	  int32 Create(int32 altitude, int32 height, int64 flags, const fstring& wallTexture, const fstring& floorTexture, const fstring& ceilingTexture) override
	  {
		  SectorPalette palette
		  {
			  wallTexture,
			  floorTexture,
			  ceilingTexture
		  };

		  auto* s = CreateSector(platform, *this);
		  s->SetPalette(palette);
		  s->AddFlag((SectorFlag)flags);

		  try
		  {
			  dirty.clear();
			  s->Build(&vertices[0], vertices.size(), 0.01f * altitude, 0.01f * (altitude + height ));
			  sectors.push_back(s);
			  RebuildDirtySectors(s->IterationFrame());
		  }
		  catch (IException&)
		  {
			  vertices.clear();
			  throw;
		  }

		  vertices.clear();

		  return s->Id();
	  }

      void AddSector(const SectorPalette& palette, const Vec2* positionArray, size_t nVertices) override
      {
         auto* s = CreateSector(platform, *this);
         s->SetPalette(palette);

         try
         {
			dirty.clear();
            s->Build(positionArray, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight);
            sectors.push_back(s);
			RebuildDirtySectors(s->IterationFrame());
         }
		 catch (IException& ex)
		 {
			 dirty.clear();
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

				 AddSector(palette, positionArray, nVertices);
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

		 sector->Decouple();

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

      SectorAndSegment GetFirstSectorWithVertex(Vec2 a)  override
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

	  virtual ISectorBuilder* Builder()
	  {
		  return this;
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
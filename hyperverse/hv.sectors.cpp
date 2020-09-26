#include "hv.h"
#include "rococo.mplat.h"
#include <rococo.maths.h>
#include <vector>
#include <algorithm>
#include <rococo.hashtable.h>
#include <rococo.clock.h>
#include <rococo.sexy.api.h>

namespace HV
{
	ITagsSupervisor* CreateTagsSupervisor(ISectors& sectors);
	bool CanSeeThrough(const Matrix4x4& camera, cr_vec3 forward, const Gap& gap, Vec2& leftPoint, Vec2& rightPoint, bool fromHome);
}

#include "hv.action.factory.inl"
#include "hv.sector.layout.null.h"

namespace ANON
{
   using namespace Rococo;
   using namespace HV;

   EventIdRef evPopulateSectors = "sectors.populate"_event;

   NullSectorLayout nullSectorLayout;

   class Sectors : 
	   public ISectors,  
	   public Rococo::Events::IObserver,
	   public ISectorEnumerator
   {
	   const Metres defaultFloorLevel{ 0.0f };
	   const Metres defaultRoomHeight{ 4.0f };
	   std::vector<ISector*> sectors;
	   Platform& platform;
	   ActionFactoryCreateContext afcc;
	   AutoFree<ISectorBuilderSupervisor> sectorBuilder;
	   AutoFree<ITagsSupervisor> tags;
   public:
	   Sectors(Platform& _platform) :
		   platform(_platform),
		   sectorBuilder(CreateSectorBuilder(sectorBuilderAPI)),
		   sectorBuilderAPI(_platform, *this),
		   tags(CreateTagsSupervisor(*this))
	   {
		   platform.publisher.Subscribe(this, evPopulateSectors);
	   }

	   ~Sectors()
	   {
		   platform.publisher.Unsubscribe(this);
		   Clear();
	   }

	   int32 Count() override
	   {
		   return (int32)sectors.size();
	   }

	   HV::ISectorLayout* GetSector(int32 index)  override
	   {
		   if (index < 0 || index >= sectors.size())
		   {
			   if (sectors.empty()) 
				   Throw(0, "No sectors");
			   else 
				   Throw(0, "Invalid sector index %d. Range is [0,%d]", index, (int32) sectors.size() - 1);
		   }
		   return sectors[index]->Layout();
	   }

	   HV::ISectorLayout* GetSectorById(int32 id)  override
	   {
		   int32 index = id - 1;
		   if (index < 0 || index >= sectors.size())
		   {
			   if (sectors.empty())
				   Throw(0, "No sectors");
			   else
				   Throw(0, "Invalid sector id %d. Range is [1,%d]", index, (int32)sectors.size());
		   }

		   auto& s = sectors[index];
		   if (s->Id() != id) Throw(0, "Sector id mismatch #%d to #%d", s->Id(), id);
		   return sectors[index]->Layout();
	   }


	   HV::ISectorLayout* GetSelectedSector() override
	   {
		   if (selectedIndex >= sectors.size())
		   {
			   return &ANON::nullSectorLayout;
		   }

		   return sectors[selectedIndex]->Layout();
	   }

	   ISectorEnumerator& Enumerator() override
	   {
		   return *this;
	   }

	   struct SectorBuilderAPI : ISectorBuildAPI
	   {
		   Platform& platform;
		   Sectors& sectors;
		   SectorBuilderAPI(Platform& _platform, Sectors& _sectors) :
			   platform(_platform), sectors(_sectors) 
		   {
		   }

		   void Attach(ISector* s) override
		   {
			   sectors.sectors.push_back(s);
		   }

		   ISector* CreateSector() override
		   {
			   return HV::CreateSector(platform, sectors);
		   }

		   MaterialId GetMaterialId(cstr name) override
		   {
			   return platform.renderer.GetMaterialId(name);
		   }

		   MaterialId GetRandomMaterialId(Rococo::Graphics::MaterialCategory cat) override
		   {
			   return platform.instances.GetRandomMaterialId(cat);
		   }

		   void UpdateProgress(int id) override
		   {
			   platform.utilities.ShowBusy(true, "Loading level", "Created sector %u", id);
		   }

		   void GenerateMeshes() override
		   {
			   for (auto sector : sectors.sectors)
			   {
				   if (sector->IsDirty())
				   {
					   platform.utilities.ShowBusy(true, "Generating meshes", "Building sector %u", sector->Id());
					   sector->Rebuild();
				   }
			   }
		   }

		   void ClearSectors() override
		   {
			   sectors.Clear();
		   }
	   } sectorBuilderAPI;

	   bool IsMeshGenerationEnabled() const override
	   {
		   return sectorBuilder->IsMeshGenerationEnabled();
	   }

	   ITags& Tags()
	   {
		   return *tags;
	   }

	   IIActionFactoryCreateContext& AFCC()
	   {
		   return afcc;
	   }

	   void Clear()
	   {
		   for (auto* s : sectors)
		   {
			   s->Free();
		   }

		   sectors.clear();
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

	   void RecurseVisibilityScanOnGapsBy(const Matrix4x4& cameraMatrix, cr_vec3 eye, cr_vec3 forward, ISector& current, IEventCallback<VisibleSector>& cb, size_t& count, bool fromHome, int64 iterationFrame)
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
					   RecurseVisibilityScanOnGapsBy(cameraMatrix, eye, forward, *gap->other, cb, count, false, iterationFrame);
					   lineOfSight.pop_back();
				   }
			   }
		   }
	   }

	   std::vector<const Gap*> lineOfSight; // Quick, nasty but effective BSP test

	   bool isScanning = false;

	   size_t ForEverySectorVisibleBy(const Matrix4x4& cameraMatrix, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector>& cb)
	   {
		   if (isScanning)
		   {
			   Throw(0, "Cannot nest ForEverySectorVisibleBe");
		   }

		   isScanning = true;

		   iterationFrame++;

		   lineOfSight.clear();

		   size_t count = 0;
		   ISector* home = GetFirstSectorContainingPoint({ eye.x, eye.y });
		   if (home)
		   {
			   CallbackOnce(*home, cb, iterationFrame);
			   RecurseVisibilityScanOnGapsBy(cameraMatrix, eye, forward, *home, cb, count, true, iterationFrame);
		   }

		   isScanning = false;

		   return count;
	   }

	   ISector** begin() { return (sectors.empty() ? nullptr : &sectors[0]); }
	   ISector** end() { return (sectors.empty() ? nullptr : &sectors[0] + sectors.size()); }

	   void AddSector(const Vec2* floorPlan, size_t nVertices) override
	   {
		   auto* s = CreateSector(platform, *this);

		   try
		   {
			   s->Build(floorPlan, nVertices, defaultFloorLevel, defaultFloorLevel + defaultRoomHeight);
			   sectors.push_back(s);
		   }
		   catch (IException& ex)
		   {
			   s->Free();
			   platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, "Algorithmic error creating sector. Try something simpler");

#ifdef _DEBUG
			   if (platform.utilities.QueryYesNo(platform.renderer.Window(), "Try again?"))
			   {
				   OS::TripDebugger();
				   OS::PrintDebug("\n\n\n // Troublesome perimeter: \n");
				   OS::PrintDebug("const Vec2 perimeter[%d] = { ", nVertices);
				   for (const Vec2* p = floorPlan; p < floorPlan + nVertices; p++)
				   {
					   OS::PrintDebug("{%f,%f},", p->x, p->y);
				   }
				   OS::PrintDebug("};\n\n\n");

				   AddSector(floorPlan, nVertices);
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
			   if (s->GetAABB().HoldsPoint(a))
			   {
				   int32 seg = s->GetPerimeterIndex(a);
				   if (seg >= 0)
				   {
					   return{ s, seg };
				   }
			   }
		   }

		   return{ nullptr, -1 };
	   }

	   ISector* GetFirstSectorContainingPoint(Vec2 a)  override
	   {
		   for (auto* s : sectors)
		   {
			   auto& aabb = s->GetAABB();

			   if (aabb.HoldsPoint(a))
			   {
				   int32 i = s->GetFloorTriangleIndexContainingPoint(a);
				   if (i >= 0)
				   {
					   return s;
				   }
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

	   ISectorBuilderSupervisor* Builder()
	   {
		   return sectorBuilder;
	   }

	   U8FilePath populateScript = { 0 };

	   void BindProperties(IBloodyPropertySetEditor& editor) override
	   {
		   editor.AddPingPath("Populate Script", populateScript.buf, IO::MAX_PATHLEN, "#objects/pop.default.sxy", 140);
		   editor.AddButton("Populate", evPopulateSectors.name);
	   }

	   void NotifyChanged() override
	   {
		   if (*populateScript)
		   {
			   try
			   {
				   WideFilePath sysPath;
				   platform.installation.ConvertPingPathToSysPath(populateScript, sysPath);
				   platform.installation.ConvertSysPathToMacroPath(sysPath, populateScript, "#objects");
			   }
			   catch (IException&)
			   {

			   }
		   }
	   }

	   void OnEvent(Event& ev) override
	   {
		   if (ev == evPopulateSectors)
		   {
			   Populate();
		   }
	   }

	   size_t selectedIndex = -1;

	   size_t GetSelectedSectorId() const
	   {
		   return selectedIndex;
	   }

	   void SelectSector(size_t index) override
	   {
		   selectedIndex = index;
	   }

	   void Populate()
	   {
		   struct : IEventCallback<ScriptCompileArgs>
		   {
			   Sectors* This;
			   void OnEvent(ScriptCompileArgs& args) override
			   {
				   HV::AddMathsEx(args.ss);
				   AddNativeCalls_HVISectorEnumerator(args.ss, This);
				   AddNativeCalls_HVISectorLayout(args.ss, nullptr);
			   }
		   } p;

		   p.This = this;

		   cstr thePopulateScript = *populateScript != 0 ? populateScript : "#objects/pop.default.sxy";
		   platform.utilities.RunEnvironmentScript(p, thePopulateScript, true, false);
	   }

	   void OnTick(const IUltraClock& clock) override
	   {
		   if (clock.DT() > 0.0f)
		   {
			   for (auto s : sectors)
			   {
				   s->OnTick(clock);
			   }
		   }
	   }
   }; // class Sectors
}// HV

namespace HV
{
   ISectors* CreateSectors(Platform& platform)
   {
      return new ANON::Sectors(platform);
   }
}
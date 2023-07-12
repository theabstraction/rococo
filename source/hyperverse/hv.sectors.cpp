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
	ISectorVisibilityBuilder* CreateSectorVisibilityBuilder();
}

#include "hv.action.factory.inl"
#include "hv.sector.layout.null.inl"

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
	   AutoFree<ISectorVisibilityBuilder> sectorVisibilityBuilder;
   public:
	   Sectors(Platform& _platform) :
		   platform(_platform),
		   sectorBuilder(CreateSectorBuilder(sectorBuilderAPI)),
		   sectorBuilderAPI(_platform, *this),
		   tags(CreateTagsSupervisor(*this)),
		   sectorVisibilityBuilder(CreateSectorVisibilityBuilder())
	   {
		   platform.plumbing.publisher.Subscribe(this, evPopulateSectors);
	   }

	   ~Sectors()
	   {
		   platform.plumbing.publisher.Unsubscribe(this);
		   Clear();
	   }

	   int32 Count() override
	   {
		   return (int32)sectors.size();
	   }

	   bool empty() const override
	   {
		   return sectors.empty();
	   }

	   size_t size() const override
	   {
		   return sectors.size();
	   }

	   HV::ISectorLayout* GetSector(int32 index)  override
	   {
		   return HV::GetSector(index, *this);
	   }

	   HV::ISectorLayout* GetSectorById(int32 id)  override
	   {
		   return HV::GetSectorById(id, *this);
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
			   return platform.graphics.renderer.Materials().GetMaterialId(name);
		   }

		   MaterialId GetRandomMaterialId(Rococo::Graphics::MaterialCategory cat) override
		   {
			   return platform.graphics.instances.GetRandomMaterialId(cat);
		   }

		   void UpdateProgress(int id) override
		   {
			   platform.plumbing.utilities.ShowBusy(true, "Loading level", "Created sector %u", id);
		   }

		   void GenerateMeshes() override
		   {
			   for (auto sector : sectors.sectors)
			   {
				   if (sector->IsDirty())
				   {
					   platform.plumbing.utilities.ShowBusy(true, "Generating meshes", "Building sector %u", sector->Id());
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

	   ISector** begin() { return (sectors.empty() ? nullptr : &sectors[0]); }
	   ISector** end() { return (sectors.empty() ? nullptr : &sectors[0] + sectors.size()); }

	   ISector& operator[](int index) override
	   {
		   return *sectors[index];
	   }

	   const ISector& operator[](int index) const
	   {
		   return *sectors[index];
	   }

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
			   platform.plumbing.utilities.ShowErrorBox(platform.os.mainWindow, ex, "Algorithmic error creating sector. Try something simpler");

#ifdef _DEBUG
			   if (platform.plumbing.utilities.QueryYesNo(platform.os.mainWindow, "Try again?"))
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

	   U8FilePath populateScript;

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
				   platform.os.installation.ConvertPingPathToSysPath(populateScript, sysPath);
				   platform.os.installation.ConvertSysPathToMacroPath(sysPath, populateScript, "#objects");
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
		   platform.plumbing.utilities.RunEnvironmentScript(HVDefaultIncludes(), p, thePopulateScript, true, false);
	   }

	   size_t ForEverySectorVisibleBy(cr_m4x4 worldToScreen, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector>& cb)
	   {
		   return sectorVisibilityBuilder->ForEverySectorVisibleBy(*this, worldToScreen, eye, forward, cb);
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
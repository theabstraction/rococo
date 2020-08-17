#include "hv.h"
#include "rococo.mplat.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

namespace ANON
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

   EventIdRef evPopulateSectors = "sectors.populate"_event;

   struct NullSectorLayout : public ISectorLayout
   {
	   int32 CountSquares() override { return 0; }
	   boolean32 Exists() override { return false; }
	   void GetSquare(int32 sqIndex, AABB2d& sq) override {}
	   void CeilingQuad(int32 sqIndex, QuadVertices& q) override {}
	   void FloorQuad(int32 sqIndex, QuadVertices& q) override {}
	   void Altitude(Vec2& altitudes) override {}
	   int32 NumberOfSegments()  override { return 0; }
	   int32 NumberOfGaps() override { return 0; }
	   void GetSegment(int32 segIndex, HV::WallSegment& segment) override {}
	   void GetGap(int32 gapIndex, HV::GapSegment& segment) override {}
	   ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) override { return ID_ENTITY(); }
	   ID_ENTITY AddItemToLargestSquare(const fstring& mesh, int32 addItemFlags, const HV::ObjectCreationSpec& ocs) override { return ID_ENTITY(); }
	   boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) override { return false; }
	   void DeleteScenery()  override {}
	   void DeleteItemsWithMesh(const fstring& prefix) override {}
	   void ClearManagedEntities()  override {}
	   void ManageEntity(ID_ENTITY id) override {}
	   void UseUpFacingQuads(ID_ENTITY id)  override {}
   } nullSectorLayout;

   struct ActionFactoryCreateContext : IIActionFactoryCreateContext, IGlobalVariables
   {
	   IGlobalVariables& GetGlobals() override { return *this; }

	   mutable std::unordered_map<std::string, int32> int32Variables;

	   bool GetValue(cstr name, int32& outputValue) const override
	   {
		   auto i = int32Variables.find(name);
		   if (i != int32Variables.end())
		   {
			   outputValue = i->second;
			   return true;
		   }
		   else
		   {
			   int32Variables[name] = outputValue;
			   return false;
		   }
	   }

	   int32 SetValue(cstr name, int32 value) override
	   {
		   auto i = int32Variables.find(name);
		   if (i != int32Variables.end())
		   {
			   int32 output = i->second;
			   i->second = value;
			   return output;
		   }
		   else
		   {
			   int32Variables[name] = value;
			   return 0;
		   }
	   }

	   void ValidateName(cstr name) const override
	   {
		   if (name == nullptr || *name == 0) Throw(0, "Global variable name was null.");
		   for (auto p = name; *p != 0; p++)
		   {
			   char c = *p;
			   if (!IsAlphaNumeric(c) && c != '.' && c != '_')
			   {
				   Throw(0, "Global variable %s had bad character at position %u. Must be alphanumeric, . or _", name, p - name);
			   }
		   }

		   if (strlen(name) >= IGlobalVariables::MAX_VARIABLE_NAME_LENGTH)
		   {
			   Throw(0, "Global variable too long %s. Max length is ", name, IGlobalVariables::MAX_VARIABLE_NAME_LENGTH);
		   }
	   }
   };

   struct JapaneseFrodoMachine : ITags
   {
	   ISectors* sectors;

	   typedef std::unordered_map<std::string, std::vector<ISector*>> TMapStringToSectors;
	   TMapStringToSectors map;

	   bool inTagCall = false;

	   void Invalidate() override
	   {
		   if (inTagCall) Throw(0, "Tags.Invalidate(): callback tried to delete hashtable");
		   map.clear();
	   }

	   void RefreshHashtables()
	   {
		   Invalidate();

		   cstr undefined = GET_UNDEFINDED_TAG();

		   for (auto* s : *sectors)
		   {
			   auto& tags = s->GetTags();
			   for (int32 i = 0; i < tags.Count(); ++i)
			   {
				   char varName[IGlobalVariables::MAX_VARIABLE_NAME_LENGTH];
				   tags.GetItem(i, varName, IGlobalVariables::MAX_VARIABLE_NAME_LENGTH);

				   if (Eq(varName, undefined))
				   {
					   continue;
				   }
				   
				   auto j = map.find(varName);
				   if (j == map.end())
				   {
					   j = map.insert(std::make_pair(std::string(varName), std::vector<ISector*>())).first;
				   }

				   j->second.push_back(s);
			   }
		   }
	   }

	   void ForEachSectorWithTagProtected(cstr tag, ITagCallback& cb)
	   {
		   auto i = map.find(tag);
		   if (i != map.end())
		   {
			   int32 count = 0;

			   for (auto* sector : i->second)
			   {
				   TagContext context{ *sector, tag,  count++ };
				   cb.OnTag(context);
			   }
		   }
	   }

	   void ForEachSectorWithTag(cstr tag, ITagCallback& cb) override
	   {
		   if (inTagCall) Throw(0, "Tags.ForEachSectorWithTag(%s,...): callback tried to recurse", tag);

		   if (map.empty())
		   {
			   RefreshHashtables();
		   }

		   try
		   {
			   inTagCall = true;
			   ForEachSectorWithTagProtected(tag, cb);
			   inTagCall = false;
		   }
		   catch(IException&)
		   {
			   inTagCall = false;
		   }
	   }
   };

   class Sectors : public ISectors, public ISectorBuilder, public Rococo::Events::IObserver
   {
	   const Metres defaultFloorLevel{ 0.0f };
	   const Metres defaultRoomHeight{ 4.0f };
	   std::vector<ISector*> sectors;
	   Platform& platform;
	   ActionFactoryCreateContext afcc;
	   JapaneseFrodoMachine tags;
	   bool generateMesh = true;

   public:
	   Sectors(Platform& _platform) :
		   platform(_platform)
	   {
		   tags.sectors = this;
		   platform.publisher.Subscribe(this, evPopulateSectors);
	   }

	   ~Sectors()
	   {
		   platform.publisher.Unsubscribe(this);

		   Clear();

		   for (auto i : temp.nameToMaterials)
		   {
			   delete i.second;
		   }
	   }

	   void DisableMeshGeneration() override
	   {
		   generateMesh = false;
	   }

	   void EnableMeshGeneration() override
	   {
		   generateMesh = true;
	   }

	   void GenerateMeshes() override
	   {
		   for (auto sector : sectors)
		   {
			   if (sector->IsDirty())
			   {
				   sector->Rebuild();
			   }
		   }
	   }

	   bool IsMeshGenerationEnabled() const override
	   {
		   return generateMesh;
	   }

	   ITags& Tags()
	   {
		   return tags;
	   }

	   IIActionFactoryCreateContext& AFCC()
	   {
		   return afcc;
	   }

	   void RebaseIds()
	   {

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

	   void SaveAsFunction(StringBuilder& sb) override
	   {
		   sb.AppendFormat("(using HV)\n");
		   sb.AppendFormat("(using Rococo.Graphics)\n\n");

		   sb.AppendFormat("(function AddSectorsToLevel -> :\n");
		   sb.AppendFormat("\t(ISectors sectors (SectorBuilder))\n\n");
		   sb.AppendFormat("\t(sectors.Clear)\n");
		   sb.AppendFormat("\t(sectors.DisableMeshGeneration)\n\n");

		   uint32 index = 0;
		   for (auto s : sectors)
		   {
			   sb.AppendFormat("\t(AddSector%u sectors)%s\n", index++, index == 0 ? " // entrance" : "");
		   }

		   sb.AppendFormat("\n\t(sectors.EnableMeshGeneration)\n");
		   sb.AppendFormat("\t(sectors.GenerateMeshes)\n");

		   sb.AppendFormat(")\n\n");

		   index = 0;
		   for (auto s : sectors)
		   {
			   sb.AppendFormat("(function AddSector%u (ISectors sectors) -> :\n", index++);

			   int z0 = (int)(s->Z0() * 100);
			   int z1 = (int)(s->Z1() * 100);

			   size_t nVertices;
			   auto* v = s->WallVertices(nVertices);

			   for (size_t i = 0; i < nVertices; i++)
			   {
				   sb.AppendFormat("\t(sectors.AddVertex %f %f)\n", v[i].x, v[i].y);
			   }

			   s->SaveTemplate(sb);

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

	   std::vector<Vec2> vertices;

	   void AddVertex(float x, float y) override
	   {
		   vertices.push_back(Vec2{ x, y });
	   }

	   struct
	   {
		   std::string door_scriptName;
		   bool door_useScript;

		   std::string wall_scriptName;
		   bool wall_useScript;

		   std::string floor_scriptName;
		   bool floor_useScript;

		   std::unordered_map<std::string, Material*> nameToMaterials;
		   std::unordered_map<std::string, float> doorVars;
		   std::unordered_map<std::string, float> floorVars;
		   std::unordered_map<std::string, float> wallVars;
	   } temp;

	   void SetTemplateWallScript(boolean32 useScript, const fstring& scriptName) override
	   {
		   temp.wall_useScript = useScript;
		   temp.wall_scriptName = scriptName;
	   }

	   void SetTemplateDoorScript(boolean32 hasDoor, const fstring& scriptName)  override
	   {
		   temp.door_useScript = hasDoor;
		   temp.door_scriptName = scriptName;
	   }

	   void SetTemplateFloorScript(boolean32 useScript, const fstring& scriptName) override
	   {
		   temp.floor_useScript = useScript;
		   temp.floor_scriptName = scriptName;
	   }

	   cstr GetTemplateFloorScript(bool& usesScript) const override
	   {
		   usesScript = this->temp.floor_useScript;
		   return temp.floor_scriptName.empty() ? "" : temp.floor_scriptName.c_str();
	   }

	   cstr GetTemplateDoorScript(bool& hasDoor) const override
	   {
		   hasDoor = this->temp.door_useScript;
		   return temp.door_scriptName.empty() ? "" : temp.door_scriptName.c_str();
	   }

	   cstr GetTemplateWallScript(bool& usesScript) const override
	   {
		   usesScript = this->temp.wall_useScript;
		   return temp.wall_scriptName.empty() ? "" : temp.wall_scriptName.c_str();
	   }

	   void EnumerateDoorVars(IEventCallback<VariableCallbackData>& cb) override
	   {
		   for (auto& v : temp.doorVars)
		   {
			   cb.OnEvent(VariableCallbackData{ v.first.c_str(), v.second });
		   }
	   }

	   void EnumerateWallVars(IEventCallback<VariableCallbackData>& cb) override
	   {
		   for (auto& v : temp.wallVars)
		   {
			   cb.OnEvent(VariableCallbackData{ v.first.c_str(), v.second });
		   }
	   }

	   void EnumerateFloorVars(IEventCallback<VariableCallbackData>& cb) override
	   {
		   for (auto& v : temp.floorVars)
		   {
			   cb.OnEvent(VariableCallbackData{ v.first.c_str(), v.second });
		   }
	   }

	   void SetWallScriptF32(const fstring& name, float value) override
	   {
		   temp.wallVars[(cstr)name] = value;
	   }

	   void SetFloorScriptF32(const fstring& name, float value) override
	   {
		   temp.floorVars[(cstr)name] = value;
	   }

	   void SetCorridorScriptF32(const fstring& name, float value) override
	   {
		   temp.doorVars[(cstr)name] = value;
	   }

	   void SetTemplateMaterial(const fstring& bodyClass, Graphics::MaterialCategory cat, RGBAb colour, const fstring& persistentId)  override
	   {
		   auto i = temp.nameToMaterials.find((cstr)bodyClass);
		   if (i == temp.nameToMaterials.end())
		   {
			   i = temp.nameToMaterials.insert(std::make_pair(std::string(bodyClass), new Material)).first;
		   }

		   i->second->category = cat;
		   i->second->mvd.colour = colour;
		   SafeFormat(i->second->persistentName, IO::MAX_PATHLEN, "%s", (cstr)persistentId);
		   i->second->mvd.materialId = platform.renderer.GetMaterialId(i->second->persistentName);
		   if (i->second->mvd.materialId < 0)
		   {
			   i->second->mvd.materialId = platform.instances.GetRandomMaterialId(cat);
		   }
	   }

	   int32 CreateFromTemplate(int32 altitude, int32 height) override
	   {
		   struct : MatEnumerator
		   {
			   Sectors* This;
			   virtual void Enumerate(IEventCallback<MaterialArgs>& cb)
			   {
				   for (auto i : This->temp.nameToMaterials)
				   {
					   MaterialArgs args{ i.second, i.first.c_str() };
					   cb.OnEvent(args);
				   }
			   }
		   } mats;

		   mats.This = this;

		   auto* s = CreateSector(platform, *this);
		   s->SetTemplate(mats);

		   try
		   {
			   s->Build(&vertices[0], vertices.size(), 0.01f * altitude, 0.01f * (altitude + height));
			   sectors.push_back(s);
		   }
		   catch (IException&)
		   {
			   s->Free();
			   vertices.clear();
			   throw;
		   }

		   vertices.clear();

		   temp.wallVars.clear();
		   temp.floorVars.clear();
		   temp.doorVars.clear();

		   platform.utilities.ShowBusy(true, "Loading level", "Created sector %u", s->Id());

		   return s->Id();
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

	   ISectorBuilder* Builder()
	   {
		   return this;
	   }

	   char populateScript[IO::MAX_PATHLEN] = { 0 };

	   void BindProperties(IBloodyPropertySetEditor& editor) override
	   {
		   editor.AddPingPath("Populate Script", populateScript, IO::MAX_PATHLEN, "#objects/pop.default.sxy", 140);
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
				   platform.installation.ConvertSysPathToMacroPath(sysPath, populateScript, IO::MAX_PATHLEN, "#objects");
			   }
			   catch (IException&)
			   {

			   }
		   }
	   }

	   virtual void OnEvent(Event& ev)
	   {
		   if (ev == evPopulateSectors)
		   {
			   Populate();
		   }
	   }

	   size_t selectedIndex = -1;

	   virtual size_t GetSelectedSectorId() const
	   {
		   return selectedIndex;
	   }

	   virtual void SelectSector(size_t index)
	   {
		   selectedIndex = index;
	   }

	   void Populate()
	   {
		   struct : IEventCallback<ScriptCompileArgs>, public ISectorEnumerator
		   {
			   Sectors* This;
			   void OnEvent(ScriptCompileArgs& args) override
			   {
				   AddNativeCalls_HVISectorEnumerator(args.ss, this);
				   AddNativeCalls_HVISectorLayout(args.ss, nullptr);
			   }

			   int32 Count() override
			   {
				   return (int32) This->sectors.size();
			   }

			   HV::ISectorLayout* GetSector(int32 index)  override
			   {
				   return This->sectors[index]->Layout();
			   }

			   HV::ISectorLayout* GetSelectedSector() override
			   {
				   if (This->selectedIndex >= This->sectors.size())
				   {
					   return &ANON::nullSectorLayout;
				   }

				   return This->sectors[This->selectedIndex]->Layout();
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

	   HV::ISectorAIBuilder* GetSectorAIBuilder(int32 id)
	   {
		   for (auto s : sectors)
		   {
			   if (s->Id() == (uint32)id)
			   {
				   return &s->GetSectorAIBuilder();
			   }
		   }

		   Throw(0, "Could not find sector with id %d", id);
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
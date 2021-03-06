#include "hv.events.h"
#include <rococo.maths.h>
#include <vector>
#include <rococo.hashtable.h>
#include <algorithm>
#include <rococo.rings.inl>
#include <rococo.variable.editor.h>
#include <rococo.clock.h>
#include <rococo.sexy.api.h>
#include <rococo.random.h>
#include "hv.rings.h"

#include "hv.trianglelist.inl"

#include "hv.sector.perimeter.inl"

namespace HV
{
	ISectorContents* CreateSectorContents(Platform& platform, ISector& sector, ISectorAIBuilderSupervisor& ai);
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Entities;
	using namespace Rococo::Graphics;
	using namespace HV;
	using namespace HVMaths;

	cstr const DEFAULT_WALL_SCRIPT = "#walls/stretch.bricks.sxy";

   uint32 nextSectorId = 1;

   struct Sector : 
	   public ISector,
	   public ICorridor,
	   IMaterialPalette,
	   public IEventCallback<MaterialArgs>,
	   public ISectorLayout
   {
	   IInstancesSupervisor& instances;
	   ISectors& co_sectors;

	   // 2-D map co-ordinates of sector perimeter. Advances clockwise
	   std::vector<Vec2> floorPerimeter;

	   // Indices into floor perimeter for each section of solid wall
	   std::vector<Segment> wallSegments;
	   std::vector<Gap> gapSegments;

	   // Triangle list for the physics and graphics meshes
	   std::vector<VertexTriangle> wallTriangles;
	   std::vector<VertexTriangle> floorTriangles;
	   std::vector<VertexTriangle> ceilingTriangles;

	   AutoFree<IScriptConfigSet> scriptConfig;

	   uint32 id; // unique sector Id

		// One instance for each major mesh of the sector
	   ID_ENTITY floorId;
	   ID_ENTITY ceilingId;
	   ID_ENTITY wallId;

	   float uvScale{ 0.2f };
	   Vec2 uvOffset{ 0,0 };

	   float z0; // Floor height
	   float z1; // Ceiling height (> floor height)

	   int32 altitudeInCm;
	   int32 heightInCm;

	   std::vector<Barrier> barriers; // Barrier 0 is always the door if there is one

	   // N.B we need addresses of material fields to remain constant when map is resized
	   // So use heap generated argument in nameToMaterial. Do not refactor pointer to Material as Material!
	   stringmap<Material*> nameToMaterial;

	   U8FilePath corridorScript = { 0 };
	   U8FilePath wallScript = { 0 };
	   U8FilePath floorScript = { 0 };

	   bool scriptCorridor = false;
	   bool scriptWalls = false;
	   bool scriptFloor = false;

	   std::vector<LightSpec> lights;

	   AABB2d aabb;

	   AutoFree<ISectorAIBuilderSupervisor> ai;
	   AutoFree<ISectorContents> contents;

	   ISectorAIBuilder& GetSectorAIBuilder()
	   {
		   return *ai;
	   }

	   IIActionFactoryCreateContext& AFCC()
	   {
		   return co_sectors.AFCC();
	   }

	   ITagContainer& Tags()
	   {
		   return ai->Tags();
	   }

	   ISectorContents& Contents()
	   {
		   return *contents;
	   }

	   void Altitude(Vec2& altitudes)
	   {
		   altitudes = { z0, z1 };
	   }

	   void ClearManagedEntities() override
	   {
		   contents->ClearManagedEntities();
	   }

	   void DeleteItemsWithMesh(const fstring& prefix) override
	   {
		   contents->DeleteItemsWithMesh(prefix);
	   }

	   boolean32 Exists() override
	   {
		   return true;
	   }

	   void ManageEntity(ID_ENTITY id)  override
	   {
		   contents->ManageEntity(id);
	   }

	   const AABB2d& GetAABB() const override
	   {
		   return aabb;
	   }

	   // Any quads in the scenery that face (0 0 1) marked for utility
	   // by appending into scenery.levelSurfaces stack
	   void UseUpFacingQuads(ID_ENTITY id) override
	   {
		   contents->UseUpFacingQuadsOnScenery(id);
	   }

	   bool TryPlaceItemOnQuad(const Quad& qModel, ID_ENTITY quadsEntityId, ID_ENTITY itemId)
	   {
		   return contents->TryPlaceItemOnQuad(qModel, quadsEntityId, itemId);
	   }

	   boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) override
	   {
		   return contents->PlaceItemOnUpFacingQuad(id);
	   }

	   boolean32 TryGetAsRectangle(GuiRectf& rect) override
	   {
		   return HV::TryGetAsRectangle(rect, floorPerimeter);
	   }

	   void SyncEnvironmentMapToSector() override
	   {
		   int32 wallId     = (int32) nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Brickwork)->second->mvd.materialId;
		   int32 groundId   = (int32) nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Floor)->second->mvd.materialId;
		   int32 ceilingId  = (int32) nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Ceiling)->second->mvd.materialId;

		   platform.renderer.SyncCubeTexture(wallId, wallId, wallId, wallId, groundId, ceilingId);
	   }

	   const LightSpec* Lights(size_t& numberOfLights) const override
	   {
		   numberOfLights = lights.size();
		   return !lights.empty() ? &lights[0] : nullptr;
	   }

	   bool TryGetMaterial(BodyComponentMatClass name, MaterialVertexData& vd) const override
	   {
		   auto i = nameToMaterial.find(name);
		   if (i == nameToMaterial.end())
		   {
			   vd.colour = RGBAb(0, 0, 0, 255);
			   vd.materialId = -1;
			   return false;
		   }
		   else
		   {
			   vd = i->second->mvd;
			   return true;
		   }
	   }

	   void SetTemplate(MatEnumerator& enumerator) override
	   {
		   enumerator.Enumerate(*this);
	   }

	  const Barrier* Barriers(size_t& barrierCount) const override
	  {
		  if (!barriers.empty())
		  {
			  if (!contents->TraversalBlocked())
			  {
				  barrierCount = 0;
				  return nullptr;
			  }
		  }
		  barrierCount = barriers.size();
		  return barriers.empty() ? nullptr : &barriers[0];
	  }

      float Z0() const override
      {
         return z0;
      }

      float Z1() const override
      {
         return z1;
      }
      
      Segment GetSegment(Vec2 p, Vec2 q) override
      {
		  return HV::GetSegment(p, q, floorPerimeter);
      }

      virtual const Segment* GetWallSegments(size_t& count) const
      {
         count = wallSegments.size();
         return wallSegments.empty() ? nullptr : &wallSegments[0];
      }

      const Gap* Gaps(size_t& count) const
      {
         if (gapSegments.empty())
         {
            count = 0;
            return nullptr;
         }
         else
         {
            count = gapSegments.size();
            return &gapSegments[0];
         }
      }

	  void UpdatedWallGraphicMesh()
	  {
		  char name[32];
		  SafeFormat(name, sizeof(name), "sector.%u.walls", id);

		  auto& mb = instances.MeshBuilder();
		  mb.Clear();
		  mb.Begin(to_fstring(name));

		  for (auto& t : wallTriangles)
		  {
			  mb.AddTriangle(t.a, t.b, t.c);
		  }

		  mb.End(true, false);

		  if (!wallId)
		  {
			  wallId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		  }
		  else
		  {
			  auto entity = instances.GetEntity(wallId);
			  ID_SYS_MESH meshId;
			  AABB bounds;
			  platform.meshes.TryGetByName(name, meshId, bounds);
			  entity->SetMesh(meshId);
		  }
	  }

	  bool IsCorridor() const
	  {
		  if (!Is4PointRectangular()) return false;
		  if (gapSegments.size() != 2) return false;
		  if (gapSegments[0].other->Is4PointRectangular() || gapSegments[1].other->Is4PointRectangular())
		  {
			  return false;
		  }
		  Vec2 ab = gapSegments[0].b - gapSegments[0].a;
		  Vec2 cd = gapSegments[1].b - gapSegments[1].a;
		  return Dot(ab, cd) != 0;
	  }

	  const Gap* GetGapAtSegment(const Vec2& a, const Vec2& b) const
	  {
		  for (auto& g : gapSegments)
		  {
			  if (g.a == a && g.b == b)
			  {
				  return &g;
			  }
		  }

		  return nullptr;
	  }

	  typedef std::unordered_map<ISector*, uint32> TDirtySectors;

	  void CorrectOppositeGap(const Gap& notThisOne, TDirtySectors& dirty)
	  {
		  if (!IsCorridor())
		  {
			  Throw(0, "Expecting corridor at sector %u", id);
		  }

		  Gap& g = (gapSegments[0].a == notThisOne.a) ? gapSegments[1] : gapSegments[0];
		 
		  g.z0 = g.other->Z0();
		  g.z1 = g.other->Z1();

		  auto& other = *(Sector*) g.other;
		  auto* otherGap = const_cast<Gap*>(other.GetGapAtSegment(g.b, g.a));

		  if (otherGap == nullptr)
		  {
			 Throw(0, "Expecting gap sector %u from %u", other.Id(), Id());
		  }

		  otherGap->z0 = g.z0;
		  otherGap->z1 = g.z1;

		  MakeBounds(g);
		  MakeBounds(*otherGap);

		  otherGap->bounds = g.bounds;

		  dirty[&other] = 0;
	  }

	  void TesselateWalls()
	  {
		  auto walls = nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Brickwork);
		  TesselateWallsFromSegments(walls->second->mvd);
	  }

	  void MakeBounds(Gap& g)
	  {
		  Vec3 topLeft = { g.a.x, g.a.y, g.z1 };
		  Vec3 bottomRight = { g.b.x, g.b.y, g.z0 };
		  Vec3 centre = 0.5f * (topLeft + bottomRight);
		  Metres radius{ Length(topLeft - centre) };
		  g.bounds = Sphere{ centre, radius };
	  }

	  void FinalizeGaps()
	  {
		  TDirtySectors dirtList;

		  for (auto& g : gapSegments)
		  {
			  g.z0 = g.other->Z0();
			  g.z1 = g.other->Z1();

			  MakeBounds(g);

			  auto* otherGap = const_cast<Gap*>(g.other->GetGapAtSegment(g.b, g.a));
			  if (!otherGap)
			  {
				  Throw(0, "Expecting gap in sector %u to sector %u", g.other->Id(), id);
			  }

			  otherGap->bounds = g.bounds;

			  if (IsCorridor())
			  {
				  otherGap->z0 = g.z0;
				  otherGap->z1 = g.z1;
			  }
			  else
			  {
				  otherGap->z0 = Z0();
				  otherGap->z1 = Z1();

				  if (g.other->IsCorridor())
				  {
					  ((Sector*)g.other)->CorrectOppositeGap(*otherGap, dirtList);
				  }
			  }

			  MakeBounds(*otherGap);

			  dirtList[g.other] = 0;
		  }

		  for (auto& politician : dirtList)
		  {
			  politician.first->Rebuild();
		  }
	  }

      float AddWallSegment(const Vec2& p, const Vec2& q, float h0, float h1, float u, const MaterialVertexData& brickwork)
      {
         Vec3 up{ 0, 0, 1 };
         Vec3 P0 = { p.x, p.y, h0 };
         Vec3 Q0 = { q.x, q.y, h0 };
         Vec3 P1 = { p.x, p.y, h1 };
         Vec3 Q1 = { q.x, q.y, h1 };

         Vec3 delta = Q0 - P0;

         float segmentLength = round(Length(delta));

         Vec3 normal = Normalize( Cross(delta, up) );

         ObjectVertex PV0, PV1, QV0, QV1;

         PV0.position = P0;
         PV1.position = P1;
         QV0.position = Q0;
         QV1.position = Q1;

         PV0.normal = PV1.normal = QV0.normal = QV1.normal = normal;
		 PV0.material = PV1.material = QV0.material = QV1.material = brickwork;

         PV0.uv.y = QV0.uv.y = uvScale * h0;
         PV1.uv.y = QV1.uv.y = uvScale * h1;
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

         return u;
      }

	  IPropertyHost* host = nullptr;

	  void Assign(IPropertyHost* host) override
	  {
		  if (!deleting)
		  {
			  this->host = host;

			  for (auto& mb : nameToMaterial)
			  {
				  try
				  {
					  if (*mb.second->persistentName == '!' || *mb.second->persistentName == '#')
					  {
						  mb.second->mvd.materialId = platform.instances.GetMaterialDirect(to_fstring(mb.second->persistentName));
						  continue;
					  }
				  }
				  catch (IException&)
				  {
				  }

				  mb.second->mvd.materialId = platform.instances.GetRandomMaterialId(mb.second->category);
			  }

			  InvokeSectorRebuild(false);
		  }
	  }

      float AddSlopedWallSegment(const Vec2& p, const Vec2& q, float pFloor, float qFloor, float pCeiling, float qCeiling, float u, const MaterialVertexData& brickwork)
      {
         Vec3 up{ 0, 0, 1 };
         Vec3 P0 = { p.x, p.y, pFloor };
         Vec3 Q0 = { q.x, q.y, qFloor };
         Vec3 P1 = { p.x, p.y, pCeiling };
         Vec3 Q1 = { q.x, q.y, qCeiling };

         Vec3 delta = Q0 - P0;

         float segmentLength = round(Length(delta));

         Vec3 normal = Cross(delta, up);

         ObjectVertex PV0, PV1, QV0, QV1;

         PV0.position = P0;
         PV1.position = P1;
         QV0.position = Q0;
         QV1.position = Q1;

         PV0.normal = PV1.normal = QV0.normal = QV1.normal = Normalize(normal);
		 PV0.material = PV1.material = QV0.material = QV1.material = brickwork;

         PV0.uv.y = uvScale * pFloor;
         QV0.uv.y = uvScale * qFloor;
         PV1.uv.y = uvScale * pCeiling;
         QV1.uv.y = uvScale * qCeiling;
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

         return u;
      }

      void RaiseSlopeBetweenGaps(const MaterialVertexData& brickwork)
      {
         float h00, h01, h10, h11;

         Vec2 p = floorPerimeter[wallSegments[0].perimeterIndexStart];
         Vec2 q = floorPerimeter[wallSegments[0].perimeterIndexEnd];

         if (gapSegments[0].a == p || gapSegments[0].b == p)
         {
            h00 = gapSegments[0].z0;
            h01 = gapSegments[1].z0;
            h10 = gapSegments[0].z1;
            h11 = gapSegments[1].z1;
         }
         else
         {
            h00 = gapSegments[1].z0;
            h01 = gapSegments[0].z0;
            h10 = gapSegments[1].z1;
            h11 = gapSegments[0].z1;
         }

         AddSlopedWallSegment(p, q, h00, h01, h10, h11, 0.0f, brickwork);

         p = floorPerimeter[wallSegments[1].perimeterIndexStart];
         q = floorPerimeter[wallSegments[1].perimeterIndexEnd];

         AddSlopedWallSegment(p, q, h01, h00, h11, h10, 0.0f, brickwork);
      }

	  bool RunSectorGenWallScript()
	  {
		  if (wallSegments.empty()) return false;

		  // Script has to iterate through wallSegments and append to wallTriangles
		  struct ANON: 
			  IEventCallback<ScriptCompileArgs>,
			  public ISectorWallTesselator,
			  public ISectorComponents,
			  public TriangleListBinding
		  {
			  Sector* This;

			  ANON(Sector* _This): This(_This), TriangleListBinding(_This->wallTriangles)
			  {
			  }

			  int32 NumberOfSegments() override
			  {
				  return (int32) This->wallSegments.size();
			  }

			  int32 NumberOfGaps() override
			  {
				  return (int32) This->gapSegments.size();
			  }

			  void GetSegment(int32 index, HV::WallSegment& segment) override
			  {
				  This->GetSegment(index, segment);
			  }

			  void GetGap(int32 index, GapSegment& segment) override
			  {
				  This->GetGap(index, segment);
			  }

			  ITriangleList* WallTriangles() override
			  {
				  return this;
			  }

			  void OnEvent(ScriptCompileArgs& args) override
			  {
				  This->wallTriangles.clear();
				  
				  HV::AddMathsEx(args.ss);

				  AddNativeCalls_HVISectorWallTesselator(args.ss, this);
				  AddNativeCalls_HVISectorComponents(args.ss, this);
				  AddNativeCalls_HVITriangleList(args.ss, this);
				  AddNativeCalls_HVIScriptConfig(args.ss, &This->scriptConfig->Current());
				  AddNativeCalls_HVISectorEnumerator(args.ss, &This->co_sectors.Enumerator());
				  AddNativeCalls_HVISectorLayout(args.ss, nullptr);
			  }

			  void GetMaterial(MaterialVertexData& mat, const fstring& componentClass) override
			  {
				  if (!This->TryGetMaterial(componentClass, mat))
				  {
					  Throw(0, "ISectorWallTesselator::GetMaterial(...) Unknown component class: %s", (cstr)componentClass);
				  }
			  }

			  std::string localName;
			  std::string meshName;

			  void AddTriangle(const VertexTriangle& t) override
			  {
				  This->platform.meshes.AddTriangleEx(t);
			  }

			  void AddPhysicsHull(const Triangle& t) override
			  {
				  This->platform.meshes.AddPhysicsHull(t);
			  }

			  void BuildComponent(const fstring& componentName) override
			  {
				  this->localName = componentName;

				  char fullMeshName[256];
				  SafeFormat(fullMeshName, "sector.%d.%s", This->id, (cstr)componentName);

				  this->meshName = fullMeshName;

				  This->platform.meshes.Clear();
				  This->platform.meshes.Begin(to_fstring(fullMeshName));
			  }

			  void ClearComponents(const fstring& componentName) override
			  {
				  This->contents->ClearComponents(componentName);
			  }

			  void CompleteComponent(boolean32 preserveMesh)  override
			  {
				  This->platform.meshes.End(preserveMesh, false);
				  This->contents->AddComponent(Matrix4x4::Identity(), localName.c_str(), meshName.c_str());
			  }
		  } scriptCallback(this);  

		  try
		  {
			  cstr theWallScript = *wallScript ? wallScript : DEFAULT_WALL_SCRIPT;
			  scriptConfig->SetCurrentScript(theWallScript);
			  platform.utilities.RunEnvironmentScript(scriptCallback, id, theWallScript, true, false, false);
			  return true;
		  }
		  catch (IException& ex)
		  {
			  char title[256];
			  SafeFormat(title, "sector %u: %s failed", id, wallScript);
			  platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, title);
			  return false;
		  }
	  }

	  bool RunSectorGenFloorAndCeilingScript()
	  {
		  if (wallSegments.empty()) return false;

		  // Script has to iterate through wallSegments and append to wallTriangles
		  struct ANON : IEventCallback<ScriptCompileArgs>, public HV::ISectorFloorTesselator, public ISectorComponents
		  {
			  Sector* This;
			  float uvScale = 1.0f;
			  std::string localName;
			  std::string meshName;

			  ANON(Sector* _This) : This(_This)
			  {
			  }

			  int32 NumberOfSegments() override
			  {
				  return This->NumberOfSegments();
			  }

			  int32 NumberOfGaps() override
			  {
				  return This->NumberOfGaps();
			  }

			  void GetSegment(int32 segIndex, HV::WallSegment& segment) override
			  {
				  This->GetSegment(segIndex, segment);
			  }

			  void GetGap(int32 gapIndex, HV::GapSegment& segment)  override
			  {
				  This->GetGap(gapIndex, segment);
			  }

			  int32 NumberOfSquares() override
			  {
				  return (int32) This->completeSquares.size();
			  }

			  void GetSquare(int32 index, AABB2d& sq) override
			  {
				  return This->GetSquare(index, sq);
			  }

			  void CeilingQuad(int32 index, QuadVertices& q) override
			  {
				  This->CeilingQuad(index, q);
			  }

			  void SetUVScale(float scale)
			  {
				  uvScale = scale;
			  }

			  void FloorQuad(int32 index, QuadVertices& q) override
			  {
				  This->FloorQuad(index, q);
			  }

			  boolean32 FoundationsExist() override
			  {
				  return !HV::IsAxisAlignedRectangular(This->floorPerimeter);
			  }

			  void AddCeilingTriangle(const VertexTriangle& t) override
			  {
				  This->ceilingTriangles.push_back(t);
			  }

			  void AddFloorTriangle(const VertexTriangle& t) override
			  {
				  This->floorTriangles.push_back(t);
			  }

			  void OnEvent(ScriptCompileArgs& args) override
			  {
				  // N.B we reset mesh here, so that if the debug loop takes us here repeatedly
				  // we clean up the damage from the previous script crash
				  This->floorTriangles.clear();
				  This->ceilingTriangles.clear();

				  if (!HV::IsAxisAlignedRectangular(This->floorPerimeter))
				  {
					  This->TesselateFloorAndCeiling();
				  }

				  HV::AddMathsEx(args.ss);
				  AddNativeCalls_HVISectorFloorTesselator(args.ss, this);
				  AddNativeCalls_HVISectorComponents(args.ss, this);
				  AddNativeCalls_HVIScriptConfig(args.ss, &This->scriptConfig->Current());
				  AddNativeCalls_HVISectorEnumerator(args.ss, &This->co_sectors.Enumerator());
				  AddNativeCalls_HVISectorLayout(args.ss, nullptr);
			  }

			  void GetMaterial(MaterialVertexData& mat, const fstring& componentClass) override
			  {
				  if (Eq(GraphicsEx::BodyComponentMatClass_Physics_Hull, componentClass))
				  {
					  mat.colour = RGBAb(255, 0, 0, 0);
					  mat.gloss = 1.1f;
					  mat.materialId = -1;
				  }
				  else if (!This->TryGetMaterial(componentClass, mat))
				  {
					  Throw(0, "SectorFloorTesselator::GetMaterial(...) Unknown component class: %s", (cstr)componentClass);
				  }
			  }

			  void AddTriangle(const VertexTriangle& t) override
			  {
				  This->platform.meshes.AddTriangleEx(t);
			  }

			  void AddPhysicsHull(const Triangle& t) override
			  {
				  This->platform.meshes.AddPhysicsHull(t);
			  }

			  void BuildComponent(const fstring& componentName) override
			  {
				  this->localName = componentName;

				  char fullMeshName[256];
				  SafeFormat(fullMeshName, 256, "sector.%d.%s", This->id, (cstr)componentName);

				  this->meshName = fullMeshName;

				  This->platform.meshes.Clear();
				  This->platform.meshes.Begin(to_fstring(fullMeshName));
			  }

			  void ClearComponents(const fstring& componentName) override
			  {
				  This->contents->ClearComponents(componentName);
			  }

			  void CompleteComponent(boolean32 preserveMesh) override
			  {
				  This->platform.meshes.End(preserveMesh, false);
				  This->contents->AddComponent(Matrix4x4::Identity(), localName.c_str(), meshName.c_str());
			  }

		  } scriptCallback(this);

		  try
		  {
			  cstr theFloorScript = *floorScript ? floorScript : "#floors/square.mosaics.sxy";
			  scriptConfig->SetCurrentScript(theFloorScript);
			  platform.utilities.RunEnvironmentScript(scriptCallback, id, theFloorScript, true, false);
			  return true;
		  }
		  catch (IException& ex)
		  {
			  char title[256];
			  SafeFormat(title, 256, "sector %u: %s failed", id, wallScript);
			  platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, title);
			  return false;
		  }
	  }

	  void CreateColumnAt(float z0, float z1, Vec2 posXY, const MaterialVertexData& brickwork)
	  {
		  auto radius = 0.4_metres; // width

		  auto& rod = platform.tesselators.rod;
		  rod.Clear();

		  float z0min = z0;
		  float z1max = z1;

		  for (auto& gap : gapSegments)
		  {
			  z0min = min(z0min, gap.z0);
			  z1max = max(z1max, gap.z1);
		  }

		  z0 = z0min;
		  z1 = z1max;

		  auto height = Metres{ z1 - z0 };
		  rod.SetMaterialMiddle(brickwork);

		  rod.SetOrigin(Vec3{ posXY.x, posXY.y, z0 });
		  rod.UseSmoothNormals();

		  if (!IsSloped())
		  {
			  rod.AddTube(0.10_metres, Metres{ radius + 0.2_metres }, Metres{ radius + 0.1_metres }, 12);
			  rod.AddTube(Metres{ height - 0.3_metres }, radius, radius, 8);
			  rod.AddTube(Metres{ 0.2_metres }, Metres{ radius + 0.1_metres }, Metres{ radius + 0.3_metres }, 8);
		  }
		  else
		  {
			  rod.AddTube(height, radius, radius, 8);
		  }

		  VertexTriangle t;
		  while (rod.PopNextTriangle(t))
		  {
			  wallTriangles.push_back(t);
		  }
	  }

	  void CreateColumnsForGap(const Gap& gap, const MaterialVertexData& brickwork)
	  {
		  // Compare the memory address of the sector on the other side of the gap 
		  if (gap.other < this)
		  {
			  return; // Not our job to create the column. The other side will handle it
		  }

		  float zMin = min(z0, gap.other->Z0());
		  float zMax = max(z1, gap.other->Z1());

		  // Our column hides the joint between sectors, so extends as high and as low as needed.

		  CreateColumnAt(z0, z1, gap.a, brickwork);
		  CreateColumnAt(z0, z1, gap.b, brickwork);
	  }

	  void CreateGapColumns(const MaterialVertexData& brickwork)
	  {
		  for (auto& gap : gapSegments)
		  {
			  CreateColumnsForGap(gap, brickwork);
		  }
	  }

	  void CreateFlatWallsBetweenGaps(const MaterialVertexData& brickwork)
	  {
		  float u = 0;

		  if (scriptWalls && RunSectorGenWallScript())
		  {

		  }
		  else
		  {
			  for (auto segment : wallSegments)
			  {
				  Vec2 p = floorPerimeter[segment.perimeterIndexStart];
				  Vec2 q = floorPerimeter[segment.perimeterIndexEnd];

				  u = AddWallSegment(p, q, z0, z1, u, brickwork);
			  }
		  }

		  for (auto& gap : gapSegments)
		  {
			  float foreignHeight = gap.z1;
			  float currentHeight = z1;

			  if (foreignHeight < currentHeight && !(gap.other->IsCorridor() || IsCorridor()))
			  {
				  if (foreignHeight < z0)
				  {
					  foreignHeight = z0;
				  }

				  Vec2 p = gap.a;
				  Vec2 q = gap.b;

				  AddWallSegment(p, q, foreignHeight, z1, 0, brickwork);
			  }

			  float foreignFloorHeight = gap.z0;
			  if (foreignFloorHeight > z0 && !(gap.other->IsCorridor() || IsCorridor()))
			  {
				  if (foreignFloorHeight > z1)
				  {
					  foreignFloorHeight = z1;
				  }

				  Vec2 p = gap.a;
				  Vec2 q = gap.b;

				  AddWallSegment(p, q, z0, foreignFloorHeight, 0, brickwork);
			  }

			  if (gap.other->IsCorridor())
			  {
				  gap.z0 = z0;
				  gap.z1 = z1;
				  MakeBounds(gap);
			  }
		  }
	  }

      void TesselateWallsFromSegments(const MaterialVertexData& brickwork)
      {
         wallTriangles.clear();

         bool isCorridor = IsCorridor();
         if (isCorridor)
         {
            RaiseSlopeBetweenGaps(brickwork);
         }
         else
         {
			 CreateFlatWallsBetweenGaps(brickwork);
         }

		 CreateGapColumns(brickwork);

		 isDirty = true;
      }

      Platform& platform;

	  void PrepMat(BodyComponentMatClass bcmc, cstr persistentName, Graphics::MaterialCategory cat)
	  {
		  auto mat = new Material;
		  mat->category = cat;
		  memcpy(mat->persistentName, persistentName, IO::MAX_PATHLEN);
		  mat->mvd.colour = RGBAb(0, 0, 0, 0);
		  mat->mvd.materialId = platform.renderer.GetMaterialId(persistentName);
		  if (mat->mvd.materialId < 0)
		  {
			  mat->mvd.materialId = platform.instances.GetRandomMaterialId(cat);
			  mat->mvd.colour.red = rand() % 256;
			  mat->mvd.colour.green = rand() % 256;
			  mat->mvd.colour.blue = rand() % 256;
			  mat->mvd.colour.alpha = 191 + rand() % 64;
		  }

		  nameToMaterial[bcmc] = mat;
	  }

	  Graphics::MaterialCategory RandomWoodOrMetal()
	  {
		  return  ((rand() % 2) == 0) ? Graphics::MaterialCategory_Metal : Graphics::MaterialCategory_Wood;
	  }

	  Graphics::MaterialCategory RandomRockOrMarble()
	  {
		  return  ((rand() % 2) == 0) ? Graphics::MaterialCategory_Rock : Graphics::MaterialCategory_Marble;
	  }
   public:
      Sector(Platform& _platform, ISectors& _co_sectors) :
         instances(_platform.instances),
         id(nextSectorId++),
         platform(_platform),
         co_sectors(_co_sectors),
		 scriptConfig(CreateScriptConfigSet()),
		 ai(CreateSectorAI(co_sectors.AFCC())),
		 contents(CreateSectorContents(_platform, *this, *ai))
      {
		  PrepMat(GraphicsEx::BodyComponentMatClass_Brickwork, "random", Graphics::MaterialCategory_Stone);
		  PrepMat(GraphicsEx::BodyComponentMatClass_Cement,    "random", Graphics::MaterialCategory_Rock);
		  PrepMat(GraphicsEx::BodyComponentMatClass_Floor,     "random", RandomRockOrMarble());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Ceiling,   "random", RandomRockOrMarble());

		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Mullions, "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Rails,   "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Panels,  "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Casing,  "random", RandomWoodOrMetal());

		  cstr wscript = co_sectors.Builder()->GetTemplateWallScript(scriptWalls);
		  Format(wallScript, "%s", wscript);

		  struct VariableEnumerator : IEventCallback<VariableCallbackData>
		  {
			  Sector& sector;
			  void OnEvent(VariableCallbackData& v) override
			  {
				  sector.scriptConfig->SetVariable(v.name, v.value);
			  }

			  VariableEnumerator(Sector& _sector) : sector(_sector) {}
		  };

		  if (wscript)
		  {
			  VariableEnumerator foreachWallVariable(*this);
			  scriptConfig->SetCurrentScript(wscript);
			  co_sectors.Builder()->EnumerateWallVars(foreachWallVariable);
		  }

		  cstr cscript = co_sectors.Builder()->GetTemplateDoorScript(scriptCorridor);
		  Format(corridorScript, "%s", cscript);

		  if (cscript)
		  {
			  VariableEnumerator foreachCorridorVariable(*this);
			  scriptConfig->SetCurrentScript(cscript);
			  co_sectors.Builder()->EnumerateWallVars(foreachCorridorVariable);
		  }

		  cstr fscript = co_sectors.Builder()->GetTemplateFloorScript(scriptFloor);
		  Format(floorScript, "%s", fscript);

		  if (fscript)
		  {
			  VariableEnumerator foreachFloorVariable(*this);
			  scriptConfig->SetCurrentScript(fscript);
			  co_sectors.Builder()->EnumerateWallVars(foreachFloorVariable);
		  }
      }

	  void OnEvent(MaterialArgs& args)
	  {
		  auto i = nameToMaterial.find(args.bcmc);
		  if (i == nameToMaterial.end())
		  {
			  i = nameToMaterial.insert(args.bcmc, new Material()).first;
		  }

		  *i->second = *args.mat;
	  }

	  uint32 Id() const override
	  {
		  return id;
	  }

	  void DeleteScenery() override
	  {
		  contents->DeleteScenery();
	  }

      void DeleteFloor()
      {
         if (floorId)
         {
            instances.Delete(floorId);
            floorId = ID_ENTITY::Invalid();

            char name[32];
            SafeFormat(name, sizeof(name), "sector.%u.floor", id);
            instances.MeshBuilder().Delete(to_fstring(name));
         }
      }

      void DeleteCeiling()
      {
         if (ceilingId)
         {
            instances.Delete(ceilingId);
            ceilingId = ID_ENTITY::Invalid();

            char name[32];
            SafeFormat(name, sizeof(name), "sector.%u.ceiling", id);
            instances.MeshBuilder().Delete(to_fstring(name));
         }
      }

      void DeleteWalls()
      {
		  if (wallId)
		  {
			  char name[32];
			  SafeFormat(name, sizeof(name), "sector.%u.walls", id);
			  instances.Delete(wallId);
			  instances.MeshBuilder().Delete(to_fstring(name)); 
		  }
      }

	  bool deleting = false;

	  ~Sector()
	  {
		  Decouple();
		  DeleteFloor();
		  DeleteCeiling();
		  DeleteWalls();

		  if (host)
		  {
			  deleting = true;
			  host->SetPropertyTarget(nullptr);
		  }

		  for (auto m : nameToMaterial)
		  {
			  delete m.second;
		  }
	  }

      virtual ObjectVertexBuffer FloorVertices() const
      {
         return{ &floorTriangles[0].a, 3 * floorTriangles.size() };
      }

      RGBAb GetGuiColour(float intensity) const override
      { 
         intensity = max(0.0f, intensity);
         intensity = min(1.0f, intensity);

		 Rococo::Random::RandomMT mt;
		 Rococo::Random::Seed(mt, id + 1);

		 uint32 index = id % 3;

         uint32 rgb[3] = { 0 };

         rgb[index] = mt() % 256;
		 rgb[(index+1)%3] = mt() % 256;
		 rgb[(index+2)%3] = mt() % 256;

		 uint32 sum = rgb[0] + rgb[1] + rgb[2];
		 if (sum > 512)
		 {
			 rgb[index] = 0;
		 }

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

      int32 GetPerimeterIndex(Vec2 a) const override
      {
		  return HV::GetPerimeterIndex(a, floorPerimeter);
      }

      bool DoesLineCrossSector(Vec2 a, Vec2 b) override
      {
		 int32 index = GetFloorTriangleIndexContainingPoint(b);
		 if (index >= 0)
		 {
			 return true;
		 }

         size_t nVertices = floorPerimeter.size();
         for (size_t i = 0; i <= nVertices; ++i)
         {
            auto c = GetRingElement(i, &floorPerimeter[0], nVertices);
            auto d = GetRingElement(i + 1, &floorPerimeter[0], nVertices);

            float t, u;
            if (GetLineIntersect(a, b, c, d, t, u))
            {
               if (u > 0 && u < 1 && t > 0 && t <= 1)
               {
                  return true;
               }
            }
         }

         return false;
      }

	  void UpdateFloorGraphicMesh()
	  {
		  char name[32];
		  SafeFormat(name, sizeof(name), "sector.%u.floor", id);

		  auto& mb = instances.MeshBuilder();
		  mb.Begin(to_fstring(name));

		  for (auto& t : floorTriangles)
		  {
			  if (t.a.material.gloss <= 1.0f)
			  {
				  mb.AddTriangle(t.a, t.b, t.c);
			  }
		  }

		  mb.End(true, false);

		  struct
		  {
			  bool operator()(const VertexTriangle& t) const
			  {
				  return t.a.material.gloss <= 1.0f;
			  }
		  } if_floor_glossy;

		  auto i = std::remove_if(floorTriangles.begin(), floorTriangles.end(), if_floor_glossy);
		  floorTriangles.erase(i, floorTriangles.end());

		  if (!floorId)
		  {
			  floorId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		  }
		  else
		  {
			  ID_SYS_MESH meshId;
			  AABB bounds;
			  platform.meshes.TryGetByName(name, meshId, bounds);
			  auto* entity = instances.GetEntity(floorId);
			  entity->SetMesh(meshId);
		  }
	  }

      void UpdateCeilingGraphicMesh()
      {
         char name[32];
         SafeFormat(name, sizeof(name), "sector.%u.ceiling", id);

         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (auto& t : ceilingTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End(true, false);

		 if (!ceilingId)
		 {
			 ceilingId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		 }
		 else
		 {
			 ID_SYS_MESH meshId;
			 AABB bounds;
			 platform.meshes.TryGetByName(name, meshId, bounds);
			 auto* entity = instances.GetEntity(ceilingId);
			 entity->SetMesh(meshId);
		 }
      }

      void GetVerticalMetrics(const Vec2& perimeterVertex, float& z0, float& z1)
      {
         if (IsCorridor())
         {
            if (gapSegments[0].a == perimeterVertex || gapSegments[0].b == perimeterVertex)
            {
               z0 = gapSegments[0].z0;
               z1 = gapSegments[0].z1;
            }
            else
            {
               z0 = gapSegments[1].z0;
               z1 = gapSegments[1].z1;
            }
         }
         else
         {
            z0 = this->z0;
            z1 = this->z1;
         }
      }

      void GetSpan(Vec3& span) override
      {
         // Return span of corridor in generation co-ordinates
         // In such a system:
         //                  the corridor faces North, 
         //                  0 is ground height, that runs West to East (x=0) mid-way North to South (Y = 0)
         //                  Z is up

         if (gapSegments.size() != 2) Throw(0, "Expecting two gaps in sector");

         auto& g = gapSegments[0];
         auto& h = gapSegments[1];

         if (g.a.x - g.b.x == 0)
         {
            // corridor is West-East. 
            span.x = fabsf(g.a.y - g.b.y);
            span.y = fabsf(g.a.x - h.a.x);
         }
         else
         {
            // corridor is North-South
            span.x = fabsf(g.a.x - g.b.x);
            span.y = fabsf(g.a.y - h.a.y);
         }

         if (IsSloped())
         {
            span.z = fabsf(0.5f * (g.z1 + h.z1) - 0.5f* (g.z0 + h.z0));
         }
         else
         {
            span.z = z1 - z0;
         }
      }

      boolean32 IsSloped() override
      {
		  return (gapSegments.size() > 1) &&
			  (
				  (gapSegments[0].z0 != gapSegments[1].z0) ||
				  (gapSegments[0].z1 != gapSegments[1].z1)
			  );
      }

      void CorridorModelMatrix(Matrix4x4& model)
      {
         Vec2 centre = 0.5f * (floorPerimeter[0] + floorPerimeter[2]);
         float z = IsSloped() ? (0.5f * (gapSegments[0].z0 + gapSegments[1].z0)) : z0;

         auto& g = gapSegments[0];
         auto& h = gapSegments[0];

         Matrix4x4 Rz;

         if (g.a.x - g.b.x == 0)
         {
            // corridor is West-East, so rotate the component 90 degrees
            Rz = Matrix4x4::RotateRHAnticlockwiseZ(90_degrees);
         }
         else
         {
            Rz = Matrix4x4::Identity();
         }

		 model = Matrix4x4::Translate({ centre.x, centre.y, z }) * Rz; 
      }

      virtual void GetComponentMeshName(const fstring& componentName, Rococo::IStringPopulator& meshName)
      {
         char fullMeshName[256];
         SafeFormat(fullMeshName, "sector.%d.%s", id, (cstr)componentName);
         meshName.Populate(fullMeshName);
      }

	  const Triangle* GetPhysicsHull(const fstring& componentName, size_t& nTriangles) const
	  {
		  char fullMeshName[256];
		  SafeFormat(fullMeshName, "sector.%d.%s", id, (cstr)componentName);

		  ID_SYS_MESH id;
		  AABB bounds;
		  platform.meshes.TryGetByName(fullMeshName, id, bounds);

		  return platform.meshes.GetPhysicsHull(id, nTriangles);
	  }

      void RunGenCorridorScript()
      {
         struct : IEventCallback<ScriptCompileArgs>, public ICorridor, public ISectorComponents
         {
            Sector *This;
			std::string localName;
			std::string meshName;

            void OnEvent(ScriptCompileArgs& args) override
            {
				HV::AddMathsEx(args.ss);
               AddNativeCalls_HVICorridor(args.ss, this);
			   AddNativeCalls_HVISectorComponents(args.ss, this);
			   AddNativeCalls_HVISectorEnumerator(args.ss, &This->co_sectors.Enumerator());
			   AddNativeCalls_HVISectorLayout(args.ss, nullptr);
            }

			void GetSpan(Vec3& span) override
			{
				return This->GetSpan(span);
			}

			boolean32 IsSloped()  override
			{
				return This->IsSloped();
			}

			void AddTriangle(const VertexTriangle& t) override
			{
				This->platform.meshes.AddTriangleEx(t);
			}

			void BuildComponent(const fstring& componentName) override
			{
				this->localName = componentName;

				char fullMeshName[256];
				SafeFormat(fullMeshName, "sector.%d.%s", This->id, (cstr)componentName);

				this->meshName = fullMeshName;

				This->platform.meshes.Clear();
				This->platform.meshes.Begin(to_fstring(fullMeshName));
			}

			void AddPhysicsHull(const Triangle& t) override
			{
				This->platform.meshes.AddPhysicsHull(t);
			}

			void ClearComponents(const fstring& componentName) override
			{
				This->contents->ClearComponents(componentName);
			}

			void CompleteComponent(boolean32 preserveMesh) override
			{
				This->platform.meshes.End(preserveMesh, false);

				Matrix4x4 model;
				This->CorridorModelMatrix(model);
				This->contents->AddComponent(model, localName.c_str(), meshName.c_str());
			}

			void GetMaterial(MaterialVertexData& mat, const fstring& componentClass) override
			{
				if (!This->TryGetMaterial(componentClass, mat))
				{
					Throw(0, "ISectorComponents::GetMaterial(...) Unknown component class: %s", (cstr)componentClass);
				}
			}
         } scriptCallback;
         scriptCallback.This = this;
		 
		 cstr genCorridor = *corridorScript.buf ? corridorScript : "#corridor/gen.door.sxy";

		 try
		 {	
			 platform.utilities.RunEnvironmentScript(scriptCallback, id, genCorridor, true, false);
		 }
		 catch (IException& ex)
		 {
			 char title[256];
			 SafeFormat(title, "sector %u: %s failed", id, genCorridor);
			 platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, title);
		 }
      }

	  void ResetBarriers()
	  {
		  barriers.clear();
		
		  if (IsCorridor() && scriptCorridor)
		  {
			  Barrier b{};
			  auto& g = gapSegments[0];
			  auto& h = gapSegments[1];

			  b.z0 = 0.5f * (g.z0 + h.z0);
			  b.z1 = 0.5f * (g.z1 + h.z1);

			  if (g.a.x - g.b.x == 0)
			  {
				  // corridor is West-East. 
				  b.p.x = b.q.x = 0.5f * (g.a.x + h.a.x);
				  b.p.y = g.a.y;
				  b.q.y = g.b.y;
			  }
			  else
			  {
				  // corridor is North-South
				  b.p.y = b.q.y = 0.5f * (g.a.y + h.a.y);
				  b.p.x = g.a.x;
				  b.q.x = g.b.x;
			  }  
			  barriers.push_back(b);
		  }	
	  }

	  void RandomizeLight()
	  {
		  lights.clear();

		  if (ceilingTriangles.empty()) return;

		  LightSpec light;

		  if (IsCorridor())
		  {
			  int gIndex = rand() % 2;
			  auto& g0 = gapSegments[gIndex];
			  Vec3 eye = g0.bounds.centre;
			  eye.z = g0.z1;

			  auto& g1 = gapSegments[1 - gIndex];
			  Vec3 target = g1.bounds.centre;

			  Vec3 dir = Normalize(target - eye);

			  light.ambience = RGBA(0.0f, 0.0f, 0.0f, 1.0f);
			  light.diffuse = RGBA(2.25f, 2.25f, 2.25f, 1.0f);
			  light.direction = dir;
			  light.position = eye;
		  }
		  else
		  {
			  auto index = rand() % (int32) ceilingTriangles.size();
			  auto& t = ceilingTriangles[index];
			  auto eye = (t.a.position + t.b.position + t.c.position) * 0.33333f + Vec3{ 0, 0, -0.1f };
			  Vec3 dir = { 0, 0, -1 };

			  light.ambience = RGBA(0.2f, 0.2f, 0.21f, 1.0f);
			  light.diffuse = RGBA(2.25f, 2.25f, 2.25f, 1.0f);
			  light.direction = dir;
			  light.position = eye;
		  }

		  light.cutoffPower = 64.0f;
		  light.cutoffAngle = 30_degrees;
		  light.fov = 90_degrees;
		  light.attenuation = -0.5f;
		  light.nearPlane = 0.5_metres;
		  light.farPlane = 25_metres;
		  light.fogConstant = -0.1f;

		  lights.push_back(light);
	  }

	  void Rebuild()
	  {
		  isDirty = true;

		  if (!co_sectors.IsMeshGenerationEnabled())
		  {
			  return;
		  }

		  TesselateWalls();

		  if (scriptFloor && !completeSquares.empty())
		  {
			  RunSectorGenFloorAndCeilingScript();
		  }
		  else
		  {
			  TesselateFloorAndCeiling();
		  }

		  if (IsCorridor() && scriptCorridor)
		  {
			  ResetBarriers();
			  RunGenCorridorScript();
		  }

		  RandomizeLight();
	  }

	  void TesselateFloorAndCeiling()
	  {
		  auto floor = nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Floor);
		  auto ceiling = nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Ceiling);

		  size_t len = sizeof(Vec2) * floorPerimeter.size();
		  Vec2* tempArray = (Vec2*)alloca(len);

		  for (size_t i = 0; i != floorPerimeter.size(); i++)
		  {
			  tempArray[i] = floorPerimeter[i];
		  }

		  RingManipulator<Vec2> ring(tempArray, floorPerimeter.size());

		  struct ANON : I2dMeshBuilder
		  {
			  float z0;
			  float z1;
			  float uvScale;
			  Vec2 uvOffset;
			  std::vector<VertexTriangle>* floorTriangles;
			  std::vector<VertexTriangle>* ceilingTriangles;
			  Sector* This;

			  MaterialVertexData floorMat;
			  MaterialVertexData ceilingMat;

			  void Append(const Triangle2d& t) override
			  {
				  ObjectVertex a, b, c;

				  float hA0, hA1;
				  This->GetVerticalMetrics(t.A, hA0, hA1);

				  float hB0, hB1;
				  This->GetVerticalMetrics(t.B, hB0, hB1);

				  float hC0, hC1;
				  This->GetVerticalMetrics(t.C, hC0, hC1);

				  a.position = { t.A.x, t.A.y, hA0 };
				  b.position = { t.B.x, t.B.y, hB0 };
				  c.position = { t.C.x, t.C.y, hC0 };

				  Vec3 upAny = Cross(b.position - a.position, c.position - b.position);
				  if (upAny.x == 0 && upAny.y == 0 && upAny.z == 0)
				  {
					  return;
				  }

				  Vec3 up = -Normalize(upAny);

				  a.normal = b.normal = c.normal = up;
				  a.material = b.material = c.material = floorMat;

				  a.uv.x = Vec2{ uvScale * Vec2{ t.A.x,t.A.y } +uvOffset }.x;
				  a.uv.y = Vec2{ uvScale * Vec2{ t.A.x,t.A.y } +uvOffset }.y;
				  b.uv.x = Vec2{ uvScale * Vec2{ t.B.x,t.B.y } +uvOffset }.x;
				  b.uv.y = Vec2{ uvScale * Vec2{ t.B.x,t.B.y } +uvOffset }.y;
				  c.uv.x = Vec2{ uvScale * Vec2{ t.C.x,t.C.y } +uvOffset }.x;
				  c.uv.y = Vec2{ uvScale * Vec2{ t.C.x,t.C.y } +uvOffset }.y;

				  VertexTriangle T;
				  T.a = a;
				  T.b = b;
				  T.c = c;
				  floorTriangles->push_back(T);

				  VertexTriangle PhysicsT = T;
				  PhysicsT.a.material.gloss = 1.1f;
				  floorTriangles->push_back(PhysicsT); // physics mesh

				  a.normal = b.normal = c.normal = -up;
				  a.position.z = hA1;
				  b.position.z = hB1;
				  c.position.z = hC1;
				  T.a = b;
				  T.b = a;
				  T.c = c;

				  T.a.material = T.b.material = T.c.material = ceilingMat;
				  ceilingTriangles->push_back(T);
			  }
		  } builder;

		  floorTriangles.clear();
		  ceilingTriangles.clear();

		  builder.ceilingMat = ceiling->second->mvd;
		  builder.floorMat = floor->second->mvd;
		  builder.z0 = z0;
		  builder.z1 = z1;
		  builder.uvOffset = uvOffset;
		  builder.uvScale = uvScale;
		  builder.floorTriangles = &floorTriangles;
		  builder.ceilingTriangles = &ceilingTriangles;
		  builder.This = this;

		  TesselateByEarClip(builder, ring);
	  }

	  void RemoveWallSegment(Vec2 p, Vec2 q)
	  {
		  struct
		  {
			  Vec2 p;
			  Vec2 q;
			  Sector* This;
			  bool operator()(const Segment& segment) const
			  {
				  auto P = This->floorPerimeter[segment.perimeterIndexStart];
				  auto Q = This->floorPerimeter[segment.perimeterIndexEnd];

				  return p == P && q == Q;
			  }
		  } segmentMatch{ p,q, this };

		  auto i = std::remove_if(wallSegments.begin(), wallSegments.end(), segmentMatch);
		  wallSegments.erase(i, wallSegments.end());
	  }

	  void FillGap(Vec2 a, Vec2 b)
	  {
		  struct
		  {
			  Vec2 p;
			  Vec2 q;
			  bool operator()(const Gap& gap) const
			  {
				  return p == gap.a && q == gap.b;
			  }
		  } segmentMatch{ a, b };

		  auto i = std::remove_if(gapSegments.begin(), gapSegments.end(), segmentMatch);
		  gapSegments.erase(i, gapSegments.end());

		  // Now add the wall
		  auto leftIndex = GetPerimeterIndex(a);
		  auto rightIndex = GetPerimeterIndex(b);

		  if (leftIndex < 0 || rightIndex < 0)
		  {
			  Throw(0, "Unexpected mismatch between gap vertex");
		  }

		  Segment s{ leftIndex, rightIndex };
		  wallSegments.push_back(s);

		  struct
		  {
			  bool operator ()(const Segment& a, const Segment& b) const
			  {
				  return a.perimeterIndexStart < b.perimeterIndexStart;
			  }
		  } byLeftIndex;
		  std::sort(wallSegments.begin(), wallSegments.end(), byLeftIndex);

		  Rebuild();
	  }

	  void AddGap(Vec2 p, Vec2 q, ISector* other)
	  {
		  gapSegments.push_back({ p, q, -1, -1, other, Sphere{ { 0,0,0 },0 }, 0 });
	  }

	  void BuildGapsAndSegments(IRing<Vec2>& perimeter)
	  {
		  gapSegments.clear();
		  wallSegments.clear();

		  for (size_t i = 0; i < perimeter.ElementCount(); i++)
		  {
			  Vec2 p = perimeter[i];
			  Vec2 q = perimeter[i + 1];

			  bool deleteSection = false;

			  for (auto* other : co_sectors)
			  {
				  if (other != static_cast<ISector*>(this))
				  {
					  Segment segment = other->GetSegment(q, p);
					  if (segment.perimeterIndexStart >= 0)
					  {
						  deleteSection = true;

						  auto* otherGap = other->GetGapAtSegment(q, p);

						  if (!otherGap)
						  {
							  Sector* otherGapConcrete = (Sector*)other;
							  otherGapConcrete->AddGap(q, p, this);
							  otherGapConcrete->RemoveWallSegment(q, p);
						  }

						  AddGap(p, q, other);
						  break;
					  }
					  else
					  {
						  Segment segment = other->GetSegment(p, q);
						  if (segment.perimeterIndexStart >= 0)
						  {
							  // Encapsulation
							  Throw(0, "Sector::Build - sector intersects existing sectors");
						  }
					  }
				  }
			  }

			  if (!deleteSection)
			  {
				  wallSegments.push_back({ (int32)i, (int32)(i + 1) % (int32)perimeter.ElementCount() });
			  }
		  }
	  }

	  bool IsInSector(Vec2 p) const
	  {
		  auto result = CountRayCrossingsThroughSector(p, { 1,0 }, floorPerimeter);
		  return result.fromVertex || ((result.count % 2) == 1);
	  }

	  bool IsFullyOccupied(std::vector<int>& occupancyMatrix, int DX, int DY, int span, int startX, int startY)
	  {
		  for (int j = startY; j < startY + span; ++j)
		  {
			  for (int i = startX; i < startX + span; ++i)
			  {
				  if (occupancyMatrix[i + DX * j] == 0)
				  {
					  return false;
				  }
			  }
		  }

		  return true;
	  }

	  void SetEntriesToZero(std::vector<int>& occupancyMatrix, int DX, int DY, int span, int startX, int startY)
	  {
		  for (int j = startY; j < startY + span; ++j)
		  {
			  for (int i = startX; i < startX + span; ++i)
			  {
				  occupancyMatrix[i + DX * j] = 0;
			  }
		  }
	  }

	  std::vector<AABB2d> completeSquares;

	  void ExtractSquares(std::vector<int>& occupancyMatrix, int DX, int DY)
	  {
		  int DS = min(DX, DY);

		  for (int ds = DS; ds > 0; ds--)
		  {
			  for (int j = 0; j <= DY - ds; ++j)
			  {
				  for (int i = 0; i <= DX - ds; ++i)
				  {
					  if (IsFullyOccupied(occupancyMatrix, DX, DY, ds, i, j))
					  {
						  AABB2d square;
						  square.left   = i + floorf(aabb.left) + 1.0f;
						  square.right  = i + ds + floorf(aabb.left) + 1.0f;
						  square.bottom = j + floorf(aabb.bottom) + 1.0f;
						  square.top    = j + ds + floorf(aabb.bottom) + 1.0f;
						  completeSquares.push_back(square);

						  SetEntriesToZero(occupancyMatrix, DX, DY, ds, i, j);
					  }
				  }
			  }
		  }
	  }

	  void BuildFloorRectangles()
	  {
		  int nX = 0;
		  int nY = 0;
		  for (float x = aabb.left + 1.0f; x < aabb.right; x += 1.0f)
		  {
			  nX++;
		  }

		  for (float y = aabb.bottom + 1.0f; y < aabb.top; y += 1.0f)
		  {
			  nY++;
		  }

		  static std::vector<int> floorRectArray;
		  floorRectArray.resize(nX * nY);

		  for (auto& i : floorRectArray)
		  {
			  i = 0;
		  }

		  int X = 0;

		  for (float x = aabb.left + 1.0f; x < aabb.right; x += 1.0f)
		  {
			  int Y = 0;

			  for (float y = aabb.bottom + 1.0f; y < aabb.top; y += 1.0f)
			  {
				  float x0 = floorf(x);
				  float y0 = floorf(y);
				  float x1 = x0 + 1.0f;
				  float y1 = y0 + 1.0f;
				  
				  Vec2 p00{ x0, y0 };
				  Vec2 p01{ x1, y0 };
				  Vec2 p10{ x0, y1 };
				  Vec2 p11{ x1, y1 };

				  if (IsInSector(p00) && IsInSector(p01) && IsInSector(p10) && IsInSector(p11))
				  {
					  if (!DoesSegmentCrossPerimeter(p00, p01, floorPerimeter))
					  {
						  if (!DoesSegmentCrossPerimeter(p01, p11, floorPerimeter))
						  {
							  if (!DoesSegmentCrossPerimeter(p11, p10, floorPerimeter))
							  {
								  if (!DoesSegmentCrossPerimeter(p10, p00, floorPerimeter))
								  {
									  floorRectArray[X + Y * nX] = 1;
								  }
							  }
						  }
					  }
				  }

				  Y++;
			  }

			  X++;
		  }

		  ExtractSquares(floorRectArray, nX, nY);
	  }

	  void Build(const Vec2* floorPlan, size_t nVertices, float z0, float z1) override
	  {
		  // N.B the sector is not part of the co-sectors collection until this function returns

		  altitudeInCm = (int32)(z0 * 100.0f);
		  heightInCm = (int32)((z1 -z0) * 100.0f);
			  
		  if (!floorPerimeter.empty())
		  {
			  Throw(0, "The floor perimeter has already been built");
		  }

		  this->z0 = z0;
		  this->z1 = z1;

		  if (nVertices < 3 || floorPlan[0] == floorPlan[nVertices - 1])
		  {
			  Throw(0, "Sector::Build: Bad floor plan. Algorithimic error - first point must not match end point.");
		  }

		  if (nVertices > 256)
		  {
			  Throw(0, "Sector::Build: Too many elements in the floor plan. Maximum is 256. Simplify");
		  }

		  for (size_t i = 0; i != nVertices; i++)
		  {
			  aabb << floorPlan[i];
			  floorPerimeter.push_back(floorPlan[i]);
		  }

		  Expand(aabb, 0.1_metres); // Add a skin to remove rounding error issues.

		  Ring<Vec2> ring_of_unknown_sense(floorPlan, nVertices);

		  if (!IsClockwiseSequential(ring_of_unknown_sense))
		  {
			  std::reverse(floorPerimeter.begin(), floorPerimeter.end());
		  }

		  Ring<Vec2> clockwiseRing(&floorPerimeter[0], floorPerimeter.size());

		  for (auto* other : co_sectors)
		  {
			  size_t nVertices;
			  auto* v = other->WallVertices(nVertices);
			  for (size_t i = 0; i < nVertices; ++i)
			  {
				  int j = GetPerimeterIndex(v[i]);
				  if (j < 0)
				  {
					  int index = GetFloorTriangleIndexContainingPoint(v[i]);
					  if (index >= 0)
					  {
						  Throw(0, "Sector::Build: Sector would have intersected another");
					  }
				  }
			  }
		  }

		  if (completeSquares.empty()) BuildFloorRectangles();

		  BuildGapsAndSegments(clockwiseRing);
		  FinalizeGaps();
		  Rebuild();
	  }

	  void Decouple() override
	  {
		  static int64 decoupleIterationFrame = 0x820000000000;
		  decoupleIterationFrame++;

		  floorPerimeter.clear();
		  wallSegments.clear();

		  SetIterationFrame(decoupleIterationFrame);

		  for (auto& g : gapSegments)
		  {
			  auto *concreteSector = (Sector*) g.other;
			  concreteSector->FillGap(g.b, g.a);
		  }

		  gapSegments.clear();
	  }

      void Free() override
      {
         delete this;
      }

	  bool propertiesChanged = true;

	  bool IsDirty() const override
	  {
		  return propertiesChanged;
	  }

	  void OnMaterialCategoryChanged(cstr component)
	  {
		  auto i = nameToMaterial.find(component);
		  if (i != nameToMaterial.end())
		  {
			  SafeFormat(i->second->persistentName, IO::MAX_PATHLEN, "random");
		  }
	  }

	  void OnMaterialIdChanged(cstr component)
	  {
		  auto i = nameToMaterial.find(component);
		  if (i != nameToMaterial.end())
		  {
			  auto& id = i->second->mvd.materialId;
			  if (id > 0)
			  {
				  MaterialCategory category = platform.instances.GetMaterialCateogry(id);
				  if (category != i->second->category)
				  {
					  id = platform.instances.GetMaterialId(i->second->category, 0);
				  }
			  }
		  }
	  }

	  void NotifyChanged(BloodyNotifyArgs& args) override
	  {
		  if (Eq("MaterialCategory", args.sourceName))
		  {
			  OnMaterialCategoryChanged(args.notifyId);
		  }
		  else if (Eq("MaterialId", args.sourceName))
		  {
			  OnMaterialIdChanged(args.notifyId);
		  }

		  propertiesChanged = true;
		  WideFilePath sysPath;

		  if (*wallScript)
		  {
			  try
			  {
				  platform.installation.ConvertPingPathToSysPath(wallScript, sysPath);
				  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, "#walls");
			  }
			  catch (IException&)
			  {

			  }
		  }
	  }

	  void InvokeSectorRebuild(bool force) override
	  {
		  if (propertiesChanged || force)
		  {
			  propertiesChanged = false;

			  z0 = (float)altitudeInCm / 100;
			  z1 = z0 + (float)heightInCm / 100;

			  contents->ClearComponents(""_fstring);
			  contents->ClearAllComponents();

			  ResetBarriers();

			  FinalizeGaps();
			  Rebuild();
		  }
	  }

	  void OnSectorScriptChanged(const FileModifiedArgs& args) override
	  {
		  static int64 anySectorScriptChangedUpdate = 0x900000000000;

		  cstr theCorridorScript = *corridorScript ? corridorScript : "!scripts/hv/sector/corridor/gen.door.sxy";

		  WideFilePath u16CorridorScript;
		  platform.installation.ConvertPingPathToSysPath(theCorridorScript, u16CorridorScript);
		  if (Eq(u16CorridorScript, args.sysPath) && IsCorridor() && scriptCorridor)
		  {
			  isDirty = true;
			  Rebuild();
			  ResetBarriers();
			  RunGenCorridorScript();
		  }

		  U8FilePath pingPath;
		  platform.installation.ConvertSysPathToPingPath(args.sysPath, pingPath);

		  cstr theWallScript = *wallScript ? wallScript : "#walls/stretch.bricks.sxy";
		  if (platform.installation.DoPingsMatch(pingPath, theWallScript) && scriptWalls)
		  {
			  FinalizeGaps();
			  Rebuild();
		  }

		  cstr theFloorScript = *floorScript ? floorScript : "#floors/square.mosaic.sxy";
		  if (platform.installation.DoPingsMatch(pingPath, theFloorScript) && scriptFloor && !completeSquares.empty())
		  {
			  isDirty = true;
			  RunSectorGenFloorAndCeilingScript();
		  }
	  }

	  bool isDirty = false;

	  void ForEveryObjectInSector(IEventCallback<const ID_ENTITY>& cb) override
	  {
		  if (isDirty)
		  {
			  UpdateCeilingGraphicMesh();
			  UpdateFloorGraphicMesh();
			  UpdatedWallGraphicMesh();
			  isDirty = false;
		  }

		  cb.OnEvent(wallId);
		  cb.OnEvent(floorId);
		  cb.OnEvent(ceilingId);

		  contents->ForEveryObjectInContent(cb);
	  }

	  int64 iterationFrame = 0;

	  int64 IterationFrame() const override
	  {
		  return iterationFrame;
	  }

	  void SetIterationFrame(int64 value) override
	  {
		  iterationFrame = value;
	  }

	  void AddToProperties(BodyComponentMatClass bcmc, IBloodyPropertySetEditor& editor)
	  {
		  auto i = nameToMaterial.find(bcmc);

		  char name[32];
		  SafeFormat(name, sizeof(name), "%s mat", bcmc);

		  editor.AddSpacer();
		  editor.AddMaterialCategory(name, i->first, &i->second->category);

		  char id[32];
		  SafeFormat(id, sizeof(id), "%s id", bcmc);
		  editor.AddMaterialString(id, i->second->mvd.materialId, i->first, i->second->persistentName, IO::MAX_PATHLEN);

		  char colour[32];
		  SafeFormat(colour, sizeof(colour), "%s colour", bcmc);
		  editor.AddColour(colour, &i->second->mvd.colour);

		  char gloss[32];
		  SafeFormat(gloss, sizeof(gloss), "%s gloss", bcmc);
		  editor.AddFloat(gloss, &i->second->mvd.gloss, 0, 1);
	  }

	  void SaveTemplate(StringBuilder& sb)
	  {
		  for (auto& i : nameToMaterial)
		  {
			  char bodyClass[16];
			  SafeFormat(bodyClass, 16, "\"%s\"", (cstr) i.first);
			  sb.AppendFormat("\n\t(sectors.SetTemplateMaterial %-12s (#MaterialCategory%s) 0x%8.8x \"%s\")", bodyClass, platform.utilities.ToShortString(i.second->category).buffer, *(int32*)&i.second->mvd.colour, i.second->persistentName);
		  }

		  if (Is4PointRectangular())
		  {
			  if (corridorScript)
			  {
				scriptConfig->SetCurrentScript(corridorScript);
				struct Anon : IEventCallback<VariableCallbackData>
				{
					StringBuilder& sb;
					Anon(StringBuilder& _sb) : sb(_sb) {}
					void OnEvent(VariableCallbackData& vd) override
					{
						sb.AppendFormat("\n\t(sectors.SetCorridorScriptF32 \"%s\" %f)", vd.name, vd.value);
					}
				} foreachVariable(sb);
				scriptConfig->Current().Enumerate(foreachVariable);
			  }
			  sb.AppendFormat("\n\t(sectors.SetTemplateDoorScript %s \"%s\")", scriptCorridor ? "true" : "false", corridorScript);
		  }

		  cstr theWallScript = *wallScript ? wallScript : DEFAULT_WALL_SCRIPT;

		  if (this->scriptWalls)
		  {
			  scriptConfig->SetCurrentScript(theWallScript);
			  struct Anon : IEventCallback<VariableCallbackData>
			  {
				  StringBuilder& sb;
				  Anon(StringBuilder& _sb) : sb(_sb) {}
				  void OnEvent(VariableCallbackData& vd) override
				  {
					  sb.AppendFormat("\n\t(sectors.SetWallScriptF32 \"%s\" %f)", vd.name, vd.value);
				  }
			  } foreachVariable(sb);
			  scriptConfig->Current().Enumerate(foreachVariable);
		  }
		  sb.AppendFormat("\n\t(sectors.SetTemplateWallScript %s \"%s\")", scriptWalls ? "true" : "false", theWallScript);

		  if (*floorScript)
		  {
			  scriptConfig->SetCurrentScript(floorScript);
			  struct Anon : IEventCallback<VariableCallbackData>
			  {
				  StringBuilder& sb;
				  Anon(StringBuilder& _sb) : sb(_sb) {}
				  void OnEvent(VariableCallbackData& vd) override
				  {
					  sb.AppendFormat("\n\t(sectors.SetFloorScriptF32 \"%s\" %f)", vd.name, vd.value);
				  }
			  } foreachVariable(sb);
			  scriptConfig->Current().Enumerate(foreachVariable);
		  }
		  sb.AppendFormat("\n\t(sectors.SetTemplateFloorScript %s \"%s\")", scriptFloor ? "true" : "false", floorScript);

		  sb.AppendFormat("\n\t(Int32 id = (sectors.CreateFromTemplate %d %d))\n", altitudeInCm, heightInCm);

		  ai->SaveAsScript(sb);
	  }

	  void GetProperties(cstr category, IBloodyPropertySetEditor& editor) override
	  {
		  char msg[256];

		  if (Is4PointRectangular())
		  {
			  SafeFormat(msg, sizeof(msg), "Sector #%u%s", id, IsCorridor() ? " (corridor)" : " (4 pt rectangle)");
		  }
		  else if (HV::IsAxisAlignedRectangular(floorPerimeter))
		  {
			  SafeFormat(msg, sizeof(msg), "Sector #%u%s", id, " Axis-Aligned rectangular room");
		  }
		  else
		  {
			  SafeFormat(msg, sizeof(msg), "Sector #%u", id);
		  }
		  editor.AddMessage(msg);

		  if (Eq(category, "walls"))
		  {
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Brickwork, editor);
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Cement, editor);
			  editor.AddSpacer();
			  editor.AddBool("script walls", &scriptWalls);
			  editor.AddMessage("Default: #walls/stretch.bricks.sxy");

			  cstr theWallScript = *wallScript ? wallScript : "#walls/stretch.bricks.sxy";

			  scriptConfig->SetCurrentScript(theWallScript);
			  scriptConfig->Current().BindProperties(editor);

			  try
			  {
				  WideFilePath sysPath;
				  if (*wallScript)
				  {
					  platform.installation.ConvertPingPathToSysPath(wallScript, sysPath);
					  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, "#walls");
				  }
				  editor.AddPingPath("wall script", wallScript.buf, IO::MAX_PATHLEN, "#walls/*.sxy", 90);
			  }
			  catch (IException&)
			  {
				  editor.AddPingPath("wall script", wallScript.buf, IO::MAX_PATHLEN, "#walls/*.sxy", 90);
			  }
		  }
		  else if (Eq(category, "ceiling"))
		  {
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Ceiling, editor);
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Mullions, editor);
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Rails, editor);
			  editor.AddSpacer();
			  editor.AddInt("height (cm)", false, &heightInCm);
			  
		  }
		  else if (Eq(category, "floor"))
		  {
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Floor, editor);
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Casing, editor);
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Panels, editor);
			  editor.AddSpacer();
			  editor.AddInt("altitude (cm)", false, &altitudeInCm);
			  editor.AddSpacer();

			  cstr theFloorScript = *floorScript ? floorScript : "#floors/square.mosaics.sxy";
			  scriptConfig->SetCurrentScript(theFloorScript);
			  scriptConfig->Current().BindProperties(editor);

			  if (!completeSquares.empty())
			  {
				  editor.AddBool("use script", &scriptFloor);
				  editor.AddMessage("Default: \"#floors/square.mosaics.sxy\"");
				  editor.AddPingPath("script file", floorScript.buf, IO::MAX_PATHLEN, "#floors/*.sxy", 120);
			  }
			  else
			  {
				  editor.AddMessage("Sector floor unsuitable for scripting");
			  }
		  }
		  else if (Eq(category, "corridor"))
		  {
			  if (IsCorridor())
			  {
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Panels, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Mullions, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Rails, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Casing, editor);

				  editor.AddSpacer();
				  editor.AddBool("script corridor", &scriptCorridor);
				  editor.AddMessage("Defaults to !scripts/hv/sector/gen.door.sxy");
				  editor.AddPingPath("corridor script", corridorScript.buf, IO::MAX_PATHLEN, "!scripts/hv/sector/*.sxy", 130);
			  }
			  else
			  {
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddMessage("The selected sector is not a corridor.");
				  editor.AddMessage("Corridors are rectangular");
				  editor.AddMessage("...and have only four vertices.");
				  editor.AddMessage("Opposite ends link to rooms");
				  editor.AddMessage("...which must not be 4pt rectangles.");
			  }
		  }
		  else if (Eq(category, "tags"))
		  {
			  editor.AddSpacer();
		  }
	  }

	  ISectorLayout* Layout() override
	  {
		  return this;
	  }

	  SectorSquares Squares() const override
	  {
		  if (completeSquares.empty()) 
		  {
			  return { nullptr, nullptr };
		  }
		  else
		  {
			  return SectorSquares{ completeSquares.data(), completeSquares.data() + completeSquares.size() };
		  }
	  }

	  ID_ENTITY AddItemToLargestSquare(const fstring& meshName, int addItemFlags, const HV::ObjectCreationSpec& obs) override
	  {
		  return contents->AddItemToLargestSquare(meshName, addItemFlags, obs);
	  }

	  ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) override
	  {
		  return contents->AddSceneryAroundObject(mesh, centrePieceId, iis, ocs);
	  }

	  bool TryGetScenery(ID_ENTITY id, AABB& worldBounds) const
	  {
		  return contents->TryGetScenery(id, worldBounds);
	  }

		bool DoesSceneryCollide(const AABB& aabb) const
		{
			return contents->DoesSceneryCollide(aabb);
		}

	  int32 CountSquares() override
	  {
		  return (int32)completeSquares.size();
	  }

	  void GetSquare(int32 sqIndex, AABB2d& sq)
	  {
		  int32 nElements = (int32)completeSquares.size();
		  if (sqIndex < 0 || sqIndex >= nElements)
		  {
			  Throw(0, "Sector::GetSquare(%d, ...): index out of range", sqIndex);
		  }

		  sq = completeSquares[sqIndex];
	  }

	  void CeilingQuad(int32 index, QuadVertices& q) override
	  {
		  int32 nElements = (int32)completeSquares.size();
		  if (index < 0 || index >= nElements)
		  {
			  Throw(0, "Sector::CeilingQuad(%d, ...): index out of range", index);
		  }

		  auto& aabb = completeSquares[index];

		  q.positions.a = { aabb.left,  aabb.bottom, z1 };
		  q.positions.b = { aabb.right, aabb.bottom, z1 };
		  q.positions.c = { aabb.right, aabb.top, z1 };
		  q.positions.d = { aabb.left,  aabb.top, z1 };

		  q.normals.a = q.normals.b = q.normals.c = q.normals.d = { 0,0,-1 };

		  q.colours.a = q.colours.b = q.colours.c = q.colours.d = RGBAb(128, 128, 128, 255);

		  if (uvScale == 0)
		  {
			  q.uv.left = 0;
			  q.uv.bottom = 0;
			  q.uv.right = 1.0f;
			  q.uv.top = 1.0f;
		  }
		  else
		  {
			  q.uv.left = uvScale * aabb.left;
			  q.uv.right = uvScale * aabb.right;
			  q.uv.top = uvScale * aabb.top;
			  q.uv.bottom = uvScale * aabb.bottom;
		  }
	  }

	  void FloorQuad(int32 index, QuadVertices& q) override
	  {
		  int32 nElements = (int32)completeSquares.size();
		  if (index < 0 || index >= nElements)
		  {
			  Throw(0, "Sector::FloorQuad(%d, ...): index out of range", index);
		  }

		  auto& aabb = completeSquares[index];

		  q.positions.a = { aabb.left,  aabb.top, z0 };
		  q.positions.b = { aabb.right, aabb.top, z0 };
		  q.positions.c = { aabb.right, aabb.bottom, z0 };
		  q.positions.d = { aabb.left,  aabb.bottom, z0 };

		  q.normals.a = q.normals.b = q.normals.c = q.normals.d = { 0,0,1 };

		  q.colours.a = q.colours.b = q.colours.c = q.colours.d = RGBAb(128, 128, 128, 255);

		  if (uvScale == 0)
		  {
			  q.uv.left = 0;
			  q.uv.bottom = 0;
			  q.uv.right = 1.0f;
			  q.uv.top = 1.0f;
		  }
		  else
		  {
			  q.uv.left = uvScale * aabb.left;
			  q.uv.right = uvScale * aabb.right;
			  q.uv.top = uvScale * aabb.top;
			  q.uv.bottom = uvScale * aabb.bottom;
		  }
	  }

	  int32 NumberOfSegments() override
	  {
		  return (int32) wallSegments.size();
	  }

	  int32 NumberOfGaps() override
	  {
		  return (int32) gapSegments.size();
	  }

	  void GetSegment(int32 index, HV::WallSegment& segment) override
	  {
		  if (index < 0) Throw(0, "Sector::GetSegment(...): Index %d < 0", index);
		  int32 i = index % (int32) wallSegments.size();
		  auto& s = wallSegments[i];

		  Vec2 p = floorPerimeter[s.perimeterIndexStart];
		  Vec2 q = floorPerimeter[s.perimeterIndexEnd];

		  float z0 = Z0();
		  float z1 = Z1();

		  segment.quad.a = { p.x, p.y, z1 };
		  segment.quad.b = { q.x, q.y, z1 };
		  segment.quad.c = { q.x, q.y, z0 };
		  segment.quad.d = { p.x, p.y, z0 };

		  Vec3 rawTangent = Vec3{ q.x - p.x, q.y - p.y, 0 };

		  segment.vertical = { 0, 0, 1 };
		  segment.normal = Normalize({ rawTangent.y, -rawTangent.x, 0 });

		  segment.leftEdgeIsGap = false;
		  segment.rightEdgeIsGap = false;

		  for (auto& gap : gapSegments)
		  {
			  if (gap.a == q)
			  {
				  segment.leftEdgeIsGap = true;
			  }
			  else if (gap.b == p)
			  {
				  segment.rightEdgeIsGap = true;
			  }
		  }

		  float tangentLen = Length(rawTangent);
		  segment.tangent = rawTangent * (1.0f / tangentLen);
		  segment.span = { tangentLen, segment.quad.a.z - segment.quad.d.z };
	  }

	  void GetGap(int32 index, HV::GapSegment& segment) override
	  {
		  if (index < 0) Throw(0, "Sector::GetGap(...): Index %d < 0", index);
		  if (gapSegments.empty()) Throw(0, "Sector::GetGap(...) : There are no gaps");
		  auto& gap = gapSegments[index % (int32)gapSegments.size()];

		  segment.quad.a = { gap.a.x, gap.a.y, gap.z1 };
		  segment.quad.b = { gap.b.x, gap.b.y, gap.z1 };
		  segment.quad.c = { gap.b.x, gap.b.y, gap.z0 };
		  segment.quad.d = { gap.a.x, gap.a.y, gap.z0 };

		  Vec3 tangent = segment.quad.b - segment.quad.a;

		  segment.normal = Normalize(Vec3{ tangent.y, -tangent.x, 0 });
		  segment.tangent = Normalize(tangent);
		  segment.vertical = { 0,0,1 };
		  segment.leadsToCorridor = gap.other->IsCorridor();
		  segment.otherZ0 = Z0();
		  segment.otherZ1 = Z1();
	  }

	  void OnTick(const IUltraClock& clock) override
	  {
		  ai->AdvanceInTime(platform.publisher, clock);
		  contents->OnTick(clock);
	  }

	  bool TryClickLever(ID_ENTITY idLever, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach)
	  {
		  if (TryClickGraphicsMesh(idLever, probePoint, probeDirection, reach, platform))
		  {
			  contents->ClickLever();
		  }

		  return false;
	  }

	  bool TryClickButton(ID_ENTITY idButton, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach)
	  {
		  return contents->TryClickButton(idButton, probePoint, probeDirection, reach);
	  }

	  bool UseAnythingAt(cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach) override
	  {
		  struct ANON : IEventCallback<const ComponentRef>
		  {
			  bool found = false;
			  Sector* sector;
			  Vec3 probePoint;
			  Vec3 probeDirection;
			  Metres reach;

			  void OnEvent(const ComponentRef& c)
			  {
				  if (found) return;

				  if (Eq(c.name, "door.button.1") || Eq(c.name, "door.button.2"))
				  {
					  if (sector->TryClickButton(c.id, probePoint, probeDirection, reach))
					  {
						  found = true;
					  }
				  }

				  if (Eq(c.name, "wall.lever"))
				  {
					  if (sector->TryClickLever(c.id, probePoint, probeDirection, reach))
					  {
						  found = true;
					  }
				  }
			  }
		  } clickIt;
		  clickIt.sector = this;
		  clickIt.probePoint = probePoint;
		  clickIt.probeDirection = probeDirection;
		  clickIt.reach = reach;

		  contents->ForEachComponent(clickIt);

		  return clickIt.found;
	  }

	  bool Is4PointRectangular() const override
	  {
		  return HV::Is4PointRectangular(floorPerimeter);
	  }

	  ITriggersAndActions& TriggersAndActions() override { return ai->TriggersAndActions(); }
   };
}

namespace HV
{
	ISector* CreateSector(Platform& platform, ISectors& co_sectors)
	{
		return new Sector(platform, co_sectors);
	}

	void RebaseSectors()
	{
		nextSectorId = 1;
	}
}

namespace HV
{
	namespace GraphicsEx
	{
		BodyComponentMatClass BodyComponentMatClass_Brickwork			= "brickwork";
		BodyComponentMatClass BodyComponentMatClass_Cement				= "cement";
		BodyComponentMatClass BodyComponentMatClass_Floor				= "floor";
		BodyComponentMatClass BodyComponentMatClass_Ceiling				= "ceiling";
		BodyComponentMatClass BodyComponentMatClass_Door_Mullions		= "mullions";
		BodyComponentMatClass BodyComponentMatClass_Door_Panels			= "panels";
		BodyComponentMatClass BodyComponentMatClass_Door_Casing			= "casing";
		BodyComponentMatClass BodyComponentMatClass_Door_Rails			= "rails";
		BodyComponentMatClass BodyComponentMatClass_Physics_Hull		= "physics.hull";
	}
}
#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <unordered_map>
#include <algorithm>

#include <rococo.rings.inl>

#include <rococo.variable.editor.h>

namespace HV
{
   HV::ICorridor* FactoryConstructHVCorridor(HV::ICorridor* _context)
   {
      return _context;
   }
}

namespace ANON
{
   using namespace Rococo;
   using namespace Rococo::Entities;
   using namespace HV;

   struct Component
   {
      std::string name;
      std::string meshName;
      ID_ENTITY id;
   };

   bool operator == (const Component& a, const fstring& b)
   {
      return a.name.length() == b.length && Eq(a.name.c_str(), b);
   }

   // If [array] represents a ring, then GetRingElement(i...) returns the ith element using modular arithmetic
   template<class T>
   T GetRingElement(size_t index, const T* array, size_t capacity)
   {
      return array[index % capacity];
   }

   uint32 nextSectorId = 1;

   void AddPointToAABB(AABB2d& rect, cr_vec2 p)
   {
	   if (rect.left  > p.x) rect.left = p.x;
	   if (rect.right < p.x) rect.right = p.x;
	   if (rect.bottom > p.y) rect.bottom = p.y;
	   if (rect.top < p.y) rect.top = p.y;
   }

   void Expand(AABB2d& rect, Metres ds)
   {
	   rect.left -= ds;
	   rect.right += ds;
	   rect.bottom -= ds;
	   rect.top += ds;
   }

   struct Sector : public ISector, public ICorridor, IMaterialPalette, public IEventCallback<MaterialArgs>
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

	   IUtilitiies& utilities;

	   float uvScale{ 0.2f };
	   Vec2 uvOffset{ 0,0 };

	   float z0; // Floor height
	   float z1; // Ceiling height (> floor height)

	   int32 altitudeInCm;
	   int32 heightInCm;

	   std::vector<Barrier> barriers;

	   // N.B we need addresses of material fields to remain constant when map is resized
	   // So use heap generated argument in nameToMaterial. Do not refactor pointer to Material as Material!
	   std::unordered_map<std::string, Material*> nameToMaterial;

	   char doorScript[IO::MAX_PATHLEN] = { 0 };
	   char wallScript[IO::MAX_PATHLEN] = { 0 };
	   bool hasDoor = false;
	   bool scriptWalls = false;

	   std::vector<LightSpec> lights;

	   AABB2d aabb;

	   const AABB2d& AABB() const override
	   {
		   return aabb;
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

	   virtual void SetTemplate(MatEnumerator& enumerator)
	   {
		   enumerator.Enumerate(*this);
	   }

	  const Barrier* Barriers(size_t& barrierCount) const override
	  {
		  barrierCount = barriers.size();
		  return barriers.empty() ? nullptr : &barriers[0];
	  }

      virtual float Z0() const
      {
         return z0;
      }

      virtual float Z1() const
      {
         return z1;
      }
   
      uint32 id; // unique sector Id

      // One instance for each major mesh of the sector
      ID_ENTITY floorId;
      ID_ENTITY ceilingId;
      ID_ENTITY wallId;
      
      Segment GetSegment(Vec2 p, Vec2 q) override
      {
		  if (!floorPerimeter.empty())
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
		  }
		  else
		  {
			  // Decoupled from the co-sectors
		  }

         return{ -1,-1 };
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
		  rchar name[32];
		  SafeFormat(name, sizeof(name), "sector.%u.walls", id);

		  auto& mb = instances.MeshBuilder();
		  mb.Clear();
		  mb.Begin(to_fstring(name));

		  for (auto& t : wallTriangles)
		  {
			  mb.AddTriangle(t.a, t.b, t.c);
		  }

		  mb.End();

		  if (!wallId)
		  {
			  wallId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		  }
		  else
		  {
			  auto entity = instances.GetEntity(wallId);
			  ID_SYS_MESH meshId;
			  platform.meshes.TryGetByName(name, meshId);
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

	  virtual void Assign(IPropertyHost* host)
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
		  struct ANON: IEventCallback<ScriptCompileArgs>, public HV::ISectorWallTesselator
		  {
			  Sector* This;

			  ANON(Sector* _This): This(_This)
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
				  if (index < 0) Throw(0, "ISectorWallTesselator::GetSegment(...): Index %d < 0", index);
				  int32 i = index % (int32) This->wallSegments.size();
				  auto& s = This->wallSegments[i];

				  Vec2 p = This->floorPerimeter[s.perimeterIndexStart];
				  Vec2 q = This->floorPerimeter[s.perimeterIndexEnd];

				  float z0 = This->Z0();
				  float z1 = This->Z1();

				  segment.quad.a = { p.x, p.y, z1 };
				  segment.quad.b = { q.x, q.y, z1 };
				  segment.quad.c = { q.x, q.y, z0 };
				  segment.quad.d = { p.x, p.y, z0 };
				  
				  Vec3 rawTangent  = Vec3 { q.x - p.x, q.y - p.y, 0 };

				  segment.vertical = { 0, 0, 1 };
				  segment.normal   = Normalize({ rawTangent.y, -rawTangent.x, 0 });

				  segment.leftEdgeIsGap = false;
				  segment.rightEdgeIsGap = false;

				  for (auto& gap : This->gapSegments)
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
				  if (index < 0) Throw(0, "ISectorWallTesselator::GetGap(...): Index %d < 0", index);
				  if (This->gapSegments.empty()) Throw(0, "ISectorWallTesselator::GetGap(...) : There are no gaps");
				  auto& gap = This->gapSegments[index % (int32)This->gapSegments.size()];

				  segment.quad.a = { gap.a.x, gap.a.y, gap.z1 };
				  segment.quad.b = { gap.b.x, gap.b.y, gap.z1 };
				  segment.quad.c = { gap.b.x, gap.b.y, gap.z0 };
				  segment.quad.d = { gap.a.x, gap.a.y, gap.z0 };
				 
				  Vec3 tangent = segment.quad.b - segment.quad.a;

				  segment.normal = Normalize(Vec3{ tangent.y, -tangent.x, 0 });
				  segment.vertical = { 0,0,1 };
				  segment.leadsToCorridor = gap.other->IsCorridor();
				  segment.otherZ0 = This->Z0();
				  segment.otherZ1 = This->Z1();
			  }

			  void AddWallTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c) override
			  {
				  if (a.position == b.position) return;
				  if (a.position == c.position) return;
				  if (b.position == c.position) return;

				  enum { MAX = 1000 };
				  if (This->floorTriangles.size() > MAX)
				  {
					  Throw(0, "ISectorWallTesselator::AddWallTriangle maximum %d triangles reached", MAX);
				  }

				  This->wallTriangles.push_back({ a,b,c });
			  }

			  void OnEvent(ScriptCompileArgs& args) override
			  {
				  This->wallTriangles.clear();
				  AddNativeCalls_HVISectorWallTesselator(args.ss, this);
			  }

			  void GetMaterial(MaterialVertexData& mat, const fstring& componentClass) override
			  {
				  if (!This->TryGetMaterial(componentClass, mat))
				  {
					  Throw(0, "ISectorWallTesselator::GetMaterial(...) Unknown component class: %s", (cstr)componentClass);
				  }
			  }
		  } scriptCallback(this);  

		  try
		  {
			  cstr theWallScript = *wallScript ? wallScript : "!scripts/hv/sector/walls/stretch.bricks.sxy";
			  platform.utilities.RunEnvironmentScript(platform, scriptCallback, theWallScript, true, false);
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
         utilities(_platform.utilities),
         id(nextSectorId++),
         platform(_platform),
         co_sectors(_co_sectors),
		 aabb(EmptyAABB2dBox())
      {
		  PrepMat(GraphicsEx::BodyComponentMatClass_Brickwork, "random", Graphics::MaterialCategory_Stone);
		  PrepMat(GraphicsEx::BodyComponentMatClass_Cement,    "random", Graphics::MaterialCategory_Rock);
		  PrepMat(GraphicsEx::BodyComponentMatClass_Floor,     "random", RandomRockOrMarble());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Ceiling,   "random", RandomRockOrMarble());

		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Mullions, "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Rails,   "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Panels,  "random", RandomWoodOrMetal());
		  PrepMat(GraphicsEx::BodyComponentMatClass_Door_Casing,  "random", RandomWoodOrMetal());

		  cstr wscript = co_sectors.GetTemplateWallScript(scriptWalls);
		  SafeFormat(wallScript, IO::MAX_PATHLEN, "%s", wscript);

		  cstr dscript = co_sectors.GetTemplateDoorScript(hasDoor);
		  SafeFormat(doorScript, IO::MAX_PATHLEN, "%s", dscript);
      }

	  void OnEvent(MaterialArgs& args)
	  {
		  auto i = nameToMaterial.find(args.bcmc);
		  if (i == nameToMaterial.end())
		  {
			  i = nameToMaterial.insert(std::make_pair(std::string(args.bcmc), new Material())).first;
		  }

		  *i->second = *args.mat;
	  }

	  virtual uint32 Id() const
	  {
		  return id;
	  }

      void DeleteFloor()
      {
         if (floorId)
         {
            instances.Delete(floorId);
            floorId = ID_ENTITY::Invalid();

            rchar name[32];
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

            rchar name[32];
            SafeFormat(name, sizeof(name), "sector.%u.ceiling", id);
            instances.MeshBuilder().Delete(to_fstring(name));
         }
      }

      void DeleteWalls()
      {
		  if (wallId)
		  {
			  rchar name[32];
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

		 Random::RandomMT mt;
		 Random::Seed(mt, id + 1);

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
         rchar name[32];
         SafeFormat(name, sizeof(name), "sector.%u.floor", id);
         
         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (auto& t : floorTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End();

		 if (!floorId)
		 {
			 floorId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		 }
		 else
		 {
			 ID_SYS_MESH meshId;
			 platform.meshes.TryGetByName(name, meshId);
			 auto* entity = instances.GetEntity(floorId);
			 entity->SetMesh(meshId);
		 }
      }

      void UpdateCeilingGraphicMesh()
      {
         rchar name[32];
         SafeFormat(name, sizeof(name), "sector.%u.ceiling", id);

         auto& mb = instances.MeshBuilder();
         mb.Begin(to_fstring(name));

         for (auto& t : ceilingTriangles)
         {
            mb.AddTriangle(t.a, t.b, t.c);
         }

         mb.End();

		 if (!ceilingId)
		 {
			 ceilingId = instances.AddBody(to_fstring(name), Matrix4x4::Identity(), { 1,1,1 }, ID_ENTITY::Invalid());
		 }
		 else
		 {
			 ID_SYS_MESH meshId;
			 platform.meshes.TryGetByName(name, meshId);
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

      virtual void GetSpan(Vec3& span)
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

      boolean32 IsSloped()
      {
         return (gapSegments[0].z0 != gapSegments[1].z0) || (gapSegments[0].z1 != gapSegments[1].z1);
      }

      std::vector<Component> components;

      virtual void ClearComponents(const fstring& componentName)
      {
		 for (auto& c : components)
		 {
			 platform.instances.Delete(c.id);
		 }

         components.erase(std::remove(components.begin(), components.end(), componentName), components.end());
      }

      virtual void CentreComponent(const fstring& componentName, const fstring& meshName)
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

         Matrix4x4 model = Matrix4x4::Translate({ centre.x, centre.y, z }) * Rz;

         auto id = platform.instances.AddBody(meshName, model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());;

         for (auto& c : components)
         {
            if (Eq(componentName, c.name.c_str()) && Eq(meshName, c.meshName.c_str()))
            {
               platform.instances.Delete(c.id);
               c.id = id;
               return;
            }
         }

         Component c{ componentName, meshName, id };
         components.push_back(c);
      }

      virtual void GetComponentMeshName(const fstring& componentName, Rococo::IStringPopulator& meshName)
      {
         char fullMeshName[256];
         SafeFormat(fullMeshName, 256, "sector.%d.%s", id, (cstr)componentName);
         meshName.Populate(fullMeshName);
      }

      void RunSectorGenScript(cstr name)
      {
         struct : IEventCallback<ScriptCompileArgs>
         {
            ICorridor *corridor;
            virtual void OnEvent(ScriptCompileArgs& args)
            {
               AddNativeCalls_HVICorridor(args.ss, corridor);
            }
         } scriptCallback;
         scriptCallback.corridor = this;

		 try
		 {
			 platform.utilities.RunEnvironmentScript(platform, scriptCallback, name, true);
		 }
		 catch (IException& ex)
		 {
			 char title[256];
			 SafeFormat(title, 256, "sector %u: %s failed", id, name);
			 platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, title);
		 }
      }

	  void ResetBarriers()
	  {
		  barriers.clear();
		
		  if (IsCorridor() && hasDoor)
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
		  TesselateWalls();

		  auto floor = nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Floor);
		  auto ceiling = nameToMaterial.find(GraphicsEx::BodyComponentMatClass_Ceiling);

		  TesselateFloorAndCeiling(floor->second->mvd, ceiling->second->mvd);

		  if (IsCorridor() && hasDoor)
		  {
			  ResetBarriers();

			  cstr theDoorScript = *doorScript ? doorScript : "!scripts/hv/sector/gen.door.sxy";
			  RunSectorGenScript(theDoorScript);
		  }

		  RandomizeLight();
	  }

      void TesselateFloorAndCeiling(const MaterialVertexData& floorMat, const MaterialVertexData& ceilingMat)
      {
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
            Sector* This;

			MaterialVertexData floorMat;
			MaterialVertexData ceilingMat;

            virtual void Append(const Triangle2d& t)
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

		 builder.ceilingMat = ceilingMat;
		 builder.floorMat = floorMat;
         builder.z0 = z0;
         builder.z1 = z1;
         builder.uvOffset = uvOffset;
         builder.uvScale = uvScale;
         builder.floorTriangles = &floorTriangles;
         builder.ceilingTriangles = &ceilingTriangles;
         builder.This = this;

         TesselateByEarClip(builder, ring);

		 isDirty = true;
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

		  dirty = true;
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
							  Sector* otherGapConcrete = (Sector*) other;
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
			  AddPointToAABB(aabb, floorPlan[i]);
			  floorPerimeter.push_back(floorPlan[i]);
		  }

		  Expand(aabb, 0.1_metres); // Add a skin to remove rounding error issues.

		  Ring<Vec2> ring_of_unknown_sense(floorPlan, nVertices);

		  if (!IsClockwiseSequential(ring_of_unknown_sense))
		  {
			  std::reverse(floorPerimeter.begin(), floorPerimeter.end());
		  }

		  Ring<Vec2> clockwiseRing(&floorPerimeter[0], floorPerimeter.size());
		  BuildGapsAndSegments(clockwiseRing);
		  FinalizeGaps();
		  Rebuild();

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
	  }

	  virtual void Decouple()
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

	  bool dirty = true;

	  void NotifyChanged()
	  {
		  dirty = true;
		  char sysPath[IO::MAX_PATHLEN];

		  if (*wallScript)
		  {
			  try
			  {
				  platform.installation.ConvertPingPathToSysPath(wallScript, sysPath, IO::MAX_PATHLEN);
				  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, IO::MAX_PATHLEN, "#walls");
			  }
			  catch (IException&)
			  {

			  }
		  }
	  }

	  void InvokeSectorRebuild(bool force) override
	  {
		  if (dirty || force)
		  {
			  dirty = false;

			  z0 = (float)altitudeInCm / 100;
			  z1 = z0 + (float)heightInCm / 100;

			  ClearComponents(""_fstring);
			  components.clear();

			  ResetBarriers();

			  FinalizeGaps();
			  Rebuild();
		  }
	  }

      bool Is4PointRectangular() const
      {
         if (floorPerimeter.size() != 4) return false;

         Vec2 ab = floorPerimeter[1] - floorPerimeter[0];
         Vec2 bc = floorPerimeter[2] - floorPerimeter[1];
         Vec2 cd = floorPerimeter[3] - floorPerimeter[2];
         Vec2 da = floorPerimeter[0] - floorPerimeter[3];

         if (Dot(ab,bc) != 0)
         {
            return false;
         }

         if (Dot(bc,cd) != 0)
         {
            return false;
         }

         if (Dot(cd, da) != 0)
         {
            return false;
         }

         if (Dot(da, ab) != 0)
         {
            return false;
         }

         return true;
      }

	  void OnSectorScriptChanged(const FileModifiedArgs& args) override
	  {
		  static int64 anySectorScriptChangedUpdate = 0x900000000000;

		  if (args.Matches(doorScript) && IsCorridor() && hasDoor)
	      {
			  ResetBarriers();
		      RunSectorGenScript(doorScript);
		  }

		  char path[256];
		  args.GetPingPath(path, 256);

		  cstr theWallScript = *wallScript ? wallScript : "!scripts/hv/sector/walls/stretch.bricks.sxy";
		  if (platform.installation.DoPingsMatch(path, theWallScript) && scriptWalls)
		  {
			  FinalizeGaps();
			  Rebuild();
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

		  for (const auto& c : components)
		  {
			  cb.OnEvent(c.id);
		  }
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
		  editor.AddMaterialCategory(name, &i->second->category);

		  char id[32];
		  SafeFormat(id, sizeof(id), "%s id", bcmc);
		  editor.AddMaterialString(id, i->second->persistentName, IO::MAX_PATHLEN);

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
			  SafeFormat(bodyClass, 16, "\"%s\"", i.first.c_str());
			  sb.AppendFormat("\n\t(sectors.SetTemplateMaterial %-12s %2d 0x%8.8x \"%s\")", bodyClass, i.second->category, *(int32*)&i.second->mvd.colour, i.second->persistentName);
		  }

		  if (Is4PointRectangular())
		  {
			  sb.AppendFormat("\n\n\t(sectors.SetTemplateDoorScript %s \"%s\")", hasDoor ? "true" : "false", doorScript);
		  }

		  sb.AppendFormat("\n\t(sectors.SetTemplateWallScript %s \"%s\")\n", scriptWalls ? "true" : "false", wallScript);

		  sb.AppendFormat("\n\t(sectors.CreateFromTemplate %d %d)\n", altitudeInCm, heightInCm);
	  }

	  void GetProperties(cstr category, IBloodyPropertySetEditor& editor) override
	  {
		  char msg[256];

		  if (Is4PointRectangular())
		  {
			  SafeFormat(msg, sizeof(msg), "Sector #%u%s", id, IsCorridor() ? " (corridor)" : " (4 pt rectangle)");
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

			  try
			  {
				  char sysPath[IO::MAX_PATHLEN];
				  if (*wallScript)
				  {
					  platform.installation.ConvertPingPathToSysPath(wallScript, sysPath, IO::MAX_PATHLEN);
					  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, IO::MAX_PATHLEN, "#walls");
				  }
				  editor.AddPingPath("wall script", wallScript, IO::MAX_PATHLEN, "!scripts/hv/sector/walls/*.sxy");
			  }
			  catch (IException&)
			  {
				  editor.AddPingPath("wall script", wallScript, IO::MAX_PATHLEN, "!scripts/hv/sector/walls/*.sxy");
			  }
		  }
		  else if (Eq(category, "ceiling"))
		  {
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Ceiling, editor);
			  editor.AddSpacer();
			  editor.AddInt("height (cm)", false, &heightInCm);
			  
		  }
		  else if (Eq(category, "floor"))
		  {
			  AddToProperties(GraphicsEx::BodyComponentMatClass_Floor, editor);
			  editor.AddSpacer();
			  editor.AddInt("altitude (cm)", false, &altitudeInCm);
		  }
		  else if (Eq(category, "door"))
		  {
			  if (IsCorridor())
			  {
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Panels, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Mullions, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Rails, editor);
				  AddToProperties(GraphicsEx::BodyComponentMatClass_Door_Casing, editor);

				  editor.AddSpacer();
				  editor.AddBool("has door", &hasDoor);
				  editor.AddMessage("Defaults to !scripts/hv/sector/gen.door.sxy");
				  editor.AddPingPath("door script", doorScript, IO::MAX_PATHLEN, "!scripts/hv/sector/");
			  }
			  else
			  {
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddSpacer();
				  editor.AddMessage("Only sectors that are corridors can have doors");
				  editor.AddMessage("Corridors are rectangular and have four vertices");
				  editor.AddMessage("Doors will not show unless opposite sides of a");
				  editor.AddMessage("corridor are linked to non-corridor sectors");
			  }
		  }
	  }
   };
}

namespace HV
{
   ISector* CreateSector(Platform& platform, ISectors& co_sectors)
   {
      return new ANON::Sector(platform, co_sectors);
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
		BodyComponentMatClass BodyComponentMatClass_Door_Mullions		= "mulliions";
		BodyComponentMatClass BodyComponentMatClass_Door_Panels			= "panels";
		BodyComponentMatClass BodyComponentMatClass_Door_Casing			= "casing";
		BodyComponentMatClass BodyComponentMatClass_Door_Rails			= "rails";
	}
}
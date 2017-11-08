#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
//#include <unordered_map>
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

namespace
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

   const char* const genDoorScript = "!scripts/hv/sector/gen.door.sxy";

   class Sector : public ISector, public ICorridor
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

	  std::vector<Barrier> barriers;

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

      int64 sectorFlags = 0;

      virtual void AddFlag(SectorFlag flag)
      {
         sectorFlags |= flag;
      }

      virtual void RemoveFlag(SectorFlag flag)
      {
         sectorFlags &= ~flag;
      }

      virtual bool IsFlagged(SectorFlag flag) const
      {
         return (sectorFlags & flag) != 0;
      }

      virtual void SetFlag(SectorFlag flag, bool value)
      {
         if (value) AddFlag(flag);
         else RemoveFlag(flag);
      }
      
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

      void BuildGapsAndTesselateWalls(IRing<Vec2>& perimeter)
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
						if (!otherGap || otherGap->z0 != z0 || otherGap->z1 != z1)
						{
							co_sectors.AddDirty(other);
						}

						float gz0, gz1;
						if (other->IsCorridor())
						{
							gz0 = z0;
							gz1 = z1;
						}
						else
						{
							gz0 = other->Z0();
							gz1 = other->Z1();
						}

						Vec3 centre = { 0.5f * (p.x + q.x), 0.5f * (p.y + q.y), 0.5f * (gz0 + gz1) };
						float radius = Length(Vec3{ p.x, q.y, z0 - centre.z });
						Sphere bounds{ centre, radius };

						gapSegments.push_back({ p, q, gz0, gz1, other, bounds, 0 });
						break;
					}
					else
					{
						Segment segment = other->GetSegment(p, q);
						if (segment.perimeterIndexStart >= 0)
						{
							// Encapsulation
							Throw(0, "Sector::Build - sector intersectors existing sectors");
						}
					}
				}
			}

            if (deleteSection) continue;

            wallSegments.push_back({ (int32) i, (int32) (i + 1) % (int32)perimeter.ElementCount() });
         }

		 auto wallMatId = platform.instances.GetRandomMaterialId(Rococo::Graphics::MaterialCategory_Stone);
		 TesselateWallsFromSegments(wallMatId);
      }

      float AddWallSegment(const Vec2& p, const Vec2& q, float h0, float h1, float u, MaterialId id)
      {
         Vec3 up{ 0, 0, 1 };
         Vec3 P0 = { p.x, p.y, h0 };
         Vec3 Q0 = { q.x, q.y, h0 };
         Vec3 P1 = { p.x, p.y, h1 };
         Vec3 Q1 = { q.x, q.y, h1 };

         Vec3 delta = Q0 - P0;

         float segmentLength = round(Length(delta));

         Vec3 normal = Cross(delta, up);

         ObjectVertex PV0, PV1, QV0, QV1;

         PV0.position = P0;
         PV1.position = P1;
         QV0.position = Q0;
         QV1.position = Q1;

         PV0.normal = PV1.normal = QV0.normal = QV1.normal = normal;
		 PV0.material = PV1.material = QV0.material = QV1.material = { RGBAb(0, 0, 0, 255), id };

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
		  this->host = host;
	  }

      float AddSlopedWallSegment(const Vec2& p, const Vec2& q, float pFloor, float qFloor, float pCeiling, float qCeiling, float u, MaterialId id)
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
		 PV0.material = PV1.material = QV0.material = QV1.material = { RGBAb(0, 0, 0, 255), id };

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

      void RaiseSlopeBetweenGaps(MaterialId id)
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

         AddSlopedWallSegment(p, q, h00, h01, h10, h11, 0.0f, id);

         p = floorPerimeter[wallSegments[1].perimeterIndexStart];
         q = floorPerimeter[wallSegments[1].perimeterIndexEnd];

         AddSlopedWallSegment(p, q, h01, h00, h11, h10, 0.0f, id);
      }

	  void RunSectorGenWallScript()
	  {
		  if (wallSegments.empty()) return;

		  // Script has to iterate through wallSegments and append to wallTriangles
		  struct ANON: IEventCallback<ScriptCompileArgs>, public HV::ISectorWallTesselator
		  {
			  Sector* This;

			  ANON(Sector* _This): This(_This)
			  {
			  }

			  virtual int32 NumberOfSegments()
			  {
				  return (int32) This->wallSegments.size();
			  }

			  virtual int32 NumberOfGaps()
			  {
				  return (int32) This->gapSegments.size();
			  }

			  virtual void GetSegment(int32 index, HV::WallSegment& segment)
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

			  virtual void GetGap(int32 index, HV::GapSegment& segment)
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

			  virtual void AddWallTriangle(const ObjectVertex& a, const ObjectVertex& b, const ObjectVertex& c)
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

			  virtual void OnEvent(ScriptCompileArgs& args)
			  {
				  This->wallTriangles.clear();
				  AddNativeCalls_HVISectorWallTesselator(args.ss, this);
			  }
		  } scriptCallback(this);  

		  platform.utilities.RunEnvironmentScript(platform, scriptCallback, "!scripts/hv/sector/walls/gen/default.sxy", true);
	  }

	  void CreateFlatWallsBetweenGaps(MaterialId id)
	  {
		  float u = 0;

		  if (IsFlagged(SectorFlag_ScriptedWalls))
		  {
			  RunSectorGenWallScript();
		  }
		  else
		  {
			  for (auto segment : wallSegments)
			  {
				  Vec2 p = floorPerimeter[segment.perimeterIndexStart];
				  Vec2 q = floorPerimeter[segment.perimeterIndexEnd];

				  u = AddWallSegment(p, q, z0, z1, u, id);
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

				  AddWallSegment(p, q, foreignHeight, z1, 0, id);
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

				  AddWallSegment(p, q, z0, foreignFloorHeight, 0, id);
			  }
		  }
	  }

      void TesselateWallsFromSegments(MaterialId id)
      {
         wallTriangles.clear();

         bool isCorridor = IsCorridor();
         if (isCorridor)
         {
            RaiseSlopeBetweenGaps(id);
         }
         else
         {
			 CreateFlatWallsBetweenGaps(id);
         }

		 isDirty = true;
      }

      Platform& platform;
   public:
      Sector(Platform& _platform, ISectors& _co_sectors) :
         instances(_platform.instances),
         utilities(_platform.utilities),
         id(nextSectorId++),
         platform(_platform),
         co_sectors(_co_sectors)
      {
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

      ~Sector()
      {
         DeleteFloor();
         DeleteCeiling();
         DeleteWalls();

		 if (host)
		 {
			 host->SetPropertyTarget(nullptr);
		 }
      }

      virtual void SetPalette(const SectorPalette& palette)
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

      virtual void CentreComponent(const fstring& componentName, const fstring& meshName, const fstring& textureName)
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

         platform.utilities.RunEnvironmentScript(platform, scriptCallback, name, true);
      }

	  void ResetBarriers()
	  {
		  barriers.clear();
		
		  if (IsCorridor() && IsFlagged(SectorFlag_Has_Door))
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

	  void Rebuild(int64 iterationFrame)
	  {
		  if (IterationFrame() == iterationFrame)
		  {
			 return;
		  }
		  else
		  {
			  SetIterationFrame(iterationFrame);
		  }

		  BuildGapsAndTesselateWalls(Ring<Vec2>(&floorPerimeter[0], floorPerimeter.size()));

		  auto floorMatId = platform.instances.GetRandomMaterialId(Rococo::Graphics::MaterialCategory_Rock);
		  auto ceilingMatId = platform.instances.GetRandomMaterialId(Rococo::Graphics::MaterialCategory_Rock);

		  TesselateFloorAndCeiling(floorMatId, ceilingMatId);

		  if (IsCorridor() && IsFlagged(SectorFlag_Has_Door))
		  {
			  ResetBarriers();
			  RunSectorGenScript("!scripts/hv/sector/gen.door.sxy");
		  }
	  }

      void TesselateFloorAndCeiling(MaterialId floorId, MaterialId ceilingId)
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

			MaterialId floorId;
			MaterialId ceilingId;

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
			   a.material = b.material = c.material = { RGBAb(0, 0, 0, 255), floorId };

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

			   T.a.material.materialId = T.b.material.materialId = T.c.material.materialId = ceilingId;
               ceilingTriangles->push_back(T);
            }
         } builder;

         floorTriangles.clear();
         ceilingTriangles.clear();

		 builder.ceilingId = ceilingId;
		 builder.floorId = floorId;
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

	  void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) override
	  {
		  // N.B the sector is not part of the co-sectors collection until this function returns

		  if (!floorPerimeter.empty())
		  {
			  Throw(0, "The floor perimeter is already built");
		  }

		  this->z0 = z0;
		  this->z1 = z1;

		  if (nVertices < 3 || positionArray[0] == positionArray[nVertices - 1])
		  {
			  Throw(0, "Sector::Build: Bad position array");
		  }

		  if (nVertices > 256)
		  {
			  Throw(0, "Sector::Build: Too many elements in mesh. Maximum is 256. Simplify");
		  }

		  for (size_t i = 0; i != nVertices; i++)
		  {
			  floorPerimeter.push_back(positionArray[i]);
		  }

		  Ring<Vec2> ring_of_unknown_sense(positionArray, nVertices);

		  if (!IsClockwiseSequential(ring_of_unknown_sense))
		  {
			  std::reverse(floorPerimeter.begin(), floorPerimeter.end());
		  }

		  static int64 constructIterationFrame = 0x840000000000;
		  Rebuild(constructIterationFrame++);

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
			  g.other->Rebuild(decoupleIterationFrame);
		  }

		  gapSegments.clear();
	  }

      void Free() override
      {
         delete this;
      }

	  virtual int64 Flags() const
	  {
		  return sectorFlags;
	  }

      void InvokeSectorDialog(Rococo::Windows::IWindow& parent, IEditorState& state) override
      {
         struct : IVariableEditorEventHandler
         {
            IEditorState* state;

            std::string textures[3];

            virtual void OnButtonClicked(cstr variableName)
            {
            }
         } handler;

         handler.state = &state;

         rchar title[32];

         if (Is4PointRectangular())
         {
            SafeFormat(title, sizeof(title), "Sector %u - (4 point rectangle)", id);
         }
         else
         {
            SafeFormat(title, sizeof(title), "Sector %u", id);
         }

		 Vec2i topLeft = { 240, 240 };
         AutoFree<IVariableEditor> editor = utilities.CreateVariableEditor(parent, { 640, 400 }, 120, title, "Walls, Floor and Ceiling", "Edit mesh parameters", &handler, &topLeft);
         editor->AddIntegerEditor("Altitiude", "Altitiude - centimetres", 0, 100000, (int32)(z0 * 100.0f));
         editor->AddIntegerEditor("Height", "Height - centimetres", 250, 100000, (int32)((z1 - z0) * 100.0f));
         editor->AddTab("Textures", "Set and get default textures");

         editor->AddTab("Occlusion", "Set occlusion properties");
         editor->AddBooleanEditor("Players", IsFlagged(SectorFlag_Occlude_Players));
         editor->AddBooleanEditor("Friends", IsFlagged(SectorFlag_Occlude_Friends));
         editor->AddBooleanEditor("Enemies", IsFlagged(SectorFlag_Occlude_Enemies));

		 editor->AddTab("Scripting", "Set generation logic");
		 editor->AddBooleanEditor("Script Walls", IsFlagged(SectorFlag_ScriptedWalls));
		 editor->AddBooleanEditor("Script Ceiling", IsFlagged(SectorFlag_ScriptedCeiling));
		 editor->AddBooleanEditor("Script Floors", IsFlagged(SectorFlag_ScriptedFloor));

		 if (Is4PointRectangular())
		 {
            editor->AddBooleanEditor("Door",    IsFlagged(SectorFlag_Has_Door));
         }

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


            SetFlag(SectorFlag_ScriptedWalls,   editor->GetBoolean("Script Walls"));
            SetFlag(SectorFlag_ScriptedCeiling, editor->GetBoolean("Script Ceiling"));
            SetFlag(SectorFlag_ScriptedFloor,   editor->GetBoolean("Script Floors"));

			SetFlag(SectorFlag_Occlude_Players, editor->GetBoolean("Players"));
			SetFlag(SectorFlag_Occlude_Enemies, editor->GetBoolean("Enemies"));
			SetFlag(SectorFlag_Occlude_Friends, editor->GetBoolean("Friends"));

			if (Is4PointRectangular())
			{
				SetFlag(SectorFlag_Has_Door, editor->GetBoolean("Door"));
			}

			static int64 updateHeightIterationFrame = 0x830000000000;
			updateHeightIterationFrame++;

			ClearComponents(""_fstring);
			components.clear();

			ResetBarriers();

            Rebuild(updateHeightIterationFrame);

			co_sectors.RebuildDirtySectors(updateHeightIterationFrame);
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

		  if (args.Matches(genDoorScript) && IsCorridor() && IsFlagged(SectorFlag_Has_Door))
	      {
			  ResetBarriers();
		      RunSectorGenScript(genDoorScript);
		  }

		  char path[256];
		  args.GetPingPath(path, 256);

		  if (strstr(path,"hv/sector/walls/") && IsFlagged(SectorFlag_ScriptedWalls))
		  {
			  Rebuild(anySectorScriptChangedUpdate++);
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

	  void GetProperties(cstr category, IBloodyPropertySetEditor& editor) override
	  {
		  if (Eq(category, "walls"))
		  {
			  editor.AddSpacer();
			  editor.AddMaterialCategory("brickwork mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("brickwork id", platform.renderer.GetMaterialTextureName(wallId));
			  editor.AddColour("brick colour", RGBAb(0,0,0,0));

			  editor.AddSpacer();
			  editor.AddMaterialCategory("cement", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("cement id", "random");
			  editor.AddColour("cement colour", RGBAb(0, 0, 0, 0));
		  }
		  else if (Eq(category, "ceiling"))
		  {
			  editor.AddSpacer();
			  editor.AddMaterialCategory("ceiling mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("ceiling id", "random");
			  editor.AddColour("ceiling colour", RGBAb(0, 0, 0, 0));
			  editor.AddSpacer();
			  editor.AddInt("altitude (cm)", false, 0);
		  }
		  else if (Eq(category, "floor"))
		  {
			  editor.AddSpacer();
			  editor.AddMaterialCategory("floor mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("floor id", "random");
			  editor.AddColour("floor colour", RGBAb(0, 0, 0, 0));
			  editor.AddSpacer();
			  editor.AddInt("height (cm)", false, 400);
			  editor.AddSpacer();
			  editor.AddBool("occlude players", false);
			  editor.AddBool("occlude friends", false);
			  editor.AddBool("occlude enemies", false);
		  }
		  else if (Eq(category, "door"))
		  {
			  editor.AddSpacer();
			  editor.AddMaterialCategory("panel mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("panel id", "random");
			  editor.AddColour("panel colour", RGBAb(0, 0, 0, 0));

			  editor.AddSpacer();
			  editor.AddMaterialCategory("mullion mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("mullion id", "random");
			  editor.AddColour("mullion colour", RGBAb(0, 0, 0, 0));

			  editor.AddSpacer();
			  editor.AddMaterialCategory("rail mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("rail id", "random");
			  editor.AddColour("rail colour", RGBAb(0, 0, 0, 0));

			  editor.AddSpacer();
			  editor.AddMaterialCategory("casing mat", Rococo::Graphics::MaterialCategory_Rock);
			  editor.AddMaterialString("casing id", "random");
			  editor.AddColour("casing colour", RGBAb(0, 0, 0, 0));

			  editor.AddSpacer();
			  editor.AddBool("has door", false);
			  editor.AddPingPath("door script", "");
		  }
	  }
   };
}

namespace HV
{
   ISector* CreateSector(Platform& platform, ISectors& co_sectors)
   {
      return new Sector(platform, co_sectors);
   }
}
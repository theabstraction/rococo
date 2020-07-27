#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>

#include <vector>
#include <unordered_map>
#include <algorithm>

#include <rococo.rings.inl>

#include <rococo.variable.editor.h>

#include <random>

namespace
{
	cstr const DEFAULT_WALL_SCRIPT = "#walls/stretch.bricks.sxy";
}

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
   using namespace Rococo::Graphics;
   using namespace HV;

   Rococo::Random::RandomMT rng;

   namespace Roll
   {
	   uint32 d(uint32 maxValue)
	   {
		   return (rng() % maxValue) + 1;
	   }

	   uint32 x(uint32 oneAboveMaxValue)
	   {
		   return rng() % oneAboveMaxValue;
	   }

	   // 50% chance to return true, else it returns false
	   boolean32 FiftyFifty()
	   {
		   return (rng() % 2) == 0;
	   }

	   float AnyOf(float minValue, float maxValue)
	   {
		  return Rococo::Random::NextFloat(rng, minValue, maxValue);
	   }
   }

   bool IsTriangleFacingUp(cr_m4x4 model, const VertexTriangle& t)
   {
	   Vec3 up{ 0, 0, 1 };
	   Vec3 Na, Nb, Nc;
	   TransformDirection(model, t.a.normal, Na);
	   TransformDirection(model, t.a.normal, Nb);
	   TransformDirection(model, t.a.normal, Nc);
	   return (Na == up && Nb == up && Nc == up);
   }

   struct TriangleListBinding : public  ITriangleList
   {
	   std::vector<VertexTriangle>& tris;

	   TriangleListBinding(std::vector<VertexTriangle>& _tris): tris(_tris)
	   {

	   }

	   void AddTriangleByVertices(ObjectVertex& a, ObjectVertex& b, ObjectVertex& c) override
	   {
		   if (a.position == b.position) return;
		   if (a.position == c.position) return;
		   if (b.position == c.position) return;

		   VertexTriangle vt{ a, b, c };
		   AddTriangle(vt);
	   }

	   void AddTriangle(VertexTriangle& abc) override
	   {
		   enum { MAX = 100000 };
		   if (tris.size() > MAX)
		   {
			   Throw(0, "TriangleListBinding::AddTriangle... maximum %d triangles reached. Be kind to laptop users.", MAX);
		   }
		   tris.push_back(abc);
	   }

	   void AddQuad(ObjectVertex& a, ObjectVertex& b, ObjectVertex& c, ObjectVertex& d) override
	   {
		   AddTriangleByVertices(a, b, c);
		   AddTriangleByVertices(c, d, a);
	   }

	   int32 CountVertices() override
	   {
		   return (int32)tris.size() * 3;
	   }

	   int32 CountTriangles()  override
	   {
		   return (int32)tris.size();
	   }
   };

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

   void Expand(AABB2d& rect, Metres ds)
   {
	   rect.left -= ds;
	   rect.right += ds;
	   rect.bottom -= ds;
	   rect.top += ds;
   }

   bool TryGetOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, Degrees theta)
   {
	   Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);

	   newBounds = bounds.RotateBounds(Rz);

	   Vec3 newSpan = newBounds.Span();

	   if (newSpan.x <= containerSpan.x && newSpan.y <= containerSpan.y)
	   {
		   return true;
	   }
	   else
	   {
		   return false;
	   }
   }

   bool GetRandomOrientationAndBoundsToFitBoxInAnother(Matrix4x4& Rz, AABB& newBounds, const AABB& bounds, cr_vec2 containerSpan, int32 guesses)
   {
	   for (int i = 0; i < guesses; ++i)
	   {
		   Degrees theta{ Roll::x(360) * 1.0f };
		   return TryGetOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, theta);
	   }

	   return false;
   }

   bool TryGetRotationToFit(Matrix4x4& Rz, bool randomizeHeading, const AABB& bounds, cr_vec2 containerSpan)
   {
	   Vec3 objectSpan = bounds.Span();

	   if (randomizeHeading)
	   {
		   AABB newBounds;
		   return GetRandomOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, 30);
	   }
	   else
	   {
		   int delta = Roll::x(4);
		   for (int i = 0; i < 4; ++i)
		   {
			   Degrees angle{ fmodf(90.0f * (i + delta), 360.0f) };
			   AABB newBounds;
			   if (TryGetOrientationAndBoundsToFitBoxInAnother(Rz, newBounds, bounds, containerSpan, angle))
			   {
				   return true;
			   }
		   }

		   return false;
	   }
   }

   bool TryGetRandomTransformation(Matrix4x4& model, AABB& worldBounds, bool randomHeading, bool randomizePosition, const AABB& bounds, const AABB2d& container, float z0, float z1)
   {
	   Vec2 containerSpan = container.Span();
	   Vec3 objectSpan = bounds.Span();

	   if (objectSpan.z > (z1 - z0))
	   {
		   return false;
	   }

	   Matrix4x4 Rz;
	   if (!TryGetRotationToFit(Rz, randomHeading, bounds, containerSpan))
	   {
		   return false;
	   }
	   else
	   {
		   AABB newBounds = bounds.RotateBounds(Rz);
		   Vec3 newSpan = newBounds.Span();

		   float dx = containerSpan.x - newSpan.x;
		   float dy = containerSpan.y - newSpan.y;

		   float x0 = !randomizePosition ? dx * 0.5f : Roll::AnyOf(0, dx);
		   float y0 = !randomizePosition ? dy * 0.5f : Roll::AnyOf(0, dy);

		   Vec3 originDisplacement = Vec3{ x0, y0, 0 } -newBounds.minXYZ;
		   Vec2 tileBottomLeft = { container.left, container.bottom };
		   Vec3 position = Vec3{ tileBottomLeft.x, tileBottomLeft.y, z0 + 0.001f } +originDisplacement;

		   auto T = Matrix4x4::Translate(position);

		   model = T * Rz;

		   Vec3 tileOrigin{ tileBottomLeft.x, tileBottomLeft.y, 0 };

		   worldBounds = AABB();
		   worldBounds << (tileOrigin + Vec3{ x0, y0, z0 });
		   worldBounds << (tileOrigin + Vec3{ x0 + newSpan.x, y0 + newSpan.y, z0 + newSpan.z });

		   return true;
	   }
   }

   void SortQuadsByArea(std::vector<Quad>& quads)
   {
	   struct
	   {
		   static float AreaSq(const Quad& q)
		   {
			   Vec3 ab = q.b - q.a;
			   Vec3 bc = q.c - q.b;
			   Vec3 K = Cross(ab,bc);
			   return LengthSq(K);
		   }

		   bool operator()(const Quad& p, const Quad& q) const
		   {
			   return AreaSq(p) > AreaSq(q);
		   }
	   } byAreaDescedning;
	   std::sort(quads.begin(), quads.end(), byAreaDescedning);
   }

   bool IsQuadRectangular(const Quad& q)
   {
	   Vec3 ab = q.b - q.a;
	   Vec3 bc = q.c - q.b;
	   Vec3 cd = q.d - q.c;
	   Vec3 da = q.a - q.d;

	   if (Dot(ab, bc) == 0 && Dot(bc, cd) == 0 && Dot(cd, da) == 0 && Dot(da, ab) == 0)
	   {
		   return true;
	   }
	   else
	   {
		   return false;
	   }
   }

   struct Sector : 
	   public ISector,
	   public ICorridor,
	   IMaterialPalette,
	   public IEventCallback<MaterialArgs>,
	   public ISectorLayout,
	   public ITriggersAndActions
   {
	   struct AIBuilder : ISectorAIBuilder
	   {
		   Sector* sector;

		   void ClearTriggers() override
		   {
			   sector->triggers.clear();
		   }

		   HString lastTrigger;

		   void AddTrigger(const fstring& name) override
		   {
			   int32 index = (int32) sector->triggers.size();
			   sector->AddTrigger(index);

			   lastTrigger = name;

			   TriggerType type;
			   if (!HV::TryShortParse(name, type))
			   {
				   Throw(0, "ISectorAIBuilder.AddTrigger(...): Unrecognized trigger type: %s", (cstr)name);
			   }

			   sector->triggers[index]->SetType(type);
		   }

		   IAction& GetLastAction()
		   {
			   if (sector->triggers.empty())
			   {
				   Throw(0, "GetLastAction(): no trigger to which to append action");
			   }

			   int32 triggerIndex = (int32)sector->triggers.size() - 1;

			   auto& actions = sector->triggers[triggerIndex]->Actions();

			   if (actions.Count() == 0)
			   {
				   Throw(0, "GetLastAction(): no actions for last trigger");
			   }

			   int32 actionIndex = actions.Count() - 1;

			   return actions[actionIndex];
		   }

		   HString lastFactory;

		   void AddAction(const fstring& factoryName) override
		   {
			   if (sector->triggers.empty())
			   {
				   Throw(0, "ISectorAIBuilder::AddAction(%s): no trigger to which to append action", (cstr) factoryName);
			   }

			   lastFactory = factoryName;

			   int32 triggerIndex = (int32)sector->triggers.size() - 1;
			   sector->AddAction(triggerIndex);

			   auto& actions = sector->triggers[triggerIndex]->Actions();

			   int32 actionIndex = actions.Count() - 1;

			   IActionFactory& factoryRef = GetActionFactory(factoryName);
			   actions.SetAction(actionIndex, factoryRef, sector->AFCC());
		   }

		   int32 FindParameterIndex(IAction& a, const fstring& argName)
		   {
			   for (int32 i = 0; i < a.ParameterCount(); ++i)
			   {
				   auto desc = a.GetParameterName(i);
				   if (Eq(desc.name, argName))
				   {
					   return i;
				   }
			   }

			   Throw(0, "Cannot find argument '%s' to %s of %s", (cstr)argName, lastFactory.c_str(), lastTrigger.c_str());
		   }

		   void AddActionArgumentI32(const fstring& argName, int32 value) override
		   {
			   IAction& a = GetLastAction();
			   int32 paramIndex = FindParameterIndex(a, argName);

			   char buf[32];
			   SafeFormat(buf, sizeof buf, "%d", value);
			   a.SetParameter(paramIndex, buf);
		   }

		   void AddActionArgumentF32(const fstring& argName, float value) override
		   {
			   IAction& a = GetLastAction();
			   int32 paramIndex = FindParameterIndex(a, argName);

			   char buf[32];
			   SafeFormat(buf, sizeof buf, "%f", value);
			   a.SetParameter(paramIndex, buf);
		   }

		   void AddActionArgumentString(const fstring& argName, const fstring& value) override
		   {
			   IAction& a = GetLastAction();
			   int32 paramIndex = FindParameterIndex(a, argName);
			   a.SetParameter(paramIndex, value);
		   }
	   } aiBuilder;
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

	   IUtilitiies& utilities;

	   float uvScale{ 0.2f };
	   Vec2 uvOffset{ 0,0 };

	   float z0; // Floor height
	   float z1; // Ceiling height (> floor height)

	   int32 altitudeInCm;
	   int32 heightInCm;

	   std::vector<Barrier> barriers; // Barrier 0 is always the door if there is one

	   // N.B we need addresses of material fields to remain constant when map is resized
	   // So use heap generated argument in nameToMaterial. Do not refactor pointer to Material as Material!
	   std::unordered_map<std::string, Material*> nameToMaterial;

	   char corridorScript[IO::MAX_PATHLEN] = { 0 };
	   char wallScript[IO::MAX_PATHLEN] = { 0 };
	   char floorScript[IO::MAX_PATHLEN] = { 0 };

	   bool scriptCorridor = false;
	   bool scriptWalls = false;
	   bool scriptFloor = false;

	   std::vector<LightSpec> lights;

	   AABB2d aabb;

	   std::vector<ID_ENTITY> managedEntities;

	   struct SceneryBind
	   {
		   ID_ENTITY id;
		   AABB worldBounds;
		   std::vector<Quad> levelSurfaces;
	   };

	   std::vector<SceneryBind> scenery;

	   std::vector<ITriggerSupervisor*> triggers;

	   ISectorAIBuilder& GetSectorAIBuilder()
	   {
		   return aiBuilder;
	   }

	   int32 TriggerCount() const
	   {
		   return (int32)triggers.size();
	   }

	   ITrigger& operator[](int32 i)
	   {
		   return *triggers[i];
	   }

	   IIActionFactoryCreateContext& AFCC()
	   {
		   return co_sectors.AFCC();
	   }

	   void AddAction(int32 triggerIndex) override
	   {
		   if (triggerIndex < 0 || triggerIndex > (int32) triggers.size())
		   {
			   Throw(0, "Sector.AddAction(%d) -> index out of range", triggerIndex);
		   }

		   auto& t = *triggers[triggerIndex];
		   t.Actions().AddAction(GetDefaultActionFactory(), co_sectors.AFCC());
	   }

	   void RemoveAction(int32 triggerIndex, int32 actionIndex) override
	   {
		   if (triggerIndex < 0 || triggerIndex > (int32) triggers.size())
		   {
			   Throw(0, "Sector.RemoveAction(%d) -> index out of range", triggerIndex);
		   }

		   auto& t = *triggers[triggerIndex];
		   if (actionIndex >= 0 && actionIndex < t.Actions().Count())
		   {
			   t.Actions().RemoveAction(actionIndex);
		   }
	   }

	   void AddTrigger(int32 pos) override
	   {
		   auto* t = CreateTrigger();

		   if (pos >= 0 && pos < (int32) triggers.size())
		   {
			   auto i = triggers.begin();
			   std::advance(i, pos);
			   triggers.insert(i, t);
		   }
		   else
		   {
			   triggers.push_back(t);
		   }
	   }

	   void RemoveTrigger(int32 pos)  override
	   {
		   if (pos >= 0 && pos < (int32)triggers.size())
		   {
			   auto i = triggers.begin();
			   std::advance(i, pos);
			   triggers.erase(i);
		   }
		   else
		   {
			   Throw(0, "Sector.TriggersAndActions.RemoveTrigger[%d] Bad trigger index", pos);
		   }
	   }

	   void Altitude(Vec2& altitudes)
	   {
		   altitudes = { z0, z1 };
	   }

	   void ClearManagedEntities() override
	   {
		   managedEntities.clear();
	   }

	   void DeleteItemsWithMesh(const fstring& prefix) override
	   {
		   struct
		   {
			   Sector* This;
			   const fstring prefix;
			   bool operator()(const ID_ENTITY id) const
			   {
				   auto& p = This->platform;
				   auto* e = p.instances.GetEntity(id);
				   if (e)
				   {
					   auto meshId = e->MeshId();
					   auto name = p.meshes.GetName(meshId);
					   if (StartsWith(name, prefix))
					   {
						   return true;
					   }
				   }
				   return false;
			   }

			   bool operator()(const SceneryBind& bind) const
			   {
				   return operator()(bind.id);
			   }
		   } meshPrefixed{ this, prefix };
		   auto i = std::remove_if(managedEntities.begin(), managedEntities.end(), meshPrefixed);
		   managedEntities.erase(i, managedEntities.end());

		   auto j = std::remove_if(scenery.begin(), scenery.end(), meshPrefixed);
		   scenery.erase(j, scenery.end());
	   }

	   boolean32 Exists() override
	   {
		   return true;
	   }

	   void ManageEntity(ID_ENTITY id)  override
	   {
		   managedEntities.push_back(id);
	   }

	   const AABB2d& GetAABB() const override
	   {
		   return aabb;
	   }

	   // Any quads in the scenery that face (0 0 1) marked for utility
	   void UseUpFacingQuads(ID_ENTITY id) override
	   {
		   for (auto& i : scenery)
		   {
			   if (i.id == id)
			   {
				   i.levelSurfaces.clear();

				   auto* e = platform.instances.GetEntity(id);
				   if (e)
				   {
					   const Vec3 up{ 0, 0, 1 };
					   auto& model = e->Model();
					   size_t triangleCount = 0;
					   auto tris = platform.meshes.GetTriangles(e->MeshId(), triangleCount);

					   if (triangleCount > 1)
					   {
						   for (auto k = 0; k < triangleCount - 1; ++k)
						   {
							   auto& t0 = tris[k];
							   auto& t1 = tris[k+1];
							   if (IsTriangleFacingUp(model, t0) && IsTriangleFacingUp(model, t1))
							   {
									// We may have a quad, if so then t0 forms abc and t1 forms cda
								   if (t0.a.position == t1.c.position && t0.c.position == t1.a.position)
								   {
									   Quad q;
									   q.a = t0.a.position;
									   q.b = t0.b.position;
									   q.c = t0.c.position;
									   q.d = t1.b.position;
									   i.levelSurfaces.push_back(q);
									   k++;
								   }
							   }
						   }
					   }

					   SortQuadsByArea(i.levelSurfaces);
				   }
			   }
		   }
	   }

	   std::vector<SceneryBind*> randomizedSceneryList;

	   bool TryPlaceItemOnQuad(const Quad& qModel, ID_ENTITY quadsEntityId, ID_ENTITY itemId)
	   {
		   auto e = platform.instances.GetEntity(quadsEntityId);
		   auto item = platform.instances.GetEntity(itemId);
		   if (e && item)
		   {
			   if (IsQuadRectangular(qModel) && qModel.a.x == qModel.b.x || qModel.a.y == qModel.b.y)
			   {
				   Quad qWorld;
				   TransformPositions(&qModel.a, 4, e->Model(), &qWorld.a);

				   auto bounds = platform.meshes.Bounds(item->MeshId());
				   if (bounds.minXYZ.x < bounds.maxXYZ.x)
				   {
					   AABB2d minSquare;
					   minSquare << AsVec2(qModel.a) << AsVec2(qModel.b) << AsVec2(qModel.c) << AsVec2(qModel.d);

					   Matrix4x4 randomModel;
					   AABB worldBounds;
					   if (TryGetRandomTransformation(randomModel, worldBounds, true, true, bounds, minSquare, qModel.a.z, z1 - z0))
					   {
						   ManageEntity(itemId);
						   item->Model() = e->Model() * randomModel;
						   return true;
					   }
				   }
			   }
		   }

		   return false;
	   }

	   boolean32 PlaceItemOnUpFacingQuad(ID_ENTITY id) override
	   {
		   randomizedSceneryList.clear();

		   for (auto& i : scenery)
		   {
			   randomizedSceneryList.push_back(&i);
		   }

		   std::random_shuffle(randomizedSceneryList.begin(), randomizedSceneryList.end());

		   for (auto&i : randomizedSceneryList)
		   {
			   if (!i->levelSurfaces.empty())
			   {
				   auto& q = i->levelSurfaces[Roll::x((uint32)i->levelSurfaces.size())];
				   if (TryPlaceItemOnQuad(q, i->id, id))
				   {
					   return true;
				   }
			   }
		   }

		   return false;
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
			  if (doorElevation > 2)
			  {
				  barrierCount = 0;
				  return nullptr;
			  }
		  }
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
				  AddNativeCalls_HVISectorWallTesselator(args.ss, this);
				  AddNativeCalls_HVISectorComponents(args.ss, this);
				  AddNativeCalls_HVITriangleList(args.ss, this);
				  AddNativeCalls_HVIScriptConfig(args.ss, &This->scriptConfig->Current());
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
				  SafeFormat(fullMeshName, 256, "sector.%d.%s", This->id, (cstr)componentName);

				  this->meshName = fullMeshName;

				  This->platform.meshes.Clear();
				  This->platform.meshes.Begin(to_fstring(fullMeshName));
			  }

			  void ClearComponents(const fstring& componentName) override
			  {
				  This->ClearComponents(componentName);
			  }

			  void CompleteComponent(boolean32 preserveMesh)  override
			  {
				  This->platform.meshes.End(preserveMesh, false);
				  This->AddComponent(Matrix4x4::Identity(), localName.c_str(), meshName.c_str());
			  }
		  } scriptCallback(this);  

		  try
		  {
			  cstr theWallScript = *wallScript ? wallScript : DEFAULT_WALL_SCRIPT;
			  scriptConfig->SetCurrentScript(theWallScript);
			  platform.utilities.RunEnvironmentScript(scriptCallback, theWallScript, true, false);
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
				  return !This->IsAxisAlignedRectangular();
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

				  if (!This->IsAxisAlignedRectangular())
				  {
					  This->TesselateFloorAndCeiling();
				  }

				  AddNativeCalls_HVISectorFloorTesselator(args.ss, this);
				  AddNativeCalls_HVISectorComponents(args.ss, this);
				  AddNativeCalls_HVIScriptConfig(args.ss, &This->scriptConfig->Current());
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
				  This->ClearComponents(componentName);
			  }

			  void CompleteComponent(boolean32 preserveMesh) override
			  {
				  This->platform.meshes.End(preserveMesh, false);
				  This->AddComponent(Matrix4x4::Identity(), localName.c_str(), meshName.c_str());
			  }

		  } scriptCallback(this);

		  try
		  {
			  cstr theFloorScript = *floorScript ? floorScript : "#floors/square.mosaics.sxy";
			  scriptConfig->SetCurrentScript(theFloorScript);
			  platform.utilities.RunEnvironmentScript(scriptCallback, theFloorScript, true, false);
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

		  if (IsSloped())
		  {
			  z1 += 100.0;
			  z0 -= 100.0f;
		  }

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
         utilities(_platform.utilities),
         id(nextSectorId++),
         platform(_platform),
         co_sectors(_co_sectors),
		 scriptConfig(CreateScriptConfigSet())
      {
		  aiBuilder.sector = this;

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
			  co_sectors.EnumerateWallVars(foreachWallVariable);
		  }

		  cstr cscript = co_sectors.GetTemplateDoorScript(scriptCorridor);
		  SafeFormat(corridorScript, IO::MAX_PATHLEN, "%s", cscript);

		  if (cscript)
		  {
			  VariableEnumerator foreachCorridorVariable(*this);
			  scriptConfig->SetCurrentScript(cscript);
			  co_sectors.EnumerateWallVars(foreachCorridorVariable);
		  }

		  cstr fscript = co_sectors.GetTemplateFloorScript(scriptFloor);
		  SafeFormat(floorScript, IO::MAX_PATHLEN, "%s", fscript);

		  if (fscript)
		  {
			  VariableEnumerator foreachFloorVariable(*this);
			  scriptConfig->SetCurrentScript(fscript);
			  co_sectors.EnumerateWallVars(foreachFloorVariable);
		  }
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

            char name[32];
            SafeFormat(name, sizeof(name), "sector.%u.floor", id);
            instances.MeshBuilder().Delete(to_fstring(name));
         }
      }

	  void DeleteScenery() override
	  {
		  for (auto s : scenery)
		  {
			  platform.instances.Delete(s.id);
		  }

		  scenery.clear();
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

		  for (auto* t : triggers)
		  {
			  t->Free();
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

      int32 GetPerimeterIndex(Vec2 a) const override
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

	  void AddComponent(cr_m4x4 model, cstr componentName, cstr meshName)
	  {
		  auto id = platform.instances.AddBody(to_fstring(meshName), model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());;

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
         SafeFormat(fullMeshName, 256, "sector.%d.%s", id, (cstr)componentName);
         meshName.Populate(fullMeshName);
      }

	  const Triangle* GetPhysicsHull(const fstring& componentName, size_t& nTriangles) const
	  {
		  char fullMeshName[256];
		  SafeFormat(fullMeshName, 256, "sector.%d.%s", id, (cstr)componentName);

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
               AddNativeCalls_HVICorridor(args.ss, this);
			   AddNativeCalls_HVISectorComponents(args.ss, this);
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
				SafeFormat(fullMeshName, 256, "sector.%d.%s", This->id, (cstr)componentName);

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
				This->ClearComponents(componentName);
			}

			void CompleteComponent(boolean32 preserveMesh) override
			{
				This->platform.meshes.End(preserveMesh, false);

				Matrix4x4 model;
				This->CorridorModelMatrix(model);
				This->AddComponent(model, localName.c_str(), meshName.c_str());
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
		 
		 cstr genCorridor = *corridorScript ? corridorScript : "#corridor/gen.door.sxy";

		 try
		 {	
			 platform.utilities.RunEnvironmentScript(scriptCallback, genCorridor, true);
		 }
		 catch (IException& ex)
		 {
			 char title[256];
			 SafeFormat(title, 256, "sector %u: %s failed", id, genCorridor);
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

	  bool IsAxisAlignedRectangular() const
	  {
		  Ring<Vec2> ring(&floorPerimeter[0], floorPerimeter.size());

		  for (size_t i = 0; i < floorPerimeter.size(); ++i)
		  {
			  auto p = ring[i];
			  auto q = ring[i + 1];
			  auto r = ring[i + 2];

			  Vec2 pq = q - p;
			  Vec2 qr = r - q;

			  if (pq.x != 0 && pq.y != 0)
			  {
				  return false;
			  }

			  if (qr.x != 0 && qr.y != 0)
			  {
				  return false;
			  }

			  float x = Cross(pq, qr);
			  if (x > 0)
			  {
				  return false;
			  }
		  }

		  return true;
	  }

	  void Rebuild()
	  {
		  isDirty = true;

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

	  struct RayCrossingsEnum
	  {
		  int count = 0;
		  bool fromVertex = false;
	  };

	  RayCrossingsEnum CountRayCrossingsThroughSector(Vec2 a, Vec2 dir) const
	  {
		  Ring<Vec2> ring(&floorPerimeter[0], floorPerimeter.size());

		  RayCrossingsEnum rce;

		  if (GetPerimeterIndex(a) >= 0)
		  {
			  rce.fromVertex = true;
		  }

		  for (size_t i = 0; i < floorPerimeter.size(); ++i)
		  {
			  Vec2 p = ring[i];
			  Vec2 q = ring[i + 1];

			  float t, u;
			  if (GetLineIntersect(a, a + dir, p, q, t, u))
			  {
				  if (u >= 0 && u <= 1)
				  {
					  if (t == 0 || u == 0 || u == 1)
					  {
						  rce.fromVertex = true;
					  }
					  else if (t > 0)
					  {
						  rce.count++;
					  }
				  }
			  }
		  }

		  return rce;
	  }

	  bool IsInSector(Vec2 p) const
	  {
		  auto result = CountRayCrossingsThroughSector(p, { 1,0 });
		  return result.fromVertex || ((result.count % 2) == 1);
	  }

	  bool DoesSegmentCrossPerimeter(Vec2 p, Vec2 q)
	  {
		  size_t nVertices = floorPerimeter.size();
		  for (size_t i = 0; i <= nVertices; ++i)
		  {
			  auto c = GetRingElement(i, &floorPerimeter[0], nVertices);
			  auto d = GetRingElement(i + 1, &floorPerimeter[0], nVertices);

			  float t, u;
			  if (GetLineIntersect(p, q, c, d, t, u))
			  {
				  if (u > 0 && u < 1 && t > 0 && t < 1)
				  {
					  return true;
				  }
			  }
		  }

		  return false;
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
					  if (!DoesSegmentCrossPerimeter(p00, p01))
					  {
						  if (!DoesSegmentCrossPerimeter(p01, p11))
						  {
							  if (!DoesSegmentCrossPerimeter(p11, p10))
							  {
								  if (!DoesSegmentCrossPerimeter(p10, p00))
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
				  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, IO::MAX_PATHLEN, "#walls");
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

		  for (const auto& c : components)
		  {
			  cb.OnEvent(c.id);
		  }

		  for (auto& s : scenery)
		  {
			  cb.OnEvent(s.id);
		  }

		  for (auto& m : managedEntities)
		  {
			  cb.OnEvent(m);
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
		  editor.AddMaterialCategory(name, i->first.c_str(), &i->second->category);

		  char id[32];
		  SafeFormat(id, sizeof(id), "%s id", bcmc);
		  editor.AddMaterialString(id, i->second->mvd.materialId, i->first.c_str(), i->second->persistentName, IO::MAX_PATHLEN);

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
	  }

	  void GetProperties(cstr category, IBloodyPropertySetEditor& editor) override
	  {
		  char msg[256];

		  if (Is4PointRectangular())
		  {
			  SafeFormat(msg, sizeof(msg), "Sector #%u%s", id, IsCorridor() ? " (corridor)" : " (4 pt rectangle)");
		  }
		  else if (IsAxisAlignedRectangular())
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
					  platform.installation.ConvertSysPathToMacroPath(sysPath, wallScript, IO::MAX_PATHLEN, "#walls");
				  }
				  editor.AddPingPath("wall script", wallScript, IO::MAX_PATHLEN, "#walls/*.sxy", 90);
			  }
			  catch (IException&)
			  {
				  editor.AddPingPath("wall script", wallScript, IO::MAX_PATHLEN, "#walls/*.sxy", 90);
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
				  editor.AddPingPath("script file", floorScript, IO::MAX_PATHLEN, "#floors/*.sxy", 120);
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
				  editor.AddPingPath("corridor script", corridorScript, IO::MAX_PATHLEN, "!scripts/hv/sector/*.sxy", 130);
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
	  }

	  ISectorLayout* Layout() override
	  {
		  return this;
	  }

	  ID_ENTITY AddItemToLargestSquare(const fstring& meshName, int addItemFlags, const HV::ObjectCreationSpec& obs) override
	  {
		  if (IsCorridor() || completeSquares.empty()) return ID_ENTITY::Invalid();

		  auto& aabb = completeSquares[0];

		  ID_SYS_MESH meshId;
		  AABB bounds;
		  if (!platform.meshes.TryGetByName(meshName, meshId, bounds)) return ID_ENTITY::Invalid();

		  AABB worldBounds;
		  Matrix4x4 model;
		  if (!TryGetRandomTransformation(model, worldBounds, addItemFlags & AddItemFlags_RandomHeading, addItemFlags & AddItemFlags_RandomPosition, bounds, aabb, z0, z1))
		  {
			  return ID_ENTITY::Invalid();
		  }

		  auto id = platform.instances.AddBody(meshName, model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
		  scenery.push_back({ id,worldBounds });
		  return id;
	  }

	  bool TryGetScenery(ID_ENTITY id, AABB& worldBounds) const
	  {
		  for (auto& cp : scenery)
		  {
			  if (cp.id == id)
			  {
				  worldBounds = cp.worldBounds;
				  return true;
			  }
		  }

		  return false;
	  }

	  struct PlacementCandidate
	  {
		  AABB worldBounds;
		  Matrix4x4 model;
	  };

		bool DoesSceneryCollide(const AABB& aabb) const
		{
			for (auto& s : scenery)
			{
				if (s.worldBounds.Intersects(aabb))
				{
					return true;
				}
			}

			return false;
		}

		std::vector<PlacementCandidate> candidates;

		virtual ID_ENTITY AddSceneryAroundObject(const fstring& mesh, ID_ENTITY centrePieceId, const HV::InsertItemSpec& iis, const HV::ObjectCreationSpec& ocs) override
		{
			if (IsCorridor() || completeSquares.empty()) return ID_ENTITY::Invalid();

			auto& aabb = completeSquares[0];

			AABB centreBounds;
			if (!TryGetScenery(centrePieceId, centreBounds))
			{
				return ID_ENTITY::Invalid();
			}

			auto* e = platform.instances.GetEntity(centrePieceId);
			if (e == nullptr)
			{
				return ID_ENTITY::Invalid();
			}

			ID_SYS_MESH meshId;
			AABB bounds;
			if (!platform.meshes.TryGetByName(mesh, meshId, bounds)) return ID_ENTITY::Invalid();

			candidates.clear();

			for (int i = 0; i < 100; i++)
			{
				AABB worldBounds;
				Matrix4x4 model;
				if (!TryGetRandomTransformation(model, worldBounds, iis.insertFlags & AddItemFlags_RandomHeading, iis.insertFlags & AddItemFlags_RandomPosition, bounds, aabb, z0, z1))
				{
					return ID_ENTITY::Invalid();
				}

				if (!DoesSceneryCollide(worldBounds))
				{
					candidates.push_back({ worldBounds, model });
				}
			}

			if (candidates.empty())
			{
				return ID_ENTITY::Invalid();
			}

			struct
			{
				Vec3 centrePieceOrigin;
				bool operator ()(const PlacementCandidate& a, const PlacementCandidate& b) const
				{
					auto Pa = a.worldBounds.Centre();
					auto Pb = b.worldBounds.Centre();

					float Rao = LengthSq(Pa - centrePieceOrigin);
					float Rbo = LengthSq(Pb - centrePieceOrigin);

					return Rao < Rbo;
				}
			} sortByRangeFromCentrePiece;

			struct
			{
				float optimalRangeSq;
				Vec3 centrePieceOrigin;

				bool operator ()(const PlacementCandidate& a, const PlacementCandidate& b) const
				{
					auto Pa = a.worldBounds.Centre();
					auto Pb = b.worldBounds.Centre();

					Vec3 ao = centrePieceOrigin - Pa;
					Vec3 bo = centrePieceOrigin - Pb;

					float Rao = LengthSq(ao);
					float Rbo = LengthSq(bo);

					if (Rao < optimalRangeSq && Rbo < optimalRangeSq)
					{
						Vec3 ao_dir;
						Vec3 bo_dir;
						if (!TryNormalize(ao, ao_dir)) ao_dir = ao;
						if (!TryNormalize(bo, bo_dir)) bo_dir = bo;

						Vec3 aDir = a.model.GetForwardDirection();
						Vec3 bDir = b.model.GetForwardDirection();

						return Dot(aDir, ao_dir) > Dot(bDir, bo_dir);
					}
					else
					{
						return Rao < Rbo;
					}
				}
			} sortByRangeFromCentrePieceAndOrientation;

			bool sortByOrientation = iis.insertFlags & AddItemFlags_AlignEdge;

			if (sortByOrientation)
			{
				sortByRangeFromCentrePieceAndOrientation.optimalRangeSq = Sq(iis.maxDistance.value);
				sortByRangeFromCentrePieceAndOrientation.centrePieceOrigin = e->Position();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePieceAndOrientation);
			}
			else
			{
				sortByRangeFromCentrePiece.centrePieceOrigin = e->Position();
				std::sort(candidates.begin(), candidates.end(), sortByRangeFromCentrePiece);
			}

			auto id = platform.instances.AddBody(mesh, candidates[0].model, Vec3{ 1,1,1 }, ID_ENTITY::Invalid());
			scenery.push_back({ id,candidates[0].worldBounds });

			return id;
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

	  float doorElevation = 0;
	  float doorDirection = 0.0f;

	  float padIntrusion = 0;
	  float padDirection = 0.0f;

	  const Metres DOOR_MAX_ELEVATION = 3.9_metres;
	  const MetresPerSecond DOOR_ELEVATION_SPEED = 0.4_mps;

	  const Metres PAD_MAX_INTRUSION = 0.1_metres;
	  const MetresPerSecond PAD_DESCENT_SPEED = 0.40_mps;
	  const MetresPerSecond PAD_ASCENT_SPEED = 1.0_mps;

	  OS::ticks lastPlayerOccupiedTime;

	  void UpdateDoor(ID_ENTITY idDoor, const IUltraClock& clock)
	  {
		  auto* door = platform.instances.GetEntity(idDoor);
		  if (!door) return;

		  doorElevation += doorDirection * clock.DT();

		  if (doorElevation < 0)
		  {
			  doorDirection = 0.0f;
			  doorElevation = 0.0f;
		  }

		  if (doorElevation > DOOR_MAX_ELEVATION)
		  {
			  doorDirection = 0.0f;
			  doorElevation = DOOR_MAX_ELEVATION;
		  }
		 
		  door->Model().row2.w = doorElevation;
	  }

	  void NotifySectorPlayerIsInSector(const IUltraClock& clock)
	  {
		  lastPlayerOccupiedTime = clock.FrameStart();
	  }

	  void UpdatePressurePad(ID_ENTITY idPressurePad, const IUltraClock& clock)
	  {
		  auto* pad = platform.instances.GetEntity(idPressurePad);
		  if (!pad) return;

		  padIntrusion += padDirection * clock.DT();

		  if (lastPlayerOccupiedTime >= clock.FrameStart())
		  {
			  padDirection = PAD_DESCENT_SPEED;
		  }
		  else
		  {
			  padDirection = -PAD_ASCENT_SPEED;
		  }

		  if (padIntrusion > PAD_MAX_INTRUSION)
		  {
			  padIntrusion = PAD_MAX_INTRUSION;
		  }

		  if (padIntrusion < 0)
		  {
			  padIntrusion = 0;
		  }

		  pad->Model().row2.w = -padIntrusion;
	  }

	  float leverElevation = 0_degrees;
	  float leverOmega = 90.0f;

	  const float leverSpeed = 180.0f;

	  const Degrees LEVER_UP_ANGLE = 45_degrees;
	  const Degrees LEVER_DOWN_ANGLE = -45_degrees;

	  void UpdateLever(ID_ENTITY idLeverBase, ID_ENTITY idLever, const IUltraClock& clock)
	  {
		  auto* base = platform.instances.GetEntity(idLeverBase);
		  if (base == nullptr) return;

		  auto* lever = platform.instances.GetEntity(idLever);
		  if (lever == nullptr) return;

		  size_t nTriangles;
		  auto* tris = platform.meshes.GetTriangles(base->MeshId(), nTriangles);
		  if (!tris) return;

		  auto& t = *tris;
		  Vec3 pos = (t.a.position + t.c.position) * 0.5f;
		  Vec3 normalU = Cross(t.b.position - t.a.position, t.c.position - t.a.position);
		  if (LengthSq(normalU) <= 0)
		  {
			  return;
		  }

		  leverElevation += leverOmega * clock.DT();
		  if (leverElevation < LEVER_DOWN_ANGLE)
		  {
			  leverElevation = LEVER_DOWN_ANGLE;
			  leverOmega = 0;
		  }
		  else if (leverElevation > LEVER_UP_ANGLE)
		  {
			  leverElevation = LEVER_UP_ANGLE;
			  leverOmega = 0;
		  }

		  Vec3 normal = Normalize(normalU);
		  Vec3 tangent = Normalize(t.a.position - t.b.position);
		  Vec3 bitangent = Cross(normal, tangent);

		  Matrix4x4 model =
		  {
			{ tangent.x, -bitangent.x, -normal.x,  0 },
			{ tangent.y, -bitangent.y, -normal.y,  0 },
			{ tangent.z, -bitangent.z, -normal.z,  0 },
			{         0,        0,           0, 1.0f },
		  };

		  auto Ry = Matrix4x4::RotateRHAnticlockwiseY(Degrees{ -leverElevation });

		  Matrix4x4 T = Ry * model;
		  T.row0.w = pos.x;
		  T.row1.w = pos.y;
		  T.row2.w = pos.z;

		  lever->Model() = T;
	  }

	  void OnTick(const IUltraClock& clock) override
	  {
		  if (components.empty()) return;

		  for (auto c : components)
		  {
			  if (Eq(c.name.c_str(), "door.body"))
			  {
				  UpdateDoor(c.id, clock);
				  break;
			  }
			  else if (Eq(c.name.c_str(), "pressure_pad"))
			  {
				  UpdatePressurePad(c.id, clock);
				  break;
			  }
			  else if (Eq(c.name.c_str(), "wall.lever.base"))
			  {
				  for (auto d : components)
				  {
					  if (Eq(d.name.c_str(), "wall.lever"))
					  {
						  UpdateLever(c.id, d.id, clock);
						  break;
					  }
				  }
			  }
		  }
	  }

	  void ClickButton()
	  {
		  if (doorDirection == 0)
		  {
			  if (doorElevation > 1.5f)
			  {
				  doorDirection = -DOOR_ELEVATION_SPEED;
			  }
			  else
			  {
				  doorDirection = DOOR_ELEVATION_SPEED;
			  }
		  }
	  }

	  bool TryClickGraphicsMesh(ID_ENTITY idObject, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach)
	  {
		  auto* e = platform.instances.GetEntity(idObject);
		  auto idMesh = e->MeshId();
		  cr_m4x4 model = e->Model();

		  size_t nTriangles;
		  auto* triangles = platform.meshes.GetTriangles(idMesh, nTriangles);
		  if (triangles)
		  {
			  for (size_t i = 0; i < nTriangles; ++i)
			  {
				  Triangle T;
				  T.A = triangles[i].a.position;
				  T.B = triangles[i].b.position;
				  T.C = triangles[i].c.position;

				  Triangle ABC;
				  TransformPositions(&T.A, 3, model, &ABC.A);

				  Collision c = Rococo::CollideLineAndTriangle(ABC, probePoint, probeDirection);

				  if (c.contactType == ContactType_Face)
				  {
					  if (c.t > 0 && c.t < reach)
					  {
						  if (Dot(probeDirection, T.EdgeCrossProduct()) < 0)
						  {
							  return true;
						  }
					  }
				  }
			  }
		  }

		  return false;
	  }

	  bool TryClickLever(ID_ENTITY idLever, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach)
	  {
		  if (TryClickGraphicsMesh(idLever, probePoint, probeDirection, reach))
		  {
			  if (leverElevation < 0)
			  {
				  leverOmega = leverSpeed;
			  }
			  else
			  {
				  leverOmega = -leverSpeed;
			  }
		  }

		  return false;
	  }

	  bool TryClickButton(ID_ENTITY idButton, cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach)
	  {
		  auto* e = platform.instances.GetEntity(idButton);
		  auto idMesh = e->MeshId();
		  cr_m4x4 model = e->Model();

		  size_t nTriangles;
		  auto* triangles = platform.meshes.GetPhysicsHull(idMesh, nTriangles);
		  if (triangles)
		  {
			  for (size_t i = 0; i < nTriangles; ++i)
			  {
				  const auto& T = triangles[i];

				  Triangle ABC;
				  TransformPositions(&T.A, 3, model, &ABC.A);

				  Collision c = Rococo::CollideLineAndTriangle(ABC, probePoint, probeDirection);

				  if (c.contactType == ContactType_Face)
				  {
					  if (c.t > 0 && c.t < reach)
					  {
						  if (Dot(probeDirection, T.EdgeCrossProduct()) < 0)
						  {
							  ClickButton();
							  return true;
						  }
					  }
				  }
			  }
		  }

		  return false;
	  }

	  bool UseAnythingAt(cr_vec3 probePoint, cr_vec3 probeDirection, Metres reach) override
	  {
		  for (auto c : components)
		  {
			  if (Eq(c.name.c_str(), "door.button.1") || Eq(c.name.c_str(), "door.button.2"))
			  {
				  if (TryClickButton(c.id, probePoint, probeDirection, reach))
				  {
					  return true;
				  }
			  }

			  if (Eq(c.name.c_str(), "wall.lever"))
			  {
				  if (TryClickLever(c.id, probePoint, probeDirection, reach))
				  {
					  return true;
				  }
			  }
		  }

		  return false;
	  }

	  ITriggersAndActions& TriggersAndActions() override { return *this; }
   };
}

namespace HV
{
	ISector* CreateSector(Platform& platform, ISectors& co_sectors)
	{
		return new ANON::Sector(platform, co_sectors);
	}

	void RebaseSectors()
	{
		ANON::nextSectorId = 1;
	}

	float GetHeightAtPointInSector(cr_vec3 p, ISector& sector)
	{
		int32 index = sector.GetFloorTriangleIndexContainingPoint({ p.x, p.y });
		if (index >= 0)
		{
			auto* v = sector.FloorVertices().v;
			Triangle t;
			t.A = v[3 * index].position;
			t.B = v[3 * index + 1].position;
			t.C = v[3 * index + 2].position;

			float h;
			if (GetTriangleHeight(t, { p.x,p.y }, h))
			{
				return h;
			}
		}

		return 0.0f;
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
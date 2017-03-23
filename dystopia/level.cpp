#include <rococo.types.h>
#include "dystopia.h"
#include <rococo.io.h>
#include "meshes.h"
#include <vector>
#include <unordered_map>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <sexy.vm.cpu.h>

#include <rococo.maths.h>
#include <rococo.strings.h>
#include <rococo.sexy.ide.h>

#include "component.system.inl"

#include "human.types.h"

#include "dystopia.post.h"

#include "skeleton.h"


using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct : IScene
	{
		virtual RGBA GetClearColour() const
		{
			return clearColour;
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			Graphics::RenderHorizontalCentredText(grc, text.c_str(), RGBAb(255, 255, 255, 255), 1, { metrics.cursorPosition.x >> 1, 100 });
		}

		virtual void RenderObjects(IRenderContext& rc)
		{

		}

      virtual void AddOverlay(int zorder, IUIOverlay* overlay)
      {

      }

      virtual void RemoveOverlay(IUIOverlay* overlay)
      {

      }

		std::wstring text = L"Loading...";
		RGBA clearColour = RGBA(0.5f, 0, 0);
	} customLoadScene;

	struct Solid
	{
		Matrix4x4 transform;
		RGBA highlightColour;
		Vec3 position;

		// scale gives x y and z scale factors in model co-ordinates
		Vec3 scale;
		// theta gives rotation around z axis, from x to y
		// phi gives rotation around y axis, giving elevation
		Radians theta;
		Radians phi;

		ID_SYS_MESH sysMeshId;
		ID_MESH meshId;
		Metres boundingRadius;
		ID_BITMAP bitmapId;
		uint32 flags;

		void AddFlag(SolidFlags flag) { flags |= flag; }
		bool HasFlag(SolidFlags flag) const { return (flags & flag) != 0; }
		void RemoveFlag(SolidFlags flag) { flags &= ~flag; }
	};

	struct Projectile
	{
		ID_ENTITY attacker;
		Vec3 position;
		Vec3 direction;
		Vec3 velocity;
		float lifeTime;
		float creationTime;
		ID_SYS_MESH bulletMesh;
	};

	struct Dynamics
	{
		Vec3 velocity;
	};

	struct Human
	{
		Dynamics dynamics;
		HumanType type;
		float nextAIcheck;
		AutoFree<IInventorySupervisor> inventory;
		AutoFree<IHumanAISupervisor> ai;
	};

	Sphere BoundingSphere(const Solid& solid)
	{
		return Sphere{ solid.transform.GetPosition() , solid.boundingRadius };
	}

	ID_ENTITY GetFirstIntersect(cr_vec3 start, cr_vec3 end, const EntityTable<Solid>& solids, IQuadTree& quadTree)
	{
		struct : IObjectEnumerator
		{
			float leastT = 2.0f;
			ID_ENTITY firstTarget;
			const EntityTable<Solid>* solids;
			Vec3 start;
			Vec3 end;
			virtual void OnId(uint64 id)
			{
				const Solid& solid = solids->find(ID_ENTITY(id))->second;
				auto sphere = BoundingSphere(solid);

				float t0, t1;
				if (TryGetIntersectionLineAndSphere(t0, t1, start, end, sphere))
				{
					if (t0 > 0 && t0 < 1 && t0 < leastT)
					{
						leastT = t0;
						firstTarget = ID_ENTITY(id);
					}
				}
			}
		} onId;

		onId.start = start;
		onId.end = end;
		onId.solids = &solids;
		float radius = sqrtf(Square(end.x - start.x) + Square(end.y -start.y));
		Sphere boundingSphere{ start, radius };
		quadTree.EnumerateItems(boundingSphere, onId);

		return onId.firstTarget;
	}

	bool Intersects(cr_vec3 start, cr_vec3 end, ID_ENTITY id, const EntityTable<Solid>& solids)
	{
		const Solid& solid = solids.find(id)->second;
		auto sphere = BoundingSphere(solid);

		float t0, t1;
		if (TryGetIntersectionLineAndSphere(t0, t1, start, end, sphere))
		{
			if (t0 > 0 && t0 < 1)
			{
				return true;
			}
		}

		return false;
	}

	struct Equipment
	{
		IInventorySupervisor* inventory;
	};

	float GetLateralDisplacement(cr_vec3 a, cr_vec3 b)
	{
		return sqrtf(Square(a.x - b.x) + Square(a.y - b.y));
	}

	void TransformCube(const BoundingCube& cube, BoundingCube& rotatedCube, cr_m4x4 transform)
	{
		TransformPositions(cube.topVertices.vertices, 4, transform, rotatedCube.topVertices.vertices);
		TransformPositions(cube.bottomVertices.vertices, 4, transform, rotatedCube.bottomVertices.vertices);

		for (int i = 0; i < 6; ++i)
		{
			rotatedCube.P.planes[i].pointInPlane = transform * cube.P.planes[i].pointInPlane;
			rotatedCube.P.planes[i].normal = Vec4::FromVec3(Normalize(transform * cube.P.planes[i].normal), 0.0f);
		}
	}

	Collision FindIntersectSphereVsSolid(cr_vec3 start, cr_vec3 target, const Solid& obstacle, float projectileRadius, IMeshLoader& meshes)
	{
		Vec3 delta = target - start;

		if (Dot(delta, obstacle.position - start) < 0)
		{
			return NoCollision();
		}

		struct : IEnumerator<BoundingCube>
		{
			Vec3 start;
			Vec3 target; 
			const Solid* obstacle;
			float radius;
			Collision firstCollision = NoCollision();

			virtual void operator()(const BoundingCube& cube)
			{
				BoundingCube rotatedCube;
				TransformCube(cube, rotatedCube, obstacle->transform);
				auto col = CollideBoundingBoxAndSphere(rotatedCube, Sphere{ start, radius }, target);
				if (col.contactType != ContactType_None)
				{
					firstCollision = col;
				}
			}
		} collideAgainstHull;

		collideAgainstHull.start = start;
		collideAgainstHull.target = target;
		collideAgainstHull.obstacle = &obstacle;
		collideAgainstHull.radius = projectileRadius;

		meshes.ForEachPhysicsHull(obstacle.meshId, collideAgainstHull);

		return collideAgainstHull.firstCollision;
	}

	Collision CollideBodies(cr_vec3 start, cr_vec3 target, Metres projectileRadius, const Solid& obstacle, IMeshLoader& meshes)
	{
		return FindIntersectSphereVsSolid(start, target, obstacle, projectileRadius, meshes);
	}

	struct CollisionEx
	{
		Collision collision;
		ID_ENTITY id;
	};

	CollisionEx CollideWithGeometry(cr_vec3 start, cr_vec3 target, ID_ENTITY projectileId, Metres projectileRadius, IQuadTree& quadTree, EntityTable<Solid>& solids, IMeshLoader& meshes)
	{
		float radius = GetLateralDisplacement(start, target);
		Sphere boundingSphere{ start, radius + projectileRadius };

		struct : IObjectEnumerator
		{
			ID_ENTITY projectileId;
			Metres projectileRadius;
			Solid* target;
			EntityTable<Solid>* solids;
			Vec3 start;
			Vec3 end;
			CollisionEx cex = { NoCollision(), ID_ENTITY::Invalid() };
			IMeshLoader* meshes;

			virtual void OnId(uint64 idNumber)
			{
				ID_ENTITY obstacleId(idNumber);
				if (obstacleId != projectileId)
				{
					auto& c = solids->find(obstacleId);
					if (c != solids->end() && c->second.HasFlag(SolidFlags_Obstacle))
					{
						try
						{
							auto col = CollideBodies(start, end, projectileRadius, c->second, *meshes);
							if (col.t < cex.collision.t)
							{
								cex = { col, obstacleId };
							}
						}
						catch (IException& ex)
						{
							Throw(ex.ErrorCode(), L"Error colliding with body %I64u, against mesh %d:\n %s", (uint64)projectileId, c->second.meshId, ex.Message());
						}
					}
				}
			}
		} onTarget;

		onTarget.projectileId = projectileId;
		onTarget.projectileRadius = projectileRadius;
		onTarget.solids = &solids;
		onTarget.start = start;
		onTarget.end = target;
		onTarget.meshes = &meshes;

		quadTree.EnumerateItems(boundingSphere, onTarget);

		return onTarget.cex;
	}

	class Level : public ILevelSupervisor, public ILevelBuilder, public Post::IRecipient
	{
		Environment& e; // not valid until constructor returns
		IHumanFactory& hf;
		ID_ENTITY idPlayer;
		EntityTable<Solid> solids;
		EntityTable<Projectile> projectiles;
		EntityTable<Human*> enemies;
		EntityTable<Human*> allies;
		EntityTable<Equipment> equipment;
		EntityTable<ISkeletonSupervisor*> skeletons;
		EntityTable<std::wstring> names;
		Vec3 groundZeroCursor;
		ID_ENTITY selectedId;
		AutoFree<IQuadTreeSupervisor> quadTree;
		float gameTime;
		float lastDt;
		ID_ENTITY nearestRoadSection;

		std::vector<ID_ENTITY> lastRendered;
		std::vector<std::wstring> streetNames;
		std::vector<ID_ENTITY> abattoir;

      Metres viewRadius;
	public:
		Level(Environment& _e, IHumanFactory& _hf) : 
			e(_e),
			hf(_hf),
			groundZeroCursor{ 0,0,0 },
			quadTree(CreateLooseQuadTree(4096.0_metres, 3.0_metres)),
			gameTime(0),
			lastDt(0),
         viewRadius(20.0_metres)
		{
			
		}

		virtual void OnCreated()
		{
			e.postbox.Subscribe<VerbInventoryChanged>(this);
		}

		~Level()
		{
			Clear();
		}

		virtual void SetLevel(const fstring& filename)
		{
			e.levelLoader.SetNextLevel(filename);
		}

      virtual void SetRenderRadius(Metres radius)
      {
         viewRadius = radius;
      }

		virtual ID_ENTITY NearestRoadId() const
		{
			return nearestRoadSection;
		}

		virtual const wchar_t* TryGetName(ID_ENTITY id)
		{
			auto i = names.find(id);
			return (i != names.end()) ? i->second.c_str() : nullptr;
		}

		virtual void GenerateCity(const fstring& name, Metres radius)
		{
			struct : IEnumerable<const wchar_t*>
			{
				virtual const wchar_t* operator[](size_t index)
				{
					return (*names)[index].c_str();
				}

				virtual size_t Count() const
				{
					return names->size();
				}

				virtual void Enumerate(IEnumerator<const wchar_t*>& cb)
				{
					for (auto& i : *names)
					{
						cb(i.c_str());
					}
				}
				std::vector<std::wstring>* names;
			} enumerable;

			enumerable.names = &streetNames;

			BuildRandomCity_V2(name, radius, 0, e, enumerable);
		}

		virtual void AddStreetName(const fstring& name)
		{
			streetNames.push_back(std::wstring(name));
		}

		virtual void Name(ID_ENTITY entityId, const fstring& name)
		{
			if (!names.insert(entityId, std::wstring(name)).second)
			{
				Throw(0, L"Could not insert name: %s", name.buffer);
			}
		}

		virtual void PopulateCity(float populationDensity)
		{
			std::vector<Vec3> spawnPoints;

			for (auto i : solids)
			{
				if (i.second.meshId.value > 0x21000000 && i.second.meshId.value <= 0x21FFFFFF)
				{
					spawnPoints.push_back(i.second.position);
				}
			}

			size_t updateMod = spawnPoints.size() / 10;
			size_t index = 0;
			for (auto i : spawnPoints)
			{
				if ((index % updateMod) == 0)
				{
					customLoadScene.clearColour.red = 1.0f - index / (float) spawnPoints.size();
					wchar_t text[256];
					SafeFormat(text, _TRUNCATE, L"Spawning %I64u of %I64u enemies", index, spawnPoints.size());
					customLoadScene.text = text;
					e.renderer.Render(customLoadScene);
				}

				float f = populationDensity;
				while (f > 1.0f)
				{
					AddEnemy(i, ID_MESH(3));
					f -= 1.0f;
				}

				float g = rand() / (float) RAND_MAX;

				if (f >= g)
				{
					AddEnemy(i, ID_MESH(3));
				}

				index++;
			}
		}

		virtual ILevelBuilder& Builder()
		{
			return *this;
		}

		virtual void Clear()
		{
			FreeTable(allies);
			FreeTable(enemies);
			FreeTable(skeletons);
			solids.clear();
			for (auto i : equipment)
			{
				i.second.inventory->Free();
			}
			equipment.clear();
			quadTree->Clear();
			streetNames.clear();
			names.clear();
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddAmmunition(cr_vec3 position, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 ammoType, float massPerBullet, float massPerClip, int32 count)
		{
			auto id = AddSolid(position, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ massPerClip };
			itemData.bitmapId = e.bitmapCache.Cache(imageFile);
			inv->Swap(0, CreateAmmo({ massPerBullet, ammoType, count }, itemData));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddRangedWeapon(cr_vec3 position, ID_MESH editorId, const fstring& name, const fstring& imageName, float muzzleVelocity, float flightTime, int32 ammoType, float mass)
		{
			auto id = AddSolid(position, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ mass };
			itemData.bitmapId = e.bitmapCache.Cache(imageName);
			inv->Swap(0, CreateRangedWeapon({ flightTime, muzzleVelocity, ammoType }, itemData ));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddArmour(cr_vec3 position, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 bulletProt, int32 dollSlot, float massKg)
		{
			if (dollSlot < 0 || dollSlot >= PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO)
			{
				Throw(0, L"Could not add armour: %s. dollslot was %d. 0...%d", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO-1);
			}

			if (bulletProt < 0)
			{
				Throw(0, L"Could not add armour: %s. bulletProt was -ve", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO - 1);
			}

			auto id = AddSolid(position, editorId, SolidFlags_None);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ massKg };
			itemData.bitmapId = e.bitmapCache.Cache(imageFile);
			itemData.slot = (PAPER_DOLL_SLOT) dollSlot;
			inv->Swap(0, CreateArmour(ArmourValue {(uint32) bulletProt}, itemData));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual bool TryGetEquipment(ID_ENTITY id, EquipmentDesc& desc) const
		{
			auto x = equipment.find(id);
			if (x != equipment.end())
			{
				auto& solid = solids.find(id);
				desc.worldPosition = solid->second.transform.GetPosition();
				desc.inventory = x->second.inventory;
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual void Delete(ID_ENTITY id)
		{
			abattoir.push_back(id);
		}

		virtual ID_ENTITY AddSolid(cr_vec3 position, ID_MESH meshId, int32 flags)
		{
			auto id = GenerateEntityId();
			ID_SYS_MESH sysId = e.meshes.GetRendererId(meshId);

         auto normalRadius = e.meshes.GetNormalBoundingRadius(meshId);

			Matrix4x4 transform = Matrix4x4::Translate(position);

			solids.insert(id, 
				/*
					Matrix4x4 transform;
					RGBA highlightColour;
					Vec3 position;

					// scale gives x y and z scale factors in model co-ordinates
					Vec3 scale;
					// theta gives rotation around z axis, from x to y
					// phi gives rotation around y axis, giving elevation
					Degrees theta;
					Degrees phi;

					ID_SYS_MESH sysMeshId;
					ID_MESH meshId;
					Metres boundingRadius;
					ID_BITMAP bitmapId;
					uint32 flags;
				*/
				Solid
				{ 
					transform, {0,0,0,0}, position, {1,1,1}, 0, 0, sysId, meshId, normalRadius, ID_BITMAP::Invalid(), (uint32)flags
				}
			);

			quadTree->AddEntity(Sphere{ transform.GetPosition(), normalRadius }, id);
			return id;
		}

		virtual ID_ENTITY AddAlly(cr_vec3 lateralPosition, ID_MESH meshId)
		{
			Vec3 position{ lateralPosition.x, lateralPosition.y, 1.3_metres };
			ID_ENTITY id = AddSolid(position, meshId, SolidFlags_Skeleton);
			auto& s = solids.find(id);
			s->second.boundingRadius = 0.5_metres;
			auto* inv = CreateInventory({ 4, 10 }, true, true);
			auto h = new Human{ {0,0,0}, HumanType_Vigilante, 0.0f, inv, hf.CreateHuman(id, HumanType_Vigilante) };
			skeletons.insert(id, CreateSkeleton(e));
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			allies.insert(id, h);
			return id;
		}

		virtual ID_ENTITY AddEnemy(cr_vec3 lateralPosition, ID_MESH meshId)
		{
			Vec3 position{ lateralPosition.x, lateralPosition.y, 1.3_metres };
			ID_ENTITY id = AddSolid(position, meshId, SolidFlags_Skeleton);
			auto* inv = CreateInventory({ 3,4 }, true, true);
			auto h = new Human{ { 0,0,0 }, HumanType_Bobby, 0.0f, inv, hf.CreateHuman(id, HumanType_Bobby) };
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			enemies.insert(id, h);
			skeletons.insert(id, CreateSkeleton(e));
			return id;
		}

		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime)
		{
			auto id = GenerateEntityId();

			Vec3 direction = Normalize(def.velocity);

			projectiles.insert(id, Projectile{ def.attacker, def.origin, direction, def.velocity, def.lifeTime, currentTime, def.bulletMesh });

			return id;
		}

	    virtual ID_ENTITY CreateStash(IItem* item, cr_vec3 location)
		{
			ID_MESH stashId(0x40000000);
			if (e.meshes.GetRendererId(stashId) == ID_SYS_MESH::Invalid())
			{
				e.meshes.Load(L"!mesh/stash.sxy"_fstring, stashId);
			}
			
			auto id = Builder().AddSolid(location, stashId, SolidFlags_Selectable);

			SetScale(id, Vec3{ 0.37f, 0.37f, 0.37f });

			auto& solid = solids.find(id)->second;

			solid.bitmapId = item ? item->Data().bitmapId : ID_BITMAP::Invalid();

			Equipment eq{ CreateInventory({ 4,4 }, false, false) };
			equipment.insert(id, eq);
			eq.inventory->Swap(0, item); // This is new inventory, so returned item is nullptr

			return id;
		}

		virtual HumanSpec GetHuman(ID_ENTITY id)
		{
			auto i = enemies.find(id);
			if (i == enemies.end())
			{
				i = allies.find(id);
				if (i == allies.end())
				{
					return{ HumanType_None, nullptr, nullptr };
				}
			}

			auto& h = *i->second;
			return{ h.type, h.inventory, h.ai };
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* change = Post::InterpretAs<VerbInventoryChanged>(mail);
			if (change)
			{
				EquipmentDesc eq;
				if (TryGetEquipment(change->containerId, eq))
				{
					if (eq.inventory->EnumerateItems(nullptr) == 0)
					{
						Delete(change->containerId);
					}
				}
			}
		}

		virtual void SetGroundCursorPosition(cr_vec3 groundZero)
		{
			groundZeroCursor = groundZero;
		}

		virtual ID_ENTITY SelectedId()
		{
			return selectedId;
		}

		void UpdateTransform(ID_ENTITY id, Solid& solid)
		{
			if (solid.HasFlag(SolidFlags_IsDirty))
			{
				Matrix4x4 S = Matrix4x4::Scale(solid.scale.x, solid.scale.y, solid.scale.z);
				Matrix4x4 R = Matrix4x4::RotateRHAnticlockwiseX(solid.phi) * Matrix4x4::RotateRHAnticlockwiseZ(solid.theta);
				Matrix4x4 T = Matrix4x4::Translate(solid.position);

				cr_vec3 currentPos = solid.transform.GetPosition();
				if (currentPos != solid.position)
				{
					quadTree->DeleteEntity(Sphere{ currentPos, solid.boundingRadius }, id);
					quadTree->AddEntity(Sphere{ solid.position, solid.boundingRadius }, id);
				}

				solid.transform = T * R * S;

				solid.RemoveFlag(SolidFlags_IsDirty);
			}
		}

		virtual bool TryGetTransform(ID_ENTITY id, Matrix4x4& transform)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				return false;
			}
			else
			{
				UpdateTransform(id, i->second);
				transform = i->second.transform;
				return true;
			}
		}

		virtual void SetHeading(ID_ENTITY id, Radians theta)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity %I64u", (uint64)id);
			}

			i->second.theta = theta;
			i->second.AddFlag(SolidFlags_IsDirty);
		}

		virtual void SetElevation(ID_ENTITY id, Radians phi)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64) id);
			}

			i->second.phi = phi;
			i->second.AddFlag(SolidFlags_IsDirty);
		}

		virtual void SetScale(ID_ENTITY id, cr_vec3 scale)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64)id);
			}

			i->second.scale = scale;
			i->second.AddFlag(SolidFlags_IsDirty);

         auto R = e.meshes.GetNormalBoundingRadius(i->second.meshId);

         // Hack here ->our game's bounding radius is mainly for use in the xy plane, for AI and physics, the z direction is
         // generally irrelevant. so ignore the z scale factor
         float maxScale = max(scale.x, scale.y);
         i->second.boundingRadius = Metres{ R * maxScale };
		}

		virtual cr_vec3 GetPosition(ID_ENTITY id) const
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			return i->second.position;
		}

		virtual const Vec3* TryGetPosition(ID_ENTITY id) const
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				return nullptr;
			}

			return &i->second.position;
		}

		virtual void SetNextAIUpdate(ID_ENTITY id, float nextUpdateTime)
		{
			auto i = allies.find(id);
			if (i == allies.end())
			{
				i = enemies.find(id);
				if (i == enemies.end())
				{
					Throw(0, L"Invalid entity Id %I64u", (uint64)id);
				}
			}

			i->second->nextAIcheck = nextUpdateTime;
		}

		virtual void SetPosition(ID_ENTITY id, cr_vec3 targetPos)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id %I64u", (uint64)id);
			}

			auto& solid = i->second;
			Vec3 currentPos = solid.transform.GetPosition();
			
			if (targetPos != currentPos)
			{
				quadTree->DeleteEntity(Sphere{ currentPos, solid.boundingRadius }, id);
				quadTree->AddEntity(Sphere { targetPos, solid.boundingRadius } , id);

				solid.position = targetPos;

				solid.transform.row0.w = targetPos.x;
				solid.transform.row1.w = targetPos.y;
				solid.transform.row2.w = targetPos.z;
			}
		}

		virtual cr_vec3 GetVelocity(ID_ENTITY id) const
		{
			auto i = allies.find(id);
			if (i == allies.end())
			{
				i = enemies.find(id);
				if (i == enemies.end())
				{
					Throw(0, L"Invalid entity Id %I64u", (uint64)id);
				}
			}

			return i->second->dynamics.velocity;
		}

		virtual void SetVelocity(ID_ENTITY id, cr_vec3 velocity)
		{
			auto i = allies.find(id);
			if (i == allies.end())
			{
				i = enemies.find(id);
				if (i == enemies.end())
				{
					Throw(0, L"Invalid entity Id %I64u", (uint64)id);
				}
			}

			i->second->dynamics.velocity = velocity;
		}

		// This should only be called by the abattoir clean up code
		void DeleteSolid(ID_ENTITY id)
		{
			auto i = solids.find(id);
			if (i != solids.end())
			{
				auto& entity = i->second;
				auto& t = i->second.transform;

				names.erase(id);

				if (entity.HasFlag(SolidFlags_Skeleton))
				{
					auto j = skeletons.find(id);
					if (j != skeletons.end())
					{
						j->second->Free();
						skeletons.erase(j);
					}

					auto k = allies.find(id);
					if (k != allies.end())
					{
						k->second->ai->Free();
						k->second->inventory->Free();
						allies.erase(k);
					}

					k = enemies .find(id);
					if (k != enemies.end())
					{
						k->second->ai->Free();
						k->second->inventory->Free();
						enemies.erase(k);
					}
				}
				else
				{
					auto eq = equipment.find(id);
					if (eq != equipment.end())
					{
						eq->second.inventory->Free();
						equipment.erase(eq);
					}
				}

				quadTree->DeleteEntity(Sphere{ t.GetPosition(), entity.boundingRadius }, id);
				solids.erase(i);
			}
		}

		virtual ID_ENTITY SelectedId() const
		{
			return selectedId;
		}

		void RenderSolidsBySearch(IRenderContext& rc)
		{
			cr_vec3 centre = GetPosition(idPlayer);

			struct : IObjectEnumerator
			{
				virtual void OnId(uint64 idValue)
				{
					ID_ENTITY id(idValue);
					auto& i = solids->find(id);
					if (i == solids->end())
					{
						Throw(0, L"Entity #%I64u was registered in the quad tree, but not the solids table. BUG!", idValue);
					}

					auto& entity = i->second;

					This->UpdateTransform(id, i->second);

					cr_vec3 pos = entity.position;
					Vec3 delta = pos - groundZeroCursor;

					if (entity.HasFlag(SolidFlags_RoadSection))
					{
						float l2 = LengthSq(centre - pos);
						if (l2 < roadSectionRange)
						{
							roadSectionRange = l2;
							closestRoadSectionId = id;
						}
					}

					float DS2 = Square(delta.x) + Square(delta.y);
					bool isHighlighted = entity.HasFlag(SolidFlags_Selectable) &&  DS2 < Square(entity.boundingRadius);

					if (entity.HasFlag(SolidFlags_Skeleton))
					{
						auto sk = skeletons->find(id);
						if (sk == skeletons->end())
						{
							Throw(0, L"Entity #%I64u was marked as having a skeleton, but none could be found.", idValue);
						}
						
						lastRendered->push_back(id);

                  if (renderSkeletons) sk->second->Render(*rc, ObjectInstance{ entity.transform, entity.highlightColour }, Seconds{ gameTime });
					}
					else
					{
						if (entity.bitmapId != ID_BITMAP::Invalid())
						{
							bitmaps->SetMeshBitmap(*rc, entity.bitmapId);
						}

						if (isHighlighted && !foundItem)
						{
							selectedId = i->first;
							foundItem = true;
							ObjectInstance instance{ entity.transform,{ 1.0f, 1.0f, 1.0f, 0.5f } };
							rc->Draw(entity.sysMeshId, &instance, 1);
						}
						else
						{
							ObjectInstance instance{ entity.transform, entity.highlightColour };
							rc->Draw(entity.sysMeshId, &instance, 1);
						}
					}

					count++;
				}

				IRenderContext* rc;
				EntityTable<Solid>* solids;
				EntityTable<ISkeletonSupervisor*>* skeletons;
				ID_ENTITY selectedId;
				bool foundItem = false;
				Vec3 groundZeroCursor;
				IBitmapCache* bitmaps;
				int count = 0;
				float gameTime;
				float lastDt;
				Level* This;
				std::vector<ID_ENTITY>* lastRendered;
				ID_ENTITY closestRoadSectionId;
				float roadSectionRange;
				Vec3 centre;
            bool renderSkeletons;
			} renderSolid;

			renderSolid.centre = centre;
			renderSolid.roadSectionRange = 1.0e20f;
			renderSolid.This = this;
			renderSolid.lastRendered = &lastRendered;
			renderSolid.groundZeroCursor = groundZeroCursor;
			renderSolid.solids = &solids;
			renderSolid.rc = &rc;
			renderSolid.bitmaps = &e.bitmapCache;
			renderSolid.skeletons = &skeletons;
			renderSolid.gameTime = gameTime;
			renderSolid.lastDt = lastDt;
         renderSolid.renderSkeletons = viewRadius < 100.0_metres;

			lastRendered.clear();
			quadTree->EnumerateItems(Sphere{ centre, viewRadius }, renderSolid);
			
			selectedId = renderSolid.selectedId;
			nearestRoadSection = renderSolid.closestRoadSectionId;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			RenderSolidsBySearch(rc);

			// N.B projectiles are not put in the quad tree, so do not appear in a search
			for (auto& i : projectiles)
			{
				auto& p = i.second;

				ObjectInstance pi{
					Matrix4x4
					{
						Vec4{0.25f,		0,		   0,  p.position.x},
						Vec4{0,		0.25f,		0,  p.position.y},
						Vec4{0,			0,	  0.25f,  p.position.z},
						Vec4{0,			0,		   0,			      1}
					},
					RGBA(0,0,0,0)
				};

				rc.Draw(p.bulletMesh, &pi, 1);
			}
		}

		void UpdateProjectile(Projectile& p, float dt, ID_ENTITY projectileId)
		{
			Vec3 g = { 0, 0, -9.81f };
			Vec3 newPos = dt * p.velocity + p.position + g * dt * dt * 0.5f;
			Vec3 newVelocity = p.velocity + g * dt;
			Vec3 oldPos = p.position;

			auto cex = CollideWithGeometry(oldPos, newPos, projectileId, 0.02_metres, *quadTree, solids, e.meshes);
			if (LengthSq(newVelocity) != 0 && cex.collision.contactType != ContactType_None)
			{
				if (cex.collision.t > 0 && cex.collision.t < 1.0f)
				{
					float skinDepth = 0.01f; // Prevent object exactly reaching target to try to avoid numeric issues
					Vec3 collisionPos = oldPos + newVelocity * dt * cex.collision.t * (1 - skinDepth);
					Vec3 impulseDisplacement = collisionPos - cex.collision.touchPoint;

					Vec3 impulseDirection = Normalize(impulseDisplacement);

					float restitution = 0.25f;
						
					newVelocity = (newVelocity + 2.0f * impulseDirection * Length(newVelocity)) * restitution;
					newPos = collisionPos + dt * newVelocity * (1 - cex.collision.t);

					auto cex2 = CollideWithGeometry(collisionPos, newPos, projectileId, 0.02_metres, *quadTree, solids, e.meshes);
					if (cex2.collision.contactType != ContactType_None)
					{
						p.lifeTime = 0;
						return;
					}
				}
			}

			p.velocity = newVelocity;
			p.position = newPos;

			if (!TryNormalize(p.velocity, p.direction))
			{
				p.direction = Vec3{ 0, 0, 1 };
			}

			if (p.position.z < 0)
			{
				p.lifeTime = 0;
			}

			if (p.attacker == idPlayer)
			{
				ID_ENTITY idEnemy = GetFirstIntersect(oldPos, newPos, solids, *quadTree);
				if (idEnemy)
				{
					auto i = enemies.find(idEnemy);
					if (i != enemies.end())
					{
						i->second->ai->OnHit(idPlayer);
					}

					p.lifeTime = 0;
				}
			}
			else
			{
				if (Intersects(oldPos, newPos, idPlayer, solids))
				{
					allies[idPlayer]->ai->OnHit(p.attacker);
					p.lifeTime = 0;
				}
			}
		}

		void UpdatePosition(ID_ENTITY id, float dt, EntityTable<Human*>& humans)
		{
			cr_vec3 pos = GetPosition(id);

			auto i = humans.find(id);
			if (i == humans.end())
			{
				return;
			}
			
			auto skel = skeletons.find(id);
			if (skel == skeletons.end())
			{
				return;
			}
			
			auto j = solids.find(id);
			if (j != solids.end())
			{
				Vec3& velocity = i->second->dynamics.velocity;
				Vec3 direction;
				if (!TryNormalize(velocity, direction))
				{
					skel->second->SetCurrentAnimation(AnimationType_Standstill);
					return;
				}

				skel->second->SetCurrentAnimation(AnimationType_Running);

				float speed = Length(velocity);

				if (speed == 0.0f) return;

				Vec3 newPos = pos + velocity * dt;

				auto cex = CollideWithGeometry(pos, newPos, id, j->second.boundingRadius, *quadTree, solids, e.meshes);
				if (cex.collision.contactType != ContactType_None)
				{
					if (cex.collision.t > 0 && cex.collision.t < 1.0f)
					{
						float skinDepth = 0.01f; // Prevent object exactly reaching target to try to avoid numeric issues
						Vec3 collisionPos = pos + velocity * dt * cex.collision.t * (1 - skinDepth);
						Vec3 impulseDisplacement = collisionPos - cex.collision.touchPoint;

						impulseDisplacement.z = 0;

						Vec3 impulseDirection = Normalize(impulseDisplacement);

						float componentOfVelocityAlongImpulse = Dot(velocity, impulseDirection);
	
						const float restitution = 0.25f; 
						// N.B velocity is reference to velocity, we are changing the actual velocity here
						velocity = restitution * (velocity + componentOfVelocityAlongImpulse * impulseDirection);
						
						newPos = collisionPos + velocity * (1 - cex.collision.t);

						AICollision aiNotify = 
						{
							id, 
							cex.id,
							collisionPos,
							impulseDirection
						};

						i->second->ai->Send(aiNotify);

						auto col2 = CollideWithGeometry(collisionPos, newPos, id, j->second.boundingRadius, *quadTree, solids, e.meshes);
						if (col2.collision.contactType != ContactType_None)
						{
							return; // If a second collision occurs in the same timestep prevent motion altogether to avoid race conditions
						}	
					}
				}

				SetPosition(id, newPos);
			}	
		}

		void UpdateEnemy(Human& enemy, ID_ENTITY id, Seconds gameTime, Seconds dt)
		{
			UpdatePosition(id, dt, enemies);

			if (enemy.nextAIcheck < gameTime)
			{
				enemy.ai->Update(gameTime, dt);
			}
		}

		virtual IInventory* GetInventory(ID_ENTITY id)
		{
			auto i = enemies.find(id);
			if (i == enemies.end())
			{
				i = allies.find(id);
				if (i == allies.end())
				{
					auto j = equipment.find(id);
					if (j != equipment.end())
					{
						return j->second.inventory;	
					}
					else
					{
						return nullptr;
					}
				}
			}

			return i->second->inventory;
		}

		virtual void UpdateGameTime(float dt)
		{
			gameTime += dt;
			lastDt = dt;

			AdvanceTimestepEvent ate{ dt, gameTime };
			e.postbox.SendDirect(ate);

			auto i = projectiles.begin();
			while (i != projectiles.end())
			{
				auto& p = i->second;
				if (p.creationTime + p.lifeTime < gameTime)
				{
					i = projectiles.erase(i);
				}
				else
				{
					UpdateProjectile(p, dt, i->first);
					i++;
				}
			}

			for (auto id : lastRendered)
			{
				auto en = enemies.find(id);
				if (en != enemies.end())
				{
					auto& enemy = en->second;
					if (enemy->ai->IsAlive())
					{
						UpdateEnemy(*enemy, id, Seconds{ gameTime }, Seconds{ dt });
					}
					else
					{
						abattoir.push_back(id);
						break;
					}
				}
			}

			for (auto dead : abattoir)
			{
				DeleteSolid(dead);
			}

			abattoir.clear();

			allies[idPlayer]->ai->Update(Seconds{ gameTime }, Seconds{ dt });

			cr_vec3 playerPos = GetPosition(idPlayer);

			UpdatePosition(idPlayer, dt, allies);
		}

		Vec3 GetForwardDirection(ID_ENTITY id)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Bad id %I64d in call to GetForwardDirection.", (uint64)id);
			}

			UpdateTransform(id, i->second);
			return Normalize(i->second.transform.GetForwardDirection());
		}

		virtual ID_ENTITY GetPlayerId() const
		{
			return idPlayer;
		}

		virtual void SetPlayerId(ID_ENTITY id)
		{
			idPlayer = id;
		}
	};

	class LevelLoader : public ILevelLoader, public IEventCallback<ScriptCompileArgs>
	{
		Environment& e;
		Rococo::IDE::IPersistentScript* levelScript;

		std::wstring nextLevelName;
	public:
		LevelLoader(Environment& _e): levelScript(nullptr), e(_e)
		{

		}

		~LevelLoader()
		{
			if (levelScript)
			{
				levelScript->Free();
				levelScript = nullptr;
			}
		}

		virtual bool NeedsUpdate() const
		{
			return !nextLevelName.empty();
		}

		virtual void SetNextLevel(const fstring& filename)
		{
			nextLevelName = filename;
		}

		virtual void Update()
		{
			if (!nextLevelName.empty())
			{
				e.level.Builder().Clear();
				e.meshes.Clear();
				e.journal.Clear();
				Load(nextLevelName.c_str(), false);
				nextLevelName.clear();
			}
		}

		virtual void ExecuteLevelFunction(const wchar_t* functionName, IArgEnumerator& args)
		{
			if (!levelScript) Throw(0, L"No level script loaded!");
			levelScript->ExecuteFunction(functionName, args, e.exceptionHandler);
		}

		virtual void ExecuteLevelFunction(ArchetypeCallback fn, IArgEnumerator& args)
		{
			if (!levelScript) Throw(0, L"No level script loaded!");
			levelScript->ExecuteFunction(fn, args, e.exceptionHandler);
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnEvent(ScriptCompileArgs& args)
		{
			AddNativeCalls_DystopiaIMeshes(args.ss, &e.meshes);
			AddNativeCalls_DystopiaILevelBuilder(args.ss, &e.level);
			AddNativeCalls_DystopiaIGui(args.ss, &e.gui);
			AddNativeCalls_DystopiaIJournal(args.ss, &e.journal);
		}

		virtual void Load(const wchar_t* resourceName, bool isReloading)
		{
			if (levelScript)
			{
				levelScript->Free();
				levelScript = nullptr;
			}

			levelScript = IDE::CreatePersistentScript(16384, e.sourceCache, e.debuggerWindow, resourceName, (int32)16_megabytes, *this, e.exceptionHandler);

			struct : IArgEnumerator
			{
				virtual void PushArgs(IArgStack& args)
				{
					args.PushInt32(0);
				}

				virtual void PopOutputs(IOutputStack& args)
				{
					int32 exitCode = args.PopInt32();
				}
			} args;
			levelScript->ExecuteFunction(L"Main", args, e.exceptionHandler);
		}
	};
}

namespace Dystopia
{
	ILevelLoader* CreateLevelLoader(Environment& e)
	{
		return new LevelLoader(e);
	}

	ILevelSupervisor* CreateLevel(Environment& e, IHumanFactory& hf)
	{
		return new Level(e, hf);
	}
}
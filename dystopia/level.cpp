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


#include "component.system.inl"

#include "human.types.h"

#include "dystopia.post.h"


using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct Solid
	{
		ObjectInstance instance;
		ID_MESH meshId;
		Metres boundingRadius;
		ID_BITMAP bitmapId;
	};

	struct Projectile
	{
		ID_ENTITY attacker;
		Vec3 position;
		Vec3 direction;
		Vec3 velocity;
		float lifeTime;
		float creationTime;
		ID_MESH bulletMesh;
	};

	struct Human
	{
		HumanType type;
		float nextAIcheck;
		AutoFree<IInventorySupervisor> inventory;
		AutoFree<IHumanAISupervisor> ai;
	};

	Sphere BoundingSphere(const Solid& solid)
	{
		return Sphere{ solid.instance.orientation.GetPosition() , solid.boundingRadius };
	}

	template<class ROW> ID_ENTITY GetFirstIntersect(cr_vec3 start, cr_vec3 end, const EntityTable<ROW>& table, const EntityTable<Solid>& solids, IQuadTree& quadTree)
	{
		struct : IQuadEnumerator
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

	struct CollisionResult
	{
		float t;
	};

	float GetLateralDisplacement(cr_vec3 a, cr_vec3 b)
	{
		return sqrtf(Square(a.x - b.x) + Square(a.y - b.y));
	}

	float FindIntersectLineVsHull(cr_vec3 start, cr_vec3 target, const Solid& obstacle, float projectileRadius, IMeshLoader& meshes)
	{
		struct : IHullEnumerator
		{
			Vec3 start;
			Vec3 target; 
			const Solid* obstacle;
			float radius;
			float firstCollision;

			virtual void OnHull(const BoundingCube& cube)
			{

			}
		} hull;

		hull.start = start;
		hull.target = target;
		hull.obstacle = &obstacle;
		hull.radius = projectileRadius;
		hull.firstCollision = 1.0f;

		meshes.EvaluatePhysicsHull(obstacle.meshId, hull);

		return hull.firstCollision;
	}

	CollisionResult CollideBodies(cr_vec3 start, cr_vec3 target, const Solid& projectile, const Solid& obstacle, IMeshLoader& meshes)
	{
		float t = FindIntersectLineVsHull(start, target, obstacle, projectile.boundingRadius, meshes);
		return{ min(t, 1.0f) };
	}

	CollisionResult CollideWithGeometry(cr_vec3 start, cr_vec3 target, ID_ENTITY id, IQuadTree& quadTree, EntityTable<Solid>& solids, IMeshLoader& meshes)
	{
		auto& s = solids.find(id);
		if (s == solids.end()) return CollisionResult{ 1.0f };

		float radius = GetLateralDisplacement(start, target);
		Sphere boundingSphere{ start, radius + s->second.boundingRadius };

		struct : IQuadEnumerator
		{
			ID_ENTITY projectileId;
			Solid* projectile;
			EntityTable<Solid>* solids;
			Vec3 start;
			Vec3 end;
			float collisionParameter;
			IMeshLoader* meshes;

			virtual void OnId(uint64 idNumber)
			{
				ID_ENTITY id(idNumber);
				if (id != projectileId)
				{
					auto& c = solids->find(id);
					if (c != solids->end())
					{
						auto result = CollideBodies(start, end, *projectile, c->second, *meshes);
						if (result.t < collisionParameter)
						{
							collisionParameter = result.t;
						}
					}
				}
			}
		} onTarget;

		onTarget.projectileId = id;
		onTarget.projectile = &s->second;
		onTarget.solids = &solids;
		onTarget.start = start;
		onTarget.end = target;
		onTarget.collisionParameter = 1.0f;
		onTarget.meshes = &meshes;

		quadTree.EnumerateItems(boundingSphere, onTarget);

		return CollisionResult{ onTarget.collisionParameter };
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
		Vec3 groundZeroCursor;
		ID_ENTITY selectedId;
		AutoFree<IQuadTreeSupervisor> quadTree;
	public:
		Level(Environment& _e, IHumanFactory& _hf) : 
			e(_e),
			hf(_hf),
			groundZeroCursor{ 0,0,0 },
			quadTree(CreateLooseQuadTree(Metres{ 4096.0 }, Metres{ 4 }))
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

		virtual ILevelBuilder& Builder()
		{
			return *this;
		}

		virtual void Clear()
		{
			FreeTable(allies);
			FreeTable(enemies);
			solids.clear();
			for (auto i : equipment)
			{
				i.second.inventory->Free();
			}
			equipment.clear();
			quadTree->Clear();
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddAmmunition(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 ammoType, float massPerBullet, float massPerClip, int32 count)
		{
			auto id = AddSolid(transform, editorId);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ massPerClip };
			itemData.bitmapId = e.bitmapCache.Cache(imageFile);
			inv->Swap(0, CreateAmmo({ massPerBullet, ammoType, count }, itemData));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageName, float muzzleVelocity, float flightTime, int32 ammoType, float mass)
		{
			auto id = AddSolid(transform, editorId);
			auto inv = CreateInventory({ 1,1 }, false, false);
			ItemData itemData;
			itemData.name = name;
			itemData.mass = Kilograms{ mass };
			itemData.bitmapId = e.bitmapCache.Cache(imageName);
			inv->Swap(0, CreateRangedWeapon({ flightTime, muzzleVelocity, ammoType }, itemData ));
			equipment.insert(id, Equipment{ inv });
			return id;
		}

		virtual ID_ENTITY AddArmour(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, const fstring& imageFile, int32 bulletProt, int32 dollSlot, float massKg)
		{
			if (dollSlot < 0 || dollSlot >= PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO)
			{
				Throw(0, L"Could not add armour: %s. dollslot was %d. 0...%d", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO-1);
			}

			if (bulletProt < 0)
			{
				Throw(0, L"Could not add armour: %s. bulletProt was -ve", name.buffer, dollSlot, PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO - 1);
			}

			auto id = AddSolid(transform, editorId);
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
				desc.worldPosition = solid->second.instance.orientation.GetPosition();
				desc.inventory = x->second.inventory;
				return true;
			}
			else
			{
				return false;
			}
		}

		virtual void DeleteEquipment(ID_ENTITY id)
		{
			auto eq = equipment.find(id);
			if (eq != equipment.end())
			{
				eq->second.inventory->Free();
			}
			equipment.erase(id);
			DeleteSolid(id);
		}

		virtual ID_ENTITY AddSolid(const Matrix4x4& transform, ID_MESH meshId)
		{
			auto id = GenerateEntityId();
			ID_MESH sysId = e.meshes.GetRendererId(meshId);
			solids.insert(id, Solid{ {transform, {0,0,0,0}}, sysId, 1.0_metres, 0 });

			quadTree->AddEntity(Sphere{ transform.GetPosition(), 1.0_metres }, id);
			return id;
		}

		virtual ID_ENTITY AddAlly(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			auto* inv = CreateInventory({ 4, 10 }, true, true);
			auto h = new Human { HumanType_Vigilante, 0.0f, inv, hf.CreateHuman(id, *inv, HumanType_Vigilante ) };
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			allies.insert(id, h);
			return id;
		}

		virtual ID_ENTITY AddEnemy(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			auto* inv = CreateInventory({ 3,4 }, true, true);
			auto h = new Human{ HumanType_Bobby, 0.0f, inv, hf.CreateHuman(id, *inv, HumanType_Bobby) };
			ItemData data;
			data.name = L"Bag of stones";
			data.bitmapId = e.bitmapCache.Cache(L"!inventory/bagofstones.tif");
			data.mass = 5.0_kg;
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5_seconds, 10.0_mps, 0 }, data));
			enemies.insert(id, h);
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
			auto id = GenerateEntityId();
			Equipment eq{ CreateInventory({ 4,4 }, false, false) };
			equipment.insert(id, eq);
			eq.inventory->Swap(0, item);

			e.meshes.Load(L"!mesh/stash.sxy"_fstring, 0x4000000);
			ID_MESH sysId = e.meshes.GetRendererId(0x4000000);

			Matrix4x4 stashLoc = Matrix4x4::Translate(location + Vec3{ 0,0,0.1f });
			Matrix4x4 scale = Matrix4x4::Scale(0.37f, 0.37f, 0.37f);
			Solid solid{ stashLoc * scale,{ 0,0,0,0 }, sysId, 1.0f, item->Data().bitmapId  };
			solids.insert(id, solid);
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
						DeleteEquipment(change->containerId);
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

		virtual void SetTransform(ID_ENTITY id, const Matrix4x4& transform)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			i->second.instance.orientation = transform;
		}

		virtual void GetPosition(ID_ENTITY id, Vec3& pos) const
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& t = i->second.instance.orientation;
			pos = t.GetPosition();
		}

		virtual void SetPosition(ID_ENTITY id, cr_vec3 pos)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& entity = i->second;
			auto& t = i->second.instance.orientation;
			
			Sphere boundingSphere{ t.GetPosition(), 1.0_metres };

			if (boundingSphere.centre != pos)
			{
				quadTree->DeleteEntity(boundingSphere, id);
				quadTree->AddEntity(boundingSphere, id);
				t.SetPosition(pos);
			}
		}

		void DeleteSolid(ID_ENTITY id)
		{
			auto i = solids.find(id);
			if (i != solids.end())
			{
				auto& entity = i->second;
				auto& t = i->second.instance.orientation;

				Sphere boundingSphere{ t.GetPosition(), 1.0_metres };
				quadTree->DeleteEntity(boundingSphere, id);
				solids.erase(i);
			}
		}

		virtual ID_ENTITY SelectedId() const
		{
			return selectedId;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			selectedId = ID_ENTITY::Invalid();

			bool foundItem = false;
			for (auto& i : solids)
			{
				auto& entity = i.second;
				
				Vec3 pos = entity.instance.orientation.GetPosition();
				Vec3 delta = pos - groundZeroCursor;

				float DS2 = Square(delta.x) + Square(delta.y);
				bool isHighlighted = DS2 < Square(entity.boundingRadius);

				if (entity.bitmapId != 0)
				{
					e.bitmapCache.SetMeshBitmap(rc, entity.bitmapId);
				}

				if (isHighlighted && !foundItem)
				{
					selectedId = i.first;
					foundItem = true;
					ObjectInstance instance{ entity.instance.orientation, { 1.0f, 1.0f, 1.0f, 0.5f } };

					rc.Draw(entity.meshId, &instance, 1);
				}
				else
				{
					rc.Draw(entity.meshId, &entity.instance, 1);
				}
			}

			for (auto& i : projectiles)
			{
				auto& p = i.second;

				ObjectInstance pi{
					Matrix4x4
					{
						Vec4{0.25f,		0,		0,  p.position.x},
						Vec4{0,		0.25f,		0,  p.position.y},
						Vec4{0,			0,	0.25f,  p.position.z},
						Vec4{0,			0,		0,			   1}
					},
					RGBA(0,0,0,0)
				};

				rc.Draw(p.bulletMesh, &pi, 1);
			}
		}

		void UpdateProjectile(Projectile& p, float dt)
		{
			Vec3 g = { 0, 0, -9.81f };
			Vec3 newPos = dt * p.velocity + p.position + g * dt * dt * 0.5f;
			Vec3 newVelocity = p.velocity + g * dt;
			p.velocity = newVelocity;
			Vec3 oldPos = p.position;
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
				ID_ENTITY idEnemy = GetFirstIntersect(oldPos, newPos, enemies, solids, *quadTree);
				if (idEnemy)
				{
					enemies[idEnemy]->ai->OnHit(idPlayer);
				}
			}
			else
			{
				if (Intersects(oldPos, newPos, idPlayer, solids))
				{
					allies[idPlayer]->ai->OnHit(p.attacker);
				}
			}
		}

		void UpdatePosition(ID_ENTITY id, float dt, EntityTable<Human*>& humans)
		{
			Vec3 pos;
			GetPosition(id, pos);

			auto i = humans.find(id);
			if (i == humans.end())
			{
				return;
			}

			auto j = solids.find(id);
			if (j != solids.end())
			{
				Vec3 direction;
				if (!TryNormalize(i->second->ai->Velocity(), direction)) return;

				float speed = Length(i->second->ai->Velocity());

				Vec4 velDir = Vec4::FromVec3(direction, 0.0f);

				Vec4 velDirRotated = j->second.instance.orientation * velDir;
				if (!TryNormalize(velDirRotated, direction)) return;

				Vec3 newPos = pos + direction * dt * speed;

				CollideWithGeometry(pos, newPos, id, *quadTree, solids, e.meshes);

				SetPosition(id, newPos);
			}
			
		}

		void UpdateEnemy(Human& enemy, ID_ENTITY id, float gameTime, float dt)
		{
			const float oorMax = 1.0f / (float)RAND_MAX;

			UpdatePosition(id, dt, enemies);

			if (enemy.nextAIcheck < gameTime)
			{
				enemy.ai->Update(gameTime, dt);
				enemy.nextAIcheck = gameTime + GenRandomFloat(0.2f, 1.0f);
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

		virtual void UpdateObjects(float gameTime, float dt)
		{
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
					UpdateProjectile(p, dt);
					i++;
				}
			}

			auto j = enemies.begin();
			while (j != enemies.end())
			{
				auto& enemy = j->second;
				if (enemy->ai->IsAlive())
				{
					UpdateEnemy(*enemy, j->first, gameTime, dt);
					j++;
				}
				else
				{
					DeleteSolid(j->first);
					delete j->second;
					j = enemies.erase(j);		
				}
			}

			allies[idPlayer]->ai->Update(gameTime, dt);

			Vec3 playerPos;
			GetPosition(idPlayer, playerPos);

			UpdatePosition(idPlayer, dt, allies);
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

	public:
		LevelLoader(Environment& _e):
			e(_e)
		{

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
		}

		virtual void Load(const wchar_t* resourceName, bool isReloading)
		{
			ExecuteSexyScriptLoop(16384, e, resourceName, 0, (int32) 16_megabytes, *this);
		}

		virtual void SyncWithModifiedFiles()
		{
			struct : IEventCallback<FileModifiedArgs>
			{
				IMeshLoader* meshLoader;
				virtual void OnEvent(FileModifiedArgs& args)
				{
					meshLoader->UpdateMesh(args.resourceName);
				}
			} monitor;
			monitor.meshLoader = &e.meshes;

			GetOS(e).EnumerateModifiedFiles(monitor);
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
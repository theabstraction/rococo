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

using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct Solid
	{
		ObjectInstance instance;
		ID_MESH meshId;
		float boundingRadius;
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

	template<class ROW> ID_ENTITY GetFirstIntersect(cr_vec3 start, cr_vec3 end, const EntityTable<ROW>& table, const EntityTable<Solid>& solids)
	{
		float leastT = 2.0f;
		ID_ENTITY firstTarget;

		for (auto& i : table)
		{
			const Solid& solid = solids.find(i.first)->second;
			auto sphere = BoundingSphere(solid);

			float t0, t1;
			if (TryGetIntersectionLineAndSphere(t0, t1, start, end, sphere))
			{
				if (t0 > 0 && t0 < 1 && t0 < leastT)
				{
					leastT = t0;
					firstTarget = i.first;
				}
			}
		}

		return firstTarget;
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

	class Level : public ILevelSupervisor, public ILevelBuilder
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
	public:
		Level(Environment& _e, IHumanFactory& _hf) : e(_e), hf(_hf), idPlayer{ 0 }, groundZeroCursor{ 0,0,0 } {}

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
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, float muzzleVelocity, float flightTime)
		{
			auto id = AddSolid(transform, editorId);
			auto inv = CreateInventory({ 1,1 });
			inv->Swap(0, CreateRangedWeapon({ muzzleVelocity, flightTime }, name.buffer));
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
			solids.erase(id);
		}

		virtual ID_ENTITY AddSolid(const Matrix4x4& transform, ID_MESH meshId)
		{
			auto id = GenerateEntityId();
			ID_MESH sysId = e.meshes.GetRendererId(meshId);
			solids.insert(id, Solid{ {transform, {0,0,0,0}}, sysId, 1.0f });
			return id;
		}

		virtual ID_ENTITY AddAlly(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			auto* inv = CreateInventory({ 5, 8 });
			auto h = new Human { HumanType_Vigilante, 0.0f, inv, hf.CreateHuman(id, *inv, HumanType_Vigilante ) };
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5f, 320.0f, }, L"Bag of stones"));
			allies.insert(id, h);
			return id;
		}

		virtual ID_ENTITY AddEnemy(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			auto* inv = CreateInventory({ 3,4 });
			auto h = new Human{ HumanType_Bobby, 0.0f, inv, hf.CreateHuman(id, *inv, HumanType_Bobby) };
			h->inventory->Swap(0, CreateRangedWeapon({ 2.5f, 10.0f }, L"Bag of stones"));
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

			t.SetPosition(pos);
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
				ID_ENTITY idEnemy = GetFirstIntersect(oldPos, newPos, enemies, solids);
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

			Vec3 newPos = pos + i->second->ai->Velocity() * dt;
			SetPosition(id, newPos);
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
					return nullptr;
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
					solids.erase(j->first);
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
			enum { MAX_LEVEL_FILE_SIZE = 16 * 1024 * 1024 };
			ExecuteSexyScriptLoop(16384, e, resourceName, 0, MAX_LEVEL_FILE_SIZE, *this);
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
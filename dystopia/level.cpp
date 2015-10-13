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

#include <rococo.maths.inl>

#include "component.system.inl"

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

	struct RangedWeapon
	{
		float flightTime;
		float muzzleVelocity;
		std::wstring name;
	};

	struct Projectile
	{
		ID_ENTITY attacker;
		Vec3 position;
		Vec3 direction;
		Vec3 velocity;
		float lifeTime;
		float creationTime;
	};

	struct Human
	{
		bool isAlive;
		Vec3 velocity;
		float nextAIcheck;
		RangedWeapon weapon;
	};

	Sphere BoundingSphere(const Solid& solid)
	{
		return Sphere{ GetPosition(solid.instance.orientation) , solid.boundingRadius };
	}

	template<class ROW> ID_ENTITY GetFirstIntersect(const Vec3& start, const Vec3& end, const EntityTable<ROW>& table, const EntityTable<Solid>& solids)
	{
		float leastT = 2.0f;
		ID_ENTITY firstTarget = 0;

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

	bool Intersects(const Vec3& start, const Vec3& end, ID_ENTITY id, const EntityTable<Solid>& solids)
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

	class Level : public ILevelSupervisor, public ILevelBuilder
	{
		Environment& e;
		Vec2 playerPosition;
		ID_ENTITY idPlayer;
		EntityTable<Solid> solids;
		EntityTable<Projectile> projectiles;
		EntityTable<Human> enemies;
		EntityTable<Human> allies;
		EntityTable<RangedWeapon> weapons;

	public:
		Level(Environment& _e) : e(_e), playerPosition{ 0, 0 }, idPlayer(0) {}

		virtual ILevelBuilder& Builder()
		{
			return *this;
		}

		virtual void Clear()
		{
			solids.clear();
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddRangedWeapon(const Matrix4x4& transform, ID_MESH editorId, const fstring& name, float muzzleVelocity, float flightTime)
		{
			auto id = AddSolid(transform, editorId);
			weapons.insert(id, RangedWeapon{ flightTime, muzzleVelocity, name.buffer });
			return id;
		}

		virtual ID_ENTITY AddSolid(const Matrix4x4& transform, ID_MESH meshId)
		{
			auto id = GenerateEntityId();
			ID_MESH sysId = e.meshes.GetRendererId(meshId);

			solids.insert(id, Solid{ transform, sysId, 1.0f });
			return id;
		}

		virtual ID_ENTITY AddAlly(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			allies.insert(id, Human{ true, Vec3{ 1,0,0 }, 0.0f, RangedWeapon{ 4.0f, 10.0f } });
			return id;
		}

		virtual ID_ENTITY AddEnemy(const Matrix4x4& transform, ID_MESH meshId)
		{
			ID_ENTITY id = AddSolid(transform, meshId);
			enemies.insert(id, Human{ true, Vec3{1,0,0}, 0.0f, RangedWeapon { 4.0f, 10.0f } });
			return id;
		}

		virtual ID_ENTITY AddProjectile(const ProjectileDef& def, float currentTime)
		{
			auto id = GenerateEntityId();

			Vec3 direction = Normalize(def.velocity);

			projectiles.insert(id, Projectile{ def.attacker, def.origin, direction, def.velocity, def.lifeTime, currentTime });

			return id;
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

		virtual void GetPosition(Vec3& pos, ID_ENTITY id) const
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& t = i->second.instance.orientation;
			pos = ::GetPosition(t);
		}

		virtual void SetPosition(const Vec3& pos, ID_ENTITY id)
		{
			auto i = solids.find(id);
			if (i == solids.end())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& entity = i->second;
			auto& t = i->second.instance.orientation;

			::SetPosition(t, pos);
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			for (auto& i : solids)
			{
				auto& entity = i.second;
				rc.Draw(entity.meshId, &entity.instance, 1);
			}

			for (auto& i : projectiles)
			{
				auto& p = i.second;
				auto meshId = solids[0].meshId;

				ObjectInstance pi;
				pi.orientation = Matrix4x4
				{
					Vec4{0.25f, 0,     0,     p.position.x},
					Vec4{0,     0.25f, 0,     p.position.y},
					Vec4{0,     0,     0.25f, p.position.z},
					Vec4{0,     0,     0,     1}
				};

				rc.Draw(meshId, &pi, 1);
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
					enemies[idEnemy].isAlive = false;
				}
			}
			else
			{
				if (Intersects(oldPos, newPos, idPlayer, solids))
				{
					Throw(0, L"Player died");
				}
			}
		}

		void UpdateEnemyAI(Human& enemy, ID_ENTITY id, float gameTime, float dt)
		{
			enemy.nextAIcheck = gameTime + 1.0f;

			int r = rand() % 6;
			switch (r)
			{
			case 0:
				enemy.velocity = Vec3{ 1, 0, 0 };
				break;
			case 1:
				enemy.velocity = Vec3{ -1, 0, 0 };
				break;
			case 2:
				enemy.velocity = Vec3{ 0, 1, 0 };
				break;
			case 3:
				enemy.velocity = Vec3{ 0, -1, 0 };
				break;
			case 4:
				{
					ProjectileDef def;
					def.attacker = id;
					def.lifeTime = enemy.weapon.flightTime;
					def.origin = ::GetPosition(solids[id].instance.orientation);
					Vec3 enemyToPlayer = ::GetPosition(solids[idPlayer].instance.orientation) - def.origin;
					enemyToPlayer.z = Length(enemyToPlayer);

					Vec3 dir;
					if (TryNormalize(enemyToPlayer, dir))
					{
						def.velocity = enemy.weapon.muzzleVelocity * dir;
						AddProjectile(def, gameTime);
					}	
				}
				break;
			default:
				enemy.velocity = Vec3{ 0, 0, 0 };
				break;
			}
		}

		void UpdateEnemy(Human& enemy, ID_ENTITY id, float gameTime, float dt)
		{
			auto& entity = solids[id];
			Vec3 pos = ::GetPosition(entity.instance.orientation);
			pos = pos + enemy.velocity * dt;
			::SetPosition(entity.instance.orientation, pos);

			if (enemy.nextAIcheck < gameTime)
			{
				UpdateEnemyAI(enemy, id, gameTime, dt);
			}
		}

		virtual bool TryGetWeapon(ID_ENTITY id, float& muzzleVelocity, float& flightTime)
		{
			auto i = enemies.find(id);
			if (i == enemies.end())
			{
				i = allies.find(id);
				if (i == allies.end())
				{
					return false;
				}
			}

			muzzleVelocity = i->second.weapon.muzzleVelocity;
			flightTime = i->second.weapon.flightTime;
			return true;
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
				if (enemy.isAlive)
				{
					UpdateEnemy(enemy, j->first, gameTime, dt);
					j++;
				}
				else
				{
					solids.erase(j->first);
					j = enemies.erase(j);		
				}
			}

			Vec3 playerPos;
			GetPosition(playerPos, idPlayer);

			auto w = weapons.begin(); 
			while (w != weapons.end())
			{
				auto& k = solids.find(w->first);
				Vec3 boxPos = ::GetPosition(k->second.instance.orientation);
				float lenSq = LengthSq(boxPos - playerPos);
				if (lenSq < Square(k->second.boundingRadius))
				{
					allies[idPlayer].weapon = w->second;
					solids.erase(w->first);
					weapons.erase(w);
					break;
				}
				else
				{
					++w;
				}
			}
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
		ILevel& level;

	public:
		LevelLoader(Environment& _e, ILevel& _level):
			e(_e),
			level(_level)
		{

		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnEvent(ScriptCompileArgs& args)
		{
			AddNativeCalls_DystopiaIMeshes(args.ss, &e.meshes);
			AddNativeCalls_DystopiaILevelBuilder(args.ss, &level);
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
	ILevelLoader* CreateLevelLoader(Environment& e, ILevel& level)
	{
		return new LevelLoader(e, level);
	}

	ILevelSupervisor* CreateLevel(Environment& e)
	{
		return new Level(e);
	}
}
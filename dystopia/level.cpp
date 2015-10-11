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

using namespace Dystopia;
using namespace Rococo;

namespace
{
	struct Entity
	{
		ObjectInstance instance;
		ID_MESH meshId;
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

	void Advance(Projectile& p, float dt)
	{
		Vec3 g = { 0, 0, -9.81f };
		Vec3 newPos = dt * p.velocity + p.position + g * dt * dt * 0.5f;
		Vec3 newVelocity = p.velocity + g * dt;
		p.velocity = newVelocity;
		p.position = newPos;

		if (!TryNormalize(p.velocity, p.direction))
		{
			p.direction = Vec3(0, 0, 1);
		}
	}

	class Level : public ILevelSupervisor, public ILevelBuilder
	{
		Environment& e;
		Vec2 playerPosition;
		ID_ENTITY idPlayer;
		std::vector<Entity> entities;
		std::unordered_map<ID_PROJECTILE, Projectile> projectiles;
		ID_PROJECTILE nextProjectileId;
	public:
		Level(Environment& _e) : e(_e), playerPosition(0, 0), idPlayer(0), nextProjectileId(0){}

		virtual ILevelBuilder& Builder()
		{
			return *this;
		}

		virtual void Clear()
		{
			entities.clear();
		}

		virtual void Free() { delete this; }

		virtual ID_ENTITY AddEntity(Matrix4x4& transform, ID_MESH meshId)
		{
			ID_MESH sysId = e.meshes.GetRendererId(meshId);
			entities.push_back(Entity{ transform, sysId });
			return entities.size();
		}

		virtual ID_PROJECTILE AddProjectile(const ProjectileDef& def, float currentTime)
		{
			auto id = nextProjectileId++;

			Vec3 direction = Normalize(def.velocity);

			projectiles.insert(std::make_pair(id, Projectile{ def.attacker, def.origin, direction, def.velocity, def.lifeTime, currentTime }));

			return id;
		}

		virtual void SetTransform(ID_ENTITY id, const Matrix4x4& transform)
		{
			size_t index = id - 1;
			if (index >= entities.size())
			{
				Throw(0, L"Invalid entity Id");
			}

			entities[index].instance.orientation = transform;
		}

		virtual void GetPosition(Vec3& pos, ID_ENTITY id) const
		{
			size_t index = id - 1;
			if (index >= entities.size())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& t = entities[index].instance.orientation;
			pos = Vec3{ t.row0.w, t.row1.w, t.row2.w };
		}

		virtual void SetPosition(const Vec3& pos, ID_ENTITY id)
		{
			size_t index = id - 1;
			if (index >= entities.size())
			{
				Throw(0, L"Invalid entity Id");
			}

			auto& entity = entities[index];
			entity.instance.orientation.row0.w = pos.x;
			entity.instance.orientation.row1.w = pos.y;
			entity.instance.orientation.row2.w = pos.z;
		}

		virtual void RenderObjects(IRenderContext& rc)
		{
			for (auto& entity : entities)
			{
				rc.Draw(entity.meshId, &entity.instance, 1);
			}

			for (auto& i : projectiles)
			{
				auto& p = i.second;
				auto meshId = entities[p.attacker-1].meshId;

				ObjectInstance pi;
				pi.orientation = Matrix4x4
				{
					Vec4(1, 0, 0, p.position.x),
					Vec4(0, 1, 0, p.position.y),
					Vec4(0, 0, 1, p.position.z),
					Vec4(0, 0, 0, 1)
				};

				rc.Draw(meshId, &pi, 1);
			}
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
					Advance(p, dt);
					i++;
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

		virtual void Free() { delete this; }

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
			struct : ITextCallback
			{
				IMeshLoader* meshLoader;
				virtual void OnItem(const wchar_t* sysFilename)
				{
					meshLoader->UpdateMesh(sysFilename);
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
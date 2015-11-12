#include "dystopia.h"
#include "human.types.h"

#include "rococo.maths.h"

#include <stdlib.h>

#include <math.h>

#include <vector>

#include <random>

using namespace Dystopia;
using namespace Rococo;

namespace
{	
	typedef std::mt19937 Randomizer;
	Randomizer rng;

	uint32 NextRandom()
	{
		return rng();
	}

	ROCOCOAPI IAgentManipulator
	{
		virtual void SetVelocity(cr_vec3 velocity) = 0;
		virtual void Suicide() = 0;
	};

	ROCOCOAPI IBehaviour
	{
		virtual void Invoke(Environment& e, ID_ENTITY actorId, float gameTime, IAgentManipulator& manipulator) = 0;
	};

	class Behaviour_RangedAttackAgainstPlayer : public IBehaviour
	{
	public:
		void Invoke(Environment& e, ID_ENTITY actorId, float gameTime, IAgentManipulator& manipulator)
		{
			ProjectileDef def;
			def.attacker = actorId;

			auto* pInv = e.level.GetInventory(actorId);
			if (!pInv) return;

			IInventory& inventory = *pInv;

			float muzzleSpeed;
			auto* item = inventory.GetItem(0);
			auto* ranged = item ? item->GetRangedWeaponData() : nullptr;
			if (ranged)
			{
				def.lifeTime = ranged->flightTime;
				muzzleSpeed = ranged->muzzleVelocity;
				e.level.GetPosition(actorId, def.origin);

				const Metres shoulderHeight{ 1.5f };
				def.origin.z += shoulderHeight;

				auto playerId = e.level.GetPlayerId();

				Vec3 playerPos;
				e.level.GetPosition(playerId, playerPos);

				Radians elevation = ComputeWeaponElevation(def.origin, playerPos, muzzleSpeed, Degrees{ 45.0f }, Gravity{ -9.81f }, Metres{ 2.0f });

				Vec3 enemyToPlayer = playerPos - def.origin;

				if (elevation > 0.0f)
				{
					def.velocity.z = muzzleSpeed * sinf(elevation);
					float lateralSpeed = muzzleSpeed * cosf(elevation);

					if (enemyToPlayer.y == 0)
					{
						def.velocity.y = 0;
						def.velocity.x = enemyToPlayer.x > 0 ? lateralSpeed : -lateralSpeed;
					}
					else
					{
						Radians theta{ atan2f(enemyToPlayer.y, enemyToPlayer.x) };
						def.velocity.y = sinf(theta) * lateralSpeed;
						def.velocity.x = cosf(theta) * lateralSpeed;
					}

					def.bulletMesh = ID_SYS_MESH(0);
					e.level.AddProjectile(def, gameTime);
				}
			}
		}
	};

	class Behaviour_RandomWalk : public IBehaviour
	{
	public:
		void Invoke(Environment& e, ID_ENTITY actorId, float gameTime, IAgentManipulator& agent)
		{
			int r = rand() % 5;
			switch (r)
			{
			case 0:
				agent.SetVelocity( Vec3{ 1, 0, 0 } );
				break;
			case 1:
				agent.SetVelocity(Vec3{ -1, 0, 0 });
				break;
			case 2:
				agent.SetVelocity(Vec3{ 0, 1, 0 });
				break;
			case 3:
				agent.SetVelocity(Vec3{ 0, -1, 0 });
				break;
			case 4:
				agent.SetVelocity(Vec3{ 0, 0, 0 });
				break;
			}
		}
	};

	static Behaviour_RangedAttackAgainstPlayer g_rangedAttackAgainstPlayer;
	static Behaviour_RandomWalk g_randomWalk;

	class HumanBobby : public IHumanAISupervisor, public IAgentManipulator
	{
		ID_ENTITY id;
		Vec3 velocity;
		Environment& e;
		bool isAlive;
		uint32 totalWeight;

		struct BehaviourBinding
		{
			IBehaviour& behaviour;
			uint32 weight;
		};

		std::vector<BehaviourBinding> behaviours;
	public:
		HumanBobby(ID_ENTITY _id,  Environment& _e) : e(_e),
			id(_id), velocity{ 0,0,0 }, isAlive(true)
		{
			AddBehaviour(g_rangedAttackAgainstPlayer, 10);
			AddBehaviour(g_randomWalk, 50);
		}

		virtual void AddBehaviour(IBehaviour& behaviour, uint32 weight)
		{
			totalWeight = 0;

			behaviours.push_back({ behaviour , weight });

			for (auto i : behaviours)
			{
				totalWeight += i.weight;
			}
		}

		virtual void Describe(IStringBuilder& sb, Language language)
		{
			sb.AppendFormat(L"Bobby.\n\n");
			sb.AppendFormat(L"Health: good");
		}

		virtual void Free() { delete this; }

		virtual cr_vec3 Velocity() const { return velocity; }

		virtual void SetVelocity(cr_vec3 velocity)
		{
			this->velocity = velocity;

			Vec3 direction;
			if (TryNormalize(velocity, direction))
			{
				float theta = acosf(Dot(direction, { 0,1,0 }));

				Matrix4x4 rotZ = Matrix4x4::RotateRHAnticlockwiseZ(Radians{ theta });

				Vec3 pos;
				e.level.GetPosition(id, pos);

				Matrix4x4 tra = Matrix4x4::Translate(pos);

				e.level.SetTransform(id, tra * rotZ);
			}
		}

		virtual void Suicide()
		{
			isAlive = false;
		}

		virtual bool IsAlive() const
		{
			return isAlive;
		}

		virtual Stat GetStat(StatIndex index) const
		{
			return Stat{ 1, 1 };
		}

		virtual void OnHit(ID_ENTITY attackerId)
		{
			isAlive = false;
		}

		void Update(float gameTime,float dt)
		{
			uint32 r = NextRandom() % totalWeight;

			uint32 sum = 0;

			for (auto i : behaviours)
			{
				sum += i.weight;
				if (sum > r)
				{
					i.behaviour.Invoke(e, id, gameTime, *this);
				}
			}
		}
	};
}

namespace Dystopia
{
	IHumanAISupervisor* CreateBobby(ID_ENTITY id, Environment& e)
	{
		return new HumanBobby(id, e);
	}
}
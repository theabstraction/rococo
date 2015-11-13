#include "dystopia.h"
#include "rococo.maths.h"
#include "human.types.h"

#include <stdlib.h>
#include <math.h>
#include <vector>
#include <random>

#include "dystopia.post.h"

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

	class Behaviour_RangedAttackAgainstPlayer : public IBehaviour
	{
	public:
		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& manipulator)
		{
			ProjectileDef def;
			def.attacker = actorId;

			auto* pInv = e.level.GetInventory(actorId);
			if (!pInv) return AIRoutine_Complete;

			IInventory& inventory = *pInv;

			float muzzleSpeed;
			auto* item = inventory.GetItem(0);
			auto* ranged = item ? item->GetRangedWeaponData() : nullptr;
			if (ranged)
			{
				def.lifeTime = ranged->flightTime;
				muzzleSpeed = ranged->muzzleVelocity;
				def.origin = e.level.GetPosition(actorId);

				const Metres shoulderHeight{ 1.5f };
				def.origin.z += shoulderHeight;

				auto playerId = e.level.GetPlayerId();

				cr_vec3 playerPos = e.level.GetPosition(playerId);

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

			e.level.SetNextAIUpdate(actorId, gameTime + 0.1f);
			return AIRoutine_Complete;
		}

		virtual void BeginRoutine() {}
		virtual void Free() {}
	} static g_rangedAttackAgainstPlayer;

	void RunForward(ILevel& level, ID_ENTITY actorId, MetresPerSecond speed)
	{
		level.SetVelocity(actorId, speed * level.GetForwardDirection(actorId));
	}

	void RunInRandomDirection(ILevel& level, ID_ENTITY actorId, Seconds gameTime, Seconds runPeriod, MetresPerSecond runSpeed)
	{
		Degrees theta = Degrees{ (float)(NextRandom() % 360) };
		level.SetHeading(actorId, theta);
		RunForward(level, actorId, runSpeed);
		level.SetNextAIUpdate(actorId, gameTime + runPeriod);
	}

	class Behaviour_RandomWalk : public IBehaviour
	{
	public:
		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& agent)
		{
			RunInRandomDirection(e.level, actorId, gameTime, 2.0_seconds, 4.0_mps);
			return AIRoutine_Complete;
		}

		virtual void BeginRoutine() {}
		virtual void Free() {}
	} static g_randomWalk;

	// Rotate the actor towards the target point using the given angular velocity and timestep
	// Returns -2.0f if case is degenerate, otherwise dot product of previous heading and direction unit vectors 
	// The final parameter gives the alpha value, above which the target snaps exactly onto the heading direction
	float TurnTowardsTarget(ILevel& level, ID_ENTITY actorId, cr_vec3 targetPoint, Radians turnAnglePerSec, Seconds dt, float snapAlpha = 0.950f)
	{
		cr_vec3 actorPosition = level.GetPosition(actorId);
		Vec3 delta = targetPoint - actorPosition;
		delta.z = 0;

		Vec3 targetDirection;
		if (!TryNormalize(delta, targetDirection)) return -2.0f;

		Vec3 forward = level.GetForwardDirection(actorId);

		auto headingTheta = GetHeadingOfVector(forward.x, forward.y);
		auto directionTheta = GetHeadingOfVector(targetDirection.x, targetDirection.y);

		float alpha = Dot(forward, targetDirection);

		if (alpha > snapAlpha)
		{
			level.SetHeading(actorId, directionTheta);
		}
		else
		{
			float dTheta = turnAnglePerSec * dt;

			Vec3 v = Cross(forward, targetDirection);

			// if direction is to left of heading then z component +ve, consider heading  = i, direction = j, then v = k
			if (v.z < 0)
			{
				dTheta *= -1;
			}

			float nextTheta = headingTheta + dTheta;

			level.SetHeading(actorId, Radians{ nextTheta });
		}

		return alpha;
	}

	void Stop(ILevel& level, ID_ENTITY actorId)
	{
		level.SetVelocity(actorId, Vec3{ 0,0,0 });
	}

	class Behaviour_FollowNavPoints : public IBehaviour, Post::IRecipient
	{
		struct NavPoint
		{
			Vec3 location;
			Metres boundingRadius;
		};
		std::vector<NavPoint> navpoints;
		uint32 targetIndex;
		Environment& e;
		AICollision lastCollision;
		ID_ENTITY actorId;
	public:
		Behaviour_FollowNavPoints(Environment& _e, ID_ENTITY _actorId) : e(_e), actorId(_actorId), targetIndex{ 0xFFFFFFFF }
		{
			e.postbox.Subscribe<AISetTarget>(this);
			e.postbox.Subscribe<AICollision>(this);
		}

		~Behaviour_FollowNavPoints()
		{
			e.postbox.Unsubscribe<AISetTarget>(this);
			e.postbox.Unsubscribe<AICollision>(this);
		}

		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& agent)
		{
			if (lastCollision.entityId == actorId)
			{
				lastCollision.entityId = ID_ENTITY::Invalid();	
				RunInRandomDirection(e.level, actorId, gameTime, 0.5_seconds, 4.0_mps);
				return AIRoutine_Yielded;
			}

			if (targetIndex >= navpoints.size())
			{
				// navpoint was not available, so our script is interrupted, for the moment abort so we can debug
				Throw(0, L"Navpoint target index out of range for body %I64u", (uint64) actorId);
				return AIRoutine_Interrupted;
			}

			const auto& navpoint = navpoints[targetIndex];
			Vec3 targetPoint = navpoint.location;
			
			auto pos = e.level.GetPosition(actorId);
			Vec3 delta = targetPoint - pos;
			delta.z = 0;

			if (IsInRange(delta, navpoint.boundingRadius))
			{
				targetIndex++;

				if (targetIndex >= navpoints.size())
				{
					Stop(e.level, actorId);
					return AIRoutine_Complete;
				}
				else
				{
					return AIRoutine_Yielded;
				}
			}
			else
			{
				 // delta finite, thus normalizable
			}

			Vec3 direction = Normalize(delta);

			Vec3 vel = e.level.GetVelocity(actorId);

			if (Dot(vel, direction) <= 0)
			{
				Stop(e.level, actorId);
			}

			float alpha = TurnTowardsTarget(e.level, actorId, targetPoint, 90.0_degrees, dt);
			if (alpha == 2.0f)
			{
				Stop(e.level, actorId);
				return AIRoutine_Interrupted;
			}

			if (alpha > 0.5f)
			{
				const MetresPerSecond speed { alpha * alpha * 2.0f };
				RunForward(e.level, actorId, speed);
				e.level.SetVelocity(actorId, speed * e.level.GetForwardDirection(actorId));
			}

			return AIRoutine_Yielded;
		}

		virtual void OnPost(const Mail& mail)
		{
			auto setTarget = Post::InterpretAs<AISetTarget>(mail);
			if (setTarget)
			{
				if (!setTarget->appendToNavPoints)
				{
					navpoints.clear();
					targetIndex = -1;
				}

				navpoints.push_back({ setTarget->targetPosition, max(setTarget->boundingRadius, 2.5_metres) });
			}

			auto collision = Post::InterpretAs<AICollision>(mail);
			if (collision && actorId == collision->entityId)
			{
				lastCollision = *collision;
			}
		}

		virtual void BeginRoutine()
		{
			lastCollision.entityId = ID_ENTITY::Invalid();
			targetIndex = 0;
		}

		virtual void Free()
		{
			delete this;
		}
	};

	class HumanBobby : public IHumanAISupervisor, public IAgentManipulator
	{
		ID_ENTITY id;
		Environment& e;
		bool isAlive;
		uint32 totalChoiceWeight;
		uint32 behaviourIndex;

		struct BehaviourBinding
		{
			IBehaviour* behaviour;
			uint32 weight;
		};

		std::vector<BehaviourBinding> behaviours;
	public:
		HumanBobby(ID_ENTITY _id,  Environment& _e) : e(_e), behaviourIndex(-1),
			id(_id), isAlive(true), totalChoiceWeight(0)
		{
			AddBehaviour(new Behaviour_FollowNavPoints(_e, _id), 5);
			AddBehaviour(&g_rangedAttackAgainstPlayer, 10);
			AddBehaviour(&g_randomWalk, 50);
		}

		~HumanBobby()
		{
			for (auto i : behaviours)
			{
				i.behaviour->Free();
			}
		}

		virtual void AddBehaviour(IBehaviour* behaviour, uint32 weight)
		{
			totalChoiceWeight += weight;
			behaviours.push_back({ behaviour , weight });
		}

		virtual void Describe(IStringBuilder& sb, Language language)
		{
			sb.AppendFormat(L"Bobby.\n\n");
			sb.AppendFormat(L"Health: good");
		}

		virtual void Free() { delete this; }

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

		void ChooseRandomBehaviourByWeight(Seconds gameTime, Seconds dt)
		{
			uint32 r = NextRandom() % totalChoiceWeight;

			uint32 sum = 0;

			behaviourIndex = 0;
			for (auto i : behaviours)
			{
				sum += i.weight;
				if (sum > r)
				{
					i.behaviour->BeginRoutine();
					if (i.behaviour->Invoke(e, id, gameTime, dt, *this) != AIRoutine_Yielded)
					{
						behaviourIndex = -1;
					}

					return;
				}

				behaviourIndex++;
			}
		}

		void Update(float gameTime,float dt)
		{
			if (behaviourIndex >= behaviours.size())
			{
				ChooseRandomBehaviourByWeight(Seconds{ gameTime }, Seconds{ dt });
			}
			else
			{
				if (behaviours[behaviourIndex].behaviour->Invoke(e, id, Seconds{ gameTime }, Seconds{ dt }, *this) != AIRoutine_Yielded)
				{
					behaviourIndex = -1;
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
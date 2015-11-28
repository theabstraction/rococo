#include "dystopia.h"
#include "rococo.maths.h"
#include "human.types.h"

#include <stdlib.h>
#include <math.h>
#include <vector>

#include "dystopia.post.h"
#include "rococo.strings.h"

using namespace Dystopia;
using namespace Rococo;

namespace
{	
	Random::RandomMT rng;

	class Behaviour_RangedAttackAgainstPlayer : public IBehaviour
	{
	public:
		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& manipulator)
		{
			ProjectileDef def;
			def.attacker = actorId;

			bool isUsedTwoHanded;
			RangedWeapon* ranged = AI::Low::GetRangedWeapon(e.level, actorId, /* out */ isUsedTwoHanded);

			if (!ranged) return AIRoutine_Complete;

			float muzzleSpeed;
			
			def.lifeTime = ranged->flightTime;
			muzzleSpeed = ranged->muzzleVelocity;
			def.origin = e.level.GetPosition(actorId);

			if (muzzleSpeed < 60.0f && isUsedTwoHanded)
			{
				// Probably a thrown weapon, so give it a boost
				muzzleSpeed *= 1.25f;
			}

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

			e.level.SetNextAIUpdate(actorId, gameTime + 0.1f);
			return AIRoutine_Complete;
		}

		virtual void BeginRoutine() {}
		virtual void EndRoutine() {}
		virtual void Free() {}
		virtual Post::IRecipient* GetRecipient() { return nullptr; }
	} static g_rangedAttackAgainstPlayer;

	class Behaviour_RandomWalk : public IBehaviour
	{
	public:
		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& agent)
		{
			AI::Low::RunInRandomDirection(e.level, actorId, gameTime, 2.0_seconds, 4.0_mps);
			return AIRoutine_Complete;
		}

		virtual void BeginRoutine() {}
		virtual void EndRoutine() {}
		virtual void Free() {}
		virtual Post::IRecipient* GetRecipient() { return nullptr; }
	} static g_randomWalk;

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
		}

		AIRoutine Invoke(Environment& e, ID_ENTITY actorId, Seconds gameTime, Seconds dt, IAgentManipulator& agent)
		{
			if (navpoints.empty()) return AIRoutine_Complete;

			if (lastCollision.entityId == actorId)
			{
				lastCollision.entityId = ID_ENTITY::Invalid();	
				AI::Low::RunInRandomDirection(e.level, actorId, gameTime, 0.5_seconds, 4.0_mps);
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
					AI::Low::Stop(e.level, actorId);
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
				AI::Low::Stop(e.level, actorId);
			}

			float alpha = AI::Low::TurnTowardsTarget(e.level, actorId, targetPoint, 90.0_degrees, dt);
			if (alpha == 2.0f)
			{
				AI::Low::Stop(e.level, actorId);
				return AIRoutine_Interrupted;
			}

			if (alpha > 0.5f)
			{
				const MetresPerSecond speed { alpha * alpha * 2.0f };
				AI::Low::RunForward(e.level, actorId, speed);
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

		virtual void EndRoutine()
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual Post::IRecipient* GetRecipient() { return this; }
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
			if (isAlive)
			{
				e.postbox.PostForLater(AIKilled{ attackerId, id, 1 }, false);
				isAlive = false;
			}
		}

		void ChooseRandomBehaviourByWeight(Seconds gameTime, Seconds dt)
		{
			uint32 r = Random::Next(rng, totalChoiceWeight);

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
						i.behaviour->EndRoutine();
						behaviourIndex = -1;
					}

					return;
				}

				behaviourIndex++;
			}
		}

		void Update(Seconds gameTime, Seconds dt)
		{
			if (behaviourIndex >= behaviours.size())
			{
				ChooseRandomBehaviourByWeight( gameTime, dt );
			}
			else
			{
				if (behaviours[behaviourIndex].behaviour->Invoke(e, id, Seconds{ gameTime }, Seconds{ dt }, *this) != AIRoutine_Yielded)
				{
					behaviours[behaviourIndex].behaviour->EndRoutine();
					behaviourIndex = -1;
				}
			}
		}

		virtual void SendRaw(const Mail& mail)
		{
			if (behaviourIndex < behaviours.size())
			{
				Post::IRecipient* recipient = behaviours[behaviourIndex].behaviour->GetRecipient();
				if (recipient)
				{
					recipient->OnPost(mail);
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
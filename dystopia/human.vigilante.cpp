#include "dystopia.h"
#include "rococo.strings.h"
#include "human.types.h"
#include "rococo.maths.h"
#include "dystopia.post.h"
#include <wchar.h>
#include "rpg.rules.h"

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class HumanVigilante : public IHumanAISupervisor, public Post::IRecipient
	{
		ID_ENTITY id;
		IIntent& intent;
		Environment& e;
		float lastFireTime;
		Stat stats[StatIndex_Count];

		virtual void AddBehaviour(IBehaviour* behaviour, uint32 weight)
		{

		}

		virtual bool IsAlive() const
		{
			return stats[StatIndex_Health].current > 0;
		}

		virtual void OnHit(ID_ENTITY attackerId)
		{
			stats[StatIndex_Health].current -= 10;
		}

		virtual Stat GetStat(StatIndex index) const
		{
			if (index < 0 || index >= StatIndex_Count) Throw(0, L"Bad stat index");
			return stats[index];
		}

		void UpdateViaIntent(Seconds gameTime, Seconds dt)
		{
			Vec3 pos = e.level.GetPosition(id);

			float speed = 64.0_mps;
			Vec2 impulse = intent.GetImpulse();

			if (impulse.y < 0) speed *= 0.5f;
			Vec4 localVelocity = Vec4{ 0.25f * speed * impulse.x, speed * impulse.y, 0, 0 };

			Matrix4x4 transform;
			e.level.TryGetTransform(e.level.GetPlayerId(), transform);

			Vec3 worldVel = transform * localVelocity;

			e.level.SetVelocity(id, worldVel);

			if (gameTime > lastFireTime + 0.25f)
			{
				int fireCount = intent.GetFireCount();
				if (fireCount > 2) fireCount = 2;
				if (fireCount > 0)
				{
					fireCount--;
					intent.SetFireCount(fireCount);
					lastFireTime = gameTime;

					bool isUsedTwoHanded;
					IItem* weapon;
					RangedWeapon* ranged = AI::Low::GetRangedWeapon(e.level, id, isUsedTwoHanded, &weapon);

					if (!ranged)
					{
						return;
					}

					float dz = 10.0f / ranged->muzzleVelocity;

					Vec3 aimAt = e.level.GetForwardDirection(id);
					Vec2 aim2D = Normalize(Vec2{ aimAt.x, aimAt.y });

					Vec3 dir{ aim2D.x, aim2D.y, dz };
					ProjectileDef def = { id, pos, dir * ranged->muzzleVelocity, ranged->flightTime, ID_SYS_MESH(0) };

					AI::Low::FireRangedWeapon(e.level, &e.postbox, id, weapon, def, gameTime);
				}
			}
		}
	public:
		HumanVigilante(ID_ENTITY _id, IIntent& _intent, Environment& _e) :
			id(_id), intent(_intent), e(_e), lastFireTime(0)
		{
			stats[StatIndex_Health] = { 200,200 };
			stats[StatIndex_Kudos] = { 0, RPG::GetFirstCap() };

			e.postbox.Subscribe<AIKilled>(this);
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* killed = Post::InterpretAs<AIKilled>(mail);
			if (killed && killed->killerId == id)
			{
				RPG::EarnKudos(stats[StatIndex_Kudos], killed->kudos, id, e);
			}
		}

		virtual void Describe(IStringBuilder& sb, Language language)
		{
			sb.AppendFormat(L"Vigilante.\n\n");
			sb.AppendFormat(L"Health: %d of %d", stats[StatIndex_Health].current, stats[StatIndex_Health].cap);
		}

		virtual void Free()
		{ 
			delete this;
		}

		void Update(Seconds gameTime, Seconds dt)
		{
			if (id == e.level.GetPlayerId())
			{
				UpdateViaIntent(gameTime, dt);
			}
		}

		virtual void SendRaw(const Mail& mail)
		{

		}
	};
}

namespace Dystopia
{
	IHumanAISupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, Environment& e)
	{
		return new HumanVigilante(id, intent, e);
	}
}
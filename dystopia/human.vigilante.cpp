#include "dystopia.h"
#include "human.types.h"
#include "rococo.maths.h"

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class HumanVigilante : public IHumanSupervisor
	{
		ID_ENTITY id;
		Vec3 velocity;
		IIntent& intent;
		ILevel& level;
		float lastFireTime;

		virtual bool IsAlive() const
		{
			return true;
		}

		virtual void OnHit(ID_ENTITY attackerId)
		{

		}

		void UpdateViaIntent(float gameTime, float dt)
		{
			Vec3 pos;
			level.GetPosition(pos, id);

			float speed = 4.0f;
			Vec2 impulse = intent.GetImpulse();
			velocity = speed * Vec3{ impulse.x, impulse.y, 0 };

			if (gameTime > lastFireTime + 0.25f)
			{
				int fireCount = intent.GetFireCount();
				if (fireCount > 2) fireCount = 2;
				if (fireCount > 0)
				{
					fireCount--;
					intent.SetFireCount(fireCount);
					lastFireTime = gameTime;

					float muzzleVelocity, flightTime;
					auto inv = level.GetInventory(id);
					if (inv)
					{
						inv->GetRangedWeapon(muzzleVelocity, flightTime);
						float dz = 10.0f / muzzleVelocity;
						Vec3 dir{ 0, 1, dz };
						ProjectileDef def = { id, pos, dir * muzzleVelocity, flightTime, 0 };
						level.AddProjectile(def, lastFireTime);
					}
				}
			}
		}
	public:
		HumanVigilante(ID_ENTITY _id, IIntent& _intent, ILevel& _level) :
			id(_id), velocity{ 0, 0, 0 }, intent(_intent), level(_level), lastFireTime(0)
		{}

		virtual void Free()
		{ 
			delete this;
		}

		void Update(float gameTime, float dt)
		{
			if (id == level.GetPlayerId())
			{
				UpdateViaIntent(gameTime, dt);
			}
			else
			{
				velocity = Vec3{ 0,0,0 };
			}
		}

		virtual const Vec3& Velocity() const
		{
			return velocity;
		}
	};
}

namespace Dystopia
{
	IHumanSupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, ILevel& level)
	{
		return new HumanVigilante(id, intent, level);
	}
}
#include "dystopia.h"
#include "human.types.h"
#include "rococo.maths.h"
#include "dystopia.post.h"
#include <wchar.h>

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class HumanVigilante : public IHumanAISupervisor
	{
		ID_ENTITY id;
		Vec3 velocity;
		IIntent& intent;
		ILevel& level;
		Post::IPostbox& postbox;
		float lastFireTime;
		Stat stats[StatIndex_Count];

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

		void UpdateViaIntent(float gameTime, float dt)
		{
			Vec3 pos;
			level.GetPosition(id, pos);

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

					auto inv = level.GetInventory(id);
					if (inv)
					{
						auto* item = inv->GetItem(0);
						auto* ranged = item ? item->GetRangedWeaponData() : nullptr;
						if (ranged)
						{
							float dz = 10.0f / ranged->muzzleVelocity;
							Vec3 dir{ 0, 1, dz };
							ProjectileDef def = { id, pos, dir * ranged->muzzleVelocity, ranged->flightTime, ID_SYS_MESH(0) };

							if (ranged->ammunitionIndex)
							{
								auto* magazineCache = item->GetContents();
								if (magazineCache)
								{
									auto* magazine = magazineCache->GetItem(0);
									if (magazine)
									{
										auto* ammo = magazine->GetAmmo();
										if (ammo)
										{
											if (ammo->count > 0)
											{
												ammo->count--;
												level.AddProjectile(def, lastFireTime);

												if (ammo->count == 0)
												{
													auto* detachedAmmo = magazineCache->Swap(0, nullptr);
													detachedAmmo->Free();

													HintMessage3D hint;
													hint.duration = 2.5_seconds;
													level.GetPosition(level.GetPlayerId(), hint.position);
													SafeFormat(hint.message, _TRUNCATE, L"clip empty!");
													postbox.PostForLater(hint, true);
												}
											}
										}
									}
								}
							}
							else
							{
								level.AddProjectile(def, lastFireTime);
							}
						}
					}
				}
			}
		}
	public:
		HumanVigilante(ID_ENTITY _id, IIntent& _intent, ILevel& _level, Post::IPostbox& _postbox) :
			id(_id), velocity{ 0, 0, 0 }, intent(_intent), level(_level), lastFireTime(0), postbox(_postbox)
		{
			for (int i = 0; i < StatIndex_Count; ++i)
			{
				stats[i].cap = 200;
				stats[i].current = 200;
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

		virtual cr_vec3 Velocity() const
		{
			return velocity;
		}
	};
}

namespace Dystopia
{
	IHumanAISupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, Environment& e)
	{
		return new HumanVigilante(id, intent, e.level, e.postbox);
	}
}
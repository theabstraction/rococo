#include "dystopia.h"
#include "human.types.h"

#include "rococo.maths.h"

#include <stdlib.h>

#include <math.h>

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class HumanBobby : public IHumanAISupervisor
	{
		ID_ENTITY id;
		Vec3 velocity;
		IInventory& inventory;
		ILevel& level;
		bool isAlive;
	public:
		HumanBobby(ID_ENTITY _id, IInventory& _inventory, ILevel& _level) :
			id(_id), velocity{ 0,0,0 }, inventory(_inventory), level(_level), isAlive(true)
		{}

		virtual void Describe(IStringBuilder& sb, Language language)
		{
			sb.AppendFormat(L"Bobby.\n\n");
			sb.AppendFormat(L"Health: good");
		}

		virtual void Free() { delete this; }

		virtual cr_vec3 Velocity() const { return velocity; }

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
			int r = rand() % 6;
			switch (r)
			{
			case 0:
				velocity = Vec3{ 1, 0, 0 };
				break;
			case 1:
				velocity = Vec3{ -1, 0, 0 };
				break;
			case 2:
				velocity = Vec3{ 0, 1, 0 };
				break;
			case 3:
				velocity = Vec3{ 0, -1, 0 };
				break;
			case 4:
			{
				ProjectileDef def;
				def.attacker = id;

				float muzzleSpeed;
				inventory.GetRangedWeapon(muzzleSpeed, def.lifeTime);
				level.GetPosition(id, def.origin);

				const Metres shoulderHeight{ 1.5f };
				def.origin.z += shoulderHeight;

				auto playerId = level.GetPlayerId();

				Vec3 playerPos;
				level.GetPosition(playerId, playerPos);

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
					
					def.bulletMesh = 0;
					level.AddProjectile(def, gameTime);
				}	
			}
			break;
			default:
				velocity = Vec3{ 0, 0, 0 };
				break;
			}
		}
	};
}

namespace Dystopia
{
	IHumanAISupervisor* CreateBobby(ID_ENTITY id, IInventory& inventory, ILevel& level)
	{
		return new HumanBobby(id, inventory, level);
	}
}
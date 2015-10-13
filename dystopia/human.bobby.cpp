#include "dystopia.h"
#include "human.types.h"

#include "rococo.maths.h"

#include <stdlib.h>

using namespace Dystopia;
using namespace Rococo;

namespace
{
	class HumanBobby : public IHumanSupervisor
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

		virtual void Free() { delete this; }

		virtual const Vec3& Velocity() const { return velocity; }

		virtual bool IsAlive() const
		{
			return isAlive;
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
				level.GetPosition(def.origin, id);

				auto playerId = level.GetPlayerId();

				Vec3 playerPos;
				level.GetPosition(playerPos, playerId);

				Vec3 enemyToPlayer = playerPos - def.origin;
				enemyToPlayer.z = Length(enemyToPlayer);

				Vec3 dir;
				if (TryNormalize(enemyToPlayer, dir))
				{
					def.velocity = muzzleSpeed * dir;
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
	IHumanSupervisor* CreateBobby(ID_ENTITY id, IInventory& inventory, ILevel& level)
	{
		return new HumanBobby(id, inventory, level);
	}
}
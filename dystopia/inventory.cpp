#include "dystopia.h"
#include <string>

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	struct RangedWeapon
	{
		float flightTime;
		float muzzleVelocity;
		std::wstring name;
	};

	class Inventory : public IInventorySupervisor
	{
		RangedWeapon activeWeapon;
	public:
		virtual void Free() { delete this; }

		virtual void SetRangedWeapon(float muzzleVelocity, float flightTime)
		{
			activeWeapon.muzzleVelocity = muzzleVelocity;
			activeWeapon.flightTime = flightTime;
		}

		virtual void GetRangedWeapon(float& muzzleVelocity, float& flightTime) const
		{
			muzzleVelocity = activeWeapon.muzzleVelocity;
			flightTime = activeWeapon.flightTime;
		}
	};
}

namespace Dystopia
{
	using namespace Rococo;

	IInventorySupervisor* CreateInventory()
	{
		return new Inventory();
	}
}
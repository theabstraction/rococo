#pragma once

namespace Dystopia
{
	enum HumanType : Rococo::int32
	{
		HumanType_Bobby,
		HumanType_Vigilante
	};

	IHumanSupervisor* CreateBobby(ID_ENTITY id, IInventory& inventory, ILevel& level);
	IHumanSupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, ILevel& level);
}

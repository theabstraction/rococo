#include "dystopia.h"

namespace Dystopia
{
	void InitControlMap(IControlsSupervisor& controls)
	{
		controls.AddAction(L"forward", ActionMapTypeForward, false);
		controls.AddAction(L"backward", ActionMapTypeBackward, false);
		controls.AddAction(L"left", ActionMapTypeLeft, false);
		controls.AddAction(L"right", ActionMapTypeRight, false);
		controls.AddAction(L"select", ActionMapTypeSelect, false);
		controls.AddAction(L"inventory", ActionMapTypeInventory, false);
		controls.AddAction(L"fire", ActionMapTypeFire, false);
		controls.AddAction(L"rotate", ActionMapTypeRotate, false);
		controls.AddAction(L"scale", ActionMapTypeScale, false);
	}
}
#include "dystopia.h"
#include "dystopia.post.h"
#include "human.types.h"
#include "dystopia.ui.h"
#include "rococo.renderer.h"

#include <vector>

namespace
{
	using namespace Dystopia;

	IStringBuilder&  operator << (IStringBuilder& sb, RangedWeapon& weapon)
	{
		if (weapon.muzzleVelocity < 300.0f)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %2.2f m/s", weapon.muzzleVelocity);
		}
		else if (weapon.muzzleVelocity < 3000)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: MACH %.2f", weapon.muzzleVelocity / 330.0f);
		}
		else if (weapon.muzzleVelocity < 100000000)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %.2f km/s", weapon.muzzleVelocity / 1000.0f);
		}
		else
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tParticle beam");
		}

		return sb;
	}

	void ShowSelectOptions_Container(ID_ENTITY id, Environment& e, IEventCallback<ContextMenuItem>& handler)
	{
		std::vector<ContextMenuItem> items;

		items.push_back({ L"Examine", ID_CONTEXT_COMMAND_EXAMINE, (int64)id.value, true });

		Vec3 itemPos;
		e.level.GetPosition(id, itemPos);

		Vec3 playerPos;
		e.level.GetPosition(e.level.GetPlayerId(), playerPos);
		items.push_back({ L"Pick up", ID_CONTEXT_COMMAND_PICKUP, (int64)id.value, IsInRange(itemPos - playerPos, PickupRange()) });
		items.push_back({ L"Cancel", ID_CONTEXT_COMMAND_NONE, 0, true }	);
		items.push_back({ nullptr, 0, 0, true }	);

		GuiMetrics metrics;
		e.renderer.GetGuiMetrics(metrics);
		auto cm = CreateContextMenu(e, metrics.cursorPosition, &items[0], handler);
		e.uiStack.PushTop(cm, ID_PANE_GENERIC_CONTEXT_MENU);
	}
}

namespace Dystopia
{
	namespace Verb
	{
		void Examine(const VerbExamine& target, Environment& e)
		{
			auto* inv = e.level.GetInventory(target.entityId);
			if (inv)
			{
				auto* item = inv->GetItem(target.inventoryIndex);
				if (item)
				{	
					AutoFree<IStringBuilder> sb(CreateSafeStringBuilder(4096));

					auto weapon = item->GetRangedWeaponData();
					if (weapon)
					{
						*sb << *weapon;
					}
					else
					{
						sb->AppendFormat(L"Misc item");
					}	

					struct : IEventCallback<GuiEventArgs>
					{
						virtual void OnEvent(GuiEventArgs& arg) {}
					} static nullHandler;
					e.uiStack.PushTop(CreateDialogBox(e, nullHandler, L"Examine...", *sb, L"", { 640, 480 }, 100, 50), ID_PANE_GENERIC_DIALOG_BOX);
				}
			}
		}

		void ShowSelectOptions(ID_ENTITY id, Environment& e, IEventCallback<ContextMenuItem>& handler)
		{
			auto hs = e.level.GetHuman(id);
			if (!hs.ai)
			{
				// Not a bloke
				auto* inv = e.level.GetInventory(id);
				if (inv)
				{
					// Some kind of equipment container
					ShowSelectOptions_Container(id, e, handler);
				}
			}
		}

		void PickupItem(Environment& e, ID_ENTITY itemId, ID_ENTITY collectorId, Metres maxPickupRange)
		{
			auto collector = e.level.GetInventory(collectorId);
			if (collector)
			{
				Vec3 collectorPos;
				e.level.GetPosition(collectorId, collectorPos);

				EquipmentDesc eq;
				if (e.level.TryGetEquipment(itemId, eq))
				{
					if (IsInRange(collectorPos - eq.worldPosition, maxPickupRange))
					{
						IItem* item = eq.inventory->Swap(0, nullptr);

						uint32 slot;
						if (collector->TryGetFirstFreeSlot(slot))
						{
							IItem* oldItem = collector->Swap(slot, item);
							eq.inventory->Swap(0, oldItem);
							if (!eq.inventory->EnumerateItems(nullptr)) e.level.DeleteEquipment(itemId);
						}
					}
				}
			}
		}
	}
}

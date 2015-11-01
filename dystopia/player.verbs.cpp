#include "dystopia.h"
#include "dystopia.post.h"
#include "human.types.h"
#include "dystopia.ui.h"
#include "rococo.renderer.h"
#include "dystopia.constants.h"

#include <vector>

namespace
{
	using namespace Dystopia;

	IStringBuilder&  operator << (IStringBuilder& sb, RangedWeapon& weapon)
	{
		if (weapon.muzzleVelocity < 300.0f)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tProjectile speed: %2.0f m/s", weapon.muzzleVelocity.value);
		}
		else if (weapon.muzzleVelocity < 3000)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tProjectile speed: MACH %.2f", weapon.muzzleVelocity / 330.0f);
		}
		else if (weapon.muzzleVelocity < 100000000)
		{
			sb.AppendFormat(L"Ranged Weapon:\n\t\t\tProjectile speed: %.2f km/s", weapon.muzzleVelocity / 1000.0f);
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

		items.push_back({ L"Examine", ID_CONTEXT_COMMAND_EXAMINE, (int64)(uint64)id, true });
		Vec3 itemPos;
		e.level.GetPosition(id, itemPos);

		Vec3 playerPos;
		e.level.GetPosition(e.level.GetPlayerId(), playerPos);
		items.push_back({ L"Pick up", ID_CONTEXT_COMMAND_PICKUP, (int64)(uint64)id, IsInRange(itemPos - playerPos, PickupRange()) });
		items.push_back({ L"Open",    ID_CONTEXT_COMMAND_OPEN,   (int64)(uint64)id, IsInRange(itemPos - playerPos, PickupRange()) });
		items.push_back({ L"Cancel",  ID_CONTEXT_COMMAND_NONE,   0, true }	);
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

					sb->AppendFormat(L"%s\n\n", item->Data().name);

					auto weapon = item->GetRangedWeaponData();
					if (weapon)
					{
						*sb << *weapon;
					}

					auto armour = item->GetArmourData();
					if (armour)
					{
						sb->AppendFormat(L"Armour: %u", armour->bulletProtection);
					}

					

					auto ammo = item->GetAmmo();
					if (ammo)
					{
						sb->AppendFormat(L"%d rounds at %0.2f grams per round\n", ammo->count, ammo->massPerBullet.value * 1000.0f);
						if (item->Data().mass.value < 1.0)
						{
							sb->AppendFormat(L"\nClip weight: %2.0f grams. ", item->Data().mass.value * 1000.0f);
						}
						else
						{
							sb->AppendFormat(L"\nClip weight: %2.3f kg. ", item->Data().mass.value);
						}

						float totalMass = item->Data().mass.value + ammo->count * ammo->massPerBullet.value;
						if (totalMass < 1.0)
						{
							sb->AppendFormat(L"\nTotal weight: %2.0f grams\n", totalMass * 1000.0f);
						}
						else
						{
							sb->AppendFormat(L"\nTotal weight: %2.3f kg\n", totalMass);
						}
					}
					else
					{
						if (item->Data().mass.value < 1.0)
						{
							sb->AppendFormat(L"\nWeight: %2.0f grams\n", item->Data().mass.value * 1000.0f);
						}
						else
						{
							sb->AppendFormat(L"\nWeight: %2.3f kg\n", item->Data().mass.value);	
						}
					}

					switch (item->Data().slot)
					{
					case PAPER_DOLL_SLOT_CHEST:
						sb->AppendFormat(L"Note: Fits all chest measurements");
						break;
					case PAPER_DOLL_SLOT_FEET:
						sb->AppendFormat(L"Note: fits feet of all sizes");
						break;
					case PAPER_DOLL_SLOT_HELMET:
						sb->AppendFormat(L"Note: Headwear");
						break;
					case PAPER_DOLL_SLOT_TROUSERS:
						sb->AppendFormat(L"Note: Fits all lengths of leg.");
						break;
					case PAPER_DOLL_SLOT_UNDERWEAR:
						sb->AppendFormat(L"Note: Fits a waist of any girth.");
						break;
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

		void OpenItem(Environment& e, ID_ENTITY itemId, ID_ENTITY collectorId, Metres maxPickupRange)
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
						VerbOpenInventory open;
						open.containerId = itemId;
						e.postbox.SendDirect(open);
					}
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
						uint32 slot;
						if (collector->TryGetFirstFreeSlot(slot))
						{
							IItem* item = nullptr;

							int nSlots = eq.inventory->Span().columns * eq.inventory->Span().rows;
							for (int i = 0; i < nSlots; ++i)
							{
								item = eq.inventory->Swap(i, nullptr);
								if (item != nullptr) break;
							}

							IItem* oldItem = collector->Swap(slot, item);
							eq.inventory->Swap(0, oldItem);
							if (!eq.inventory->EnumerateItems(nullptr)) e.level.DeleteEquipment(itemId);

							e.gui.Add3DHint(eq.worldPosition, L"Looted!"_fstring, 2.5f);
						}
						else
						{
							e.gui.Add3DHint(eq.worldPosition, L"Backpack full!"_fstring, 2.5f);
						}
					}
				}
			}
		}
	}
}

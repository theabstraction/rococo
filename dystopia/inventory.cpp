#include "dystopia.h"
#include <string>

#include "human.types.h"

#include <vector>

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	struct InventoryItemSlot
	{
		IItem* item;

		InventoryItemSlot() : item(nullptr)
		{
		}
	};

	struct AmmoItem : public IItem
	{
		ItemData itemData;
		Ammo ammo;
		std::string name;
	public:
		AmmoItem(const Ammo& _ammo, const ItemData& _itemData) : ammo(_ammo), itemData(_itemData), name(_itemData.name)
		{
			itemData.name = name.c_str();
			itemData.type = ITEM_TYPE_AMMO;
			itemData.slot = PAPER_DOLL_SLOT_EITHER_HAND;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual Ammo* GetAmmo()
		{
			return &ammo;
		}

		virtual ArmourValue* GetArmourData()
		{
			return nullptr;
		}

		virtual RangedWeapon* GetRangedWeaponData()
		{
			return nullptr;
		}

		virtual IInventory* GetContents()
		{
			return nullptr;
		}

		const ItemData& Data() const
		{
			return itemData;
		}
	};

	struct Armour : public IItem
	{
		ItemData itemData;
		ArmourValue armourValue;
		std::string name;
	public:
		Armour(const ArmourValue& _armour, const ItemData& _itemData) : armourValue(_armour), itemData(_itemData), name(_itemData.name)
		{
			itemData.name = name.c_str();
			itemData.type = ITEM_TYPE_ARMOUR;
		}

		virtual void Free()
		{
			delete this;
		}	

		virtual Ammo* GetAmmo()
		{
			return nullptr;
		}
		
		virtual ArmourValue* GetArmourData()
		{
			return &armourValue;
		}

		virtual RangedWeapon* GetRangedWeaponData()
		{
			return nullptr;
		}

		virtual IInventory* GetContents()
		{
			return nullptr;
		}

		const ItemData& Data() const
		{
			return itemData;
		}
	};

	struct RangedWeaponItem : public IItem
	{
		RangedWeapon rangedData;
		std::string name;
		ItemData itemData;
		AutoFree<IInventorySupervisor> clip;
	public:
		RangedWeaponItem(const RangedWeapon& _rangedData, const ItemData& _itemData) :
			rangedData(_rangedData),
			itemData(_itemData),
			name(_itemData.name),
			clip(_rangedData.ammunitionIndex > 0 ? CreateInventory({ 1,1 }, false, false) : nullptr)
		{
			itemData.name = name.c_str();
			itemData.type = ITEM_TYPE_RANGED_WEAPON;
			itemData.slot = PAPER_DOLL_SLOT_EITHER_HAND;
		}

		virtual Ammo* GetAmmo()
		{
			return nullptr;
		}

		virtual ArmourValue* GetArmourData()
		{
			return nullptr;
		}

		const ItemData& Data() const
		{
			return itemData;
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual RangedWeapon* GetRangedWeaponData()
		{
			return &rangedData;
		}

		virtual IInventory* GetContents() 
		{
			return clip;
		}
	};

	class Inventory : public IInventorySupervisor
	{
		std::vector<InventoryItemSlot> items;
		TableSpan span;
		bool hasPaperDoll;
	public:
		virtual void Free() { delete this; }

		Inventory(TableSpan _span, bool hasCursorSlot, bool _hasPaperDoll): 
			span(_span), items((_span.rows * _span.columns) + (hasCursorSlot ? 1 : 0) + (_hasPaperDoll ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0)),
			hasPaperDoll(_hasPaperDoll)
		{
		}

		~Inventory()
		{
			DeleteContents();
		}

		bool HasPaperDoll() const
		{
			return hasPaperDoll;
		}

		virtual void DeleteContents()
		{
			for (auto& i : items)
			{
				if (i.item)
				{
					i.item->Free();
					i.item = nullptr;
				}
			}
		}

		virtual uint32 GetCursorIndex() const
		{
			return (uint32) items.size() - 1;
		}

		virtual IItem* Swap(uint32 index, IItem* item)
		{
			if (index >= items.size())
			{
				return item; // In case an item cannot be swapped, it is returned
			}

			auto old = items[index].item;
			items[index].item = item;
			return old;
		}

		uint32 EnumerateItems(IItemEnumerator* cb) const
		{
			uint32 count = 0, index = 0;
			for (auto& i : items)
			{
				if (i.item)
				{
					if (cb) cb->OnItem(i.item, index);
					count++;
				}

				index++;
			}

			return count;
		}

		virtual IItem* GetItem(uint32 index)
		{
			if (index >= items.size())
			{
				return nullptr;
			}

			return items[index].item;
		}

		virtual uint32 ItemCount() const
		{
			uint32 count = 0;

			for (auto& i : items)
			{
				if (i.item) count++;
			}

			return count;
		}

		virtual TableSpan Span() const
		{
			return span;
		}

		virtual bool TryGetFirstFreeSlot(uint32& index)
		{
			uint32 firstIndex = hasPaperDoll ? PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO : 0;
			for (uint32 i = firstIndex; i < items.size(); ++i)
			{
				if (items[i].item == nullptr)
				{
					index = i;
					return true;
				}
			}

			return false;
		}
	};
}

namespace Dystopia
{
	using namespace Rococo;

	IInventorySupervisor* CreateInventory(TableSpan span, bool hasCursorSlot, bool hasPaperDoll)
	{
		return new Inventory(span, hasCursorSlot, hasPaperDoll);
	}

	IItem* CreateRangedWeapon(const RangedWeapon& rangedData, const ItemData& itemData)
	{
		auto* item = new RangedWeaponItem(rangedData, itemData);
		return item;
	}

	IItem* CreateAmmo(const Ammo& ammo, const ItemData& itemData)
	{
		auto* item = new AmmoItem(ammo, itemData);
		return item;
	}

	IItem* CreateArmour(const ArmourValue& armour, const ItemData& itemData)
	{
		auto* item = new Armour(armour, itemData);
		return item;
	}
}
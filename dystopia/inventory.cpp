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

		InventoryItemSlot(): item(nullptr)
		{
		}
	};

	struct RangedWeaponItem : public IItem
	{
		RangedWeapon data;
		std::wstring name;
		ID_BITMAP bitmapId;

		virtual ITEM_TYPE Type() const 
		{ 
			return ITEM_TYPE_RANGED_WEAPON;
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual const wchar_t* Name() const 
		{
			return name.c_str();
		}

		virtual RangedWeapon* GetRangedWeaponData()
		{
			return &data;
		}

		virtual IInventory* GetContents() 
		{
			return nullptr;
		}

		virtual ID_BITMAP BitmapId() const
		{
			return bitmapId;
		}
	};

	class Inventory : public IInventorySupervisor
	{
		std::vector<InventoryItemSlot> items;
		TableSpan span;
	public:
		virtual void Free() { delete this; }

		Inventory(TableSpan _span, bool hasCursorSlot): span(_span), items((_span.rows * _span.columns) + (hasCursorSlot ? 1 : 0))
		{
		}

		~Inventory()
		{
			DeleteContents();
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
			return span.rows * span.columns;
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
			for (uint32 i = 0; i < items.size(); ++i)
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

	IInventorySupervisor* CreateInventory(TableSpan span, bool hasCursorSlot)
	{
		return new Inventory(span, hasCursorSlot);
	}

	IItem* CreateRangedWeapon(const RangedWeapon& data, const wchar_t* name, ID_BITMAP bitmapId)
	{
		auto* item = new RangedWeaponItem();
		item->data = data;
		item->name = name;
		item->bitmapId = bitmapId;
		return item;
	}
}
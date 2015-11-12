#pragma once

namespace Dystopia
{
	struct RangedWeapon
	{
		Seconds flightTime;
		MetresPerSecond muzzleVelocity;
		int32 ammunitionIndex; // 0 => weapon does not use ammunition
	};

	enum ITEM_TYPE
	{
		ITEM_TYPE_MISC = 0,
		ITEM_TYPE_RANGED_WEAPON,
		ITEM_TYPE_ARMOUR,
		ITEM_TYPE_AMMO
	};

	enum PAPER_DOLL_SLOT
	{
		PAPER_DOLL_SLOT_EITHER_HAND = 0,
		PAPER_DOLL_SLOT_LEFT_HAND = 0, // left from the front view of the paper doll!
		PAPER_DOLL_SLOT_RIGHT_HAND = 1,
		PAPER_DOLL_SLOT_FEET = 2,
		PAPER_DOLL_SLOT_UNDERWEAR = 3,
		PAPER_DOLL_SLOT_TROUSERS = 4,
		PAPER_DOLL_SLOT_CHEST = 5,
		PAPER_DOLL_SLOT_HELMET = 6,
		PAPER_DOLL_SLOT_BACKPACK_INDEX_ZERO = 7 // this is always the last enumerated slot
	};

	struct ArmourValue
	{
		uint32 bulletProtection;
	};

	struct Ammo
	{
		Kilograms massPerBullet;
		int32 ammoType;
		int32 count;
	};

	struct ItemData
	{
		const wchar_t* name;
		ID_BITMAP bitmapId;
		Kilograms mass;
		ITEM_TYPE type;

		// DollSlot gives slot in which an item fits. Most items should return PAPER_DOLL_SLOT_EITHER_HAND. Clothes/armour returns another value.
		// Any item that can be equipped in the left hand can also be equipped in the right hand.
		// All equipment will fit in the left hand, but will not function effectively if not in the correct slot.
		// If an item returns PAPER_DOLL_SLOT_RIGHT_HAND it can be carried in either hand, but is too bulky for a backpack
		PAPER_DOLL_SLOT slot;
	};

	struct IItem
	{
		virtual void Free() = 0;
		virtual Ammo* GetAmmo() = 0;
		virtual ArmourValue* GetArmourData() = 0;	
		virtual RangedWeapon* GetRangedWeaponData() = 0;
		virtual IInventory* GetContents() = 0;
		virtual const ItemData& Data() const = 0;
	};

	IItem* CreateRangedWeapon(const RangedWeapon& rangedData, const ItemData& itemData);
	IItem* CreateArmour(const ArmourValue& armour, const ItemData& itemData);
	IItem* CreateAmmo(const Ammo& ammo, const ItemData& itemData);
	
	struct TableSpan
	{
		uint32 rows;
		uint32 columns;
	};

	enum Enumerate
	{
		Enumerate_Stop,
		Enumerate_Next
	};

	struct IItemEnumerator
	{
		virtual Enumerate OnItem(IItem* item, uint32 slot) = 0;
	};

	ROCOCOAPI IInventory
	{
		virtual uint32 EnumerateItems(IItemEnumerator* cb) const = 0;
		virtual IItem* GetItem(uint32 index) = 0;
		virtual TableSpan Span() const = 0;
		virtual IItem* Swap(uint32 index, IItem* item) = 0;
		virtual uint32 GetCursorIndex() const = 0;
		virtual bool TryGetFirstFreeSlot(uint32& index) = 0;
		virtual bool HasPaperDoll() const = 0;
	};

	ROCOCOAPI IInventorySupervisor : public IInventory
	{
		virtual void Free() = 0;
	};

	IInventorySupervisor* CreateInventory(TableSpan capacity, bool hasCursorSlot, bool hasPaperDoll);

	enum HumanType : int32
	{
		HumanType_None = 0,
		HumanType_Vigilante,
		HumanType_Bobby	
	};

	ROCOCOAPI IIntent
	{
		virtual int32 GetFireCount() const = 0;
		virtual Vec2 GetImpulse() const = 0;
		virtual void SetFireCount(int32 count) = 0;
		virtual int32 PollActivateCount() = 0; // returns number of activations since last call and sets to count to zero
	};

	typedef int32 StatValue;

	struct Stat
	{
		StatValue current;
		StatValue cap;
	}; 

	enum StatIndex : int32
	{
		StatIndex_Health = 0,
		StatIndex_Count
	};

	enum Language
	{
		Language_English
	};

	ROCOCOAPI IHumanAI
	{
		virtual void Describe(IStringBuilder& sb, Language = Language_English) = 0;
		virtual bool IsAlive() const = 0;
		virtual void OnHit(ID_ENTITY attackerId) = 0;
		virtual Stat GetStat(StatIndex index) const = 0;
	};

	ROCOCOAPI IHumanAISupervisor : public IHumanAI
	{
		virtual void Update(float gameTime, float dt) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IHumanFactory
	{
		virtual IHumanAISupervisor* CreateHuman(ID_ENTITY id, HumanType typeId) = 0;
	};

	IHumanAISupervisor* CreateBobby(ID_ENTITY id, Environment& e);
	IHumanAISupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, Environment& e);
}

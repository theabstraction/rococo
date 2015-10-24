#pragma once

namespace Dystopia
{
	ROCOCOAPI IInventory
	{
		virtual void SetRangedWeapon(float muzzleVelocity, float flightTime) = 0;
		virtual void GetRangedWeapon(float& muzzleVelocity, float& flightTime) const = 0;
	};

	ROCOCOAPI IInventorySupervisor : public IInventory
	{
		virtual void Free() = 0;
	};

	IInventorySupervisor* CreateInventory();

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
		virtual cr_vec3 Velocity() const = 0;
		virtual Stat GetStat(StatIndex index) const = 0;
	};

	ROCOCOAPI IHumanAISupervisor : public IHumanAI
	{
		virtual void Update(float gameTime, float dt) = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI IHumanFactory
	{
		virtual IHumanAISupervisor* CreateHuman(ID_ENTITY id, IInventory& inventory, HumanType typeId) = 0;
	};

	IHumanAISupervisor* CreateBobby(ID_ENTITY id, IInventory& inventory, ILevel& level);
	IHumanAISupervisor* CreateVigilante(ID_ENTITY id, IIntent& intent, ILevel& level);
}

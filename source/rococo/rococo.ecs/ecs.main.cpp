#include <rococo.ecs.h>
#include <vector>

namespace Rococo::ECS
{
	enum { CT_BITFIELD_ARRAY_SIZE = 1, CB_BITFIELD_ELEMENT_BITCOUNT = 64 };

	struct Bitfield
	{
		uint64 bits;
	};

	static_assert(8 * sizeof Bitfield == CB_BITFIELD_ELEMENT_BITCOUNT);

#pragma pack(push,4)
	struct RCObject
	{
		RCObject()
		{
			salt.cycle = 0;
			salt.isDeprecated = 1;
		}

		ROID_SALT salt;			// Gives a version number of the object.
		uint32 activeIdIndex = (uint32)-1;	// Specifies which activeId slot references this object
		Bitfield linkedComponents[CT_BITFIELD_ARRAY_SIZE] = { 0 };
		
		bool Exists() const
		{
			return salt.isDeprecated;
		}
	};
#pragma pack(pop)

	struct ECSImplementation : IECSSupervisor
	{
		std::vector<RCObject> handleTable;
		std::vector<ROID> freeIds;
		std::vector<ROID_TABLE_INDEX> activeIds;
		std::vector<ROID> deprecationList;

		uint32 maxTableEntries;
		uint32 enumLock = 0;
		uint64 newRoidCount = 0;

		std::vector<IComponentTableSupervisor*> componentTables;
		
		ECSImplementation(uint64 maxTableSizeInBytes)
		{
			static_assert(sizeof ROID == sizeof uint64);

			uint64 maxTableSize = max(128ULL, maxTableSizeInBytes / (sizeof RCObject + sizeof ROID + sizeof ROID_TABLE_INDEX));

			this->maxTableEntries = maxTableSize > 4000'000ULL ? 4000'000 : (uint32)maxTableSize;

			freeIds.reserve(this->maxTableEntries);
			activeIds.reserve(maxTableEntries);

			for (uint32 i = maxTableEntries - 1; i > 0; i--)
			{
				ROID roid;
				roid.index = i;
				roid.salt.isDeprecated = 0;
				roid.salt.cycle = 1;
				freeIds.push_back(roid);
			}

			handleTable.resize(maxTableEntries);
		}

		[[nodiscard]] size_t ActiveRoidCount() const override
		{
			return activeIds.size();
		}

		void CollectGarbage() override
		{
			if (enumLock > 0)
			{
				Throw(0, "%s: Cannot collect garbage - the ECS is locked for enumeration");
			}

			for (ROID id : deprecationList)
			{
				Deprecate(id);
			}

			deprecationList.clear();
		}

		bool Deprecate(ROID roid) override
		{
			if (!IsActive(roid))
			{
				return false;
			}
			
			RCObject& object = handleTable[roid.index];
			ROID_SALT salt = object.salt;
			if (salt.cycle != roid.salt.cycle || salt.isDeprecated)
			{
				return false;
			}

			if (enumLock > 0)
			{
				// The caller is currently enumerating the roids, which means we cannot immediately delete the roid.
				deprecationList.push_back(roid);
				return false;
			}

			if (activeIds.size() == 1)
			{
				activeIds.clear();
			}
			else
			{
				// Move the last activeId to the deleted position
				ROID_TABLE_INDEX lastRoidIndex = activeIds.back();
				handleTable[lastRoidIndex].activeIdIndex = object.activeIdIndex;
				activeIds[object.activeIdIndex] = lastRoidIndex;

				// And delete the last slot
				activeIds.pop_back();
			}

			NotifyComponentsOfDeprecation(roid, object.linkedComponents);

			object.salt.isDeprecated = 1;
			object.activeIdIndex = (uint32)-1;

			ROID nextRoid;
			nextRoid.index = roid.index;
			nextRoid.salt.cycle = object.salt.cycle + 1;
			nextRoid.salt.isDeprecated = false;

			if (nextRoid.salt.cycle == 0)
			{
				Throw(0, "%s: roid salts exhausted for index %u. Requires a change of algorithm.", roid.index);
			}

			// TODO -> insert a freeid at a random position and swap with the last valid, this reduces likelihood that salts will be exhausted
			freeIds.push_back(nextRoid);
			return true;
		}

		void NotifyComponentsOfDeprecation(ROID roid, const Bitfield linkedComponentTables[CT_BITFIELD_ARRAY_SIZE])
		{
			const Bitfield* field = &linkedComponentTables[-1];

			for (uint32 i = 0; i < componentTables.size(); i++)
			{
				uint32 bitIndex = i % CB_BITFIELD_ELEMENT_BITCOUNT;

				if (bitIndex == 0)
				{
					field++;
					if (field >= &linkedComponentTables[CT_BITFIELD_ARRAY_SIZE])
					{
						Throw(0, "%s: exhausted bitfields. Increase CT_BITFIELD_ARRAY_SIZE", __FUNCTION__);
					}
				}


			}

			for (auto table : componentTables)
			{
				table->Deprecate(roid);
			}
		}

		void DeprecateAll() override
		{
			for (uint32 i = 1; i < maxTableEntries; ++i)
			{
				auto& object = handleTable[i];
				if (object.Exists())
				{
					ROID id;
					id.index = i;
					id.salt = object.salt;
					Deprecate(id);
				}
			}
		}

		void Enumerate(IROIDCallback& cb) override
		{
			enumLock++;

			try
			{
				// Use an index to address the id - this allows the array to grow in size during enumeration without invalidating the iterator
				size_t maxCount = activeIds.size();
				for (size_t i = 0; i < maxCount; i++)
				{
					ROID_TABLE_INDEX index = activeIds[i];

					ROID id;
					id.salt = handleTable[index].salt;
					id.index = index;
					if (cb.OnROID(id) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}

			enumLock--;

			// The callback may have tried to deprecate a ROID. So that iterators are not invalidated we do this after the outermost enumeration is complete

			if (enumLock == 0 && !deprecationList.empty())
			{
				for (ROID id : deprecationList)
				{
					Deprecate(id);
				}

				deprecationList.clear();
			}
		}

		[[nodiscard]] bool IsActive(ROID id) const override
		{
			if (id.index == 0 || id.index >= maxTableEntries)
			{
				return false;
			}

			const RCObject& object = handleTable[id.index];
			return (id.salt.cycle == object.salt.cycle && !object.salt.isDeprecated);
		}

		void LinkComponentTable(IComponentTableSupervisor& table) override
		{
			auto i = std::find(componentTables.begin(), componentTables.end(), &table);
			if (i != componentTables.end())
			{
				Throw(0, "%s: Duplicate table reference provided", __FUNCTION__);
			}

			if (newRoidCount > 0)
			{
				Throw(0, "%s: Unable to link component table. ROIDS have already been created.", __FUNCTION__);
			}

			if (componentTables.size() >= CT_BITFIELD_ARRAY_SIZE * 64)
			{
				Throw(0, "%s: Insufficient elements bitfield array", __FUNCTION__);
			}

			componentTables.push_back(&table);
		}

		[[nodiscard]] uint32 MaxTableEntries() const override
		{
			return maxTableEntries;
		}

		[[nodiscard]] ROID NewROID() override
		{
			newRoidCount++;

			if (freeIds.empty())
			{
				Throw(0, "NewROID: Roids exhausted");
			}

			ROID newId = freeIds.back();
			freeIds.pop_back();

			RCObject& object = handleTable[newId.index];
			object.salt = newId.salt;

			object.activeIdIndex = (uint32)activeIds.size();
			activeIds.push_back(newId.index);

			return newId;
		}

		virtual ~ECSImplementation()
		{
		}

		bool TryLockedOperation(ROID roid, IECS_ROID_LockedSection& section) override
		{
			if (IsActive(roid))
			{
				section.OnLock(roid, *this);
				return true;
			}
			else
			{
				return false;
			}
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	ROCOCO_ECS_API IECSSupervisor* CreateECS(uint64 maxTableSizeInBytes)
	{
		return new Rococo::ECS::ECSImplementation(maxTableSizeInBytes);
	}
}
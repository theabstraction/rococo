#include <rococo.ecs.h>
#include <vector>

#include <rococo.allocators.inl>

DeclareAllocator(DefaultAllocator, ECS, g_allocator)
Rococo::Memory::AllocatorMonitor<ECS> monitor;

OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

namespace Rococo::ECS
{
	enum { CT_BITFIELD_ARRAY_SIZE = 1, CT_BITFIELD_ELEMENT_BITCOUNT = 64 };

	struct Bitfield
	{
		uint64 bits;
	};

	static_assert(8 * sizeof Bitfield == CT_BITFIELD_ELEMENT_BITCOUNT);

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

	template<typename LAMBDA, typename TABLES>
	void ForEachAssociatedTable(TABLES& componentTables, const Bitfield linkedComponentTables[CT_BITFIELD_ARRAY_SIZE], LAMBDA action)
	{
		if (componentTables.empty()) return;

		const Bitfield* field = &linkedComponentTables[-1];
		uint64 bit = 0ULL;

		uint32 tableIndex = 0;
		for (auto* table : componentTables)
		{
			if (!bit)
			{
				bit = 1ULL;

				field++;
				if (field >= &linkedComponentTables[CT_BITFIELD_ARRAY_SIZE])
				{
					Throw(0, "%s: exhausted bitfields. Increase CT_BITFIELD_ARRAY_SIZE to %u", __FUNCTION__, CT_BITFIELD_ARRAY_SIZE + 1);
				}
			}

			if ((field->bits & bit) != 0)
			{
				action(tableIndex, *table);
			}

			bit = bit << 1;
			tableIndex++;
		}
	}

	struct ECSImplementation : IECSSupervisor
	{
		std::vector<RCObject> handleTable;
		std::vector<ROID> freeIds;
		std::vector<ROID_TABLE_INDEX> activeIds;
		std::vector<ROID> deprecationList;

		// When a roid is about to be deleted, it is appended to the stack, and removed from the stack after deletion.
		// This way we can evaluate recursion issues where observers may react by trying to delete other stuff, including the roid itself
		std::vector<ROID> outgoingRoidStack;

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

		[[nodiscard]] size_t AvailableRoidCount() const override
		{
			return freeIds.size();
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
			return DeprecateWithNotify(roid, (uint32)-1);
		}

		bool DeprecateWithNotify(ROID roid, uint32 sourceTableIndex)
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

			NotifyComponentsOfDeprecation(roid, object.linkedComponents, sourceTableIndex);

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

		void NotifyComponentsOfDeprecation(ROID roid, const Bitfield roidsLinkedComponentTables[CT_BITFIELD_ARRAY_SIZE], uint32 sourceTableIndex)
		{
			auto existingCopy = std::find(outgoingRoidStack.begin(), outgoingRoidStack.end(), roid);
			if (existingCopy != outgoingRoidStack.end())
			{
				// We are already dealing with notifying everyone that the roid is being deprecated
				return;
			}

			outgoingRoidStack.push_back(roid);
			ForEachAssociatedTable(componentTables, roidsLinkedComponentTables,
				[roid, sourceTableIndex](uint32 tableIndex, IComponentTableSupervisor& table)
				{
					if (tableIndex != sourceTableIndex)
					{
						table.OnNotifyThatTheECSHasDeprecatedARoid(roid);
					}
				}
			);
			outgoingRoidStack.pop_back();
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

			if (componentTables.size() >= CT_BITFIELD_ARRAY_SIZE * CT_BITFIELD_ELEMENT_BITCOUNT)
			{
				Throw(0, "%s: Insufficient elements in bitfield array", __FUNCTION__);
			}

			uint32 index = (uint32)componentTables.size();

			componentTables.push_back(&table);

			table.BindTableIndex(index);
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

		void OnNotifyAComponentHasDeprecatedARoid(ROID roid, uint32 sourceTableIndex) override
		{
			DeprecateWithNotify(roid, sourceTableIndex);
		}

		void OnNotifyComponentAttachedToROID(ROID roid, uint32 sourceTableIndex) override
		{
			if (sourceTableIndex >= componentTables.size())
			{
				Throw(0, "%s: Invalid [sourceTableIndex=%u]", __FUNCTION__, sourceTableIndex);
			}

			if (roid.index >= handleTable.size())
			{
				Throw(0, "%s: Invalid [id=%d]", __FUNCTION__, roid.index);
			}

			RCObject& object = handleTable[roid.index];
			ROID_SALT salt = object.salt;
			if (salt.cycle != roid.salt.cycle)
			{
				Throw(0, "%s: Invalid [id=%d]. (Bad salt value %u)", __FUNCTION__, roid.index, roid.salt.cycle);
			}

			uint32 bitmapArrayIndex = sourceTableIndex / CT_BITFIELD_ELEMENT_BITCOUNT;
			if (bitmapArrayIndex >= CT_BITFIELD_ARRAY_SIZE)
			{
				Throw(0, "%s: Insufficient bits in RCObject bitfield. Increase CT_BITFIELD_ARRAY_SIZE to %u", __FUNCTION__, bitmapArrayIndex + 1);
			}

			auto& bf = object.linkedComponents[bitmapArrayIndex];
			uint64 bit = 1ULL << sourceTableIndex % 64;
			bf.bits |= bit;
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
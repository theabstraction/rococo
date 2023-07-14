#include <rococo.ecs.h>
#include <vector>

namespace Rococo::ECS
{
#pragma pack(push,4)
	struct RCObject
	{
		enum { DEPRECATED };
		RCObject() : salt(DEPRECATED)
		{
		}

		ROID_SALT salt;			// Gives a version number of the object.
		uint32 activeIdIndex = (uint32)-1;	// Specifies which activeId slot references this object
		
		bool Exists() const
		{
			return salt != DEPRECATED;
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

		IECSErrorHandler& errorHandler;

		std::vector<IComponentTable*> componentTables;

		ECSImplementation(IECSErrorHandler& _errorHandler, uint64 maxTableSizeInBytes): errorHandler(_errorHandler)
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
				roid.salt = 1;
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
				errorHandler.OnError(__FUNCTION__, __LINE__, "Cannot collect garbage - the ECS is locked for enumeration", true, IECSErrorHandler::ECS_ErrorCause::GC_Locked_Enumeration);
				return;
			}

			for (ROID id : deprecationList)
			{
				Deprecate(id);
			}

			deprecationList.clear();
		}

		bool Deprecate(ROID roid) override
		{
			if (roid.index == 0 || roid.index >= maxTableEntries)
			{
				return false;
			}

			RCObject& object = handleTable[roid.index];
			if (object.salt == roid.salt)
			{
				if (enumLock > 0)
				{
					// The caller is currently enumerating the roids, which means we cannot immediately delete the roid.
					deprecationList.push_back(roid);
					return false;
				}

				object.salt = RCObject::DEPRECATED;

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

				object.activeIdIndex = (uint32)-1;

				ROID nextRoid;
				nextRoid.index = roid.index;
				nextRoid.salt = object.salt + 1;

				nextRoid.salt = max(1U, nextRoid.salt);

				freeIds.push_back(nextRoid);
				return true;
			}

			return false;
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
			return (id.salt == object.salt);
		}

		void LinkComponentTable(IComponentTable& table) override
		{
			auto i = std::find(componentTables.begin(), componentTables.end(), &table);
			if (i != componentTables.end())
			{
				errorHandler.OnError(__FUNCTION__, __LINE__, "Duplicate table reference provided", false, IECSErrorHandler::ECS_ErrorCause::LinkComponentTable_Duplicate);
				return;
			}
			componentTables.push_back(&table);
		}

		[[nodiscard]] uint32 MaxTableEntries() const override
		{
			return maxTableEntries;
		}

		[[nodiscard]] ROID NewROID() override
		{
			if (freeIds.empty())
			{
				errorHandler.OnError(__FUNCTION__, __LINE__, "RCObjectTable is full", true, IECSErrorHandler::ECS_ErrorCause::OutOfRoids);
				return ROID::Invalid();
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
	ROCOCO_ECS_API IECSSupervisor* CreateECS(IECSErrorHandler& errorHandler, uint64 maxTableSizeInBytes)
	{
		return new Rococo::ECS::ECSImplementation(errorHandler, maxTableSizeInBytes);
	}
}
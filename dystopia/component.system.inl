#pragma once

#include <unordered_map>

namespace Dystopia
{
	ID_ENTITY GenerateEntityId()
	{
		static ID_ENTITY nextId = 0;
		nextId += 1;
		return nextId;
	}

	template<class ROW> class EntityTable : public std::unordered_map<ID_ENTITY, ROW>
	{
	public:
		std::pair<iterator, bool> insert(ID_ENTITY id, const ROW& row)
		{
			return std::unordered_map<ID_ENTITY, ROW>::insert(std::make_pair(id, row));
		}
	};

	template<class ROW> void FreeTable(EntityTable<ROW>& table)
	{
		for (auto i : table)
		{
			delete i.second;
		}

		table.clear();
	}
}
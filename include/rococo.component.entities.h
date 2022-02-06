#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	typedef uint32 ENTITY_TABLE_ID;
	typedef uint32 ENTITY_SALT;

#pragma pack(push, 1)
	struct EntityIndex
	{
		ENTITY_SALT salt;
		ENTITY_TABLE_ID id;

		typedef uint64 ENTITY_PRIMITIVE;

		ENTITY_PRIMITIVE AsPrimitive() const { return *(int64*)this; }
	};
#pragma pack(pop)

	struct EntityIndexHasher
	{
		size_t operator()(EntityIndex index) const
		{
			return (size_t)index.id;
		}
	};

	inline bool operator == (const EntityIndex& a, const EntityIndex& b)
	{
		return a.AsPrimitive() == b.AsPrimitive();
	}

	struct EntityIndexComparer
	{
		bool operator()(const EntityIndex& a, const EntityIndex& b) const
		{
			return a == b;
		}
	};
}


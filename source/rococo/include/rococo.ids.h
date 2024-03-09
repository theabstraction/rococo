#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_ID_API
# define ROCOCO_ID_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	struct UniqueIdHolder
	{
		UniqueIdHolder()
		{
			iValues[0] = iValues[1] = 0;
		}

		int64 iValues[2];
		
		bool operator == (const UniqueIdHolder& other) const
		{
			bool result = this->iValues[0] == other.iValues[0] && this->iValues[1] == other.iValues[1];
			return result;
		}

		bool operator != (const UniqueIdHolder& other) const
		{
			return !(*this == other);
		}

		operator bool() const
		{
			return iValues[0] != 0 || iValues[1] != 0;
		}

		size_t HashCode() const
		{
			return iValues[0] ^ iValues[1];
		}
	};

	ROCOCO_ID_API UniqueIdHolder MakeNewUniqueId();
}

#define MAKE_UNIQUE_TYPEID(TYPENAME)				\
struct TYPENAME										\
{													\
	Rococo::UniqueIdHolder id;						\
													\
	bool operator == (const TYPENAME& other) const	\
	{												\
		bool result = this->id == other.id;			\
		return result;								\
	}												\
													\
	bool operator != (const TYPENAME& other) const	\
	{												\
		return !(*this == other);					\
	}												\
													\
	operator bool() const							\
	{												\
		return id;									\
	}												\
													\
	struct Hasher									\
	{															\
		size_t operator()(const TYPENAME& t) const noexcept		\
		{														\
			return t.id.HashCode();								\
		}														\
	};															\
};	
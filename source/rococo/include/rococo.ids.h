// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_ID_API
# define ROCOCO_ID_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	struct UniqueIdHolder
	{
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

	namespace Ids
	{
		struct GuidString
		{
			char buffer[40];
		};

		ROCOCO_ID_API UniqueIdHolder MakeNewUniqueId();
		ROCOCO_ID_API void ToGuidString(UniqueIdHolder id, OUT GuidString& guidString);
		ROCOCO_ID_API bool TryScanGuid(OUT UniqueIdHolder& id, cstr buffer);
	}
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
	operator Rococo::UniqueIdHolder() const	        \
	{												\
		return id;									\
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
#pragma once

// N.B not for inclusion in other headers. Since we include STL we don't want to propagate
// STL into the Rococo API if we can avoid doing so.

#include <rococo.api.h>
#include <rococo.strings.h>
#include <vector>
#include <algorithm>

namespace Rococo
{
	/* 
		A THandle identifies a resource. The underlying data is 64-bits wide
		The bit fields are thus: [SALT] [INDEX]
		The right most bits index a resource, the left most bits add a 'salt' value
		If the container's salt does not match that of the handle, then the resource has expired.
		This technique allows us to maintain weak references to objects without using dangerous pointers
	*/
	template<uint64 SALT_SHIFT, uint64 INDEX_MASK>
	struct THandle
	{
		THandle(uint64 _value = 0) : value( _value)
		{
			static_assert(SALT_SHIFT > 1, "Insuficient salt");
			static_assert(INDEX_MASK >> SALT_SHIFT == 0, "Bad salt shift/mask. Not enough shift");
			static_assert(INDEX_MASK >> (SALT_SHIFT - 1) != 0, "Bad salt shift/mask. Too much shift");
		}

		uint64 value = 0; // Default value of zero means undefined, invalid, null.
		operator bool()  const { return value != 0; };
		uint64 ToIndex() const { return value & INDEX_MASK; }
		uint64 Salt()    const { return value >> SALT_SHIFT; }
		void IncrementSalt()
		{
			uint64 saltMask = ~INDEX_MASK;
			uint64 newSalt = ((Salt() + 1) << SALT_SHIFT) & saltMask;
			uint64 newValue = ToIndex() + newSalt;
			value = newValue;
		}
	};


	/* HandleTable features constant time search, insert and delete of objects. Enumeration not so fast...
	   but enumeration is not the emphasis, and the enumeration is designed to be safe even when elements
	   are inserted and deleted. This allows mutliple threads and coroutines to enumrerate and modify
	   without crashing.

	   MASK gives the bitmask on the Handle type that has a 1 for every bit in the handle that forms an index
	   ~MASK gives the bitmask on the Handle type that has a 1 for every bit in the handle that forms the salt

	   salt_shift gives to number of bits to shift right to expose the salt value.
	 */

	template<class OBJECTPTRTYPE, uint64 MASK, uint64 SALT_SHIFT>
	class HandleTable
	{
	public:
		typedef THandle<SALT_SHIFT, MASK> Handle;
	private:

		struct Binder
		{
			Handle handle;
			OBJECTPTRTYPE* object;
		};

		std::vector<Binder> handles;
		std::vector<Handle> freeHandles;
		
		size_t length = 0;

		HString name;

		uint64 GetIndex(Handle hItem)
		{
			uint64 index = hItem.ToIndex();
			return index - 1;
		}

		OBJECTPTRTYPE* nullObject;

	public:
		HandleTable(cstr _name, size_t capacity, OBJECTPTRTYPE* _nullObject) :
			nullObject(_nullObject),
			name(_name)
		{
			if (capacity == 0) Throw(0, "HandleTable<%s>::Could not construct HandeTable. Capacity was zero", Name());
			if (capacity > 1000000000) Throw(0, "HandleTable<%s>::Could not construct HandeTable. Maximum of 1 billion handles but %llu were requested", Name(), capacity);
			handles.resize(capacity);
			std::fill(handles.begin(), handles.end(), Binder{ 0, nullObject });

			for (size_t i = 0; i < handles.size(); i++)
			{
				auto hItem = Handle{ handles.size() - i };
				freeHandles.push_back(hItem);
			}
		}

		Handle CreateNew()
		{
			if (freeHandles.empty())
			{
				Throw(0, "HandleTable<%s>::CreateNew() - No more free handles", Name());
			}

			Handle handle = freeHandles.back();
			freeHandles.pop_back();

			uint64 index = GetIndex(handle);

			handle.IncrementSalt();

			handles[index].handle = handle;

			length++;

			return handle;
		}

		void SetPointer(Handle hItem, OBJECTPTRTYPE* object)
		{
			uint64 index = GetIndex(hItem);
			if (index >= handles.size() || handles[index].handle == hItem)
			{
				handles[index].object = object;
			}
			else
			{
				Throw(0, "HandleTable<%s>::SetPointer failed. The handle %%u was invalid.", Name(), hItem.value);
			}
		}

		OBJECTPTRTYPE* ToPointer(Handle hItem)
		{
			uint64 index = GetIndex(hItem);
			if (handles[index].handle == hItem)
			{
				return handles[index].object;
			}
			else
			{
				return nullptr;
			}
		}

		void Invalidate(Handle hItem)
		{
			uint64 index = GetIndex(hItem);

			if (handles[index].handle == hItem)
			{
				freeHandles.push_back(hItem);
				handles[index].handle = Handle();
				handles[index].object = nullObject;
				length--;
			}
		}

		Handle GetFirstHandle() const
		{
			for (auto i = handles.begin(); i != handles.end(); ++i)
			{
				if (i->handle)
				{
					return i->handle;
				}
			}

			return Handle();
		}

		// Typically not fast, especially when the array is mostly empty, but
		// it is safe for different coroutines to invalidate handles during the enumeration
		Handle GetNextHandle(Handle hItem)
		{
			if (handles.empty()) { return Handle(); }

			uint64 index = GetIndex(hItem);
			if (index >= handles.size())
			{
				Throw(0, "HandleTable<%s>::GetNextHandle: Bad handle %llu", Name(), index);
			}

			auto i = handles.begin();
			std::advance(i, index);

			size_t count = 0;

			while (count < length)
			{
				i++;

				if (i == handles.end())
				{
					return Handle();
				}

				if (i->handle != 0)
				{
					count++;
					return i->handle;
				}
			}

			return Handle{ 0 };
		}

		cstr Name() const { return name; }
	};
} // Rococo

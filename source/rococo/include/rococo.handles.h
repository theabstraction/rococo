// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

// N.B not for inclusion in other headers. Since we include STL we don't want to propagate
// STL into the Rococo API if we can avoid doing so.

#include <rococo.api.h>
#include <rococo.strings.h>
#include <vector>
#include <algorithm>

#include <rococo.ringbuffer.h>

namespace Rococo
{
	/* 
		A THandle identifies a resource. The underlying data is 64-bits wide
		The bit fields are thus: [SALT] [INDEX]
		The low most bits index a resource, the high order bits add a 'salt' value
		If the container's salt does not match that of the handle, then the resource has expired.
		This technique allows us to maintain weak references to objects without using dangerous pointers
	*/

	constexpr uint64 GetIndexMask(uint64 saltBitcount)
	{
		if (saltBitcount < 1 || saltBitcount > 32) throw std::exception("Bad salt");
		uint64 i = (uint64)-1LL;
		return i >> saltBitcount;
	}

	template<uint64 SALT_BITCOUNT>
	class THandle
	{
	public:
		static constexpr uint64 INDEX_MASK = GetIndexMask(SALT_BITCOUNT);
		static constexpr uint64 SALT_SHIFT = (8 * sizeof(uint64)) - SALT_BITCOUNT;
		static constexpr uint64 SALT_MASK = ~INDEX_MASK;

	private:
		uint64 value = 0; // Default value of zero means undefined, invalid, null.

	public:
		THandle(uint64 _value = 0) : value( _value)
		{
		}

		uint64 ToIndex() const { return value & INDEX_MASK; }
		uint64 Salt()    const { return value >> SALT_SHIFT; }
		uint64 Value()   const { return value; }

		void IncrementSalt()
		{
			uint64 newSalt = (Salt() + 1) << SALT_SHIFT;
			value = ToIndex() | newSalt;
		}

		bool operator == (const THandle& other) const { return value == other.value; }
		operator bool() const { return value != 0; }
		static auto Invalid() { return THandle<SALT_BITCOUNT>(0); }
	};


	/* HandleTable features constant time search, insert and delete of objects. Enumeration not so fast...
	   but enumeration is not the emphasis, and the enumeration is designed to be safe even when elements
	   are inserted and deleted. This allows mutliple threads and coroutines to enumerate and modify
	   without crashing.

	   MASK gives the bitmask on the Handle type that has a 1 for every bit in the handle that forms an index
	   ~MASK gives the bitmask on the Handle type that has a 1 for every bit in the handle that forms the salt

	   salt_shift gives to number of bits to shift right to expose the salt value.
	 */
	template<class VALUE, uint64 SALT_BITCOUNT>
	class HandleTable
	{
	public:
		static constexpr uint64 MAX_CAPACITY = 1000'000'000;
		typedef THandle<SALT_BITCOUNT> Handle;
	private:
		struct TableItem
		{
			Handle handle;
			VALUE value;
		};

		std::vector<TableItem> items;

		OneReaderOneWriterCircleBuffer<Handle> freeHandles;
		
		size_t length = 0;

		Strings::HString name;

		uint64 GetIndex(Handle hItem)
		{
			uint64 index = hItem.ToIndex();
			return index - 1;
		}

	public:
		HandleTable(cstr _name, size_t capacity) :
			name(_name), freeHandles(capacity)
		{
			if (capacity == 0) Throw(0, "HandleTable<%s>::Could not construct HandleTable. Capacity was zero", Name());
			if (capacity > MAX_CAPACITY) Throw(0, "HandleTable<%s>::Could not construct HandeTable. Maximum of %llu handles but %llu were requested", Name(), MAX_CAPACITY, capacity);
			items.resize(capacity);
			std::fill(items.begin(), items.end(), TableItem { 0, VALUE() });

			for (size_t i = 0; i < items.size(); i++)
			{
				auto hItem = Handle{ i + 1 };
				Handle* hSlot = freeHandles.GetBackSlot();
				if (!hSlot) Throw(0, "%s: Error getting backslot for free handles.", __ROCOCO_FUNCTION__);
				*hSlot = hItem;
				freeHandles.WriteBack();
			} // The freeHandles now has 1 as the back most entry then 2.... all the way to handles.size()
		}

		void Clear(const VALUE& nullObject)
		{
			Handle hItem = GetFirstHandle();
			while (hItem)
			{
				Destroy(hItem, nullObject);
				hItem = GetNextHandle(hItem);
			}
		}

		Handle CreateNew()
		{
			Handle handle;
			if (!freeHandles.TryPopFront(handle))
			{
				Throw(0, "HandleTable<%s>::CreateNew() - No more free handles", Name());
			}
			
			uint64 index = GetIndex(handle);

			handle.IncrementSalt();

			items[index].handle = handle;

			length++;

			return handle;
		}

		void Set(Handle hItem, VALUE value)
		{
			if (!TrySet(hItem, value))
			{
				Throw(0, "HandleTable<%s>::Set failed. The handle %llX was invalid.", Name(), hItem.Value());
			}
		}

		bool TrySet(Handle hItem, VALUE value)
		{
			uint64 index = GetIndex(hItem);
			if (index < items.size() && items[index].handle == hItem)
			{
				items[index].value = value;
				return true;
			}
			else
			{
				return false;
			}
		}

		bool TryGet(Handle hItem, VALUE* pValue)
		{
			uint64 index = GetIndex(hItem);
			if (index < items.size() && items[index].handle == hItem)
			{
				*pValue = items[index].value;
				return true;
			}
			else
			{
				return false;
			}
		}

		bool TryGetRef(Handle hItem, VALUE** ppValue)
		{
			uint64 index = GetIndex(hItem);
			if (index < items.size() && items[index].handle == hItem)
			{
				*ppValue = &items[index].value;
				return true;
			}
			else
			{
				return false;
			}
		}

		void Destroy(Handle hItem, const VALUE& nullObject)
		{
			uint64 index = GetIndex(hItem);

			if (index < items.size() && items[index].handle == hItem)
			{
				auto* backSlot = freeHandles.GetBackSlot(); 
				if (backSlot)
				{
					*backSlot = hItem;
					freeHandles.WriteBack();
				}
				items[index].handle = Handle::Invalid();
				items[index].value = nullObject;
				length--;
			}
		}

		Handle GetFirstHandle() const
		{
			for (auto i = items.begin(); i != items.end(); ++i)
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
			if (items.empty()) { return Handle(); }

			uint64 index = GetIndex(hItem);
			if (index >= items.size())
			{
				Throw(0, "HandleTable<%s>::GetNextHandle: Bad handle %llu", Name(), index);
			}

			auto i = items.begin();
			std::advance(i, index);

			size_t count = 0;

			while (count < length)
			{
				i++;

				if (i == items.end())
				{
					return Handle::Invalid();
				}

				if (i->handle)
				{
					count++;
					return i->handle;
				}
			}

			return Handle::Invalid();
		}

		cstr Name() const { return name; }
	};
} // Rococo

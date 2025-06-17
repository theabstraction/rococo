#include <rococo.types.h>

#define USE_HSTRING_HASH
#include <rococo.strings.h>
#include <rococo.map.h>
#include <rococo.hashtable.h>

#include <new>
#include <string.h>
#include <vector>
#include <stdlib.h>

using namespace Rococo::Strings;

namespace Rococo::Strings
{
	// Heap-allocated immutable string

	static HStringData nullData{ "", 0, 0 };

	struct DefaultAllocator: public IAllocator
	{
		virtual ~DefaultAllocator()
		{			
		}

		void AtRelease(void(*FN_AllocatorRelease)()) override
		{
			atexit(FN_AllocatorRelease);
		}

		void* Allocate(size_t capacity) override
		{
			return malloc(capacity);
		}

		void FreeData(void* data) override
		{
			free(data);
		}

		void* Reallocate(void* ptr, size_t capacity) override
		{
			return realloc(ptr, capacity);
		}

		size_t EvaluateHeapSize()
		{
			return 0;
		}
	};

	static DefaultAllocator defaultAllocator;
	static IAllocator* stringAllocator = &defaultAllocator;

	ROCOCO_API void SetStringAllocator(IAllocator* a)
	{
		stringAllocator = (a == nullptr) ? &defaultAllocator : a;
	}

	ROCOCO_API HString::HString(HString&& other) noexcept
	{
		data = other.data;
		other.data = &nullData;
	}

	ROCOCO_API HString::HString(cstr s)
	{
		if (s == nullptr)
		{
			data = &nullData;
			return;
		}

		size_t length = strlen(s);
		char* a = (char*) stringAllocator->Allocate(sizeof(HStringData) + length + 1);
		char* insertPos = a + sizeof(HStringData);
		memcpy(insertPos, s, length + 1);
		data = (HStringData*) a;
		data->currentBuffer = insertPos;
		data->length = length;
		data->refCount = 1;
	}

	ROCOCO_API HString::HString() : data{ &nullData }
	{
	}

	ROCOCO_API HString::HString(const HString& s)
	{
		if (this == &s)
		{
			Throw(0, "Error. Client of HString tried to initialize a string with itself.");
		}
		data = s.data;
		if (data->refCount > 0) data->refCount++;
	}

	ROCOCO_API void FreeHeapStringData(HStringData* data)
	{
		if (data->refCount > 0)
		{
			data->refCount--;
			if (data->refCount == 0)
			{
				stringAllocator->FreeData(data);
			}
		}
	}

	ROCOCO_API HString& HString::operator = (const HString& s)
	{
		// Edge case - source may currently match the target
		if (s.data != data)
		{
			// Assiging a thing to itself does nothing
			FreeHeapStringData(data);

			data = s.data;
			if (data != &nullData) data->refCount++;
		}

		return *this;
	}

	ROCOCO_API HString& HString::operator = (cstr s)
	{
		// Edge case - source may currently match the target
		if (s != data->currentBuffer)
		{
			FreeHeapStringData(data);
			data = &nullData;

			if (s != nullptr && *s != 0)
			{
				size_t length = strlen(s);
				char* a = (char*)stringAllocator->Allocate(sizeof(HStringData) + length + 1);
				char* insertPos = a + sizeof(HStringData);
				memcpy(insertPos, s, length + 1);
				data = (HStringData*)a;
				data->currentBuffer = insertPos;
				data->length = length;
				data->refCount = 1;
			}
		}

		return *this;
	}

	ROCOCO_API HString::~HString()
	{
		FreeHeapStringData(data);
	}

	ROCOCO_API const fstring HString::to_fstring() const
	{
		if (length() >= 0x80000000LL)
		{
			Throw(0, "HString to Fstring -> length was too large for size_t to int32 conversion");
		}

		return fstring{ c_str(), (int32)length() };
	}

	ROCOCO_API size_t FastHash(cstr text, size_t length)
	{
		int64 nBigWords = length >> 3;
		const int64 * src = (int64*)text;

		int64 sum = 0;
		for (int64 i = 0; i < nBigWords; ++i)
		{
			sum ^= *src++;
		}

		int64 remainder = length - (nBigWords << 3);

		const char* trailer = (char*)src;

		int64 finalWord = 0;

		char* dest = (char*)&finalWord;

		for (int64 i = 0; i < remainder; ++i)
		{
			*dest++ = *trailer++;
		}

		sum ^= finalWord;

		return (size_t)sum;
	}

	// size_t operator is used by std::unordered_map to generate a hash code.
	ROCOCO_API size_t HString::ComputeHash() const
	{
		return FastHash(data->currentBuffer, data->length);
	}
}

namespace ANON
{
	using namespace Rococo;
	IAllocator* moduleAllocator = &defaultAllocator;
}

#define _MODULE_ALLOCATOR_AVAILABLE
#include <allocators/rococo.allocator.via.interface.h>

namespace ANON
{
	using namespace Rococo;

	struct Value
	{
		void *data;
	};

	class KeyValuePair
	{
		Rococo::Strings::HString key;
		Value data;
	};

	// StringKey is intended for use as a hash table key type
	// It supports both stack strings and heap strings
	// As a proxy for a stack string it can be used to match a string without using heap resources.
	// As a proxy for heap strings it can be used to persist a key-value pair in the hash table.
	struct StringKey
	{
		Rococo::Strings::HString stringKey;
		const char* invariantString;
		size_t hashCode;

		StringKey(cstr stackString, int unused) : invariantString(stackString)
		{
			UNUSED(unused);
			hashCode = Rococo::Strings::FastHash(stackString, strlen(stackString));
		}

		StringKey() : invariantString(""), hashCode(0) {}
		StringKey(const StringKey& src) :
			stringKey(src.stringKey),
			invariantString(src.invariantString),
			hashCode(src.hashCode)
		{
		}

		StringKey& operator = (const StringKey& src)
		{
			stringKey = src.stringKey;
			invariantString = src.invariantString;
			hashCode = src.hashCode;
			return *this;
		}

		StringKey(cstr key) : stringKey(key)
		{
			invariantString = stringKey.c_str();
			hashCode = stringKey.ComputeHash();
		}

		StringKey(cstr key, size_t _hashCode) : stringKey(key)
		{
			invariantString = stringKey.c_str();
			hashCode = _hashCode;
		}

		static StringKey AsStackPointer(cstr key)
		{
			return StringKey(key, 0);
		}

		operator size_t() const
		{
			return hashCode;
		}

		operator cstr() const
		{
			return stringKey;
		}

		size_t length() const
		{
			return stringKey.length();
		}
	};

	bool operator == (const StringKey& a, const StringKey& b)
	{
		return a.hashCode == b.hashCode && strcmp(a.invariantString, b.invariantString) == 0;
	}

	bool operator != (const StringKey& a, const StringKey& b)
	{
		return !(a == b);
	}

	struct KeyValuePairAllocator
	{
		typedef Value value_type;
	};

	struct StringKeyHash
	{
		size_t operator () (const StringKey& k) const
		{
			return k;
		}
	};

	struct StringKeyEq
	{
		bool operator () (const StringKey& a, const StringKey& b) const
		{
			return a == b;
		}
	};

	class DictionaryImplementation : public Rococo::IDictionarySupervisor
	{
		std::unordered_map<StringKey, Value, StringKeyHash, StringKeyEq, AllocatorWithInterface<std::pair<const StringKey, Value>>> map;
	public:
		virtual ~DictionaryImplementation()
		{

		}

		bool TryAddUnique(cstr key, void* data) override
		{
			const auto& s = StringKey::AsStackPointer(key);
			auto i = map.find(s);
			if (i == map.end())
			{
				map[StringKey(key, (size_t)s)] = Value{ data };
				return true;
			}
			else
			{
				return false;
			}
		}

		void Enumerate(IDictionaryEnumerator& enumerator) override
		{
			auto i = map.begin();
			while (i != map.end())
			{
				auto control = enumerator.OnIteration(i->first, i->first.length(), i->second.data);

				switch (control)
				{
				case ENUM_CONTINUE:
					i++;
					break;
				case ENUM_BREAK:
					return;
				case ENUM_ERASE_AND_BREAK:
					map.erase(i);
					return;
				case ENUM_ERASE_AND_CONTINUE:
					i = map.erase(i);
					break;
				default:
					Throw(0, "DictionaryImplementation::Enumerate(...) -> unrecognized enum value %u", control);
					return;
				}
			}
		}

		bool TryDetach(cstr key, void*& data) override
		{
			auto i = map.find(StringKey::AsStackPointer(key));
			if (i != map.end())
			{
				data = i->second.data;
				map.erase(i);
				return true;
			}
			else
			{
				data = nullptr;
				return false;
			}
		}

		bool TryFind(cstr key, void*& data) override
		{
			auto i = map.find(StringKey::AsStackPointer(key));
			if (i != map.end())
			{
				data = i->second.data;
				return true;
			}
			else
			{
				data = nullptr;
				return false;
			}
		}

		bool TryFind(cstr key, const void*& data) const override
		{
			auto i = map.find(StringKey::AsStackPointer(key));
			if (i != map.end())
			{
				data = i->second.data;
				return true;
			}
			else
			{
				data = nullptr;
				return false;
			}
		}

		void Free() override
		{
			DictionaryImplementation::~DictionaryImplementation();
			ANON::moduleAllocator->FreeData(this);
		}
	};
}

namespace Rococo
{
	ROCOCO_API IDictionarySupervisor* CreateDictionaryImplementation()
	{
		void* buffer = ANON::moduleAllocator->Allocate(sizeof(ANON::DictionaryImplementation));
		ANON::DictionaryImplementation* x = new (buffer) ANON::DictionaryImplementation();
		Rococo::IDictionarySupervisor * retValue = static_cast<Rococo::IDictionarySupervisor * >(x);
		return retValue;
	}

	ROCOCO_API void AddUnique(IDictionary& d, cstr key, void* data)
	{
		if (!d.TryAddUnique(key, data))
		{
			Throw(0, "Error, could not add key to dictionary: %s\nIt already exists.\n", key);
		}
	}
}
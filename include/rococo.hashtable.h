#pragma once

#include <rococo.strings.h>
#include <unordered_map>

namespace Rococo
{
	using namespace Rococo::Strings;

	class FastStringKey
	{
		HString persistentData;
		cstr data;

		void Persist()
		{
			if (persistentData.length() == 0)
			{
				persistentData = data;
				data = persistentData;
			}
		}

	public:
		FastStringKey(cstr _stackData) : data(_stackData)
		{
		}

		FastStringKey()
		{
			data = nullptr;
		};

		FastStringKey& operator = (const FastStringKey& other)
		{
			data = other.data;
			persistentData = other.persistentData;
			Persist();
			return *this;
		}

		[[nodiscard]] const int length() const
		{
			return StringLength(data);
		}

		FastStringKey(const FastStringKey& other) :
			persistentData(other.persistentData)
		{
			data = other.data;
			Persist();
		};

		FastStringKey(FastStringKey&& other) noexcept:
			persistentData(other.persistentData),
			data(other.data)
		{
			Persist();
		}

		[[nodiscard]] operator cstr() const
		{ 
			return data;
		}

		[[nodiscard]] bool operator == (const FastStringKey& other) const
		{
			return Eq(data, other.data);
		}

		[[nodiscard]] size_t HashCode() const
		{
			return Rococo::Strings::FastHash(data);
		}

		struct Hash
		{
			size_t operator()(const FastStringKey& s) const noexcept
			{
				return s.HashCode();
			}
		};
	};

	template<class VALUE>
	class stringmap
	{
	private:
		typedef FastStringKey TString;
		typedef std::unordered_map<TString, VALUE, TString::Hash> TMap;
		TMap map;

	public:
		typedef typename TMap::iterator iterator;
		typedef typename TMap::const_iterator const_iterator;

		iterator begin() { return map.begin(); }
		iterator end() { return map.end(); }
		const_iterator begin() const { return map.begin(); }
		const_iterator end() const { return map.end(); }

		void clear() { map.clear(); }
		auto erase(iterator i) { return map.erase(i); }
		auto size() const { return map.size(); }
		bool empty() const { return map.empty(); }

		auto& operator[](const TString& key) { return map.operator[](key); }
		auto& operator[](TString&& key) { return map.operator[](key); }

		auto find(cstr key) { return map.find(key); }
		auto find(cstr key) const { return map.find(key); }
		auto insert(const char* key, const VALUE& item)
		{
			return map.insert(std::make_pair(TString(key), item));
		}

		void reserve(size_t nElements) { map.reserve(nElements); }

		stringmap() {}

		stringmap(std::initializer_list<std::pair<cstr,VALUE>> Z) 
		{
			for (auto i : Z)
			{
				insert(i.first, i.second);
			}
		}
	};
}

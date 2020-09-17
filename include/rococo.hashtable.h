#pragma once

#include <rococo.strings.h>
#include <unordered_map>

namespace Rococo
{
	template<class VALUE>
	class stringmap
	{
	private:
		typedef std::unordered_map<StringKey, VALUE, StringKey::Hash> TMap;

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

		auto& operator[](const StringKey& key) { return map.operator[](key); }
		auto& operator[](StringKey&& key) { return map.operator[](key); }

		auto find(cstr key) { return map.find(key); }
		auto find(cstr key) const { return map.find(key); }
		auto insert(const char* key, const VALUE& item)
		{
			return map.insert(std::make_pair(StringKey(key), item));
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

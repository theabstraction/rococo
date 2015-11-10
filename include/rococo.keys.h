#ifndef ROCOCO_TYPES_H
#define ROCOCO_TYPES_H

namespace Rococo
{
	// This class is intended for use as standard container hashable key
	// The purpose is to allow searches without having to create heap objects
	// Instances used for insertion of keys should use true in the constructor.
	// Instances used for searches of keys should use false in the constructor
	class StringKey
	{
	private:
		std::wstring s;
		const wchar_t* constString;

	public:
		StringKey(const wchar_t* _s, bool isPersistent)
		{
			if (_s == nullptr) Throw(0, L"Null pointer passed to PersistentStringKey");

			if (isPersistent)
			{
				s = _s;
				constString = nullptr;
			}
			else
			{
				constString = _s;
			}
		}

		StringKey(const StringKey& src): s(src.s)
		{
			constString = s.c_str();
		}

		StringKey& operator == (const StringKey& src)
		{
			s = src.s;
			constString = s.c_str();
			return *this;
		}

		virtual operator const wchar_t* () const 
		{
			return constString ? constString : s.c_str();
		}
	};

	inline bool operator == (const StringKey& a, const StringKey& b)
	{
		return wcscmp(a, b) == 0;
	}

	struct HashStringKey
	{
		size_t operator()(const StringKey& k) const
		{
			return FastHash(k);
		}
	};
}

#endif
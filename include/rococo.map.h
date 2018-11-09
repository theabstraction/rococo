#pragma once

namespace Rococo
{
	enum EnumControl
	{
		ENUM_CONTINUE,
		ENUM_BREAK,
		ENUM_ERASE_AND_CONTINUE,
		ENUM_ERASE_AND_BREAK
	};

	ROCOCOAPI IDictionaryEnumerator
	{
		 virtual EnumControl OnIteration(cstr key, size_t keyLength, void* buffer) = 0;
	};

	ROCOCOAPI IDictionary
	{
		 virtual bool TryAddUnique(cstr key, void* data) = 0;
		 virtual bool TryDetach(cstr key, void* data) = 0;
		 virtual bool TryFind(cstr key, void*& data) = 0;
		 virtual void Enumerate(IDictionaryEnumerator& enumerator) = 0;
	};

	ROCOCOAPI IDictionarySupervisor : public IDictionary
	{
		virtual void Free() = 0;
	};

	IDictionarySupervisor* CreateDictionaryImplementation();

	class Dictionary : public IDictionary
	{
	private:
		AutoFree<IDictionarySupervisor> imp = CreateDictionaryImplementation();

	public:
		bool TryAddUnique(cstr key, void* data) override
		{
			return imp->TryAddUnique(key, data);
		}

		void Enumerate(IDictionaryEnumerator& enumerator)
		{
			imp->Enumerate(enumerator);
		}

		bool TryDetach(cstr key, void* data) override
		{
			return imp->TryDetach(key, data);
		}

		bool TryFind(cstr key, void*& data) override
		{
			return imp->TryFind(key, data);
		}
	};

	void AddUnique(IDictionary& d, cstr key, void* data);
}

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

	ROCOCO_INTERFACE IDictionaryEnumerator
	{
		 virtual EnumControl OnIteration(cstr key, size_t keyLength, void* buffer) = 0;
	};

	ROCOCO_INTERFACE IDictionary
	{
		 virtual bool TryAddUnique(cstr key, void* data) = 0;
		 virtual bool TryDetach(cstr key, void*& data) = 0;
		 virtual bool TryFind(cstr key, void*& data) = 0;
		 virtual bool TryFind(cstr key, const void*& data) const = 0;
		 virtual void Enumerate(IDictionaryEnumerator& enumerator) = 0;
	};

	ROCOCO_INTERFACE IDictionarySupervisor : public IDictionary
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IDictionarySupervisor* CreateDictionaryImplementation();

	class Dictionary : public IDictionary
	{
	private:
		AutoFree<IDictionarySupervisor> imp;

	public:
		Dictionary(): imp ( CreateDictionaryImplementation() )
		{
		}

		virtual ~Dictionary()
		{

		}

		bool TryAddUnique(cstr key, void* data) override
		{
			return imp->TryAddUnique(key, data);
		}

		void Enumerate(IDictionaryEnumerator& enumerator) override
		{
			imp->Enumerate(enumerator);
		}

		bool TryDetach(cstr key, void*& data) override
		{
			return imp->TryDetach(key, data);
		}

		bool TryFind(cstr key, void*& data) override
		{
			return imp->TryFind(key, data);
		}

		bool TryFind(cstr key, const void*& data) const override
		{
			return imp->TryFind(key, data);
		}
	};

	ROCOCO_API void AddUnique(IDictionary& d, cstr key, void* data);
}

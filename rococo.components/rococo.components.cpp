#include <rococo.api.h>

using namespace Rococo;

namespace Rococo::Components
{
	struct ENTITY_HANDLE
	{
		int64 bitFields = 0;
	};

	struct IEntity;

	ROCOCOAPI IComponent
	{
		virtual IEntity & Owner();
	};

	struct COMPONENT_HASH
	{
		int32 hashCode = 0;
	};

	typedef size_t ComponentIndex;

	template<class INTERFACE> COMPONENT_HASH GetComponentHash()
	{
		static_assert("Not implemented");
	}

	namespace CompileTime
	{
		constexpr int jenkins_one_at_a_time_hash(cstr s)
		{
			int32 hash = 0;
			for (cstr p = s; *p != 0; p++)
			{
				hash += *p;
				hash += (hash << 10);
				hash ^= (hash >> 6);
			}
			hash += (hash << 3);
			hash ^= (hash >> 11);
			hash += (hash << 15);
			return hash;
		}

		// Computes the hash of a component interface at compile time
		constexpr COMPONENT_HASH ComputeComponentHash(const char* interfaceName)
		{
			return { jenkins_one_at_a_time_hash(interfaceName) };
		}
	}

#define REGISTER_COMPONENT_INTERFACE(INTERFACE)                \
	template<> COMPONENT_HASH GetComponentHash<INTERFACE>()   \
	{                                                         \
		return CompileTime::ComputeComponentHash(#INTERFACE); \
	}

	REGISTER_COMPONENT_INTERFACE(IComponent);

	class ROCOCO_NO_VTABLE IEntity
	{
	private:
		// Get the component with specified index, if no component with given index and hashCode then returns nullptr
		virtual IComponent* GetComponentInternal(ComponentIndex index, COMPONENT_HASH hashCode) = 0;
		// Get the number of components with a given hash code
		virtual ComponentIndex GetNumberOfComponents(COMPONENT_HASH hashCode) const = 0;

		virtual void AttachComponent(COMPONENT_HASH hashCode, IComponent* newComponent) = 0;
	public:
		virtual ENTITY_HANDLE Handle() const = 0;

		// Get the component with specified index, if no component with given index and type then returns nullptr
		template<class INTERFACE> INTERFACE* GetComponent(ComponentIndex index)
		{
			return static_cast<INTERFACE>(GetComponentInternal(index, GetComponentHash<INTERFACE>()));
		}

		// Get the component with specified index, if no component with given index and type then returns nullptr
		template<class INTERFACE> INTERFACE* GetComponent(ComponentIndex index)
		{
			return static_cast<INTERFACE>(GetComponentInternal(index, GetComponentHash<INTERFACE>()));
		}

		template<class INTERFACE> INTERFACE* AttachComponent()
		{
			INTERFACE* newComponent = NewComponent<INTERFACE>(*this);
			AttachComponent(GetComponentHash<INTERFACE>(), newComponent);
		}
	};

	template<class OBJECT_CLASS> struct REF_COUNTED_OBJECT
	{
		OBJECT_CLASS& object;

		int64 refCount = 1;
		REF_COUNTED_OBJECT(OBJECT_CLASS& refObject) : object(refObject) {}
	};

	template<class REF_COUNTED_OBJECT> class TSharedRef
	{
	private:
		REF_COUNTED_OBJECT<OBJECT_CLASS>* rcObject;

	public:
		TSharedRef(REF_COUNTED_OBJECT* refObject) : rcObject(refObject)
		{
			if (rcObject) rcObject->refCount++;
		}

		~TSharedRef()
		{
			if (rcObject)
			{
				rcObject->refCount--;
				if (rcObject->refCount == 0)
				{
					auto* memoryBlock = (char*)rcObject;
					rcObject->object.Free();
					delete[] memoryBlock;
				}
			}
		}

	};


	template<class OBJECT_CLASS> TSharedRef<OBJECT_CLASS> ConstructManagedObject()
	{
		IEntity* entity = CreateEntity();
		auto* memoryBlock = new char[sizeof REF_COUNTED_OBJECT + sizeof OBJECT_CLASS];
		auto* pObject = new (memoryBlock + sizeof REF_COUNTED_OBJECT) OBJECT_CLASS(*entity);
		auto* refCountedObject = new (memoryBlock) REF_COUNTED_OBJECT(*pObject);
		return pObject;
	}
}
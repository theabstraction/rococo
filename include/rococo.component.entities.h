#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	typedef uint32 ENTITY_TABLE_ID;
	typedef uint32 ENTITY_SALT;

#pragma pack(push, 1)
	struct EntityIndex
	{
		ENTITY_SALT salt;
		ENTITY_TABLE_ID id;

		typedef uint64 ENTITY_PRIMITIVE;

		ENTITY_PRIMITIVE AsPrimitive() const { return *(int64*)this; }
	};
#pragma pack(pop)

	struct EntityIndexHasher
	{
		size_t operator()(EntityIndex index) const
		{
			return (size_t)index.id;
		}
	};

	inline bool operator == (const EntityIndex& a, const EntityIndex& b)
	{
		return a.AsPrimitive() == b.AsPrimitive();
	}

	struct EntityIndexComparer
	{
		bool operator()(const EntityIndex& a, const EntityIndex& b) const
		{
			return a == b;
		}
	};

	struct IComponentLife
	{
		virtual int64 AddRef() = 0;
		virtual int64 GetRefCount() const = 0;
		virtual int64 ReleaseRef() = 0;

		virtual bool Deprecate() = 0;
		virtual bool IsDeprecated() const = 0;
	};

	// A reference counted object containing references to both an interface and an associated lifetime manager
	template<class INTERFACE> class Ref
	{
	private:
		INTERFACE* component;
		IComponentLife* life;
		
	public:
		Ref(INTERFACE& refComponent, IComponentLife& refLife):
			component(&refComponent),
			life(&refLife)
		{
			life->AddRef();
		}

		Ref():
			component(nullptr),
			life(nullptr)
		{

		}

		Ref(Ref<INTERFACE>& src)
		{
			component = src.component;
			life = src.life;
			if (life)
			{
				life->AddRef();
			}
		}

		Ref(Ref<INTERFACE>&& src)
		{
			component = src.component;
			life = src.life;
			src.component = nullptr;
			src.life = nullptr;
		}

		Ref<INTERFACE>& operator = (Ref<INTERFACE>& src)
		{
			if (component == src.component)
			{
				return *this;
			}

			if (component)
			{
				life->Release();
			}

			component = src.component;
			life = src.life;

			if (life)
			{
				life->AddRef();
			}

			return *this;
		}

		~Ref()
		{
			if (life)
			{
				life->ReleaseRef();
			}
		}

		bool Deprecate()
		{
			if (!life) return false;
			return life->Deprecate();
		}

		INTERFACE* operator -> ()
		{
			return component;
		}

		INTERFACE& operator * ()
		{
			return *component;
		}

		operator bool() const
		{
			return component != nullptr;
		}
	};
}


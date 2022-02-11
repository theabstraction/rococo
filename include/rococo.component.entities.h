#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	typedef uint32 ROID_TABLE_INDEX;
	typedef uint32 ROID_SALT;

#pragma pack(push, 1)
	// Rococo Object ID - a transient object identifier used in the Rococo::Components System.
	struct ROID
	{
		union
		{
			struct
			{
				ROID_SALT salt;
				ROID_TABLE_INDEX index;
			};

			uint64 asUint64 = 0;
		};
	};
#pragma pack(pop)

	inline bool operator == (const ROID& a, const ROID& b)
	{
		return a.asUint64 == b.asUint64;
	}

	struct STDROID
	{
		size_t operator()(const ROID& roid) const noexcept
		{
			return roid.index;
		}

		bool operator()(const ROID& a, const ROID& b) const noexcept
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


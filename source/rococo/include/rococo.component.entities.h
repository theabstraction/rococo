#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	typedef uint32 ROID_TABLE_INDEX;

	struct ROID_SALT
	{
		uint32 cycle : 31;
		uint32 isDeprecated : 1;
	};

	static_assert(sizeof ROID_SALT == sizeof uint32);
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

		inline operator bool() const
		{
			return asUint64 != 0;
		}

		inline auto Value() const
		{
			return asUint64;
		}

		inline bool operator == (const ROID& other) const
		{
			return asUint64 == other.asUint64;
		}

		inline bool operator != (const ROID& other) const
		{
			return asUint64 != other.asUint64;
		}

		ROID(): asUint64(0)
		{

		}

		static ROID Invalid()
		{
			return ROID();
		}

		explicit ROID(uint64 value): asUint64(value)
		{			
		}
	};

	static_assert(sizeof ROID == sizeof uint64);
#pragma pack(pop)

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

		virtual ROID GetRoid() const = 0;
	};

	// A reference counted object containing references to both an interface and an associated lifetime manager
	template<class INTERFACE> class Ref
	{
	private:
		INTERFACE* component;
		IComponentLife* life;
		
	public:
		FORCE_INLINE Ref(INTERFACE& refComponent, IComponentLife& refLife):
			component(&refComponent),
			life(&refLife)
		{
			life->AddRef();
		}

		FORCE_INLINE Ref():
			component(nullptr),
			life(nullptr)
		{

		}

		FORCE_INLINE IComponentLife& Life()
		{
			return *life;
		}

		FORCE_INLINE Ref(Ref<INTERFACE>& src) noexcept
		{
			component = src.component;
			life = src.life;
			if (life)
			{
				life->AddRef();
			}
		}

		FORCE_INLINE Ref(Ref<INTERFACE>&& src) noexcept
		{
			component = src.component;
			life = src.life;
			src.component = nullptr;
			src.life = nullptr;
		}

		FORCE_INLINE Ref<INTERFACE>& operator = (Ref<INTERFACE>& src)
		{
			if (component == src.component)
			{
				return *this;
			}

			if (component)
			{
				life->ReleaseRef();
			}

			component = src.component;
			life = src.life;

			if (life)
			{
				life->AddRef();
			}

			return *this;
		}

		FORCE_INLINE void operator = (Ref<INTERFACE>&& src)
		{
			if (life)
			{
				life->ReleaseRef();
			}

			component = src.component;
			life = src.life;
			src.component = nullptr;
			src.life = nullptr;
		}

		FORCE_INLINE ~Ref()
		{
			if (life)
			{
				life->ReleaseRef();
			}
		}

		FORCE_INLINE bool Deprecate()
		{
			if (!life) return false;
			return life->Deprecate();
		}

		FORCE_INLINE INTERFACE* operator -> ()
		{
			return component;
		}

		FORCE_INLINE INTERFACE& operator * ()
		{
			return *component;
		}

		FORCE_INLINE INTERFACE& GetComponent()
		{
			return *component;
		}

		FORCE_INLINE operator bool() const
		{
			return component != nullptr;
		}

		FORCE_INLINE bool operator == (Ref<INTERFACE>& other) const
		{
			return component == other.component;
		}

		FORCE_INLINE bool operator != (Ref<INTERFACE>& other) const
		{
			return component != other.component;
		}

		FORCE_INLINE int64 GetRefCount() const
		{
			return life == nullptr ? 0 : life->GetRefCount();
		}

		FORCE_INLINE ROID Roid() const
		{
			return life == nullptr ? ROID() : life->GetRoid();
		}
	};

	struct IRCObjectTable;

	ROCOCO_INTERFACE IROIDCallback
	{
		virtual EFlowLogic OnROID(ROID id) = 0;
	};
}

namespace Rococo
{
	template <>
	struct RemoveReference<Rococo::Components::ROID>
	{
		using Type = Rococo::Components::ROID;
	};
}
#pragma once

#include <components/rococo.ecs.roid.h>

namespace Rococo::Components
{
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
		using TInterface = INTERFACE;

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

		FORCE_INLINE void Release()
		{
			if (life)
			{
				life->ReleaseRef();
				life = nullptr;
				component = nullptr;
			}
		}
	};

	struct IRCObjectTable;
}

namespace Rococo
{
	template <>
	struct RemoveReference<Rococo::Components::ROID>
	{
		using Type = Rococo::Components::ROID;
	};
}
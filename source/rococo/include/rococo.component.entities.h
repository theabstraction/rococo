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

		operator bool() const
		{
			return asUint64 != 0;
		}

		auto Value() const
		{
			return asUint64;
		}

		inline bool operator == (const ROID& other) const
		{
			return asUint64 == other.asUint64;
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

		Ref(Ref<INTERFACE>& src) noexcept
		{
			component = src.component;
			life = src.life;
			if (life)
			{
				life->AddRef();
			}
		}

		Ref(Ref<INTERFACE>&& src) noexcept
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

		void operator = (Ref<INTERFACE>&& src)
		{
			component = src.component;
			life = src.life;
			src.component = nullptr;
			src.life = nullptr;
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

		int64 GetRefCount() const
		{
			return life == nullptr ? 0 : life->GetRefCount();
		}

		ROID Roid() const
		{
			return life == nullptr ? ROID() : life->GetRoid();
		}
	};

	struct IRCObjectTable;

	template<class ICOMPONENT>
	ROCOCO_INTERFACE IComponentFactory
	{
		virtual ICOMPONENT * ConstructInPlace(void* pMemory) = 0;
		virtual void Destruct(ICOMPONENT* pInstance) = 0;
		virtual size_t SizeOfConstructedObject() const = 0;
		virtual void Free() = 0;
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
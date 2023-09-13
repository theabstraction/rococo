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

				// unsigned index value
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

		ROID() : asUint64(0)
		{

		}

		static ROID Invalid()
		{
			return ROID();
		}

		explicit ROID(uint64 value) : asUint64(value)
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

	ROCOCO_INTERFACE IROIDCallback
	{
		virtual EFlowLogic OnROID(ROID id) = 0;
	};
}

namespace Rococo
{
	// Deprecated name
	using ID_ENTITY = Rococo::Components::ROID;
}
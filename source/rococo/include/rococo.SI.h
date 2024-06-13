#pragma once

#include <rococo.types.h>

namespace Rococo
{
	struct Kilograms
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE Kilograms operator "" _kg(long double value)
	{
		return Kilograms{ (float)value };
	}

	struct Metres
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE Metres operator "" _metres(long double value)
	{
		return Metres{ (float)value };
	}


	FORCE_INLINE Metres operator "" _metres(unsigned long long value)
	{
		return Metres{ (float)value };
	}

	struct Seconds
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE Seconds operator "" _seconds(long double value)
	{
		return Seconds{ (float)value };
	}

	struct MetresPerSecond
	{
		float value;
		FORCE_INLINE operator float() const { return value; }
	};

	FORCE_INLINE MetresPerSecond operator "" _mps(long double value)
	{
		return MetresPerSecond{ (float)value };
	}
}
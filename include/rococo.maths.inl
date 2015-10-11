#pragma once

#include <rococo.maths.h>

namespace Rococo
{
	void ThrowNumericException(const wchar_t* format)
	{
		struct : IException
		{
			wchar_t msg[256];

			virtual const wchar_t* Message() const { return msg; }
			virtual int32 ErrorCode() const { return 0;  }
		} ex;

		SafeFormat(ex.msg, _TRUNCATE, L"%s", format);

		TripDebugger();

		throw ex;
	}

	float LengthSq(const Vec3& v)
	{
		return v.x*v.x + v.y*v.y + v.z*v.z;
	}

	// Return normalized vector. In the event of a null vector, an exception is thrown
	Vec3 Normalize(const Vec3& v)
	{
		const float epsilon = 0.0000001f;
		float l = LengthSq(v);
		if (l <= epsilon)
		{
			ThrowNumericException(L"Vec3 Normalize(const Vec3& v) failed: the argument was a null vecctor");
		}

		float f = 1.0f / sqrtf(l);
		return v * f;
	}

	bool TryNormalize(const Vec3& v, Vec3& nv)
	{
		const float epsilon = 0.0000001f;
		float l = LengthSq(v);
		if (l <= epsilon)
		{
			return false;
		}

		float f = 1.0f / sqrtf(l);
		nv = v * f;
		return true;
	}
}
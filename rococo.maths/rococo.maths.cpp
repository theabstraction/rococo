#include <rococo.types.h>
#include <rococo.maths.h>
#include <rococo.maths.inl>

#include <wchar.h>

#include <math.h>

namespace Rococo
{
	bool TryGetRealRoots(float& x0, float& x1, float a /* x^2 */, float b /* x */, float c)
	{
		// Solve quadratic equation ax^2 + bx + c = 0 using formula
		// x =  -b/2a (+-) sqrt( b^2 - 4ac)/2a

		if (a == 0.0f)
		{
			// Degenerate case -> One root
			x0 = x1 = -c / b;
			return true;
		}
		else
		{
			float d = b*b - 4.0f*a*c;
			if (d < 0)
			{
				// Complex roots
				x0 = x1 = 0.0f;
				return false;
			}

			float e = -0.5f * b;
			float f = sqrtf(d);

			x0 = e + 0.5f * f;
			x1 = e - 0.5f * f;

			float ooa = 1.0f / a;
			x0 *= ooa;
			x1 *= ooa;

			return true;
		}
	}

	void swap(float& a, float &b)
	{
		float c = b;
		b = a;
		a = c;
	}

	bool TryGetIntersectionLineAndSphere(float& t0, float& t1, const Vec3& start, const Vec3& end, const Sphere& sphere)
	{
		// P(t) = start + (end - start).t for every point on the line containing start and end = a + (b-a).t
		// if P is on sphere, then (P - centre).(P - centre) = R^2
		// Thus at intersect
		// ((a-centre) + (b-a).t).(a-centre + (b-a).t) = R^2
		// (b-a).(b-a).t^2 + 2(a-centre)*(b-a)t + (a-centre)*(a-centre) - R^2 = 0
		// Yielding two intersect points, the roots of the quadratic

		Vec3 atoc = start - sphere.centre;

		if (TryGetRealRoots(t0, t1, LengthSq(end - start), 2.0f * atoc * (end - start), LengthSq(atoc) - sphere.radius*sphere.radius))
		{
			if (t0 > t1)
			{
				// Swap to 
				swap(t0, t1);
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	void ThrowNumericException(const wchar_t* format)
	{
		struct : IException
		{
			wchar_t msg[256];

			virtual const wchar_t* Message() const { return msg; }
			virtual int32 ErrorCode() const { return 0; }
		} ex;

		SafeFormat(ex.msg, _TRUNCATE, L"%s", format);

		TripDebugger();

		throw ex;
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

	float Length(const Vec3& v)
	{
		float l2 = LengthSq(v);
		return sqrtf(l2);
	}
}
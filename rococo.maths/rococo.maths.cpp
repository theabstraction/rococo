#include <rococo.types.h>
#include <rococo.maths.h>

#include <wchar.h>
#include <stdlib.h>

#include <DirectXMath.h>

namespace // globals
{
	using namespace Rococo;

	const Matrix4x4 const_identity4x4
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};

	const Matrix4x4 const_null4x4
	{
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};
}

namespace Rococo
{
	const Matrix4x4& Matrix4x4::Identity()
	{
		return const_identity4x4;
	}

	const Matrix4x4& Matrix4x4::Null()
	{
		return const_null4x4;
	}

	Matrix4x4 Matrix4x4::Translate(cr_vec3 v)
	{
		return Matrix4x4{
			{1.0f, 0,      0,   v.x},
			{0,    1.0f,   0,   v.y },
			{0,    0,   1.0f,   v.z },
			{0,    0,      0,  1.0f }
		};
	}

	Matrix4x4 Matrix4x4::Scale(float Sx, float Sy, float Sz)
	{
		return Matrix4x4{
			{ Sx,    0,    0,     0},
			{ 0,    Sy,    0,     0},
			{ 0,     0,   Sz,     0},
			{ 0,     0,    0,  1.0f}
		};
	}

	Matrix4x4 Matrix4x4::RotateRHAnticlockwiseX(Radians phi)
	{
		float s = Sin(phi);
		float c = Cos(phi);

		return Matrix4x4{
			{ 1.0f, 0,     0,    0 },
			{ 0,    c,    -s,    0 },
			{ 0,    s,     c,    0 },
			{ 0,    0,     0,  1.0f }
		};
	}

	Matrix4x4 Matrix4x4::RotateRHAnticlockwiseZ(Radians theta)
	{
		float s = Sin(theta);
		float c = Cos(theta);

		return Matrix4x4{
			{ c,   -s,     0,        0 },
			{ s,    c,     0,        0 },
			{ 0,    0,     1.0f,     0 },
			{ 0,    0,     0,     1.0f }
		};
	}

	void GetIsometricTransforms(Matrix4x4& worldMatrix, Matrix4x4& inverseWorldMatrixProj, Matrix4x4& worldMatrixAndProj, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta, Metres cameraHeight)
	{
		using namespace DirectX;

		Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(phi);
		Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(viewTheta);
		Matrix4x4 centreToOrigin = Matrix4x4::Translate(-1.0f * centre);
		Matrix4x4 Sxyz = Matrix4x4::Scale(scale * aspectRatio, scale, -scale);

		Matrix4x4 verticalShift = Matrix4x4::Translate(Vec3{ 0, 0, cameraHeight });

		worldMatrix = verticalShift * Sxyz * Rx * Rz * centreToOrigin;

		XMMATRIX xortho = XMMatrixOrthographicLH(2.0f, 2.0f, 0.0f, 1000.0f);
		XMMatrixTranspose(xortho);

		Matrix4x4 ortho;
		XMStoreFloat4x4(ortho, xortho);

		worldMatrixAndProj = ortho * worldMatrix;

		inverseWorldMatrixProj = InvertMatrix(worldMatrixAndProj);
	}

	void TransposeMatrix(const Matrix4x4& matrix, Matrix4x4& transposeOfMatrix)
	{
		using namespace DirectX;
		XMMATRIX t = XMLoadFloat4x4((DirectX::XMFLOAT4X4*) &matrix);
		XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &transposeOfMatrix, XMMatrixTranspose(t));
	}

	void InvertMatrix(const Matrix4x4& matrix, Matrix4x4& inverseMatrix)
	{
		using namespace DirectX;
		XMMATRIX m = XMLoadFloat4x4((DirectX::XMFLOAT4X4*) &matrix);
		XMVECTOR det;
		XMMATRIX im = XMMatrixInverse(&det, m);
		XMStoreFloat4x4((DirectX::XMFLOAT4X4*) &inverseMatrix, im);
	}

	Matrix4x4 InvertMatrix(const Matrix4x4& matrix)
	{
		Matrix4x4 invMatrix;
		InvertMatrix(matrix, invMatrix);
		return invMatrix;
	}

	Matrix4x4 TransposeMatrix(const Matrix4x4& matrix)
	{
		Matrix4x4 tMatrix;
		TransposeMatrix(matrix, tMatrix);
		return tMatrix;
	}

	float Dot(const Vec4& a, const Vec4& b)
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	void Multiply(Matrix4x4& product, const Matrix4x4& Ra, const Matrix4x4& Rb)
	{
		using namespace DirectX;
		auto XRa = XMLoadFloat4x4(Ra);
		auto XRb = XMLoadFloat4x4(Rb);
		auto XRaRb = XMMatrixMultiply(XRa, XRb);

		XMStoreFloat4x4(product, XRaRb);
	}

	Matrix4x4 operator * (const Matrix4x4& a, const Matrix4x4& b)
	{
		Matrix4x4 result;
		Multiply(result, a, b);
		return result;
	}

	Vec4 operator * (const Matrix4x4& R, const Vec4& v)
	{
		return Vec4{
			Dot(R.row0, v),
			Dot(R.row1, v),
			Dot(R.row2, v),
			Dot(R.row3, v),
		};
	}

	Vec4 operator * (const Vec4& v, const Matrix4x4& R)
	{
		using namespace DirectX;
		auto XR = XMLoadFloat4x4(R);
		auto XV = XMLoadFloat4(v);

		auto vprimed = XMVector4Transform(XV, XR);

		Vec4 u;
		XMStoreFloat4(u, vprimed);
		return u;
	}

	bool TryGetRealRoots(float& x0, float& x1, float a /* x^2 */, float b /* x */, float c)
	{
		// Solve quadratic equation ax^2 + bx + c = 0 using formula
		// x =  -b/2a (+-) sqrt( b^2 - 4ac)/2a

		if (a == 0.0f)
		{
			if (b == 0.0f)
			{
				x0 = x1 = 0.0f;
				return false;
			}
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

	bool TryGetIntersectionLineAndSphere(float& t0, float& t1, cr_vec3 start, cr_vec3 end, const Sphere& sphere)
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
	Vec3 Normalize(cr_vec3 v)
	{
		const float epsilon = 0.0000001f;
		float l = LengthSq(v);
		if (l <= epsilon)
		{
			ThrowNumericException(L"Vec3 Normalize(cr_vec3 v) failed: the argument was a null vecctor");
		}

		float f = 1.0f / sqrtf(l);
		return v * f;
	}

	bool TryNormalize(cr_vec3 v, Vec3& nv)
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

	float Length(cr_vec3 v)
	{
		float l2 = LengthSq(v);
		return sqrtf(l2);
	}

	// Returns elevation at which to fire projectile to hit a target. Returns negative if no sensible value
	// If the target cannot be made accurate to factor less than than the largestError value, then the query is not sensible
	Radians ComputeWeaponElevation(cr_vec3 origin, cr_vec3 target, float projectileSpeed, Degrees maxElevation, Gravity g, Metres largestError)
	{
		if (projectileSpeed <= 0 || maxElevation <= 5.0f)
		{
			return Radians{ -1.0f };
		}

		// Cap to something sensible
		if (maxElevation > 85.0f)
		{
			maxElevation = 85.0_degrees;
		}

		float S = Length(target - origin);

		const float pointBlank = 0.5f;

		if (S < pointBlank)
		{
			// At point blank, assume we hit target whatever elevation
			return Radians{ 0.0f };
		}

		float Z = target.z - origin.z;

		// Vz vertical component of projectile velocity is speed.sin(theta), where theta is angle of elevation
		// Vs horizontal component of projectile velocity is speed.cos(theta)

		// flight time t = S / Vs
		// In t change of height of projectile is Dh = Vz.t + 0.5gt^2
		// Target Dh is Z

		// 0.5g t ^ 2 + Vz.t - Z = 0..................(1)
		// t = S/Vs...................................(2)

		// 0.5gS^2/(speed^2.cos^2(theta)) + speed.sin(theta).S / (speed.cos(theta)) - DZ = 0....(3)

		// Let a = 0.5gS^2/speed^2

		// a / cos^2(theta) + S.tan(theta) = Z

		float maxTheta{ maxElevation * 3.14159f / 180.0f };
		float minTheta{ 0.0f };
		float deltaTheta = (maxTheta - minTheta) * 0.05f; // This gives 20 estimates

		float a = 0.5f * g * Square(S) / Square(projectileSpeed);
		
		float bestTheta = minTheta;
		float bestError = 1.0e30f;

		for (float theta = minTheta; theta < maxTheta; theta += deltaTheta)
		{
			float z = a / Square(cosf(theta)) + S * tanf(theta);

			float error = fabsf(z - Z);

			if (error < bestError)
			{
				bestTheta = theta;
				bestError = error;
			}
		}

		if (largestError > bestError)
		{
			return Radians{ bestTheta };
		}
		else
		{
			return Radians{ -1.0f };
		}
	}

	float GenRandomFloat(float minValue, float maxValue)
	{
		static constexpr float ooMax = 1.0f / (float)RAND_MAX;

		float q = rand() * ooMax;
		return minValue + q * (maxValue - minValue);
	}

	Vec2i TopCentre(const GuiRect& rect)
	{
		return Vec2i{ (rect.left + rect.right) >> 1, rect.top };
	}

	bool IsPointInRect(const Vec2i& p, const GuiRect& rect)
	{
		return (p.x > rect.left && p.x < rect.right && p.y > rect.top && p.y < rect.bottom);
	}
}
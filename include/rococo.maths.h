#ifndef ROCOCO_MATHS_H
#define ROCOCO_MATHS_H

#ifndef Rococo_TYPES_H
# error "#include <rococo.types.h> before including this file"
#endif

namespace Rococo
{
	inline Vec2 operator - (const Vec2& a, const Vec2& b) { return Vec2{ a.x - b.x, a.y - b.y }; }
	inline Vec2 operator + (const Vec2& a, const Vec2& b) { return Vec2{ a.x + b.x, a.y + b.y }; }
	inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
	inline float LengthSq(const Vec2& a) { return Dot(a, a); }
	inline Vec2 operator / (const Vec2& numerator, float denominator) { float q = 1.0f / denominator; return Vec2{ numerator.x * q, numerator.y * q }; }
	inline Vec2 operator * (const Vec2& q, float f) { return Vec2{ q.x * f, q.y * f }; }
	inline Vec2 operator * (float f, const Vec2& q) { return Vec2{ q.x * f, q.y * f }; }
	inline Vec2 operator += (Vec2& a, const Vec2& b) { a.x += b.x; a.y += b.y; return a; }
	inline Vec2 operator -= (Vec2& a, const Vec2& b) { a.x -= b.x; a.y -= b.y; return a; }

	inline bool operator == (const Vec2i& a, const Vec2i& b) { return a.x == b.x && a.y == b.y; }
	inline bool operator != (const Vec2i& a, const Vec2i& b) { return !(a == b); }

	inline Vec3 operator + (cr_vec3 a, cr_vec3 b) { return Vec3{ a.x + b.x, a.y + b.y, a.z + b.z }; }
	inline Vec3 operator * (cr_vec3 q, float f) { return Vec3{ q.x * f, q.y * f, q.z * f }; }
	inline Vec3 operator * (float f, cr_vec3 q) { return Vec3{ q.x * f, q.y * f, q.z * f }; }
	inline Vec3 operator - (cr_vec3 a, cr_vec3 b) { return Vec3{ a.x - b.x, a.y - b.y, a.z - b.z }; }

	inline float Dot(cr_vec3 a, cr_vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	inline float operator * (cr_vec3 a, cr_vec3 b) { return Dot(a, b); }

	inline Vec2i Quantize(const Vec2& v)
	{
		return Vec2i{ (int32)v.x, (int32)v.y };
	}

	inline Vec2 Dequantize(const Vec2i& v)
	{
		return Vec2{ (float)v.x, (float)v.y };
	}

	inline Quad Dequantize(const GuiRect& v)
	{
		return Quad((float)v.left, (float)v.top, (float) v.right, (float) v.bottom);
	}

	inline Vec2 operator *= (Vec2& p, float scale)
	{
		p.x *= scale;
		p.y *= scale;
		return p;
	}

	inline Vec2i operator - (const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x - b.x, a.y - b.y };
	}

	inline Vec2i operator + (const Vec2i& a, const Vec2i& b)
	{
		return Vec2i{ a.x + b.x, a.y + b.y };
	}

	inline GuiRect Expand(const GuiRect& rect, int32 pixels)
	{
		return GuiRect(rect.left - pixels, rect.top - pixels, rect.right + pixels, rect.bottom + pixels);
	}

	bool TryGetRealRoots(float& x0, float& x1, float a /* x^2 */, float b /* x */, float c);
	bool TryGetIntersectionLineAndSphere(float& t0, float& t1, cr_vec3 start, cr_vec3 end, const Sphere& sphere);

	inline GuiRect operator + (const GuiRect& rect, const Vec2i& delta)
	{
		return GuiRect(rect.left + delta.x, rect.top + delta.y, rect.right + delta.x, rect.bottom + delta.y);
	}

	inline Vec2 Centre(const Quad& q) { return Vec2{ (q.left + q.right) * 0.5f, (q.top + q.bottom) * 0.5f }; }
	inline float Width(const Quad& q) { return q.right - q.left; }
	inline float Height(const Quad& q) { return q.bottom - q.top; }
	inline Vec2 Span(const Quad& q) { return Vec2{ Width(q), Height(q) }; }

	inline Vec2 TopLeft(const Quad& q) { return Vec2{ q.left, q.top }; }
	inline Vec2 BottomRight(const Quad& q) { return Vec2{ q.right, q.bottom }; }
	inline Vec2 TopRight(const Quad& q) { return Vec2{ q.right, q.top }; }
	inline Vec2 BottomLeft(const Quad& q) { return Vec2{ q.left, q.bottom }; }

	inline Vec2i Centre(const GuiRect& q) { return Vec2i{ (q.left + q.right) >> 1, (q.top + q.bottom) >> 1 }; }
	inline int32 Width(const GuiRect& q) { return q.right - q.left; }
	inline int32 Height(const GuiRect& q) { return q.bottom - q.top; }
	inline Vec2i Span(const GuiRect& q) { return Vec2i{ Width(q), Height(q) }; }

	inline Vec2i TopLeft(const GuiRect& q) { return Vec2i{ q.left, q.top }; }
	inline Vec2i BottomRight(const GuiRect& q) { return Vec2i{ q.right, q.bottom }; }
	inline Vec2i TopRight(const GuiRect& q) { return Vec2i{ q.right, q.top }; }
	inline Vec2i BottomLeft(const GuiRect& q) { return Vec2i{ q.left, q.bottom }; }

	inline float LengthSq(cr_vec3 v) { return v * v; }
	inline float Square(float x) { return x * x; }
	float Length(cr_vec3 v);
	void swap(float& a, float &b);
	Vec3 Normalize(cr_vec3 v);
	bool TryNormalize(cr_vec3 v, Vec3& nv);

	Radians ComputeWeaponElevation(cr_vec3 origin, cr_vec3 target, float projectileSpeed, Degrees maxElevation, Gravity g, Metres largestError);

	float GenRandomFloat(float minValue, float maxValue);

	void GetIsometricWorldMatrix(Matrix4x4& worldMatrix, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta);
	void InverseMatrix(const Matrix4x4& matrix, Matrix4x4& inverseMatrix);

	Vec4 Transform(const Matrix4x4& matrix, const Vec4& p);
}

#endif // ROCOCO_MATHS_H
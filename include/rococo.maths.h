#ifndef ROCOCO_MATHS_H
#define ROCOCO_MATHS_H

#ifndef Rococo_TYPES_H
# error "#include <rococo.types.h> before including this file"
#endif

#include <intrin.h>
#include <math.h>

namespace Rococo
{
	struct alignas(4) Vec4 // was 16, but sexy does not align on 16 byte boundaries yet
	{
		float x;
		float y;
		float z;
		float w;

		inline static Vec4 FromVec3(cr_vec3& v, float w)
		{
			return{ v.x, v.y, v.z, w };
		}

		inline operator DirectX::XMFLOAT4* () { return reinterpret_cast<DirectX::XMFLOAT4*> (this); }
		inline operator const DirectX::XMFLOAT4* () const { return reinterpret_cast<const DirectX::XMFLOAT4*> (this); }

		inline operator const Vec3& () const { return *reinterpret_cast<const Vec3*> (this); }
		inline operator Vec3& () { return *reinterpret_cast<Vec3*> (this); }
	};

	struct alignas(4) Quat
	{
		Vec3 v;
		float s;

		Quat() : v{ 0,0,0 }, s(1.0f) {}
		Quat(cr_vec3 _v, float _s) :  v(_v), s(_s) {}

		inline operator DirectX::XMFLOAT4* () { return reinterpret_cast<DirectX::XMFLOAT4*> (this); }
		inline operator const DirectX::XMFLOAT4* () const { return reinterpret_cast<const DirectX::XMFLOAT4*> (this); }
	};

	typedef const Quat& cr_quat;

	Quat InterpolateRotations(cr_quat a, cr_quat b, float t);

	struct alignas(4) Matrix4x4 // was 16, but sexy does not align on 16 byte boundaries yet
	{
		Vec4 row0;
		Vec4 row1;
		Vec4 row2;
		Vec4 row3;

		inline operator DirectX::XMFLOAT4X4* () { return reinterpret_cast<DirectX::XMFLOAT4X4*> (this); }
		inline operator const DirectX::XMFLOAT4X4* () const { return reinterpret_cast<const DirectX::XMFLOAT4X4*> (this); }

		static const Matrix4x4& Identity();
		static const Matrix4x4& Null();
		static Matrix4x4 Translate(cr_vec3 v);
		static Matrix4x4 Scale(float Sx, float Sy, float Sz);

		// Given a right handed co-ordinate system, with X towards the observer, phi gives the rotation in radians anticlockwise about the axis
		// The matrx rotates vectors by pre multiplying the vector: MP -> P'
		static Matrix4x4 RotateRHAnticlockwiseX(Radians phi);

		// Given a right handed co-ordinate system, with Z towards the observer, phi gives the rotation in radians anticlockwise about the axis
		// The matrx rotates vectors by pre multiplying the vector: MP -> P'
		static Matrix4x4 RotateRHAnticlockwiseZ(Radians theta);

		Vec3 GetPosition() const
		{
			return Vec3{ row0.w, row1.w, row2.w };
		}

		void SetPosition(cr_vec3 pos)
		{
			row0.w = pos.x;
			row1.w = pos.y;
			row2.w = pos.z;
		}

		static void FromQuat(const Quat& quat, Matrix4x4& m);
	};

	// Multiply matrix Ra x Rb to make RaRb. This has the property that Ra X Rb x v = (Ra x Rb) x v = Ra x (Rb x v)
	void Multiply(Matrix4x4& product, const Matrix4x4& Ra, const Matrix4x4& Rb);

	Matrix4x4 operator * (const Matrix4x4& a, const Matrix4x4& b);
	Vec4 operator * (const Vec4& v, const Matrix4x4& R);
	Vec4 operator * (const Matrix4x4& R, const Vec4& v);

	inline constexpr float PI() { return 3.14159265358979323846f; }

	inline constexpr float DEGREES_TO_RADIANS_QUOTIENT()
	{
		return PI() / 180.0f;
	}

	inline constexpr float RADIANS_TO_DEGREES_QUOTIENT()
	{
		return 180.0f / PI();
	}

	struct Radians
	{
		float quantity;
		operator float() const { return quantity; }
		operator Degrees () const; 
	};

	inline Radians operator - (Radians r)
	{
		return Radians{ -(float)r };
	}

	struct Degrees
	{
		float quantity;
		operator float() const { return quantity; }

		operator Radians () const { return Radians{ DEGREES_TO_RADIANS_QUOTIENT() * quantity }; }
		Radians ToRadians() const { return Radians{ DEGREES_TO_RADIANS_QUOTIENT() * quantity }; }
	};

	inline Radians::operator Degrees () const { return Degrees{ quantity * RADIANS_TO_DEGREES_QUOTIENT() }; }

	inline Degrees operator "" _degrees(long double literalValue)
	{
		return Degrees{ (float) literalValue };
	}

	inline float Sin(Radians radians) { return sinf(radians.quantity); }
	inline float Cos(Radians radians) { return cosf(radians.quantity); }

	struct Gravity
	{
		float g; // generally negative, and in metres per second per second
		operator float() const { return g; }
	};

	inline Vec2 operator - (const Vec2& a, const Vec2& b) { return Vec2{ a.x - b.x, a.y - b.y }; }
	inline Vec2 operator + (const Vec2& a, const Vec2& b) { return Vec2{ a.x + b.x, a.y + b.y }; }
	inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
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
	inline bool operator == (const Vec3& a, const Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
	inline bool operator != (const Vec3& a, const Vec3& b) { return !(a == b); }

	inline float Dot(cr_vec3 a, cr_vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	inline float operator * (cr_vec3 a, cr_vec3 b) { return Dot(a, b); }

	inline Vec3 Cross(cr_vec3 a, cr_vec3 b)
	{
		// |i   j  k|
		// |Ax Ay Az|
		// |Bx By Bz|
		return Vec3
		{
			a.y*b.z - a.z*b.y,
			b.x*a.z - a.x*b.z,
			a.x*b.y - a.y*b.x
		};
	}

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
	inline float Cube(float x) { return x * x * x; }
	inline bool IsInRange(cr_vec3 v, const Metres range) { return LengthSq(v) < Square(range); }
	float Length(cr_vec3 v);
	void swap(float& a, float &b);
	Vec3 Normalize(cr_vec3 v);
	bool TryNormalize(cr_vec3 v, Vec3& nv);

	inline float LengthSq(Vec2 v) { return Square(v.x) + Square(v.y); }
	inline float Length(Vec2 v) { return sqrtf(LengthSq(v)); }

	Radians ComputeWeaponElevation(cr_vec3 origin, cr_vec3 target, float projectileSpeed, Degrees maxElevation, Gravity g, Metres largestError);

	float GenRandomFloat(float minValue, float maxValue);

	// Create matrices for isometric rendering. The origin is relocated to the world centre, the world rotated by elevation and heading, and then scaled up
	// The camera is vertically above the scene, with y up, x right and looking directly downwards
	// heading is the angle that the world is rotated anticlockwise
	void GetIsometricTransforms(Matrix4x4& worldMatrix, Matrix4x4& inverseWorldMatrixProj, Matrix4x4& worldMatrixAndProj, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta, Metres cameraHeight);

	void InvertMatrix(const Matrix4x4& matrix, Matrix4x4& inverseMatrix);
	void TransposeMatrix(const Matrix4x4& matrix, Matrix4x4& transposeOfMatrix);

	Matrix4x4 InvertMatrix(const Matrix4x4& matrix);
	Matrix4x4 TransposeMatrix(const Matrix4x4& matrix);

	inline Degrees operator - (Degrees theta) { return Degrees{ -theta.quantity }; }

	Vec2i TopCentre(const GuiRect& rect);
	bool IsPointInRect(const Vec2i& p, const GuiRect& rect);

	struct QuadStats
	{
		size_t quadsAllocated;
		size_t quadsFree;
		size_t nodesAllocated;
		size_t nodesFree;
		size_t quadAllocSize;
		size_t nodeAllocSize;
	};

	void TransformPositions(const Vec3* vertices, size_t nElements, cr_m4x4 transform, Vec3* transformedVertices);
	void TransformNormals(const Vec3* vertices, size_t nElements, cr_m4x4 transform, Vec3* transformedVertices);

	ROCOCOAPI IObjectEnumerator
	{
		virtual void OnId(uint64 id) = 0;
	};

	ROCOCOAPI IQuadTree
	{
		virtual void AddEntity(const Sphere& boundingSphere, uint64 id) = 0;
		virtual void Clear() = 0;
		virtual void DeleteEntity(const Sphere& boundingSphere, uint64 id) = 0;
		virtual void EnumerateItems(const Sphere& boundingSphere, IObjectEnumerator& cb) = 0;
		virtual void GetStats(QuadStats& stats) = 0;
		
	};

	ROCOCOAPI IQuadTreeSupervisor : public IQuadTree
	{
		virtual void Free() = 0;
	};

	IQuadTreeSupervisor* CreateLooseQuadTree(float width, float minBoundingRadius);

	struct Edge
	{
		Vec3 a;
		Vec3 b;
	};

	inline Vec2 Normalize(Vec2 v)
	{
		return v / Length(v);
	}

	template<class T> ROCOCOAPI IEnumerator
	{
		virtual void operator()(const T& t) = 0;
	};

	struct Plane
	{
		Vec4 normal;
		Vec4 pointInPlane;
	};

	struct ParallelPlanes
	{
		Plane P0;
		Plane P1;
	};

	struct Quadrilateral
	{
		enum
		{
			VERTEX_INDEX_SW,
			VERTEX_INDEX_SE,
			VERTEX_INDEX_NW,
			VERTEX_INDEX_NE
		};

		union
		{
			Vec3 vertices[4];

			struct
			{
				Vec3 sw;
				Vec3 se;
				Vec3 nw;
				Vec3 ne;
			} v;
		};
	};

	struct BoundingCube
	{
		union BoundingPlanes
		{
			// The bounding planes of the bounding cube all have normals that point outward.
			// The points in the plane are the centres of the cube surfaces to which the plane refers
			Plane planes[6];

			struct
			{
				ParallelPlanes westEest;
				ParallelPlanes southNorth;
				ParallelPlanes bottomTop;
			};

			ParallelPlanes parallelPlanes[3];
		} P;

		Quadrilateral topVertices;
		Quadrilateral bottomVertices;
	};

	void ForEachEdge(const Quadrilateral& q, IEnumerator<Edge>& processEdges);
	void ForEachEdge(const BoundingCube& cube, IEnumerator<Edge>& processEdges);

	enum ContactType
	{
		ContactType_None,
		ContactType_Penetration,
		ContactType_Face,
		ContactType_Edge,
		ContactType_Vertex
	};

	struct Collision
	{
		Vec3 touchPoint;
		float t;
		ContactType contactType;
		bool isDegenerate;
	};

	Collision NoCollision();

	Collision CollideBoundingBoxAndSphere(const BoundingCube& cube, const Sphere& sphere, cr_vec3 target);
	Collision CollideEdgeAndSphere(const Edge& edge, const Sphere& sphere, cr_vec3 target);
	Collision CollideVertexAndSphere(cr_vec3 v, const Sphere& sphere, cr_vec3 target);

	template<class T> T Lerp(const T& a, const T& b, float t) { return a + (b - a) * t; }
}

#endif // ROCOCO_MATHS_H
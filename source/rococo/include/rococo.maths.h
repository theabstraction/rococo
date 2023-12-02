#ifndef ROCOCO_MATHS_H
#define ROCOCO_MATHS_H

#include <rococo.types.h>

#ifdef _WIN32
# include <intrin.h>
#endif

#include <math.h>

namespace DirectX
{
   struct XMFLOAT4;
   struct XMFLOAT4X4;
}

namespace Rococo
{
	inline void swap(float& a, float& b)
	{
		float temp = a;
		a = b;
		b = temp;
	}

	void ComputeBoneQuatFromAngles(Quat& quat, const BoneAngles& angles);

	struct Ray
	{
		Vec3 eye;
		Vec3 dir;
	};

	namespace FPS
	{
		// Compute the FPS world-to-camera transform given the camera is located and oriented in map-space (x = East, y = North and z = Up)
		void SetWorldToCameraTransformToFPSRHMapSystem(OUT Matrix4x4& worldToCameraTransform, const FPSAngles& cameraOrientation, cr_vec3 cameraPosition);
	}

	namespace Rays
	{
		// Compute the pixel co-oordinate of an intersect  of the camera with a ray in the world-space frame of reference
		[[nodiscard]] bool TryGet3DRayIntersectWithScreen(const Ray& worldRay, cr_m4x4 worldToCameraTransform, cr_m4x4 projectionTransform, Vec2i screenSpan, OUT Vec2i& screenDeltaFromTopLeft);

		[[nodiscard]] bool TryGetIntersectWithZPlaneAtRay(float planeZ, float minZcomponentOfDir, const Ray& ray, OUT Vec2& target);		

		// Evaluate a ray that passes from the camera eye through a screen pixel and into the world. The world space ray is an output parameter.
		[[nodiscard]] bool TryGetRayExtendingFromCamera(Vec2i pixelPositionFromTopLeft, cr_m4x4 worldToCameraTransform, cr_m4x4 cameraToScreenProjection, Vec2i screenSpan, OUT Ray& rayFromCameraInWorldSpace);
	}

	template<class T>
	ROCOCO_INTERFACE IRingManipulator
	{
		[[nodiscard]] virtual size_t ElementCount() const = 0;
		[[nodiscard]] virtual T operator[](size_t index) const = 0;
		[[nodiscard]] virtual bool IsEmpty() const = 0;
		[[nodiscard]] virtual void Erase(size_t index) = 0;
		[[nodiscard]] virtual T* Array() = 0;
	};

	template<class T>
	ROCOCO_INTERFACE IRing
	{
		[[nodiscard]] virtual size_t ElementCount() const = 0;
		[[nodiscard]] virtual T operator[](size_t index) const = 0;
		[[nodiscard]] virtual bool IsEmpty() const = 0;
		[[nodiscard]] virtual const T* Array() const = 0;
	};

	enum LineClassify
	{
		LineClassify_Left = 1,
		LineClassify_Right = 2,
		LineClassify_OnLine = 3,
	};

	[[nodiscard]] inline Vec2 operator - (Vec2 src)
	{
		return { -src.x, -src.y };
	}

	[[nodiscard]] LineClassify ClassifyPtAgainstPlane(Vec2 a, Vec2 b, Vec2 p);

	[[nodiscard]] bool IsClockwiseSequential(IRing<Vec2>& ring);

	[[nodiscard]] Quat InterpolateRotations(cr_quat a, cr_quat b, float t);

	[[nodiscard]] bool GetLineIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d, float& t, float& u);

	// Determine whether two parallel line segments are on the same line and touch
	[[nodiscard]] bool DoParallelLinesIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d);

	struct ConeCheck
	{
		float t;
	};

	[[nodiscard]] ConeCheck GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(cr_vec3 eye, cr_vec3 dir, Radians coneAngle, cr_vec3 pos);
	[[nodiscard]] bool IsOutsideCone(cr_vec3 eye, cr_vec3 dir, Radians coneAngle, const Sphere& sphere);

	struct IntersectCounts
	{
		int32 forwardCount;
		int32 backwardCount;
		int32 edgeCases;
		int32 coincidence;
	};

	// Count the number of times a vector crosses a permeter, used to determine whether a surface faces inside or outside a sector
	[[nodiscard]] IntersectCounts CountLineIntersects(Vec2 origin, Vec2 direction, Vec2 p, Vec2 q);

	struct alignas(4) Matrix2x2
	{
		Vec2 row0;
		Vec2 row1;

		[[nodiscard]] static const Matrix2x2 Identity();
		[[nodiscard]] static const Matrix2x2 Null();

		[[nodiscard]] static const Matrix2x2 RotateAnticlockwise(Radians phi);
	};

	[[nodiscard]] inline Vec2 operator * (const Matrix2x2& m, const Vec2& v)
	{
		return Vec2{ (m.row0.x * v.x) + (m.row0.y * v.y), (m.row1.x * v.x) + (m.row1.y * v.y) };
	}

	// Generally in the Rococo libs code/assume that matrices are used to pre-multiply column vectors
	// If this is not the case, then make sure you comment/document that matrices go against the convention
	struct alignas(4) Matrix4x4
	{
		Vec4 row0;
		Vec4 row1;
		Vec4 row2;
		Vec4 row3;

		[[nodiscard]] inline operator DirectX::XMFLOAT4X4* () { return reinterpret_cast<DirectX::XMFLOAT4X4*> (this); }
		[[nodiscard]] inline operator const DirectX::XMFLOAT4X4* () const { return reinterpret_cast<const DirectX::XMFLOAT4X4*> (this); }

		[[nodiscard]] static const Matrix4x4& Identity();
		[[nodiscard]] static const Matrix4x4& Null();
		[[nodiscard]] static Matrix4x4 Translate(cr_vec3 v);
		[[nodiscard]] static Matrix4x4 Scale(float Sx, float Sy, float Sz);

		// Given a right handed co-ordinate system, with X towards the observer, phi gives the rotation in radians anticlockwise about the axis
		// The matrx rotates vectors by pre multiplying the vector: MP -> P'
		[[nodiscard]] static Matrix4x4 RotateRHAnticlockwiseX(Radians phi);

		// Given a right handed co-ordinate system, with Z towards the observer, phi gives the rotation in radians anticlockwise about the axis
		// The matrx rotates vectors by pre multiplying the vector: MP -> P'
		[[nodiscard]] static Matrix4x4 RotateRHAnticlockwiseZ(Radians theta);

		// Given a right handed co-ordinate system, with Y away from the observer, phi gives the rotation in radians anticlockwise about the axis
		// The matrx rotates vectors by pre multiplying the vector: MP -> P'
		[[nodiscard]] static Matrix4x4 RotateRHAnticlockwiseY(Radians theta);

		[[nodiscard]] static void GetRollYawPitchMatrix(Radians roll, Radians yaw, Radians pitch,OUT Matrix4x4& rotationMatrix);

		[[nodiscard]] Vec3 GetPosition() const
		{
			return Vec3{ row0.w, row1.w, row2.w };
		}

		[[nodiscard]] void SetPosition(cr_vec3 pos)
		{
			row0.w = pos.x;
			row1.w = pos.y;
			row2.w = pos.z;
		}

		[[nodiscard]] static void FromQuat(const Quat& quat, Matrix4x4& m);
		[[nodiscard]] static void FromQuatAndThenTranspose(const Quat& quat, Matrix4x4& m);
		[[nodiscard]] static void GetRotationQuat(const Matrix4x4& m, Quat& quat);

		// if the matrix represents a transformation to a camera looking down the negative z axies 
		// then this function gives the forward direction
		[[nodiscard]] Vec3 GetWorldToCameraForwardDirection() const;
		[[nodiscard]] Vec3 GetForwardDirection() const;
		[[nodiscard]] Vec3 GetRightDirection() const;
		[[nodiscard]] Vec3 GetUpDirection() const;

		// Returns a matrix in a RH system for a camera located at the origin, with stuff below z = -near visible
		// Used to pre-multiply column vector to transform into screen space
		[[nodiscard]] static Matrix4x4 GetRHProjectionMatrix(Degrees fov, float32 aspectRatio, float near, float far);
	};

	[[nodiscard]] float Determinant(const Matrix4x4& m);

	[[nodiscard]] inline Vec4 HomogenizeNormal(cr_vec3 normal)
	{
		return Vec4{ normal.x, normal.y, normal.z, 0.0f };
	}

	[[nodiscard]] inline Vec4 HomogenizePosition(cr_vec3 normal)
	{
		return Vec4{ normal.x, normal.y, normal.z, 1.0f };
	}

	[[nodiscard]] Vec2  GetIntersect(Vec2 A, Vec2 D, Vec2 B, Vec2 E);
	[[nodiscard]] Radians GetHeadingOfVector(float DX, float DY);

#ifdef _WIN32
	// Multiply matrix Ra x Rb to make RaRb. This has the property that Ra X Rb x v = (Ra x Rb) x v = Ra x (Rb x v)
	void Multiply(Matrix4x4& product, const Matrix4x4& Ra, const Matrix4x4& Rb);
#endif
	[[nodiscard]] Matrix4x4 operator * (const Matrix4x4& a, const Matrix4x4& b);
	[[nodiscard]] Vec4 operator * (const Vec4& v, const Matrix4x4& R);
	[[nodiscard]] Vec4 operator * (const Matrix4x4& R, const Vec4& v);
	[[nodiscard]] Vec4 operator * (float scale, const Vec4& v); // Scale a 4 vector. N.B w component is invariant

	[[nodiscard]] inline constexpr float PI() { return 3.14159265358979323846f; }

	[[nodiscard]] inline constexpr float DEGREES_TO_RADIANS_QUOTIENT()
	{
		return PI() / 180.0f;
	}

	[[nodiscard]] inline constexpr float RADIANS_TO_DEGREES_QUOTIENT()
	{
		return 180.0f / PI();
	}

	struct Radians
	{
		float radians;
		operator float() const { return radians; }
		operator Degrees () const;
	};

	[[nodiscard]] inline Radians operator - (Radians r)
	{
		return Radians{ -(float)r };
	}

	struct Degrees
	{
		float degrees;
		operator float() const { return degrees; }

		operator Radians () const { return Radians{ DEGREES_TO_RADIANS_QUOTIENT() * degrees }; }
		Radians ToRadians() const { return Radians{ DEGREES_TO_RADIANS_QUOTIENT() * degrees }; }
	};

	/*
		A bone treats its parent frame as Y = forward, X = right and Z = up
		A bones considers its parent to extend vertically upwards from the origin
	*/
	struct BoneAngles
	{
		/*
			Anticlockwise, rotation about parent's x-axis (parent's right),
			0 < Tilt < 90 degrees elevate the forwards direction and pull the up direction backwards
		*/
		Degrees tilt;

		/*
			Anticlockwise, rotation about parent's y-axis (parent's forward),
			0 < Roll < 90 degrees, bone tips to the right
		*/
		Degrees roll;

		/*
			Anticlockwise, rotation about parent's z-axis (parent's vertica axis aligned with its bone),
			0 < Roll < 90 degrees, bone tips to the right
		*/
		Degrees facing;
	};


	struct FPSAngles
	{
		Degrees heading;   // 0 is North, 90 is East, 180 is South, 270 is West
		Degrees elevation; // 0 is horizontal, 90 is up, -90 is down
		Degrees tilt;      // -ve is lean left angle, +ve is lean right angle
	};

	[[nodiscard]] inline Radians::operator Degrees () const { return Degrees{ radians * RADIANS_TO_DEGREES_QUOTIENT() }; }

	[[nodiscard]] inline Degrees operator "" _degrees(long double literalValue)
	{
		return Degrees{ (float)literalValue };
	}

	[[nodiscard]] inline Degrees operator "" _degrees(unsigned long long literalValue)
	{
		return Degrees{ (float)literalValue };
	}

	[[nodiscard]] inline float Sin(Radians theta) { return sinf(theta.radians); }
	[[nodiscard]] inline float Cos(Radians theta) { return cosf(theta.radians); }

	struct Gravity
	{
		float g; // generally negative, and in metres per second per second
		operator float() const { return g; }
	};

	struct Triangle2d
	{
		Vec2 A;
		Vec2 B;
		Vec2 C;

		[[nodiscard]] size_t CountInternalPoints(const Vec2* points, size_t nPoints);
		[[nodiscard]] bool IsInternal(Vec2 p) const;
		[[nodiscard]] bool IsInternalOrOnEdge(Vec2 p) const;
	};

	struct Triangle
	{
		Vec3 A;
		Vec3 B;
		Vec3 C;

		/* 
		 Compute the cross product: (B-A)(C-A). With the anticlockwise oriented triangle below:

		 C
		:
		:
		:
		:
		:
		 A-----------B

		 B-A is i, and C-A is j, so the result is i x j = k
		*/
		Vec3 EdgeCrossProduct() const;
	};

	[[nodiscard]] bool GetTriangleHeight(const Triangle& t, cr_vec2 P, float& result);

	// In General, quads are tesselated clockwise a -> b -> c -> d -> a
	struct Quad
	{
		Vec3 a;
		Vec3 b;
		Vec3 c;
		Vec3 d;
	};

	ROCOCO_INTERFACE I2dMeshBuilder
	{
	   virtual void Append(const Triangle2d& t) = 0;
	};

	[[nodiscard]] void TesselateByEarClip(I2dMeshBuilder& tb, IRingManipulator<Vec2>& ring);

	[[nodiscard]] inline Vec4 operator-(const Vec4& v) { return Vec4{ -v.x, -v.y, -v.z, v.w }; }
	[[nodiscard]] inline Vec2 operator - (const Vec2& a, const Vec2& b) { return Vec2{ a.x - b.x, a.y - b.y }; }
	[[nodiscard]] inline Vec2 operator + (const Vec2& a, const Vec2& b) { return Vec2{ a.x + b.x, a.y + b.y }; }
	[[nodiscard]] inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
	[[nodiscard]] inline Vec2 operator / (const Vec2& numerator, float denominator) { float q = 1.0f / denominator; return Vec2{ numerator.x * q, numerator.y * q }; }
	[[nodiscard]] inline Vec2 operator * (const Vec2& q, float f) { return Vec2{ q.x * f, q.y * f }; }
	[[nodiscard]] inline Vec2 operator * (float f, const Vec2& q) { return Vec2{ q.x * f, q.y * f }; }
	[[nodiscard]] inline void operator += (Vec2& a, const Vec2& b) { a.x += b.x; a.y += b.y; }
	[[nodiscard]] inline void operator -= (Vec2& a, const Vec2& b) { a.x -= b.x; a.y -= b.y; }
	[[nodiscard]] inline bool operator == (Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
	[[nodiscard]] inline bool operator != (Vec2 a, Vec2 b) { return !(a == b); }

	[[nodiscard]] inline Vec2i operator * (Vec2i a, int scalar) { return { a.x * scalar, a.y * scalar }; }
	[[nodiscard]] inline Vec2i operator * (int scalar, Vec2i a) { return { a * scalar }; }

	/*

	|i  j  k |
	|ax ay az|
	|bx by bz|

	*/

	// N.B if a is i and b is j, then the cross product is k
	// i x j is counter-clockwise and cross product is positive
	[[nodiscard]] inline float Cross(Vec2 a, Vec2 b)
	{
		return a.x * b.y - a.y * b.x;
	}

	[[nodiscard]] inline Vec3 operator + (cr_vec3 a, cr_vec3 b) { return Vec3{ a.x + b.x, a.y + b.y, a.z + b.z }; }
	[[nodiscard]] inline Vec3 operator * (cr_vec3 q, float f) { return Vec3{ q.x * f, q.y * f, q.z * f }; }
	[[nodiscard]] inline Vec3 operator * (float f, cr_vec3 q) { return Vec3{ q.x * f, q.y * f, q.z * f }; }
	[[nodiscard]] inline Vec3 operator - (cr_vec3 a, cr_vec3 b) { return Vec3{ a.x - b.x, a.y - b.y, a.z - b.z }; }
	[[nodiscard]] inline bool operator == (const Vec3& a, const Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
	[[nodiscard]] inline bool operator != (const Vec3& a, const Vec3& b) { return !(a == b); }

	inline void operator += (Vec3& a, const Vec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
	inline void operator -= (Vec3& a, const Vec3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; }

	[[nodiscard]] inline Vec3 operator - (cr_vec3 a) { return Vec3{ -a.x, -a.y, -a.z }; }

	[[nodiscard]] inline float Dot(cr_vec3 a, cr_vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	[[nodiscard]] inline float operator * (cr_vec3 a, cr_vec3 b) { return Dot(a, b); }

	void TransformDirection(cr_m4x4 t, cr_vec3 n, Vec3& tn);
	void TransformPosition(cr_m4x4 t, cr_vec3 p, Vec3& tp);

	[[nodiscard]] inline Vec3 Cross(cr_vec3 a, cr_vec3 b)
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

	[[nodiscard]] inline Vec2 Dequantize(const Vec2i& v)
	{
		return Vec2{ (float)v.x, (float)v.y };
	}

	[[nodiscard]] inline GuiRectf Dequantize(const GuiRect& v)
	{
		return GuiRectf((float)v.left, (float)v.top, (float)v.right, (float)v.bottom);
	}

	[[nodiscard]] inline GuiRectf operator + (const GuiRectf& quad, Vec2 offset)
	{
		return{ quad.left + offset.x, quad.top + offset.y, quad.right + offset.x, quad.bottom + offset.y };
	}

	inline Vec2 operator *= (Vec2& p, float scale)
	{
		p.x *= scale;
		p.y *= scale;
		return p;
	}

	[[nodiscard]] bool TryGetRealRoots(float& x0, float& x1, float a /* x^2 */, float b /* x */, float c);
	[[nodiscard]] bool TryGetIntersectionLineAndSphere(float& t0, float& t1, cr_vec3 start, cr_vec3 end, const Sphere& sphere);

	[[nodiscard]] inline Vec2 Centre(const GuiRectf& q) { return Vec2{ (q.left + q.right) * 0.5f, (q.top + q.bottom) * 0.5f }; }
	[[nodiscard]] inline float Width(const GuiRectf& q) { return q.right - q.left; }
	[[nodiscard]] inline float Height(const GuiRectf& q) { return q.bottom - q.top; }
	[[nodiscard]] inline Vec2 Span(const GuiRectf& q) { return Vec2{ Width(q), Height(q) }; }

	[[nodiscard]] inline bool IsRectClipped(const GuiRect& clipRect, const GuiRect& subjectOfQueryRect)
	{
		return subjectOfQueryRect.left < clipRect.left || subjectOfQueryRect.right > clipRect.right || subjectOfQueryRect.top < clipRect.top || subjectOfQueryRect.bottom > clipRect.bottom;
	}

	[[nodiscard]] inline bool AreRectsOverlapped(const GuiRect& a, const GuiRect& b)
	{
		bool disjoint = a.left > b.right || a.right < b.left || a.top > b.bottom || a.bottom < b.top;
		return !disjoint;
	}

	[[nodiscard]] inline Vec2 TopLeft(const GuiRectf& q) { return Vec2{ q.left, q.top }; }
	[[nodiscard]] inline Vec2 BottomRight(const GuiRectf& q) { return Vec2{ q.right, q.bottom }; }
	[[nodiscard]] inline Vec2 TopRight(const GuiRectf& q) { return Vec2{ q.right, q.top }; }
	[[nodiscard]] inline Vec2 BottomLeft(const GuiRectf& q) { return Vec2{ q.left, q.bottom }; }

	[[nodiscard]] inline float LengthSq(cr_vec3 v) { return v * v; }
	[[nodiscard]] inline float Square(float x) { return x * x; }
	[[nodiscard]] inline float Cube(float x) { return x * x * x; }
	[[nodiscard]] inline bool IsInRange(cr_vec3 v, const Metres range) { return LengthSq(v) < Square(range); }
	[[nodiscard]] float Length(cr_vec3 v);
	void swap(float& a, float &b);
	[[nodiscard]] Vec3 Normalize(cr_vec3 v);
	[[nodiscard]] bool TryNormalize(cr_vec3 v, Vec3& nv);

	[[nodiscard]] inline float LengthSq(Vec2 v) { return Square(v.x) + Square(v.y); }
	[[nodiscard]] inline float Length(Vec2 v) { return sqrtf(LengthSq(v)); }

	[[nodiscard]] Radians ComputeWeaponElevation(cr_vec3 origin, cr_vec3 target, float projectileSpeed, Degrees maxElevation, Gravity g, Metres largestError);

	// Create matrices for isometric rendering. The origin is relocated to the world centre, the world rotated by elevation and heading, and then scaled up
	// The camera is vertically above the scene, with y up, x right and looking directly downwards
	// heading is the angle that the world is rotated anticlockwise
	void GetIsometricTransforms(Matrix4x4& worldMatrix, Matrix4x4& inverseWorldMatrixProj, Matrix4x4& worldMatrixAndProj, float scale, float aspectRatio, cr_vec3 centre, Degrees phi, Degrees viewTheta, Metres cameraHeight);

	[[nodiscard]] Matrix4x4 InvertMatrix(const Matrix4x4& matrix);
	[[nodiscard]] Matrix4x4 TransposeMatrix(const Matrix4x4& matrix);

	[[nodiscard]] inline Degrees operator - (Degrees theta) { return Degrees{ -theta.degrees }; }

	[[nodiscard]] bool IsPointInRect(Vec2 p, const GuiRectf& rect);

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
	void TransformDirections(const Vec3* vertices, size_t nElements, cr_m4x4 transform, Vec3* transformedVertices);

	[[nodiscard]] inline const Vec2& AsVec2(cr_vec3 & v)
	{
		return (const Vec2&)v;
	}

	ROCOCO_INTERFACE IObjectEnumerator
	{
		virtual void OnId(uint64 id) = 0;
	};

	ROCOCO_INTERFACE IQuadTree
	{
		virtual void AddEntity(const Sphere& boundingSphere, uint64 id) = 0;
		virtual void Clear() = 0;
		virtual void DeleteEntity(const Sphere& boundingSphere, uint64 id) = 0;
		virtual void EnumerateItems(const Sphere& boundingSphere, IObjectEnumerator& cb) = 0;
		virtual void GetStats(QuadStats& stats) = 0;

	};

	ROCOCO_INTERFACE IQuadTreeSupervisor : public IQuadTree
	{
		virtual void Free() = 0;
	};

	[[nodiscard]] IQuadTreeSupervisor* CreateLooseQuadTree(float width, float minBoundingRadius);

	[[nodiscard]] Matrix4x4 RotateDirectionToNegZ(cr_vec3 direction);

	struct Edge
	{
		Vec3 a;
		Vec3 b;
	};

	inline Vec2 Normalize(Vec2 v)
	{
		return v / Length(v);
	}

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

			struct NamedPlanes
			{
				ParallelPlanes westEast;
				ParallelPlanes southNorth;
				ParallelPlanes bottomTop;
			} namedPlanes;

			ParallelPlanes parallelPlanes[3];
		} P;

		Quadrilateral topVertices;
		Quadrilateral bottomVertices;
	};

	[[nodiscard]] void ForEachEdge(const Quadrilateral& q, IEnumerator<Edge>& processEdges);
	[[nodiscard]] void ForEachEdge(const BoundingCube& cube, IEnumerator<Edge>& processEdges);

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

	[[nodiscard]] Collision NoCollision();

	[[nodiscard]] Collision CollideBoundingBoxAndSphere(const BoundingCube& cube, const Sphere& sphere, cr_vec3 target);
	[[nodiscard]] Collision CollideEdgeAndSphere(const Edge& edge, const Sphere& sphere, cr_vec3 target);
	[[nodiscard]] Collision CollideVertexAndSphere(cr_vec3 v, const Sphere& sphere, cr_vec3 target);

	[[nodiscard]] bool IsPointInTriangle(cr_vec3 P, const Triangle& T);
	[[nodiscard]] Collision CollideLineAndTriangle(const Triangle& T, cr_vec3 start, cr_vec3 direction);

	template<class T> [[nodiscard]] T Lerp(const T& a, const T& b, float t) { return a + (b - a) * t; }
	
	namespace Maths::IEEE475
	{
		[[nodiscard]] ROCOCO_API float BinaryToFloat(uint32 binaryRepresentation);
		[[nodiscard]] ROCOCO_API double BinaryToDouble(uint64 binaryRepresentation);
		[[nodiscard]] ROCOCO_API uint32 FloatToBinary(float f);
		[[nodiscard]] ROCOCO_API uint64 DoubleToBinary(double d);
	}

	void ExpandZoneToContain(GuiRect& rect, const Vec2i& p);
	void ExpandZoneToContain(GuiRectf& rect, const Vec2& p);
}

#include <rococo.maths.i32.h>

#endif // ROCOCO_MATHS_H
#include <rococo.api.h>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <DirectXMath.h>

#include <rococo.strings.h>
#include <rococo.map.h>

#include <rococo.package.h>

#include <rococo.hashtable.h>

#include <rococo.random.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.maths.debug.lib")
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.mplat.debug.lib")
# pragma comment(lib, "rococo.misc.utils.debug.lib")
#else
# pragma comment(lib, "rococo.maths.lib")
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.mplat.lib")
# pragma comment(lib, "rococo.misc.utils.lib")
#endif

#include <stdio.h>

using namespace Rococo;

void validate(bool condition, const char* expr, const char* filename, int lineNumber)
{
	if (!condition)
	{
		Throw(0, "Error validating %s at %s @line %d", expr, filename, lineNumber);
	}
}

bool Approx(float f, float g)
{
	return ::fabsf(f - g) < 0.0001f;
}

bool Approx(cr_vec3 f, cr_vec3 g)
{
	return Approx(f.x, g.x) && Approx(f.y, g.y) && Approx(f.z, g.z);
}

#define VALIDATE(p) validate(p, #p, __FILE__, __LINE__)

void ValidatePolynomialLib()
{
	// (x + 7)(2x + 4) = 2x^2 + 18x + 28

	float x0, x1;
	VALIDATE(TryGetRealRoots(x0, x1, 2.0f, 18.0f, 28.0f));
	VALIDATE(x1 == -7.0f && x0 == -2.0f);

	// 3x - 6
	VALIDATE(TryGetRealRoots(x0, x1, 0.0f, 3.0f, -6.0f));
	VALIDATE(x0 == x1 && x0 == 2.0f);

	// x2 + 1 = 0, has imaginary roots (x + i) (x - i)
	VALIDATE(!TryGetRealRoots(x0, x1, 1.0f, 0.0f, 1.0f));
}

void ValidateQuadTreeLib() 
{
	AutoFree<IQuadTreeSupervisor> qTree(CreateLooseQuadTree(Metres{ 16384.0f }, Metres{ 3.95f }));
	//	qTree->AddEntity(Sphere{ Vec3 {0,0,0}, 2.0f }, 1);
	//	qTree->DeleteEntity(Sphere{ Vec3{ 0,0,0 }, 2.0f }, 1);
	qTree->AddEntity(Sphere{ Vec3{ 1,2,3 }, 2.0f }, 4);


	struct : IObjectEnumerator
	{
		virtual void OnId(uint64 id)
		{
			this->id = id;
		}

		uint64 id;
	} qSearch;

	qTree->EnumerateItems(Sphere{ Vec3{ 1, 2, 1 }, 1.0f }, qSearch);

	qTree->DeleteEntity(Sphere{ Vec3{ 1,2,3 }, 2.0f }, 4);

	VALIDATE(qSearch.id == 4);

	QuadStats stats;
	qTree->GetStats(stats);

	printf("Quads allocated: %I64u. Quads on free list: %I64u\n", stats.quadsAllocated, stats.quadsFree);
	printf("Nodes allocated: %I64u. Nodes on free list: %I64u\n", stats.nodesAllocated, stats.nodesFree);
	printf("Nodes size: %I64u bytes. GuiRectf size: %I64u bytes\n", stats.nodeAllocSize, stats.quadAllocSize);
}

void ValidateMatrixLib()
{
	using namespace DirectX;

	Vec4 centre{ 3.0f, 4.0f, 5.0f, 1.0f };
	Matrix4x4  T = Matrix4x4::Translate(-1.0f * centre);

	Vec4 v = T * centre;
	VALIDATE(v.x == 0 && v.y == 0 && v.z == 0);

	auto phi = 90.0_degrees;

	Matrix4x4 Rx = Matrix4x4::RotateRHAnticlockwiseX(phi);

	Vec4 rotatedCentre = Rx * centre;
 
	VALIDATE(Approx(rotatedCentre.x, centre.x));
	VALIDATE(Approx(rotatedCentre.y, -centre.z));
	VALIDATE(Approx(rotatedCentre.z, centre.y)); 

	auto theta = 90.0_degrees;
	Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(theta);
	rotatedCentre = Rz *  centre;

	VALIDATE(Approx(rotatedCentre.x, -centre.y));
	VALIDATE(Approx(rotatedCentre.y, centre.x));
	VALIDATE(Approx(rotatedCentre.z, centre.z));

	Matrix4x4 RxRz = Rx * Rz;

	rotatedCentre = RxRz * centre;

	VALIDATE(Approx(rotatedCentre.x, -centre.y));
	VALIDATE(Approx(rotatedCentre.y, -centre.z));
	VALIDATE(Approx(rotatedCentre.z, centre.x));

	RxRz = Matrix4x4::RotateRHAnticlockwiseZ( 90.0_degrees );

	Matrix4x4 moveCentreToOrigin = Matrix4x4::Translate(-1.0f * centre);
	Matrix4x4 shiftVertical = Matrix4x4::Translate(Vec3{ 0, 0, Metres{ 100.0f } });

	auto R0 = shiftVertical * RxRz * moveCentreToOrigin;

	Vec4 p{ 5.0f, 8.0f, 10.0f, 1.0 }; // after origin is moved, p -> (2 4 5), Rotate anticlockwise by 90 degrees -> (-4 2 5). Shift down by 100 to get (-4 2 105)
	Vec4 pPrimed = R0 * p;

	VALIDATE(Approx(pPrimed.x,  -4.0f));
	VALIDATE(Approx(pPrimed.y,   2.0f));
	VALIDATE(Approx(pPrimed.z, 105.0f));

	float scale = 2.0f;
	float aspectRatio = 1.5f;
	Matrix4x4 aspectCorrect = Matrix4x4::Scale(scale * aspectRatio, scale, scale);

	auto R1 = aspectCorrect * R0;

	pPrimed = R1 * p;

	VALIDATE(Approx(pPrimed.x, -12.0f));
	VALIDATE(Approx(pPrimed.y, 4.0f));
	VALIDATE(Approx(pPrimed.z, 210.0f));

	Matrix4x4 invR1 = InvertMatrix(R1);

	Vec4 pPrimedInv = invR1 * pPrimed;
	VALIDATE(Approx(pPrimedInv.x,  5.0f));
	VALIDATE(Approx(pPrimedInv.y,  8.0f));
	VALIDATE(Approx(pPrimedInv.z, 10.0f));

	XMMATRIX xortho = XMMatrixTranspose(XMMatrixOrthographicLH(2.0f, 2.0f, 0.0f, 1000.0f));

	Matrix4x4 ortho;
	XMStoreFloat4x4(ortho, xortho);

	Matrix4x4 R2 = ortho * R1;
	
	Vec4 pFullTransform = R2 * p;

	VALIDATE(Approx(pFullTransform.x, -12.0f));
	VALIDATE(Approx(pFullTransform.y, 4.0f));
	VALIDATE(Approx(pFullTransform.z, 0.21f));

	auto invR2 = InvertMatrix(R2);
	pPrimedInv = invR2 * pFullTransform;
	VALIDATE(Approx(pPrimedInv.x, 5.0f));
	VALIDATE(Approx(pPrimedInv.y, 8.0f));
	VALIDATE(Approx(pPrimedInv.z, 10.0f));

	{
		Matrix4x4 rotDirToZ = RotateDirectionToNegZ({ 0,0,-1 });

		Vec3 Nz;
		TransformDirection(rotDirToZ, { 0,0,1 }, Nz);

		VALIDATE(Nz.x == 0);
		VALIDATE(Nz.y == 0);
		VALIDATE(Nz.z == 1.0f);

		Vec3 Ny;
		TransformDirection(rotDirToZ, { 0,1,0 }, Ny);

		VALIDATE(Ny.x == 0);
		VALIDATE(Ny.y == 1.0f);
		VALIDATE(Ny.z == 0);

		Vec3 Nx;
		TransformDirection(rotDirToZ, { 1,0,0 }, Nx);

		VALIDATE(Nx.x == 1.0f);
		VALIDATE(Nx.y == 0.0f);
		VALIDATE(Nx.z == 0);
	}

	{
		Matrix4x4 rotDirToZ = RotateDirectionToNegZ({ 0,0,1 });

		Vec3 Nz;
		TransformDirection(rotDirToZ, { 0,0,-1 }, Nz);

		VALIDATE(Nz.x == 0);
		VALIDATE(Nz.y == 0);
		VALIDATE(Nz.z == 1.0f);

		Vec3 Ny;
		TransformDirection(rotDirToZ, { 0,1,0 }, Ny);

		VALIDATE(Ny.x ==    0);
		VALIDATE(Ny.y == -1.0f);
		VALIDATE(Ny.z ==    0);

		Vec3 Nx;
		TransformDirection(rotDirToZ, { 1,0,0 }, Nx);

		VALIDATE(Nx.x == 1.0f);
		VALIDATE(Nx.y == 0.0f);
		VALIDATE(Nx.z == 0);
	}

	{
		Matrix4x4 rotDirToZ = RotateDirectionToNegZ({ 1,0,0 });

		Vec3 Nz;
		TransformDirection(rotDirToZ, { 1, 0, 0 }, Nz);

		VALIDATE(Nz.x ==  0);
		VALIDATE(Nz.y ==  0);
		VALIDATE(Nz.z == -1);

		Vec3 Ny;
		TransformDirection(rotDirToZ, { 0,1,0 }, Ny);

		VALIDATE(Ny.x == -1.0);
		VALIDATE(Ny.y == 0);
		VALIDATE(Ny.z == 0);

		Vec3 Nx;
		TransformDirection(rotDirToZ, { 0,0,1 }, Nx);

		VALIDATE(Nx.x == 0.0f);
		VALIDATE(Nx.y == 1.0f);
		VALIDATE(Nx.z == 0);
	}

	{
		Matrix4x4 rotDirToZ = RotateDirectionToNegZ({ 0,0,-1 });

		Vec3 Ny;
		TransformDirection(rotDirToZ, { 0, 1, 0 }, Ny);

		VALIDATE(Ny.x ==  0);
		VALIDATE(Ny.y ==  1);
		VALIDATE(Ny.z ==  0);

		Vec3 Nx;
		TransformDirection(rotDirToZ, {  1,0,0 }, Nx);

		VALIDATE(Nx.x == 1.0f);
		VALIDATE(Nx.y == 0);
		VALIDATE(Nx.z == 0);

		Vec3 Nz;
		TransformDirection(rotDirToZ, { 0,0,1 }, Nz);

		VALIDATE(Nx.x == 1.0f);
		VALIDATE(Nx.y == 0.0f);
		VALIDATE(Nx.z == 0.0f);
	}
}

void ValidateVectorLib()
{
	Vec2 p = GetIntersect({ 1,2 }, { 0,1 }, { 0,3 }, { 1,0 });
	VALIDATE(p.x == 1 && p.y == 3);

	p = GetIntersect({ 2,1 }, { 1,0 }, { 3, 0 }, { 0,1 });
	VALIDATE(p.x == 3 && p.y == 1);
}

void ValidateCollisionLib()
{
	float t0, t1;
	VALIDATE(TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3{ 7,0,0 }, 2.0f }));
	VALIDATE(Approx(t0, 0.2f) && Approx(t1, 0.4f));

	VALIDATE(!TryGetIntersectionLineAndSphere(t0, t1, Vec3{ 1, 0, 0 }, Vec3{ 21, 0, 0 }, Sphere{ Vec3{ 7,4,0 }, 2.0f }));

	BoundingCube cube;
	cube.P.parallelPlanes[0] = { { Vec4{ -1, 0, 0, 0 }, Vec4{ -1, 0, 0, 1 } },{ Vec4{ 1, 0, 0, 0 }, Vec4{ 1, 0, 0, 1 } } };
	cube.P.parallelPlanes[1] = { { Vec4{ 0, -1, 0, 0 }, Vec4{ 0, -1, 0, 1 } },{ Vec4{ 0, 1, 0, 0 }, Vec4{ 0, 1, 0, 1 } } };
	cube.P.parallelPlanes[2] = { { Vec4{ 0, 0, -1, 0 }, Vec4{ 0, 0, -1, 1 } },{ Vec4{ 0, 0, 1, 0 }, Vec4{ 0, 0, 1, 1 } } };
	cube.topVertices.v =
	{
		{-1, -1, 1}, { 1, -1, 1 },{ -1, 1, 1 }, {1, 1, 1}
	};

	cube.bottomVertices.v =
	{
		{ -1, -1, -1 },{ 1, -1, -1 },{ -1, 1, -1 },{ 1, 1, -1 }
	};

	Sphere fallingSphere{ Vec3{ 0, 0, 10.0f }, 1.0f };

	auto collision1 = CollideBoundingBoxAndSphere(cube, fallingSphere, Vec3{ 0, 0, 0 });

	VALIDATE(Approx(collision1.t, 0.8f));
	VALIDATE(Approx(collision1.touchPoint.x, 0));
	VALIDATE(Approx(collision1.touchPoint.y, 0));
	VALIDATE(Approx(collision1.touchPoint.z, 1.0f));
	VALIDATE(collision1.contactType == ContactType_Face);

	Sphere edgeTapAttackSphere{ Vec3{ -10, 1.0, 0.0f }, 2.0f };

	auto collision2 = CollideBoundingBoxAndSphere(cube, edgeTapAttackSphere, Vec3{ 0, 1.0, 0 });
	VALIDATE(Approx(collision2.t, 0.7f));
	VALIDATE(Approx(collision2.touchPoint.x, -1));
	VALIDATE(Approx(collision2.touchPoint.y, 1));
	VALIDATE(Approx(collision2.touchPoint.z, 0.0f));
	VALIDATE(collision2.contactType == ContactType_Face);

	Sphere zed{ Vec3{ -10.0f, 0, 0.5f }, 2.0f };
	Collision edge = CollideEdgeAndSphere(Edge{ { 0, 0, -1 }, { 0, 0, 1 } }, zed, Vec3{ 0, 0.0, 0.5f });
	VALIDATE(edge.contactType == ContactType_Edge);
	VALIDATE(edge.isDegenerate == false);
	VALIDATE(Approx(edge.t, 0.8f));
	VALIDATE(Approx(edge.touchPoint, Vec3{0.0f, 0.0f, 0.5f }));

	Sphere edgeAttackSphere{ Vec3{ -10, 2.0, 0.0f }, 2.0f };
	auto collision3 = CollideBoundingBoxAndSphere(cube, edgeAttackSphere, Vec3{ 0, 2.0, 0 });

	float collisionTime = (9.0f - sqrtf(3.0f)) / 10.0f;

	VALIDATE(Approx(collision3.t, collisionTime));
	VALIDATE(Approx(collision3.touchPoint.x, -1));
	VALIDATE(Approx(collision3.touchPoint.y, 1));
	VALIDATE(Approx(collision3.touchPoint.z, 0.0f));
	VALIDATE(collision3.contactType == ContactType_Edge);

	auto edgeTest2 = CollideEdgeAndSphere(Edge{ { 0, 0, 0.01f},{ 0, 0, -0.01f } }, edgeAttackSphere, Vec3{ 0, 2.0, 0 } );
	VALIDATE(edgeTest2.contactType == ContactType_Edge);

	auto edgeTest3 = CollideEdgeAndSphere(Edge{ { 0, 0, -0.01f },{ 0, 0, -0.02f } }, edgeAttackSphere, Vec3{ 0, 2.0, 0 });
	VALIDATE(edgeTest3.contactType == ContactType_None);

	Collision ptTest = CollideVertexAndSphere({ 0, 2.0f, 0.0f }, edgeAttackSphere, { 0, 2.0f, 0.0f });
	VALIDATE(ptTest.contactType == ContactType_Vertex);
	VALIDATE(Approx(ptTest.t, 0.8f));

	Collision ptTest2 = CollideVertexAndSphere({ 0, 0.001f, 0.0f }, edgeAttackSphere, { 0, 2.0f, 0.0f });
	VALIDATE(ptTest2.contactType == ContactType_Vertex);
	
	Collision ptTest3 = CollideVertexAndSphere({ 0, -0.001f, 0.0f }, edgeAttackSphere, { 0, 2.0f, 0.0f });
	VALIDATE(ptTest3.contactType == ContactType_None);

   {
      Vec2 a = { 1, 2 }, b = { 9, 2 };
      Vec2 c = { 0, 1 }, d = { 0, 3 };
      float t, u;
      if (!GetLineIntersect(a, b, c, d, t, u))
      {
         Throw(0, "Bad intersect!");
      }
      VALIDATE(u == 0.5f);
      VALIDATE(t == -0.125f);
   }

   {
      Vec2 a = { 1, 2 }, b = { 9, 2 };
      Vec2 c = { 4, 2 }, d = { 5, 2 };
      float t, u;
      if (GetLineIntersect(a, b, c, d, t, u))
      {
         Throw(0, "Bad intersect!");
      }
   }

   {
      Vec2 a{ 0, 1 }, b{ 0, 2 };
      Vec2 c{ 0, 2 }, d{ 0, 3 };
      VALIDATE(DoParallelLinesIntersect(a, b, c, d));
   }

   {
      Vec2 a{ 1, 0 }, b{ 2, 0 };
      Vec2 c{ 2, 0 }, d{ 3, 0 };
      VALIDATE(DoParallelLinesIntersect(a, b, c, d));
   }

   {
      Vec2 a{ 1, 1 }, b{ 3, 3 };
      Vec2 c{ 2, 2 }, d{ 5, 5 };
      VALIDATE(DoParallelLinesIntersect(a, b, c, d));
   }

   {
      Vec2 a{ 1, 1 }, b{ 3, 3 };
      Vec2 c{ 3.01f, 3.01f }, d{ 5, 5 };
      VALIDATE(!DoParallelLinesIntersect(a, b, c, d));
   }
}

void  ValidateProjectionLib()
{
   Matrix4x4 projection = Matrix4x4::GetRHProjectionMatrix(90_degrees, 2.0f, 0.3_metres, 3000.3_metres);

   Vec4 xNear{ 1.0f, 1.0f, -0.3_metres, 1.0f };
   Vec4 xFar1{ 1.0f, 1.0f, -2990_metres, 1.0f };
   Vec4 xFar2{ 1.0f, 1.0f, -3010_metres, 1.0f };

   Vec4 xn = projection * xNear;
   Vec4 xf1 = projection * xFar1;
   Vec4 xf2 = projection * xFar2;

   VALIDATE(xn.x == 0.5f);
   VALIDATE(xn.y == 1.0f);
   VALIDATE(xn.z == 0.0f);
   VALIDATE(xn.w > 0.0f);

   VALIDATE(xf1.x == 0.5f);
   VALIDATE(xf1.y == 1.0f);
   VALIDATE((xf1.z / xf1.w) < 1.0f);
   VALIDATE((xf2.z / xf2.w) > 1.0f);

   Matrix4x4 translation = Matrix4x4::Translate(Vec3{ 0,0,-0.3_metres });

   Matrix4x4 PT = projection * translation;

   Vec4 xAt0{ 1.0f, 1.0f, 0, 1.0f };
   Vec4 x0 = PT * xAt0;

   VALIDATE(xn.x == x0.x);
   VALIDATE(xn.y == x0.y);
   VALIDATE(xn.z == x0.z);
   VALIDATE(xn.w == x0.w);
}

void validatePenetration()
{
   // We need to test case when edge is within sphere
   Sphere penetrator{ Vec3{ 0, -0.9f, 0.0f }, 1.0f };
   auto c = CollideEdgeAndSphere(Edge{ { -10, 0, 0 },{ 10, 0, 0 } }, penetrator, Vec3{ 0, 1.2f, 0 });
   VALIDATE(c.contactType == ContactType_Penetration);
   VALIDATE(c.t == 0);

   bool match = c.touchPoint == Vec3{ 0, 0, 0 };
   VALIDATE(match);
}

#include "rococo.rings.inl"

void validateTeselator()
{
	{
		const Vec2 triangle[3] =
		{
			{ 1.0f ,1.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f }
		};

		Ring<Vec2> clockwiseTri(triangle, 3);
		VALIDATE(IsClockwiseSequential(clockwiseTri));
	}

	{
		const Vec2 triangle[3] =
		{
			{ 0.0f, 0.0f },
			{ 1.0f ,1.0f },
			{ 0.0f, 1.0f }
		};

		Ring<Vec2> anticlockwiseTri(triangle, 3);
		VALIDATE(!IsClockwiseSequential(anticlockwiseTri));
	}

	Vec2 perimeter[5] =
	{
		{ 0.0f, 2.0f },
		{ 2.0f, 2.0f },
		{ 2.0f, 0.0f },
		{ 0.0f, 0.0f },
		{ 1.0f, 1.0f }
	};

	RingManipulator<Vec2> ring(perimeter, 5);

	Ring<Vec2> clockwisePerimeter(perimeter, 5);
	VALIDATE(IsClockwiseSequential(clockwisePerimeter));

	struct :I2dMeshBuilder
	{
		void Append(const Triangle2d& t) override
		{
		}
	} builder;

	TesselateByEarClip(builder, ring);
}

void validateConeCode()
{
	{
		Vec3 eye = { 4.550099f, 1.103791f, 1.991499f };
		Vec3 dir = { -0.867786f, 0.496937f, 0.000863f };
		auto coneAngle = 0_degrees;
		Vec3 pos1 = { -18.f , 14.f, 2.f };
		auto cc = GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(eye, dir, coneAngle, pos1);
	//	VALIDATE(cc.t == 5.0f);
	}

	{
		Vec3 eye = { 0, 0, 0 };
		Vec3 dir = { 0, 1, 0 };
		auto coneAngle = 45_degrees;
		Vec3 pos1 = { -4 , 1, 0 };
		auto cc = GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(eye, dir, coneAngle, pos1);
		VALIDATE(cc.t == 5.0f);
	}

	{
		Vec3 eye = { -1, 0, 0 };
		Vec3 dir = { 1,  0, 0 };
		auto coneAngle = 45_degrees;
		Vec3 pos1 = { 1 , 4, 0 };
		auto cc = GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(eye, dir, coneAngle, pos1);
		VALIDATE(cc.t == 6.0f);
	}

	{
		Vec3 eye = { 0, 0, 0 };
		Vec3 dir = { 1,  0, 0 };
		auto coneAngle = 45_degrees;
		Vec3 pos1 = { -4 , 8, 0 };
		auto cc = GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(eye, dir, coneAngle, pos1);
		VALIDATE(cc.t == -12.0f);
	}

	{
		Vec3 eye = { 0, 0, 0 };
		Vec3 dir = { 1,  0, 0 };
		auto coneAngle = 45_degrees;
		Vec3 pos1 = { 4, 5, 0 };

		float radius = sqrtf(0.5f);
		VALIDATE(IsOutsideCone(eye, dir, coneAngle, { pos1, radius - 0.01f }));
		VALIDATE(!IsOutsideCone(eye, dir, coneAngle, { pos1, radius + 0.01f }));
	}
}

void validateAABB()
{
	AABB aabb;
	aabb << Vec3 {-2, -1, 0};
	aabb << Vec3 { 2,  1, 1};

	Matrix4x4 Rz = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ 45.0f });
	AABB newBounds = aabb.RotateBounds(Rz);

	VALIDATE(newBounds.minXYZ.x < -2.11 && newBounds.minXYZ.x > -2.13);
	VALIDATE(newBounds.maxXYZ.x > 2.11 && newBounds.maxXYZ.x < 2.13);
	VALIDATE(aabb.maxXYZ.z == newBounds.maxXYZ.z);
	VALIDATE(aabb.minXYZ.z == newBounds.minXYZ.z);
}

cstr s[8] =
{
	"apple",
	"belt",
	"bodkin",
	"mangle",
	"esperanto",
	"kickoff",
	"yada",
	"handheld-orange"
};

#ifdef _DEBUG
enum { NumberOfIterations = 1'000'000 };
#else
enum { NumberOfIterations = 33'000'000 };
#endif
void TimeSTDUNMAP()
{
	std::unordered_map<std::string, int> m =
	{ 
		{"apple", 1},
		{"bodkin", 1},
		{"candle", 1},
		{"dabble", 1},
		{"eggs", 1},
		{"flower", 1},
		{"grumpy", 1},
		{"handheld", 1}
	};

	Random::RandomMT rng;
	rng.Seed((uint32)OS::CpuTicks());

	int count = 0;

	auto start = OS::CpuTicks();

	for (int i = 0; i < NumberOfIterations; ++i)
	{
		uint32 index = rng() & 0x7;
		auto c = s[index];
		auto j = m.find(c);
		if (j != m.end())
		{
			count++;
		}
	}

	auto end = OS::CpuTicks();

	auto dt = end - start;

	double DT = dt / (double)OS::CpuHz();

	if (count == 0) return;

	printf("std::unordered_map<std::string,int> %0.lf searches per second \n", NumberOfIterations / DT);

	// string map, 33 million searches per second;
}

void TimeStringMap()
{
	stringmap<int> m =
	{
		{"apple", 1},
		{"bodkin", 1},
		{"candle", 1},
		{"dabble", 1},
		{"eggs", 1},
		{"flower", 1},
		{"grumpy", 1},
		{"handheld", 1}
	};

	Random::RandomMT rng;
	rng.Seed((uint32)OS::CpuTicks());

	int count = 0;

	auto start = OS::CpuTicks();

	for (int i = 0; i < 3'000'000; ++i)
	{
		uint32 index = rng() & 0x7;
		auto c = s[index];
		auto j = m.find(c);
		if (j != m.end())
		{
			count++;
		}
	}

	auto end = OS::CpuTicks();

	auto dt = end - start;

	double DT = dt / (double)OS::CpuHz();

	if (count == 0) return;

	printf("stringmap<int>:  %0.lf searches per second \n", NumberOfIterations / DT);
}

void ValidateDictionary()
{
	HString a = "Geoff";
	printf("%s\n", a.c_str());

	VALIDATE(a.length() == 5);

	a = "Mike";
	printf("%s\n", a.c_str());

	VALIDATE(a.length() == 4);

	HString b(a);

	printf("%s\n", b.c_str());

	VALIDATE(b.length() == 4);

	Dictionary d;
	AddUnique(d, "Geoff", &d);
	AddUnique(d, "Mike", nullptr);
	AddUnique(d, "Barbara", nullptr);

	void* ptr = &ptr;
	VALIDATE(d.TryFind("Geoff", ptr));
	VALIDATE(ptr == &d);

	struct : IDictionaryEnumerator
	{
		EnumControl OnIteration(cstr key, size_t keyLength, void* buffer) override
		{
			printf("%s: %llu chars. 0x%llX value\n", key, keyLength, (size_t)buffer);
			return ENUM_CONTINUE;
		}
	} e;
	d.Enumerate(e);

	struct : IDictionaryEnumerator
	{
		EnumControl OnIteration(cstr key, size_t keyLength, void* buffer) override
		{
			if (key[0] == 'G' || key[0] == 'M')
			{
				return ENUM_ERASE_AND_CONTINUE;
			}
			else
			{
				return ENUM_CONTINUE;
			}
		}
	} f;
	d.Enumerate(f);

	printf("Deleted Geoff and Mike\n");
	d.Enumerate(e);
}

#include <rococo.quadtree.h>

void TestQuadtree()
{
	QuadtreeCreateContext qcc;
	qcc.centre = { 0,0 };
	qcc.minSpan = 1.0f;
	qcc.span = 32.0f;
	AutoFree<IQuadtreeSupervisor> quad = CreateLooseQuadtree(qcc);

	QuadtreeObject obj;
	obj.centre = { -4.0f,12.0f };
	obj.span = 8.0f;
	auto& pocket = quad->Insert(obj);
	
	struct CLOSURE : IEventCallback<QuadtreePocket>
	{
		bool found = false;
		void OnEvent(QuadtreePocket& pocket)
		{
		//	printf("(%f %f)", pocket.object.centre.x, pocket.object.centre.y);
			found = true;
		}
	};

	CLOSURE cb1;
	quad->EnumerateDescendants({ -0.001f, 7.999f }, cb1);
	VALIDATE(!cb1.found);

	CLOSURE cb2;
	quad->EnumerateDescendants({ -0.001f, 8.001f }, cb2);
	VALIDATE(cb2.found);

	CLOSURE cb3;
	quad->EnumerateDescendants({ 0.001f, 8.001f }, cb3);
	VALIDATE(!cb3.found);

	CLOSURE cb4;
	quad->EnumerateDescendants({ -8.001f, 8.001f }, cb4);
	VALIDATE(!cb4.found);

	CLOSURE cb5;
	quad->EnumerateDescendants({ -8.00f, 8.001f }, cb5);
	VALIDATE(cb5.found);

	CLOSURE cb6;
	quad->EnumerateDescendants({ -7.99f, 8.001f }, cb6);
	VALIDATE(cb6.found);

	CLOSURE cb7;
	quad->EnumerateDescendants({ -7.0f, 16.001f }, cb7);
	VALIDATE(!cb7.found);

	CLOSURE cb8;
	quad->EnumerateDescendants({ -7.0f, 16.00f }, cb8);
	VALIDATE(cb8.found);

	CLOSURE cb9;
	quad->EnumerateDescendants({ -7.0f, 16.01f }, cb8);
	VALIDATE(!cb9.found);
}

#include <rococo.octree.h>

void TestOctree()
{
	OctreeCreateContext occ;
	occ.centre = { 0,0,0 };
	occ.span = 2000_metres;
	occ.minSpan = 0.01_metres;
	AutoFree<IOctreeSupervisor> octree = CreateLooseOctree(occ);

	OctreeObject obj1;
	obj1.centre = { 0,0,0 };
	obj1.span = 2001_metres;
	auto& pocket = octree->Insert(obj1);
	pocket.context = 1;

	OctreeObject obj2;
	obj2.centre = { 0,0,0 };
	obj2.span = 1999_metres;
	auto& pocket2 = octree->Insert(obj2);
	pocket2.context = 2;

	OctreeObject obj3;
	obj3.centre = { 0,0,0 };
	obj3.span = 2000_metres;
	auto& pocket3 = octree->Insert(obj3);
	pocket3.context = 3;


	OctreeObject obj4;
	obj4.centre = { 0,0,0 };
	obj4.span = 999_metres;
	auto& pocket4 = octree->Insert(obj4);
	pocket4.context = 4;

	OctreeObject obj5;
	obj5.centre = { -0.5,0,0 };
	obj5.span = 999_metres;
	auto& pocket5 = octree->Insert(obj5);
	pocket5.context = 5;

	OctreeObject obj6;
	obj6.centre = { 0,0.5,0 };
	obj6.span = 999_metres;
	auto& pocket6 = octree->Insert(obj6);
	pocket6.context = 6;

	OctreeObject obj7;
	obj7.centre = { 0,0,0.5 };
	obj7.span = 999_metres;
	auto& pocket7 = octree->Insert(obj7);
	pocket7.context = 7;

	OctreeObject obj8;
	obj8.centre = { 0.5,0.5,0.5 };
	obj8.span = 999_metres;
	auto& pocket8 = octree->Insert(obj8);
	pocket8.context = 8;

	struct : IEventCallback<OctreePocket>
	{
		std::vector<uint64> ids;
		void OnEvent(OctreePocket& pocket) override
		{
			ids.push_back(pocket.context);
		}
	} knockOff;
	octree->EnumerateDescendants({ 1.0f, 1.0f, 1.0f }, knockOff);

	VALIDATE(knockOff.ids.size() == 8);

	knockOff.ids.clear();

	octree->EnumerateDescendants({ -500.0f, -10.0f, -10.0f }, knockOff);

	VALIDATE(knockOff.ids.size() == 7);

	Rococo::Random::RandomMT rng;

	OctreeCreateContext occ2;
	occ2.centre = { 0,0,0 };
	occ2.span = 2000_metres;
	occ2.minSpan = 0.01_metres;
	IOctreeSupervisor* octree2 = CreateLooseOctree(occ2);

	auto start = OS::CpuTicks();
	for (int i = 0; i < 100'000; ++i)
	{
		OctreeObject object;
		object.centre.x = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.centre.y = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.centre.z = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.span = Rococo::Random::NextFloat(rng, 0.01f, 10.0f);
		octree2->Insert(object);
	}
	auto dt = OS::CpuTicks() - start;

	size_t pocketBytes, nodeBytes;
	octree2->GetApproxMemoryUse(pocketBytes, nodeBytes);

	printf("100,000 item memory cost: %llu\n", pocketBytes + nodeBytes);

	printf("100,000 item insertion time into Octree: %f seconds\n", dt / (double)OS::CpuHz());

	start = OS::CpuTicks();
	octree2->Clear();
	dt = OS::CpuTicks() - start;

	printf("100,000 item clear time from Octree: %f seconds\n", dt / (double)OS::CpuHz());

	start = OS::CpuTicks();
	for (int i = 0; i < 100'000; ++i)
	{
		OctreeObject object;
		object.centre.x = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.centre.y = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.centre.z = Rococo::Random::NextFloat(rng, -1000.0f, 1000.0f);
		object.span = Rococo::Random::NextFloat(rng, 0.01f, 10.0f);
		octree2->Insert(object);
	}
	dt = OS::CpuTicks() - start;

	printf("100,000 item second insertion time into Octree: %f seconds\n", dt / (double)OS::CpuHz());

	octree2->GetApproxMemoryUse(pocketBytes, nodeBytes);

	printf("100,000 item memory cost: %llu\n", pocketBytes + nodeBytes);

	start = OS::CpuTicks();
	octree2->Clear();
	dt = OS::CpuTicks() - start;

	printf("100,000 item second clear time from Octree: %f seconds\n", dt / (double)OS::CpuHz());

	start = OS::CpuTicks();
	octree2->Free();

	dt = OS::CpuTicks() - start;

	printf("100,000 item deletion time from Octree: %f seconds\n", dt / (double)OS::CpuHz());
}

#include <rococo.sxytype-inference.h>

void TestCodeInference()
{
	cstr bad_code = 
R"(
	(function IncCat (Float32 catastrophe)(mangy cat)(Int32 cat) -> : (cat += 1))
)";

	using namespace Rococo::Sexy;

	BadlyFormattedTypeInferenceEngine engine(bad_code);
	cstr cat1 = strstr(bad_code, "cat");
	cstr cat2 = strstr(cat1 + 1, "cat");
	cstr cat3 = strstr(cat2 + 1, "cat");

	// cat4 points to the third instance of cat. Our engine is to be used for autocompletion, in this case, we want to infer the type of cat, which should yield 'Int32'
	// (mangy cat) is a syntax error, but our type inference engine does not require code to be 100% correct.
	cstr cat4 = strstr(cat3 + 1, "cat");

	VALIDATE(cat4 != nullptr);

	auto inference = engine.InferLocalVariableVariableType({ cat4, cat4 + 3 });
	VALIDATE(Length(inference.declarationType) > 0);
	VALIDATE(Length(inference.declarationVariable) > 0);
	VALIDATE(Eq(inference.declarationType, "Int32"_fstring));
}

#include <rococo.csv.h>

void test()
{
	TestCodeInference();
	TestQuadtree();
	return;
	TestOctree();
	TimeSTDUNMAP();
	TimeStringMap();
	//ValidateDictionary();
	validateAABB();
	validateConeCode();
	validateTeselator();
	validatePenetration();
	ValidateVectorLib();
	ValidatePolynomialLib();
	ValidateMatrixLib();
	//	ValidateQuadTreeLib();
	ValidateCollisionLib();
	ValidateProjectionLib();

	printf("rococo.maths.test finished\n");
}

template<class ARGTYPE> struct IMyCallback
{
	virtual void Callback(ARGTYPE arg) = 0;
};

struct Enumerator
{
	void Enumerate(IMyCallback<int>& cb)
	{
		cb.Callback(1);
		cb.Callback(2);
		cb.Callback(3);
	}
};

int main(int argc, char* argv[])
{
	try
	{
		test();
	}
	catch (IException& ex)
	{
		int code = ex.ErrorCode();
		if (code != 0)
		{
			char msg[128];
			Rococo::OS::FormatErrorMessage(msg, 128, code);
			printf("Code: %d: %s\n", code, msg);
		}
		printf("%s\n", ex.Message());
	}
}


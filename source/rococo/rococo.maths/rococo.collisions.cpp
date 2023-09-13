#include <rococo.api.h>
#include <rococo.maths.h>

#include <algorithm>

namespace Rococo
{
	Collision NoCollision()
	{
		return
		{
			Vec3{ 0,0,0 },
			1.0e20f,
			ContactType_None,
			false
		};
	};

	Collision CollidePlaneAndSphere(const Plane& plane, const Sphere& sphere, cr_vec3 target)
	{
		float n2 = LengthSq(plane.normal);
		if (n2 < 0.996f || n2 > 1.004)
		{
			Throw(0, "Bad normals: Normalize plane normal vectors.");
		}

		// Algorithm:
		//    When a sphere touches a plane the distance from the sphere to the plane is the radius of sphere.
		//    So determine distance function of sphere along its trajectory from plane, parameterized as t. 
		// Solve to give t where distance = radius.

		// When the outer surface of a sphere touches a plane the vector from the centre of the sphere to the plane is a product of
		// a scalar and plane normal. If the plane normal has modulus 1, then the scalar is the length of the vector.

		// If P is a point in the plane, and N is the plane normal pointing from the plane to the sphere and C is the circle centre define
		// the nearest point on the plane to C is O. The distance to the plane is d and so O = C - dN and (O-P).(O-C) = 0

		// (C - dN - P).-dN = 0 => (C - dN - P).N = 0
		// (C - P).N - d = 0, thus
		// d = (C-P).N .........................................(1)

		// Sphere touches plane, and d = R, thus R = (C-P).N

		// Example: If point in plane is (0 0 0), and plane normal is (0 0 1) and C is (0 0 2), then d = 2, as expected

		// Parameterize the centre of the sphere along its line trajectory using parameter t:


		// C = A + (B - A).t.........................................(2)
		// Insert into equation (1) and replace d with radius R

		// (A + (B - A). t - P).N = R
		// R = (A - P).N + (B - A).Nt
		// ( R + (P - A).N ) / (B-A).N = t

		cr_vec3 N = plane.normal;
		cr_vec3 A = sphere.centre;
		cr_vec3 B = target;
		cr_vec3 P = plane.pointInPlane;
		float R = sphere.radius;

		float denominator = (B - A) * N;
		if (fabsf(denominator) < 0.0001f)
		{
			// Almost parallel movement to the plane, or body moving very slowly
			// check against case where sphere already intersects plane

			float distance = (P - A) * N;
			Vec3 touchPoint = A - distance * N;
			if (distance <= R)
			{
				return Collision{ touchPoint, 0.0f, ContactType_Penetration, true };
			}
			else
			{
				return  Collision{ touchPoint, 1.0e20f, ContactType_None, true };
			}
		}

		float numerator = R + (P - A) * N;

		float t = numerator / denominator;

		Vec3 centreAtCollision = A + (B - A) * t;
		Vec3 touchPoint = centreAtCollision - R * N;

		return Collision{ touchPoint, t, ContactType_Face, false };
	}

	bool IsPointOnFrontSide(cr_vec3 p, const Plane& plane)
	{
		return (p - (cr_vec3)plane.pointInPlane) * plane.normal > 0;
	}

	bool IsPointBetweenPlanes(cr_vec3 p, const ParallelPlanes& planes)
	{
		return !IsPointOnFrontSide(p, planes.P0) && !IsPointOnFrontSide(p, planes.P1);
	}

	Collision CollideOnFaces(const BoundingCube& cube, const Sphere& sphere, cr_vec3 target)
	{
		Collision collision = NoCollision();

		for (int i = 0; i < 3; i++)
		{
			auto& planes = cube.P.parallelPlanes[i];

			auto Col0 = CollidePlaneAndSphere(planes.P0, sphere, target);
			auto Col1 = CollidePlaneAndSphere(planes.P1, sphere, target);

			if (Col0.t < 0 && Col1.t < 0)
			{
				// Sphere is moving away from both planes, so it must be moving away from cube
				return NoCollision();
			}

			if (Col0.t > Col1.t)
			{
				std::swap(Col0, Col1);
			}

			if (Col0.t < 0)
			{
				continue; // Cannot hit parallel surface if moving away from another
			}

			if (Col0.isDegenerate) continue;

			int containmentCount = 0;

			for (int j = 0; j < 3; j++)
			{
				if (i != j)
				{
					if (IsPointBetweenPlanes(Col0.touchPoint, cube.P.parallelPlanes[j]))
					{
						containmentCount++;
					}
				}
			}

			if (containmentCount == 2)
			{
				if (Col0.t < collision.t)
				{
					collision.t = Col0.t;
					collision.touchPoint = Col0.touchPoint;
					collision.contactType = ContactType_Face;
				}
			}
		}

		return collision;
	}

	struct Line
	{
		Vec3 Point;
		Vec3 Direction;
	};

	bool NearZero(float f)
	{
		return fabsf(f) < 0.001f;
	}

	// Given equation 
	//    (A.x A.y)(x)  =  (C.x)
	//    (B.x B.y)(y)     (C.y)
	// Determine (x y). z component of result is 0 for invalid, 1 otherwise
	Vec3 SolveLinearEq(Vec2 A, Vec2 B, Vec2 C)
	{
		// Multiple first row by B.x => (AxBx  AyBx)(x y) = Cx.Bx 
		// Multiple second row by A.x => (AxBx  AxBy)(x y) =  Cy.Ax
		// Subtract second row from first row: (0  AyBx - AxBy)(x y) = Cx.Bx, yielding y = Cx.Bx / AyBx - AxBy

		float detAB = A.y*B.x - A.x*B.y;
		if (NearZero(detAB))
		{
			return Vec3{ 0,0,0 };
		}

		if (NearZero(A.x))
		{
			if (NearZero(B.x)) return{ 0,0,0 };
			float y = C.x / A.y;
			float x = (C.y - B.y*y) / B.x;
			return{ x, y, 1.0f };
		}

		float y = C.x*B.x / detAB;
		float x = (C.x - B.y*y) / A.x;
		return{ x, y, 1.0f };
	}

	Line Intersect(const Plane& p, const Plane& q)
	{
		if (LengthSq(p.normal) < 0.994 || LengthSq(q.normal) > 1.004)
		{
			Throw(0, "Bad normals in Intersect(const Plane& p, const Plane& q)->Line");
		}
		// Plane equation: given two points P and Q in plane, with normal N:
		// (P - Q).N = 0..................(1)

		// Given a second plane with points R and S in plane, with normal M
		// (R - S).M = 0..................(2)

		// Planes will intersect at a line, with all points on line in both, so subsitute P for R:

		// (P - Q).N = 0
		// (P - S).M = 0

		// Direction of line will be N x M.

		// Px.Nx + Py.Ny + Pz.Nz = Q.N
		// Px.Mx + Py.My + Pz.Mz = S.M

		// If Ni = Nj = 0 where i != j, then PkNk = Q.N => Pk = Q.N / Nk
		// If Ni = 0, and Nj and Nk != 0, then choose Pk = 0
		// if  Nx, Ny and Nz not zero, then choose any Pi = 0

		cr_vec3 N = p.normal;
		cr_vec3 M = q.normal;
		cr_vec3 Q = p.pointInPlane;
		cr_vec3 S = q.pointInPlane;

		Vec3 direction = Cross(N, M);

		if (NearZero(LengthSq(direction)))
		{
			return Line{ {0,0,0}, {0,0,0} };
		}

		if (NearZero(N.x))
		{
			if (NearZero(N.y))
			{
				return Line{ { 0, 0, Q.z }, direction };
			}
			else if (NearZero(N.z))
			{
				return Line{ { 0, Q.y, 0 }, direction };
			}

			// Py.Ny + Pz.Nz = Q.N
			// Px.Mx + Py.My + Pz.Mz = S.M

			// Choose Py = 0 then Pz = Q.N / Nz and Px = (S.M - Pz.Mz) / Mx, this is never degenerate

			float Pz = Q*N / N.z;
			return Line{ { (S*M - (Pz*M.z)) / M.x, 0, Q * N / N.z}, direction };
		}
		else if (NearZero(N.y))
		{
			// N.X not near zero
			if (NearZero(N.z))
			{
				return Line{ { Q.x, 0, 0 }, direction };
			}

			// Px.Nx + Pz.Nz = Q.N
			// Px.Mx + Py.My + Pz.Mz = S.M

			// Choose Px = 0, then Pz = Q.N / Nz and Py = S.M - Pz.Mz / My, this is never degenerate

			float Pz = Q*N / N.z;
			return Line{ { 0, (S*M - (Pz*M.z)) / M.y, Q * N / N.z }, direction };
		}
		else if (NearZero(N.z))
		{
			// N.x and N.y not near zero
			// Px.Nx + Py.Ny = Q.N
			// Choose Px = 0, thus Py = Q.N / Ny and Pz = (S.M - Py.My) / Mz
			float Py = Q*N / N.y;
			return Line{ {0, Py, (S*M - Py*M.y) / M.z}, direction };
		}
		else
		{
			// Px.Nx + Py.Ny + Pz.Nz = Q.N
			// Px.Mx + Py.My + Pz.Mz = S.M
			// Choose Px = 0
			// Py.Ny + Pz.Nz = Q.N......(1)
			// Py.My + Pz.Mz = S.M......(2)

			Vec3 P = SolveLinearEq({ N.y, N.z }, { M.y, M.z }, { Q*N, S*M });
			if (P.z == 0.0f)
			{
				return Line{ {0,0,0}, {0,0,0} };
			}

			return Line{ {0, P.x, P.y}, direction };
		}
	}

	void ForEachEdge(const Quadrilateral& q, IEnumerator<Edge>& processEdges)
	{
		processEdges({ q.v.sw, q.v.se });
		processEdges({ q.v.se, q.v.ne });
		processEdges({ q.v.ne, q.v.nw });
		processEdges({ q.v.nw, q.v.sw });
	}

	void ForEachEdge(const BoundingCube& cube, IEnumerator<Edge>& processEdges)
	{
		for (int i = 0; i < 4; ++i)
		{
			processEdges({ cube.topVertices.vertices[i], cube.bottomVertices.vertices[i] });
		}

		ForEachEdge(cube.topVertices, processEdges);
		ForEachEdge(cube.bottomVertices, processEdges);
	}

	void ForEachVertex(const Quadrilateral& q, IEnumerator<Vec3>& processVertices)
	{
		for (int i = 0; i < 4; i++)
		{
			processVertices(q.vertices[i]);
		}
	}

	void ForEachVertex(const BoundingCube& cube, IEnumerator<Vec3>& processVertices)
	{
		ForEachVertex(cube.topVertices, processVertices);
		ForEachVertex(cube.bottomVertices, processVertices);
	}

	bool IsPointInTriangle(cr_vec3 P, const Triangle& T)
	{
		// Given that T is clockwise A->B->C
		// Then all internal points P give clockwise for APC BCP and ABP
		Vec3 N = Cross(T.B - T.A, T.C - T.A);

		Vec3 apc = Cross(P - T.A, T.C - P);
		Vec3 bcp = Cross(T.C - T.B, P - T.C);
		Vec3 abp = Cross(T.B - T.A, P - T.B);
		if (Dot(N, apc) < 0)
		{
			return false;
		}

		if (Dot(N, bcp) < 0)
		{
			return false;
		}

		if (Dot(N, abp) < 0)
		{
			return false;
		}

		return true;
	}

	Collision CollideLineAndTriangle(const Triangle& T, cr_vec3 start, cr_vec3 direction)
	{
		// The line will penetrate the plane of the triangle at parameter t,
		// give that the line is parameterized as P + D.t where P is the start point and D is the direction

		// if I is the point of intersection = (P + D.t) then (A-I).N = 0
		// Expand to give (A - (P + D.t)).N = 0, giving  (A-P).N - D.Nt = 0
		// => (A - P).N = D.Nt. Ergo t = (A - P).N / D.N

		Vec3 N = Cross(T.B - T.A, T.C - T.A);

		float denominator = Dot(N, direction);
		const float EPSILON = 0.0000001f;

		if (fabsf(denominator) <= EPSILON)
		{
			return NoCollision();
		}

		Collision collision;
		collision.t = Dot(T.A - start, direction);
		collision.touchPoint = start + direction * collision.t;
		collision.contactType = ContactType_Face;
		collision.isDegenerate = false;

		if (IsPointInTriangle(collision.touchPoint, T))
		{
			return collision;
		}
		else
		{
			return NoCollision();
		}
	}

	Collision CollideEdgeAndSphere(const Edge& edge, const Sphere& sphere, cr_vec3 target)
	{
		// First of all, if the sphere penetrates the edge, then a collision is deemed to be at time zero
		// and the collision point is the nearest point of the edge to the centre of the sphere
		// Every point X on a line is parameterized by k: X(k) = P + Qk
		// Distance squared to centre A is (P + Qk - A).(P + Qk - A) = A.A + P.P - 2.P.A + Q.Q.k^2 + 2.(P-A).Q.k
		// Turning point at dS/dk = 2Q.Q.k + 2(P-A).Q = 0
		// k = (A-P).Q / Q.Q

		  // Any point G on surface of sphere centred at C is such that: (G - C)(G - C) = R*R, where R is radius of sphere

		  // target defines line with formula: C(t) = A + (B - A).t  = A + D.t, define D = B - A
		  // Point P is any point on edge, and Q is direction of edge. Any point on edge is P + Qu
		  // if O is nearest point on line to sphere, (O - P).(O - C) = 0 then (P + uQ - C).(P + uQ - P) = 0
		  // (P + uQ - C).uQ = 0 => (P + uQ - C).Q = 0
		  // P.Q + uQ.Q - C.Q = 0
		  // u = (C.Q - P.Q) / Q.Q

		  // u = (A - P + D.t).Q / Q.Q = k.t + k0
		  // u = k.t + k0
		  // (O - C)(O - C) = R*R
		  // (P + Qu - A - D.t).(P + Qu - A - D.t) - R*R = 0
		  // Replace u with t: 
		  // ((P - A + Q.k0) + (Q.k - D).t).((P - A + Q.k0) + (Q.k - D).t) - R*R = 0
		  // Define S = P - A + Q.k0 and T = Q.k - D
		  // (S + Tt).(S + Tt) - R*R = 0
		  // T*T.t*t + 2S.Tt + S.S - R*R = 0

		  // If the roots are real, gives two roots of t that give intersect of line with sphere

		cr_vec3 A = sphere.centre;
		cr_vec3 P = edge.a;
		cr_vec3 Q = edge.b - edge.a;
		Vec3 D = target - A;

		float QQ = LengthSq(Q);
		if (QQ == 0)
		{
			return Collision{ P, 0, ContactType_None, true };
		}

		float ooQQ = 1.0f / QQ;

		float k0 = ((A - P) * Q) * ooQQ;

		Vec3 nearestPoint = P + Q * k0;
		Vec3 S = nearestPoint - A;

		float k = (D * Q) * ooQQ;

		Vec3 T = Q * k - D;

		float a = T*T;
		float b = 2.0f *S*T;
		float c = S*S - Square(sphere.radius);

		if (c < 0)
		{
			// We start out with penetration of the infinite line
			if (k0 >= 0 && k0 <= 1)
			{
				// The nearest point is between the two end vertices
				return  Collision{ nearestPoint, 0, ContactType_Penetration, false };
			}

			float dsA_2 = LengthSq(edge.a - A);
			float dsB_2 = LengthSq(edge.b - A);

			if (dsA_2 < Sq(sphere.radius) || dsB_2 < Sq(sphere.radius))
			{
				if (dsA_2 > dsB_2)
				{
					return Collision{ edge.b, 0, ContactType_Vertex , false };
				}
				else
				{
					return Collision{ edge.a, 0, ContactType_Vertex , false };
				}
			}
			else
			{
				return Collision{ P, 0, ContactType_None, false };
			}
		}

		float t0, t1;
		if (!TryGetRealRoots(t0, t1, a, b, c))
		{
			return NoCollision();
		}

		float u0 = k0 + k * t0;
		float u1 = k0 + k * t1;

		if (u0 < 0 || u0 > 1)
		{
			t0 = 1.0e20f;
		}

		if (u1 < 0 || u1 > 1)
		{
			t1 = 1.0e20f;
		}

		if (t0 > t1) std::swap(t0, t1);

		if (t0 == 1.0e20f)
		{
			return NoCollision();
		}

		if (t0 < 0)
		{
			return NoCollision();
		}

		float u = k0 + k * t0;

		Vec3 collisionPoint = edge.a + Q * u;

		return Collision
		{
			collisionPoint,
			t0,
			ContactType_Edge,
			false
		};
	}

	Collision CollideVertexAndSphere(cr_vec3 v, const Sphere& sphere, cr_vec3 target)
	{
		// Sphere centre C(t) = A + Dt, where A is start, and D is target - start
		// Distance squared to vertex = (C - v).(C - v)
		// When sphere comes in contact with point distance = radius, giving

		// C.C - 2C.v + v.v - R*R = 0
		// (A + Dt) (A + Dt) - 2(A + Dt).v + v.v - R.R = 0
		// D.Dt.t + 2(A.D - D.v)t - 2A.v + A.A + v.v - R.R = 0

		cr_vec3 A = sphere.centre;
		Vec3 D = target - A;
		float R = sphere.radius;

		float a = D * D;
		float b = 2.0f * (A*D - D*v);
		float c = -2.0f * A*v + A*A + v*v - R*R;

		float t0, t1;
		if (!TryGetRealRoots(t0, t1, a, b, c))
		{
			return NoCollision();
		}

		float t = std::min(t0, t1);

		return Collision
		{
			v,
			t,
			ContactType_Vertex,
			false
		};
	}

	Collision CollideBoundingBoxAndSphere(const BoundingCube& cube, const Sphere& sphere, cr_vec3 target)
	{
		Collision faceCollision = CollideOnFaces(cube, sphere, target);
		if (faceCollision.t < 1.0f)
		{
			return faceCollision;
		}

		struct : IEnumerator<Edge>
		{
			float t0 = -1.0e20f;
			float t1 = 1.0e20f;
			Collision edgeCollision = NoCollision();
			Sphere sphere;
			Vec3 target;

			virtual void operator()(const Edge& edge)
			{
				Collision col = CollideEdgeAndSphere(edge, sphere, target);
				if (col.contactType == ContactType_Edge && col.t >= 0 && col.t < edgeCollision.t)
				{
					edgeCollision = col;
				}
			}
		} collideWithEdge;

		collideWithEdge.sphere = sphere;
		collideWithEdge.target = target;

		ForEachEdge(cube, collideWithEdge);

		if (collideWithEdge.edgeCollision.contactType != ContactType_None)
		{
			return collideWithEdge.edgeCollision;
		}

		struct : IEnumerator<Vec3>
		{
			float t0 = -1.0e20f;
			float t1 = 1.0e20f;
			Collision vCollision = NoCollision();
			Sphere sphere;
			Vec3 target;

			virtual void operator()(const Vec3& v)
			{
				Collision col = CollideVertexAndSphere(v, sphere, target);
				if (col.t >= 0 && col.t < vCollision.t)
				{
					vCollision = col;
				}
			}
		} collideWithVertex;

		collideWithVertex.sphere = sphere;
		collideWithVertex.target = target;

		ForEachVertex(cube, collideWithVertex);

		return collideWithVertex.vCollision;
	}

	bool GetLineIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d, float& t, float& u)
	{
		// let F = b-a, and G = d-c
		// P(t) = a + F.t
		// Q(u) = c + G.u

		// At intersect P(t)=Q(u)
		// a + F.t = c + G.u

		// ax + Fx.t = cx + Gx.u
		// ay + Fy.t = cy + Gy.u

		// Fy.ax + FxFy.t = Fy.cx + Fy.Gx.u
		// Fx.ay + FxFy.t = Fx.cy + Fx.Gy.u

		// Fx.ay - Fy.ax = Fx.cy - Fy.cx + u(Fx.Gy - Fy.Gx)
		// [ Fx(ay - cy) + Fy(cx - ax) ] / (Fx.Gy - Fy.Gx) = u

		float Fx = b.x - a.x;
		float Fy = b.y - a.y;

		float Gx = d.x - c.x;
		float Gy = d.y - c.y;

		float LHS = Fx * (a.y - c.y) + Fy * (c.x - a.x);
		float RHS = Fx*Gy - Fy*Gx;

		const float epsilon = 0.001f;

		if (RHS > -epsilon && RHS < epsilon)
		{
			u = 0;
			t = 0;
			return false;
		}

		u = LHS / RHS;

		if (Fx != 0)
		{
			t = ((c.x - a.x) + Gx * u) / Fx;
		}
		else
		{
			t = ((c.y - a.y) + Gy * u) / Fy;
		}

		return true;
	}

	bool DoParallelLinesIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d)
	{
		// Two parallel line segments only intersect if they are part of the same infinite line

		float Fx = b.x - a.x;
		float Fy = b.y - a.y;

		float Gx = d.x - c.x;
		float Gy = d.y - c.y;

		// P(t) = a + F.t
		// Q(u) = c + G.t

		const float epsilon = 0.001f;

		if ((Fx == 0 && Gx != 0) || (Fx != 0 && Gx == 0))
		{
			return false;
		}

		struct ANON
		{
			static bool Intersects_0_1(float u0, float u1)
			{
				const float epsilon = 0.001f;
				if ((u0 <= epsilon && u1 >= 1.0f + epsilon) || (u0 >= 1.0f + epsilon && u1 <= epsilon))
				{
					// ab is a subset of cd
					return true;
				}
				else if (u0 >= -epsilon && u0 <= 1.0f + epsilon)
				{
					// Point c is in the line segment ab
					return true;
				}
				else if (u1 >= -epsilon && u1 <= 1.0f + epsilon)
				{
					// Point d is in the line segment ab
					return true;
				}
				else
				{
					return false;
				}
			}
		};

		if (Fx == 0)
		{
			// Two vertical line segments
			if (a.x != c.x)
			{
				return false;
			}

			// Establish u0 at t = 0 and t = 1
			float u0 = (a.y - c.y) / Gy;
			float u1 = (b.y - c.y) / Gy;

			return ANON::Intersects_0_1(u0, u1);
		}
		else
		{
			float gradF = Fy / Fx;
			float gradG = Gy / Gx;

			if (fabsf(gradF - gradG) > epsilon)
			{
				return false;
			}

			if (Gy == 0)
			{
				if (a.y != c.y)
				{
					return false;
				}
			}
			else
			{
				// If the line segments are part of the same line then a = c + Gu
				Vec2 ca = c - a; // = Gu
				float uX = ca.x / Gx;
				float uY = ca.y / Gy;

				if (fabsf(uX - uY) > epsilon)
				{
					return false;
				}
			}

			float u0 = (a.x - c.x) / Gx;
			float u1 = (b.x - c.x) / Gx;

			return ANON::Intersects_0_1(u0, u1);
		}
	}

	IntersectCounts CountLineIntersects(Vec2 origin, Vec2 direction, Vec2 a, Vec2 b)
	{
		IntersectCounts counts = { 0 };

		float t, u;
		if (GetLineIntersect(a, b, origin, origin + direction, t, u))
		{
			if (t >= 0 && t <= 1)
			{
				// Intersection occurs within segment
				if (u > 0)
				{
					counts.forwardCount++;
				}
				else if (u < 0)
				{
					counts.backwardCount++;
				}
			}
		}
		else
		{
			// Co-incident lines
			if (b != origin) counts.coincidence++;
		}

		return counts;
	}

	// Assumes direction normalized. Check out cone.vs.sphere.png
	// There is a cone and back-cone, and one root of t for each
	ConeCheck GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(cr_vec3 eye, cr_vec3 dir, Radians coneAngle, cr_vec3 pos)
	{
		// A sphere touches the cone and its normal on the cone intersects the central line
		// If the line is parameterized by t, where
		// P(t) = E + D.t  where E is the eye of the cone and D is its normalized direction
		// then (C - P(t)) . D = |C-P(t)|sin (theta)

		// [(C - E - D.t).D]^2 = [C - E - D.t].[C - E - D.t] sin*2(theta)

		// Define A = C - E and s = sin^2(theta)
		// [A - D.t].D * [A - D.t].D = [A - D.t].[A - D.t].s

		// (A.D - t)(A.D - t) = A.As - 2A.Dst + t^2.s

		// Define B = A.D
		// (B - t)(B - t) = A.As - 2Bst + t^2s

		// B.B + t^2 - 2B.t = A.As - 2Bst + t^2s

		// t^2(1 - s) - 2B(1 - s)t + (B.B - A.As) = 0

		// Get two roots t0 and t1

		float s = Sq(Sin(coneAngle)); // N.B gives 0 <= s <= 1, positive or zero
		float a = 1.0f - s;

		Vec3 A = pos - eye;

		float B = Dot(A, dir);

		float b = -2.0f * B * a;

		float c = B * B - Dot(A, A) * s;

		float t0, t1;
		if (!TryGetRealRoots(t0, t1, a, b, c))
		{
			// Sometimes small rounding errors prevent the solution, so handle case 4ac slightly larger than b squared
			float delta = Sq(b) - 4.0f * a * c;
			if (delta < -0.1f)
			{
				// Can't think of genuine case when there are no solutions to the problem
				Throw(0, "Could not determine t for cone equation.\n eye (%f, %f %f),\n dir (%f, %f, %f),\n coneAngle (%f),\n pos (%f, %f, %f). Delta: %f", eye.x, eye.y, eye.z, dir.x, dir.y, dir.z, coneAngle, pos.x, pos.y, pos.z, delta);
			}

			// Assume delta is zero then root is -b / 2a
			t0 = t1 = -b / (2.0f * a);
		}

		if (t0 > t1) std::swap(t0, t1);
		float midpoint = 0.5f * (t0 + t1);

		return  ConeCheck{ (midpoint < 0) ? t0 : t1 };
	}

	bool IsOutsideCone(cr_vec3 eye, cr_vec3 dir, Radians coneAngle, const Sphere& sphere)
	{
		// A sphere touches the cone and its normal on the cone intersects the central line
		// If the line is parameterized by t, where
		// P(t) = E + D.t  where E is the eye of the cone and D is its normalized direction
		// then (C - P(t)) . D = |C-P(t)|sin (theta)

		// Distance from E to P(t) is Q(t) = Root(P(t) - E).(P(t) - E)

		// Distance from cone to P(t) along line is F(t) = Q(t)sin(coneAngle)

		// Distance from sphere to P(t) is G(t) = Root(C - P(t))

		// If G(t) > F(f) + radius then sphere is outside code

		ConeCheck cc = GetLineParameterAlongConeJoiningLineToPointAndCrossingNearestPointOnConeToPoint(eye, dir, coneAngle, sphere.centre);
		if (cc.t > 0)
		{
			
			float spineLength = cc.t;
			float coneAdjacentLen = spineLength * Sin(coneAngle);

			float touchLen = coneAdjacentLen + sphere.radius;

			float linePtToCentreSq = LengthSq(sphere.centre - (eye + dir * cc.t));

			return linePtToCentreSq > Sq(touchLen);
		}
		else
		{
			return LengthSq(eye - sphere.centre) < Sq(sphere.radius);
		}
	}
}
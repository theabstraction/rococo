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
			Throw(0, L"Bad normals: Normalize plane normal vectors.");
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
			Throw(0, L"Bad normals in Intersect(const Plane& p, const Plane& q)->Line");
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
		processEdges({ q.v.sw, q.v.se});
		processEdges({ q.v.se, q.v.ne });
		processEdges({ q.v.ne, q.v.nw});
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

	Collision CollideEdgeAndSphere(const Edge& edge, const Sphere& sphere, cr_vec3 target)
	{
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

		float ooQQ = 1.0f / (Q*Q);

		float k0 = ((A - P) * Q) * ooQQ;
		float k = (D * Q) * ooQQ;

		Vec3 S = P - A + Q * k0;
		Vec3 T = Q * k - D;

		float a = T*T;
		float b = 2.0f *S*T;
		float c = S*S - Square(sphere.radius);

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
				if (col.t >= 0 && col.t < edgeCollision.t)
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
}
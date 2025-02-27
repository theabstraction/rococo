#include <rococo.mplat.h>
#include <vector>
#include <math.h>

using namespace Rococo;
using namespace Rococo::Graphics;

void SetPurpose(SoftBoxQuad& q, ESoftBoxVertexPurpose purpose)
{
	q.a.purpose = q.b.purpose = q.c.purpose = q.d.purpose = purpose;
}

class SoftBoxBuilder : public ISoftBoxBuilderSupervisor
{
	float innerWidth;
	float innerBreadth;
	float zTop;
	float uvScale = 1.0f;
	float zBottom;

	std::vector<SoftBoxTriangle> triangles;
	std::vector<SoftBoxQuad> quads;

	std::vector<Vec2> edge;
	std::vector<Vec2> edgeNormal;

	void AddCircleQuadrant(int divisions, Vec2 origin, Vec2 centreToEdge, Vec2 normal, float radius, ESoftBoxVertexPurpose purpose)
	{
		// Theta is the angle from the horizontal to the vertical. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / divisions };
		Degrees theta0 = 0_degrees;

		float zAxis = Cross(centreToEdge, normal);

		SoftBoxTriangle t = { 0 };

		SoftBoxVertex& a = zAxis > 0 ? t.a : t.c;
		SoftBoxVertex& b = t.b;
		SoftBoxVertex& c = zAxis > 0 ? t.c : t.a;

		for (int i = 0; i < divisions; ++i)
		{
			Degrees theta1 = Degrees{ theta0.degrees + dTheta.degrees };

			float cosTheta0 = Cos(theta0);
			float sinTheta0 = Sin(theta0);

			float cosTheta1 = Cos(theta1);
			float sinTheta1 = Sin(theta1);

			Vec2 da = centreToEdge * (radius * cosTheta1);
			Vec2 dc = centreToEdge * (radius * cosTheta0);

			a.pos = { origin.x + da.x, origin.y + da.y, zTop - radius * (1 - sinTheta1) };
			b.pos = { origin.x, origin.y, zTop - radius };
			c.pos = { origin.x + dc.x, origin.y + dc.y, zTop - radius * (1 - sinTheta0) };
			a.normal = b.normal = c.normal = { normal.x, normal.y, 0 };
			a.uv = { uvScale * radius * cosTheta1, uvScale * t.a.pos.z };
			b.uv = { 0                           , uvScale * t.b.pos.z };
			c.uv = { uvScale * radius * cosTheta0, uvScale * t.c.pos.z };

			t.a.purpose = t.b.purpose = t.c.purpose = purpose;

			triangles.push_back(t);

			theta0 = theta1;
		}
	}

	void AddCircleQuadrantCorner(float x, float y, Vec2 leftDirCentreToEdge, Vec2 leftNormal, Vec2 rightDirCentreToEdge, Vec2 rightNormal, int leftDivisions, float leftRadius, int rightDivisions, float rightRadius, ESoftBoxVertexPurpose purpose)
	{
		if (rightDivisions <= 0 && leftDivisions > 0)
		{
			AddCircleQuadrant(leftDivisions, {x,y}, leftDirCentreToEdge, leftNormal, leftRadius, purpose);
		}
		else if (leftDivisions <= 0 && rightDivisions > 0)
		{
			AddCircleQuadrant(rightDivisions, {x,y}, rightDirCentreToEdge, rightNormal, rightRadius, purpose);
		}
	}

	// Mesh a top corner as a sphere octant, joining two edge cylinders
	void AddSphereOctantCorner(float x, float y, float xDir, float yDir, int edgeDivisions, float edgeRadius, ESoftBoxVertexPurpose purpose)
	{
		if (fabsf(xDir) != 1.0f)
		{
			Throw(0, "%s: Abs(xDir) != 1.0", __FUNCTION__);
		}

		if (fabsf(yDir) != 1.0f)
		{
			Throw(0, "%s: Abs(yDir) != 1.0", __FUNCTION__);
		}

		// Theta is the angle from the horizontal, 0 is in the z plane. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / edgeDivisions };

		// Phi of 0 to phi of 90 measures out the horizontal arc
		Degrees dPhi{ 90.0f / edgeDivisions };

		Degrees theta0 = 0_degrees;

		float R = edgeRadius;

		SoftBoxQuad quad = { 0 };

		// Choose vertex order to ensure anti-clockwise chirality of visible triangles
		SoftBoxVertex& a = (xDir * yDir == 1.0f) ? quad.b : quad.a;
		SoftBoxVertex& b = (xDir * yDir == 1.0f) ? quad.a : quad.b;
		SoftBoxVertex& c = (xDir * yDir == 1.0f) ? quad.d : quad.c;
		SoftBoxVertex& d = (xDir * yDir == 1.0f) ? quad.c : quad.d;

		for (int32 i = 0; i < edgeDivisions; i++)
		{
			Degrees phi0 = 0_degrees;
			Degrees theta1{ theta0.degrees + dTheta.degrees };

			for (int32 j = 0; j < edgeDivisions; j++)
			{
				Degrees phi1 = { phi0.degrees + dPhi.degrees };
				float cosTheta1 = Cos(theta1);
				float costTheta0 = Cos(theta0);
				float sinTheta1 = Sin(theta1);
				float sinTheta0 = Sin(theta0);
				float cosPhi0 = Cos(phi0);
				float cosPhi1 = Cos(phi1);
				float sinPhi0 = Sin(phi0);
				float sinPhi1 = Sin(phi1);

				float R1 = R * cosTheta1;
				float R0 = R * costTheta0;

				float R0cosPhi0 = R0 * cosPhi0;
				float R0cosPhi1 = R0 * cosPhi1;
				float R1cosPhi0 = R1 * cosPhi0;
				float R1cosPhi1 = R1 * cosPhi1;

				float R0sinPhi0 = R0 * sinPhi0;
				float R0sinPhi1 = R0 * sinPhi1;
				float R1sinPhi0 = R1 * sinPhi0;
				float R1sinPhi1 = R1 * sinPhi1;

				float z1 = zTop - R + R * sinTheta1;
				float z0 = zTop - R + R * sinTheta0;

				a.pos = { x + xDir * R1cosPhi1, y + yDir * R1sinPhi1, z1 };
				b.pos = { x + xDir * R1cosPhi0, y + yDir * R1sinPhi0, z1 };
				c.pos = { x + xDir * R0cosPhi1, y + yDir * R0sinPhi1, z0 };
				d.pos = { x + xDir * R0cosPhi0, y + yDir * R0sinPhi0, z0 };

				a.normal = { xDir * cosPhi1 * cosTheta1, yDir * sinPhi1 * cosTheta1, sinTheta1 };
				b.normal = { xDir * cosPhi0 * cosTheta1,  yDir * sinPhi0 * cosTheta1, sinTheta1 };
				c.normal = { xDir * cosPhi1 * costTheta0,  yDir * sinPhi1 * costTheta0, sinTheta0 };
				d.normal = { xDir * cosPhi0 * costTheta0,  yDir * sinPhi0 * costTheta0, sinTheta0 };

				a.uv = { 0, 1 };
				b.uv = { 1, 1 };
				c.uv = { 1, 0 };
				d.uv = { 1, 1 };

				SetPurpose(quad, purpose);

				quads.push_back(quad);

				phi0 = phi1;
			}


			theta0 = theta1;
		}
	}

	void AddNorthEdge(float x0, float x1, float y1, const SoftBoxTopSpec& spec)
	{
		if (spec.northEdgeDivisions > 0)
		{
			Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

			Degrees theta0 = 0_degrees;

			for (int32 i = 0; i < spec.northEdgeDivisions; i++)
			{
				Degrees theta1{ theta0.degrees + dTheta.degrees };

				float RsinTheta1 = (i == spec.northEdgeDivisions - 1) ? spec.northRadius : spec.northRadius * Sin(theta1);
				float ROMcosTheta1 = (i == spec.northEdgeDivisions - 1) ? spec.northRadius : spec.northRadius * (1.0f - Cos(theta1));
				float RsinTheta0 = spec.northRadius * Sin(theta0);
				float ROMcosTheta0 = spec.northRadius * (1.0f - Cos(theta0));

				SoftBoxQuad northEdgeQuad;
				northEdgeQuad.a.pos = { x0, y1 + RsinTheta1, zTop - ROMcosTheta1 };
				northEdgeQuad.b.pos = { x1, y1 + RsinTheta1, zTop - ROMcosTheta1 };
				northEdgeQuad.c.pos = { x0, y1 + RsinTheta0, zTop - ROMcosTheta0 };
				northEdgeQuad.d.pos = { x1, y1 + RsinTheta0, zTop - ROMcosTheta0 };

				Radians radsTheta1 = theta1;
				Radians radsTheta0 = theta0;

				// Path length across arc of a cylinder is rads * radius, so we use that with uv scale to generate tex co-ordindates
				northEdgeQuad.a.uv = { x0 * uvScale, (y1 + radsTheta1.radians * spec.northRadius * uvScale) };
				northEdgeQuad.b.uv = { x1 * uvScale, (y1 + radsTheta1.radians * spec.northRadius * uvScale) };
				northEdgeQuad.c.uv = { x0 * uvScale, (y1 + radsTheta0.radians * spec.northRadius * uvScale) };
				northEdgeQuad.d.uv = { x1 * uvScale, (y1 + radsTheta0.radians * spec.northRadius * uvScale) };

				northEdgeQuad.a.normal = northEdgeQuad.b.normal = { 0, Sin(theta1), Cos(theta1) };
				northEdgeQuad.c.normal = northEdgeQuad.d.normal = { 0, Sin(theta0), Cos(theta0) };

				SetPurpose(northEdgeQuad, ESoftBoxVertexPurpose::NorthTop);

				quads.push_back(northEdgeQuad);

				theta0 = theta1;
			}
		}
		else
		{
			SoftBoxQuad quad;
			quad.a.pos = { x1, y1, zTop };
			quad.b.pos = { x0, y1, zTop };
			quad.c.pos = { x1, y1, zBottom };
			quad.d.pos = { x0, y1, zBottom };

			quad.a.uv = { x1 * uvScale, zTop * uvScale };
			quad.b.uv = { x0 * uvScale, zTop * uvScale };
			quad.c.uv = { x1 * uvScale, zBottom * uvScale };
			quad.d.uv = { x0 * uvScale, zBottom * uvScale };

			quad.a.normal = quad.b.normal = quad.c.normal = quad.d.normal = { 0.0f, 1.0f, 0.0f };

			SetPurpose(quad, ESoftBoxVertexPurpose::NorthTop);

			quads.push_back(quad);
		}
	}

	void AddSouthEdge(float x0, float x1, float y0, const SoftBoxTopSpec& spec)
	{
		if (spec.southEdgeDivisions > 0)
		{
			Degrees dTheta{ 90.0f / spec.southEdgeDivisions };

			Degrees theta0 = 0_degrees;

			for (int32 i = 0; i < spec.southEdgeDivisions; i++)
			{
				Degrees theta1{ theta0.degrees + dTheta.degrees };

				float RsinTheta1 = (i == spec.southEdgeDivisions - 1) ? spec.southRadius : spec.southRadius * Sin(theta1);
				float ROMcosTheta1 = (i == spec.southEdgeDivisions - 1) ? spec.southRadius : spec.southRadius * (1.0f - Cos(theta1));
				float RsinTheta0 = spec.southRadius * Sin(theta0);
				float ROMcosTheta0 = spec.southRadius * (1.0f - Cos(theta0));

				SoftBoxQuad southEdgeQuad;
				southEdgeQuad.a.pos = { x0, y0 - RsinTheta0, zTop - ROMcosTheta0 };
				southEdgeQuad.b.pos = { x1, y0 - RsinTheta0, zTop - ROMcosTheta0 };
				southEdgeQuad.c.pos = { x0, y0 - RsinTheta1, zTop - ROMcosTheta1 };
				southEdgeQuad.d.pos = { x1, y0 - RsinTheta1, zTop - ROMcosTheta1 };

				Radians radsTheta1 = theta1;
				Radians radsTheta0 = theta0;

				southEdgeQuad.a.uv = { x0 * uvScale, (y0 + radsTheta0 * spec.southRadius) * uvScale };
				southEdgeQuad.b.uv = { x1 * uvScale, (y0 + radsTheta0 * spec.southRadius) * uvScale };
				southEdgeQuad.c.uv = { x0 * uvScale, (y0 + radsTheta1 * spec.southRadius) * uvScale };
				southEdgeQuad.d.uv = { x1 * uvScale, (y0 + radsTheta1 * spec.southRadius) * uvScale };

				southEdgeQuad.a.normal = southEdgeQuad.b.normal = { 0, -Sin(theta0), Cos(theta0) };
				southEdgeQuad.c.normal = southEdgeQuad.d.normal = { 0, -Sin(theta1), Cos(theta1) };

				SetPurpose(southEdgeQuad, ESoftBoxVertexPurpose::SouthTop);

				quads.push_back(southEdgeQuad);

				theta0 = theta1;
			}
		}
		else
		{
			SoftBoxQuad quad;
			quad.a.pos = { x0, y0, zTop };
			quad.b.pos = { x1, y0, zTop };
			quad.c.pos = { x0, y0, zBottom };
			quad.d.pos = { x1, y0, zBottom };

			quad.a.uv = { x0 * uvScale, zTop * uvScale };
			quad.b.uv = { x1 * uvScale, zTop * uvScale };
			quad.c.uv = { x0 * uvScale, zBottom * uvScale };
			quad.d.uv = { x1 * uvScale, zBottom * uvScale };

			quad.a.normal = quad.b.normal = quad.c.normal = quad.d.normal = { 0.0f, -1.0f, 0.0f };

			SetPurpose(quad, ESoftBoxVertexPurpose::SouthTop);

			quads.push_back(quad);
		}
	}

	void AddWestEdge(float x0, float y0, float y1, const SoftBoxTopSpec& spec)
	{
		if (spec.westEdgeDivisions > 0)
		{
			Degrees dTheta{ 90.0f / spec.westEdgeDivisions };
			Degrees theta0 = 0_degrees;

			for (int32 i = 0; i < spec.westEdgeDivisions; i++)
			{
				Degrees theta1{ theta0.degrees + dTheta.degrees };

				float RsinTheta1 = (i == spec.westEdgeDivisions - 1) ? spec.westRadius : spec.westRadius * Sin(theta1);
				float ROMcosTheta1 = (i == spec.westEdgeDivisions - 1) ? spec.westRadius : spec.westRadius * (1.0f - Cos(theta1));
				float RsinTheta0 = spec.westRadius * Sin(theta0);
				float ROMcosTheta0 = spec.westRadius * (1.0f - Cos(theta0));

				SoftBoxQuad westEdgeQuad;
				westEdgeQuad.a.pos = { x0 - RsinTheta1,  y1, zTop - ROMcosTheta1 };
				westEdgeQuad.b.pos = { x0 - RsinTheta0,  y1, zTop - ROMcosTheta0 };
				westEdgeQuad.c.pos = { x0 - RsinTheta1,  y0, zTop - ROMcosTheta1 };
				westEdgeQuad.d.pos = { x0 - RsinTheta0,  y0, zTop - ROMcosTheta0 };

				Radians radsTheta1 = theta1;
				Radians radsTheta0 = theta0;

				westEdgeQuad.a.uv = { (x0 - radsTheta1 * spec.westRadius) * uvScale, y1 * uvScale };
				westEdgeQuad.b.uv = { (x0 - radsTheta0 * spec.westRadius) * uvScale, y1 * uvScale };
				westEdgeQuad.c.uv = { (x0 - radsTheta1 * spec.westRadius) * uvScale, y0 * uvScale };
				westEdgeQuad.d.uv = { (x0 - radsTheta0 * spec.westRadius) * uvScale, y0 * uvScale };

				westEdgeQuad.a.normal = westEdgeQuad.c.normal = { -Sin(theta1), 0, Cos(theta1) };
				westEdgeQuad.b.normal = westEdgeQuad.d.normal = { -Sin(theta0), 0, Cos(theta0) };

				SetPurpose(westEdgeQuad, ESoftBoxVertexPurpose::WestTop);

				quads.push_back(westEdgeQuad);

				theta0 = theta1;
			}
		}
		else
		{
			SoftBoxQuad quad;
			quad.a.pos = { x0, y1, zTop };
			quad.b.pos = { x0, y0, zTop };
			quad.c.pos = { x0, y1, zBottom };
			quad.d.pos = { x0, y0, zBottom };

			quad.a.uv = { y1 * uvScale, zTop * uvScale };
			quad.b.uv = { y0 * uvScale, zTop * uvScale };
			quad.c.uv = { y1 * uvScale, zBottom * uvScale };
			quad.d.uv = { y0 * uvScale, zBottom * uvScale };

			quad.a.normal = quad.b.normal = quad.c.normal = quad.d.normal = { -1.0f, 0.0f, 0.0f };

			SetPurpose(quad, ESoftBoxVertexPurpose::WestTop);

			quads.push_back(quad);
		}
	}

	void AddEastEdge(float x1, float y0, float y1, const SoftBoxTopSpec& spec)
	{
		if (spec.eastEdgeDivisions > 0)
		{
			Degrees dTheta{ 90.0f / spec.eastEdgeDivisions };
			Degrees theta0 = 0_degrees;

			for (int32 i = 0; i < spec.eastEdgeDivisions; i++)
			{
				Degrees theta1 = { theta0.degrees + dTheta.degrees };

				float RsinTheta1 = (i == spec.eastEdgeDivisions - 1) ? spec.eastRadius : spec.eastRadius * Sin(theta1);
				float ROMcosTheta1 = (i == spec.eastEdgeDivisions - 1) ? spec.eastRadius : spec.eastRadius * (1.0f - Cos(theta1));
				float RsinTheta0 = spec.eastRadius * Sin(theta0);
				float ROMcosTheta0 = spec.eastRadius * (1.0f - Cos(theta0));

				SoftBoxQuad eastEdgeQuad;
				eastEdgeQuad.a.pos = { x1 + RsinTheta0, y1, zTop - ROMcosTheta0 };
				eastEdgeQuad.b.pos = { x1 + RsinTheta1, y1, zTop - ROMcosTheta1 };
				eastEdgeQuad.c.pos = { x1 + RsinTheta0, y0, zTop - ROMcosTheta0 };
				eastEdgeQuad.d.pos = { x1 + RsinTheta1, y0, zTop - ROMcosTheta1 };

				Radians radsTheta1 = theta1;
				Radians radsTheta0 = theta0;

				eastEdgeQuad.a.uv = { (x1 + radsTheta1 * spec.westRadius) * uvScale, y1 * uvScale };
				eastEdgeQuad.b.uv = { (x1 + radsTheta0 * spec.westRadius) * uvScale, y1 * uvScale };
				eastEdgeQuad.c.uv = { (x1 + radsTheta1 * spec.westRadius) * uvScale, y0 * uvScale };
				eastEdgeQuad.d.uv = { (x1 + radsTheta0 * spec.westRadius) * uvScale, y0 * uvScale };

				eastEdgeQuad.a.normal = eastEdgeQuad.c.normal = { Sin(theta0), 0, Cos(theta0) };
				eastEdgeQuad.b.normal = eastEdgeQuad.d.normal = { Sin(theta1), 0, Cos(theta1) };

				SetPurpose(eastEdgeQuad, ESoftBoxVertexPurpose::EastTop);

				quads.push_back(eastEdgeQuad);

				theta0 = theta1;
			}
		}
		else
		{
			SoftBoxQuad quad;
			quad.a.pos = { x1, y0, zTop };
			quad.b.pos = { x1, y1, zTop };
			quad.c.pos = { x1, y0, zBottom };
			quad.d.pos = { x1, y1, zBottom };

			quad.a.uv = { y0 * uvScale, zTop * uvScale };
			quad.b.uv = { y1 * uvScale, zTop * uvScale };
			quad.c.uv = { y0 * uvScale, zBottom * uvScale };
			quad.d.uv = { y1 * uvScale, zBottom * uvScale };

			quad.a.normal = quad.b.normal = quad.c.normal = quad.d.normal = { 1.0f, 0.0f, 0.0f };

			SetPurpose(quad, ESoftBoxVertexPurpose::EastTop);

			quads.push_back(quad);
		}
	}

	/* TODO, make this generic for the 4 edges:
	void AddEdge(float x0, float x1, float y0, float y1, int divisions, float radius)
	{
		// Next we add four edges, each forms the top of a cylinder that takes the mesh from the horizontal to the vertical
		Degrees dTheta{ 90.0f / divisions };
		Degrees theta0 = 0_degrees;

		SoftBoxQuad quad;

		for (int32 i = 0; i < divisions; i++)
		{
			Degrees theta1 = { theta0.degrees + dTheta.degrees };

			float RsinTheta1 = (i == divisions - 1) ? radius : radius * Sin(theta1);
			float ROMcosTheta1 = (i == divisions - 1) ? radius : radius * (1.0f - Cos(theta1));
			float RsinTheta0 = radius * Sin(theta0);
			float ROMcosTheta0 = radius * (1.0f - Cos(theta0));

			quad.a.pos = { x0 + RsinTheta0, y1, zTop - ROMcosTheta0 };
			quad.b.pos = { x1 + RsinTheta1, y1, zTop - ROMcosTheta1 };
			quad.c.pos = { x0 + RsinTheta0, y0, zTop - ROMcosTheta0 };
			quad.d.pos = { x1 + RsinTheta1, y0, zTop - ROMcosTheta1 };

			Radians radsTheta1 = theta1;
			Radians radsTheta0 = theta0;

			quad.a.uv = { (x0 + radsTheta1 * radius) * uvScale, y1 * uvScale };
			quad.b.uv = { (x1 + radsTheta0 * radius) * uvScale, y1 * uvScale };
			quad.c.uv = { (x0 + radsTheta1 * radius) * uvScale, y0 * uvScale };
			quad.d.uv = { (x1 + radsTheta0 * radius) * uvScale, y0 * uvScale };

			quad.a.normal = quad.c.normal = { Sin(theta0), 0, Cos(theta0) };
			quad.b.normal = quad.d.normal = { Sin(theta1), 0, Cos(theta1) };

			quads.push_back(quad);

			theta0 = theta1;
		}
	}
	*/

	void AddTop(float x0, float x1, float y0, float y1)
	{
		SoftBoxQuad topQuad;

		topQuad.a.pos = { x0, y1, zTop };
		topQuad.b.pos = { x1, y1, zTop };
		topQuad.c.pos = { x0, y0, zTop };
		topQuad.d.pos = { x1, y0, zTop };

		topQuad.a.uv = { uvScale * x0, uvScale * y1 };
		topQuad.b.uv = { uvScale * x1, uvScale * y1 };
		topQuad.c.uv = { uvScale * x0, uvScale * y0 };
		topQuad.d.uv = { uvScale * x1, uvScale * y0 };

		Vec3 up = { 0, 0, 1.0f };
		topQuad.a.normal = topQuad.b.normal = topQuad.c.normal = topQuad.d.normal = up;

		SetPurpose(topQuad, ESoftBoxVertexPurpose::CentreTop);

		quads.push_back(topQuad);
	}

	Vec2 ScaleUVByXYPos(Vec3 pos)
	{
		return { uvScale * pos.x, uvScale * pos.y };
	}

	void ScaleUVByXYPosAndSetNormal(SoftBoxQuad& q, Vec3 normal)
	{
		q.a.uv = ScaleUVByXYPos(q.a.pos);
		q.b.uv = ScaleUVByXYPos(q.b.pos);
		q.c.uv = ScaleUVByXYPos(q.c.pos);
		q.d.uv = ScaleUVByXYPos(q.d.pos);
		q.a.normal = q.b.normal = q.c.normal = q.d.normal = normal;
	}

	void AddBottom(float x0, float x1, float y0, float y1, float zBottom, float southDelta, float northDelta, float westDelta, float eastDelta)
	{
		/*
		 
		x0      x0 - westDelta     x1 - eastDelta      x1		
		/////////////////////////////////////////////////  yN
		//      //                             //      //
		//      //                             //      //
		//      //                             //      //
		/////////////////////////////////////////////////  y1
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		//                                             //
		/////////////////////////////////////////////////  y0
		//      //                             //      //
		//      //                             //      //
		//      //                             //      //
		//      //                             //      //
		/////////////////////////////////////////////////  yS

		*/
		SoftBoxQuad bottomQuad;

		bottomQuad.a.pos = { x1, y1, zBottom };
		bottomQuad.b.pos = { x0, y1, zBottom };
		bottomQuad.c.pos = { x1, y0, zBottom };
		bottomQuad.d.pos = { x0, y0, zBottom };

		Vec3 down = { 0, 0, -1.0f };

		ScaleUVByXYPosAndSetNormal(bottomQuad, down);

		SetPurpose(bottomQuad, ESoftBoxVertexPurpose::CentreBottom);

		quads.push_back(bottomQuad);

		float yS = y0 - southDelta;

		SoftBoxQuad bottomSouthQuad;
		bottomSouthQuad.a.pos = { x1 - eastDelta, y0, zBottom };
		bottomSouthQuad.b.pos = { x0 + westDelta, y0, zBottom };
		bottomSouthQuad.c.pos = { x1 - eastDelta, yS, zBottom };
		bottomSouthQuad.d.pos = { x0 + westDelta, yS, zBottom };

		ScaleUVByXYPosAndSetNormal(bottomSouthQuad, down);

		quads.push_back(bottomSouthQuad);

		float yN = y1 + northDelta;

		SoftBoxQuad bottomNorthQuad;
		bottomNorthQuad.a.pos = { x1 - eastDelta, yN, zBottom };
		bottomNorthQuad.b.pos = { x0 + westDelta, yN, zBottom };
		bottomNorthQuad.c.pos = { x1 - eastDelta, y1, zBottom };
		bottomNorthQuad.d.pos = { x0 + westDelta, y1, zBottom };

		ScaleUVByXYPosAndSetNormal(bottomNorthQuad, down);

		quads.push_back(bottomNorthQuad);

		SoftBoxTriangle arc;

		if (westDelta)
		{
			arc.a.pos = { x0, y1, zBottom };
			arc.b.pos = { x0 + westDelta, y1, zBottom };
			arc.c.pos = { x0 + westDelta, yN, zBottom };

			arc.a.normal = arc.b.normal = arc.c.normal = down;

			arc.a.uv = { uvScale * arc.a.pos.x, uvScale * arc.a.pos.y };
			arc.b.uv = { uvScale * arc.b.pos.x, uvScale * arc.b.pos.y };
			arc.c.uv = { uvScale * arc.c.pos.x, uvScale * arc.c.pos.y };

			PushTriangle(arc, false);
		}
	}
public:
	// Create a soft edged box top. The top has a constant z value, the front side is defined as facing North (0 1 0),  the right side is defined as facing East (1 0 0)
	void CreateSoftBoxTop(const SoftBoxTopSpec& spec) override
	{
		if (spec.westRadius < 0)
		{
			Throw(0, "%s: The West radius was < 0", __FUNCTION__);
		}

		if (spec.eastRadius < 0)
		{
			Throw(0, "%s: The East radius was < 0", __FUNCTION__);
		}

		if (spec.northRadius < 0)
		{
			Throw(0, "%s: The North radius was < 0", __FUNCTION__);
		}

		if (spec.southRadius < 0)
		{
			Throw(0, "%s: The South radius was < 0", __FUNCTION__);
		}

		innerWidth = spec.width - spec.westRadius - spec.eastRadius;
		if (innerWidth <= 0)
		{
			Throw(0, "%s: The West and East radii were so large that the inner width of the top quad was <= 0", __FUNCTION__);
		}

		innerBreadth = spec.breadth - spec.northRadius - spec.southRadius;
		if (innerBreadth <= 0)
		{
			Throw(0, "%s: The North and South radii were so large that the inner breadth of the top quad was <= 0", __FUNCTION__);
		}

		if (spec.northEdgeDivisions < 0 || spec.northEdgeDivisions > 100'000)
		{
			Throw(0, "%s: North edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.southEdgeDivisions < 0 || spec.southEdgeDivisions > 100'000)
		{
			Throw(0, "%s: South edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.westEdgeDivisions < 0 || spec.westEdgeDivisions > 100'000)
		{
			Throw(0, "%s: West edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.eastEdgeDivisions < 0 || spec.eastEdgeDivisions > 100'000)
		{
			Throw(0, "%s: East edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		zTop = spec.ztop;

		float maxRadius = max(max(max(spec.northRadius, spec.southRadius), spec.westRadius), spec.eastRadius);

		zBottom = zTop - maxRadius;

		float x0 = spec.width   * -0.5f + spec.westRadius;
		float x1 = spec.width   *  0.5f - spec.eastRadius;
		float y0 = spec.breadth * -0.5f + spec.southRadius;
		float y1 = spec.breadth *  0.5f - spec.northRadius;

		AddTop(x0, x1, y0, y1);
		AddNorthEdge(x0, x1, y1, spec);
		AddSouthEdge(x0, x1, y0, spec);
		AddWestEdge(x0, y0, y1, spec);
		AddEastEdge(x1, y0, y1, spec);

		float southDelta = spec.southRadius;
		float northDelta = spec.northRadius;

		AddBottom(-0.5f * spec.width, 0.5f * spec.width, -0.5f * spec.breadth + southDelta, 0.5f * spec.breadth - northDelta, zBottom, southDelta, northDelta, spec.westRadius, spec.eastRadius);

		if (spec.westEdgeDivisions == spec.southEdgeDivisions && spec.westRadius == spec.southRadius)
		{
			AddSphereOctantCorner(x0, y0, -1.0f, -1.0f, spec.westEdgeDivisions, spec.westRadius, ESoftBoxVertexPurpose::SWCorner); // SW
		}
		else
		{
			AddCircleQuadrantCorner(x0, y0, { -1.0f, 0.0f }, { 0.0f, -1.0f }, { 0.0f, -1.0f }, { -1.0f, 0.0f }, spec.westEdgeDivisions, spec.westRadius, spec.southEdgeDivisions, spec.southRadius, ESoftBoxVertexPurpose::SWCorner); // SW
		}

		if (spec.westEdgeDivisions == spec.northEdgeDivisions && spec.westRadius == spec.northRadius)
		{
			AddSphereOctantCorner(x0, y1, -1.0f, 1.0f, spec.westEdgeDivisions, spec.westRadius, ESoftBoxVertexPurpose::NWCorner); // NW
		}
		else
		{
			AddCircleQuadrantCorner(x0, y1, { 0.0f, 1.0f }, { -1.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 1.0f },  spec.northEdgeDivisions, spec.northRadius, spec.westEdgeDivisions, spec.westRadius, ESoftBoxVertexPurpose::NWCorner); // NW
		}
		
		if (spec.northEdgeDivisions == spec.eastEdgeDivisions && spec.eastRadius == spec.northRadius)
		{
			AddSphereOctantCorner(x1, y1, 1.0f, 1.0f, spec.westEdgeDivisions, spec.westRadius, ESoftBoxVertexPurpose::NECorner);  // NE
		}
		else
		{
			AddCircleQuadrantCorner(x1, y1, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f },  spec.eastEdgeDivisions, spec.eastRadius, spec.northEdgeDivisions, spec.northRadius, ESoftBoxVertexPurpose::NECorner); // NE
		}

		if (spec.eastEdgeDivisions == spec.southEdgeDivisions && spec.eastRadius == spec.southRadius)
		{
			AddSphereOctantCorner(x1, y0, 1.0f, -1.0f, spec.westEdgeDivisions, spec.westRadius, ESoftBoxVertexPurpose::SECorner); // SE
		}
		else
		{
			AddCircleQuadrantCorner(x1, y0, { 0, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f }, { 0, -1.0f }, spec.southEdgeDivisions, spec.southRadius, spec.eastEdgeDivisions, spec.eastRadius, ESoftBoxVertexPurpose::SECorner); // SE
		}

		// This may be pertinent around 2050
		if (quads.size() > 0x7FFFFFFFLL || triangles.size() > 0x7FFFFFFFLL)
		{
			Clear();
			Throw(0, "Maximum quad array size exceeded. Clearning soft box builder");
		}
	}

	void PushTriangle(SoftBoxTriangle& t, bool flip)
	{
		if (flip)
		{
			std::swap(t.a, t.b);
		}

		triangles.push_back(t);
	}

	void AddRoundCorner(cr_vec3 position, float radius, int divisions, cr_vec3 left, cr_vec3 right, cr_vec3 normal, bool addEdge, ESoftBoxVertexPurpose purpose)
	{
		Degrees theta0 = 0_degrees;
		Degrees dTheta = { 90.0f / divisions };

		//					left
		//						^ radius
		//		#	#	#	#	|
		//			#	theta	|
		//				#		|
		//					#	|
		//					   #|
		// < right--------------*  position
		
		SoftBoxTriangle t = { 0 };
		SoftBoxVertex& a = t.a;
		SoftBoxVertex& b = t.b;
		SoftBoxVertex& c = t.c;

		Vec3 LTheta0 = left;

		for (int32 i = 0; i < divisions; i++)
		{
			Degrees theta1 = { theta0.degrees + dTheta.degrees };

			Vec3 LTheta1 = left * Cos(theta1) + right * Sin(theta1);

			c.pos = position + (LTheta0 * radius);
			b.pos = position + (LTheta1 * radius);
			a.pos = position;

			a.normal = b.normal = c.normal = normal;

			a.uv = uvScale * Vec2{ a.pos.x, a.pos.y };
			b.uv = uvScale * Vec2{ b.pos.x, b.pos.y };
			c.uv = uvScale * Vec2{ c.pos.x, c.pos.y };

			t.a.purpose = t.b.purpose = t.c.purpose = purpose;
			PushTriangle(t, normal.z < 0);

			if (addEdge && i < divisions - 1)
			{
				edge.push_back(Vec2{ c.pos.x, c.pos.y });
				edgeNormal.push_back(Vec2{ LTheta0.x, LTheta0.y });
				edge.push_back(Vec2{ b.pos.x, b.pos.y });
				edgeNormal.push_back(Vec2{ LTheta1.x, LTheta1.y });
			}

			theta0 = theta1;
			LTheta0 = LTheta1;
		}
	}

	void PushQuad(SoftBoxQuad& q, bool flip)
	{
		if (flip)
		{
			std::swap(q.a, q.b);
			std::swap(q.c, q.d);
		}
		quads.push_back(q);
	}

	void AddHorizontalPlane(const RoundCornersShelfSpec& shelf, float z, cr_vec3 normal, bool addEdge)
	{
		SoftBoxQuad q = { 0 };
		SoftBoxVertex& a = q.a;
		SoftBoxVertex& b = q.b;
		SoftBoxVertex& c = q.c;
		SoftBoxVertex& d = q.d;

		a.pos = { shelf.width * -0.5f, shelf.breadth * 0.5f - max(shelf.radiusNW, shelf.radiusNE), z };
		b.pos = { shelf.width * 0.5f, shelf.breadth * 0.5f - max(shelf.radiusNW, shelf.radiusNE), z };
		c.pos = { shelf.width * -0.5f, shelf.breadth * -0.5f + max(shelf.radiusSE, shelf.radiusSW), z };
		d.pos = { shelf.width * 0.5f, shelf.breadth * -0.5f + max(shelf.radiusSE, shelf.radiusSW), z };

		a.uv = { a.pos.x * uvScale, a.pos.y * uvScale };
		b.uv = { b.pos.x * uvScale, b.pos.y * uvScale };
		c.uv = { c.pos.x * uvScale, c.pos.y * uvScale };
		d.uv = { d.pos.x * uvScale, d.pos.y * uvScale };

		a.normal = b.normal = c.normal = d.normal = normal;

		if (shelf.radiusNW > 0.0f && shelf.divisionsNW > 0)
		{
			AddRoundCorner(a.pos + Vec3{ shelf.radiusNW, 0, 0 }, shelf.radiusNW, shelf.divisionsNW, { 0.0f,1.0f,0 }, { -1.0f, 0.0f, 0.0f }, normal, addEdge, normal.z > 0 ? ESoftBoxVertexPurpose::NWCorner : ESoftBoxVertexPurpose::NWBottom);
		}

		if (addEdge)
		{
			edge.push_back(Vec2{ q.a.pos.x, q.a.pos.y });
			edgeNormal.push_back(Vec2{ -1.0f, 0.0f });
			edge.push_back(Vec2{ q.c.pos.x, q.c.pos.y });
			edgeNormal.push_back(Vec2{ -1.0f, 0.0f });
		}

		if (shelf.radiusSW > 0.0f && shelf.divisionsSW > 0)
		{
			AddRoundCorner(c.pos + Vec3{ shelf.radiusSW, 0, 0 }, shelf.radiusSW, shelf.divisionsSW, { -1.0f,0,0 }, { 0.0f, -1.0f, 0.0f }, normal, addEdge, normal.z > 0 ? ESoftBoxVertexPurpose::SWCorner : ESoftBoxVertexPurpose::SWBottom);
		}

		if (addEdge)
		{
			edge.push_back(Vec2{ c.pos.x + shelf.radiusSW, c.pos.y - shelf.radiusSW });
			edgeNormal.push_back(Vec2{ 0.0f, -1.0f });

			edge.push_back(Vec2{ d.pos.x - shelf.radiusSE, c.pos.y - shelf.radiusSE });
			edgeNormal.push_back(Vec2{ 0.0f, -1.0f });
		}

		if (shelf.radiusSE > 0.0f && shelf.divisionsSE > 0)
		{
			AddRoundCorner(d.pos - Vec3{ shelf.radiusSE, 0, 0 }, shelf.radiusSE, shelf.divisionsSE, { 0.0f, -1.0f,0 }, { 1.0f, 0.0f, 0.0f }, normal, addEdge, normal.z > 0 ? ESoftBoxVertexPurpose::SECorner : ESoftBoxVertexPurpose::SEBottom);
		}

		if (addEdge)
		{
			edge.push_back(Vec2{ d.pos.x, d.pos.y });
			edgeNormal.push_back(Vec2{ 1.0f, 0.0f });

			edge.push_back(Vec2{ b.pos.x, b.pos.y - shelf.radiusNE });
			edgeNormal.push_back(Vec2{ 1.0f, 0.0f });
		}

		if (shelf.radiusNE > 0.0f && shelf.divisionsNE > 0)
		{
			AddRoundCorner(b.pos - Vec3{ shelf.radiusNE, 0, 0 }, shelf.radiusNE, shelf.divisionsNE, { 1.0,0.0,0 }, { 0.0f, 1.0f, 0.0f }, normal, addEdge, normal.z > 0 ? ESoftBoxVertexPurpose::NECorner : ESoftBoxVertexPurpose::NEBottom);
		}

		if (addEdge)
		{
			edge.push_back(Vec2{ b.pos.x - shelf.radiusNE, b.pos.y + shelf.radiusNE });
			edgeNormal.push_back(Vec2{ 0.0f, 1.0f });

			edge.push_back(Vec2{ a.pos.x + shelf.radiusNE, b.pos.y + shelf.radiusNW });
			edgeNormal.push_back(Vec2{ 0.0f, 1.0f });
		}

		SetPurpose(q, normal.z > 0 ? ESoftBoxVertexPurpose::CentreTop : ESoftBoxVertexPurpose::CentreBottom);
		PushQuad(q, normal.z < 0);

		if (shelf.radiusNE > 0.0f && shelf.divisionsNE > 0 && (shelf.radiusNW != 0 || shelf.radiusNE != 0))
		{
			a.pos = { shelf.width * -0.5f + shelf.radiusNW, shelf.breadth * 0.5f, z };
			b.pos = { shelf.width * 0.5f - shelf.radiusNE, shelf.breadth * 0.5f, z };
			c.pos = { shelf.width * -0.5f + shelf.radiusNW, shelf.breadth * 0.5f - shelf.radiusNW,z };
			d.pos = { shelf.width * 0.5f - shelf.radiusNE, shelf.breadth * 0.5f - shelf.radiusNE, z };
			a.uv = uvScale * Vec2{ a.pos.x, a.pos.y };
			b.uv = uvScale * Vec2{ b.pos.x, b.pos.y };
			c.uv = uvScale * Vec2{ c.pos.x, c.pos.y };
			d.uv = uvScale * Vec2{ d.pos.x, d.pos.y };

			SetPurpose(q, normal.z > 0 ? ESoftBoxVertexPurpose::NorthTop : ESoftBoxVertexPurpose::NorthBottom);
			PushQuad(q, normal.z < 0);
		}

		if (shelf.radiusSW != 0 || shelf.radiusSE != 0)
		{
			a.pos = { shelf.width * -0.5f + shelf.radiusSW, shelf.breadth * -0.5f + shelf.radiusSW, z };
			b.pos = { shelf.width * 0.5f - shelf.radiusSE, shelf.breadth * -0.5f + shelf.radiusSE, z };
			c.pos = { shelf.width * -0.5f + shelf.radiusSW, shelf.breadth * -0.5f, z };
			d.pos = { shelf.width * 0.5f - shelf.radiusSE, shelf.breadth * -0.5f, z };
			SetPurpose(q, normal.z > 0 ? ESoftBoxVertexPurpose::SouthTop: ESoftBoxVertexPurpose:: SouthBottom);
			PushQuad(q, normal.z < 0);
		}
	}

	void CreateRoundCornersShelf(const RoundCornersShelfSpec& shelf)
	{
		if (shelf.radiusNW < 0)
		{
			Throw(0, "%s: NW radius < 0", __FUNCTION__);
		}

		if (shelf.radiusNE < 0)
		{
			Throw(0, "%s: NE radius < 0", __FUNCTION__);
		}

		if (shelf.radiusSW < 0)
		{
			Throw(0, "%s: SW radius < 0", __FUNCTION__);
		}

		if (shelf.radiusSE < 0)
		{
			Throw(0, "%s: SE radius < 0", __FUNCTION__);
		}

		if (shelf.divisionsNW < 0)
		{
			Throw(0, "%s: divisionsNE < 0", __FUNCTION__);
		}

		if (shelf.divisionsNE < 0)
		{
			Throw(0, "%s: divisionsNW < 0", __FUNCTION__);
		}

		if (shelf.divisionsSW < 0)
		{
			Throw(0, "%s: divisionsSW < 0", __FUNCTION__);
		}

		if (shelf.divisionsSE < 0)
		{
			Throw(0, "%s: divisionsSE < 0", __FUNCTION__);
		}

		if ((shelf.width < shelf.radiusNW + shelf.radiusNE) || (shelf.width < shelf.radiusSE + shelf.radiusSW))
		{
			Throw(0, "%s: shelf width insufficient for the given radii", __FUNCTION__);
		}

		if ((shelf.breadth < shelf.radiusNW + shelf.radiusNE) || (shelf.width < shelf.radiusSE + shelf.radiusSW))
		{
			Throw(0, "%s: shelf breadth insufficient for the given radii", __FUNCTION__);
		}

		if (shelf.width == 0 || shelf.breadth == 0)
		{
			Throw(0, "%s: width x breadth: area zero", __FUNCTION__);
		}

		Vec3 up = { 0.0f, 0.0f, 1.0f };
		AddHorizontalPlane(shelf, zTop, up, true);

		if (shelf.zBottom < shelf.zTop && shelf.addBottom)
		{
			Vec3 down = { 0.0f, 0.0f, -1.0f };
			AddHorizontalPlane(shelf, zBottom, down, false);
		}
		
		if (shelf.zBottom < shelf.zTop && edge.size() > 0)
		{
			float arcLen = 0;

			SoftBoxQuad q;

			for (size_t i = 0; i < edge.size() - 1; i++)
			{
				const auto& Ei0 = edge[i];
				const auto& Ei1 = edge[i+1];
				q.a.pos = Vec3::FromVec2(Ei0, zTop);
				q.b.pos = Vec3::FromVec2(Ei1, zTop);
				q.c.pos = Vec3::FromVec2(Ei0, zBottom);
				q.d.pos = Vec3::FromVec2(Ei1, zBottom);

				q.a.normal = q.c.normal = Vec3::FromVec2(edgeNormal[i], 0);
				q.b.normal = q.d.normal = Vec3::FromVec2(edgeNormal[i+1], 0);

				float len0 = arcLen;
				float len1 = arcLen + Length(q.b.pos - q.a.pos);
				arcLen = len1;
				
				q.a.uv = { uvScale * len0, 0.0f };
				q.b.uv = { uvScale * len1, 0.0f };
				q.c.uv = { uvScale * len0, uvScale * (zTop - zBottom) };
				q.d.uv = { uvScale * len1, uvScale * (zTop - zBottom) };

				quads.push_back(q);
			}
		}

		// This may be pertinent around 2050
		if (quads.size() > 0x7FFFFFFFLL || triangles.size() > 0x7FFFFFFFLL)
		{
			Clear();
			Throw(0, "Maximum quad array size exceeded. Clearning soft box builder");
		}
	}

	void Clear()
	{
		edge.clear();
		edgeNormal.clear();
		quads.clear();
		triangles.clear();
		uvScale = 1.0f;
	}

	void Free() override
	{
		delete this;
	}

	int32 NumberOfQuads() const override
	{
		return (int32) quads.size();
	}

	void GetQuad(int32 index, Rococo::Graphics::SoftBoxQuad& refQuad) const override
	{
		if (index < 0 || index >= (int32)quads.size())
		{
			Throw(0, "%s: [index=%d] out of bounds. Quad array size is %llu quads", __FUNCTION__, index, quads.size());
		}

		refQuad = quads[index];
	}

	int32 NumberOfTriangles() const override
	{
		return (int32)triangles.size();
	}

	void GetTriangle(int32 index, Rococo::Graphics::SoftBoxTriangle& outTriangle) const override
	{
		if (index < 0 || index >= (int32)triangles.size())
		{
			Throw(0, "%s: [index=%d] out of bounds. Triangle array size is %llu triangles", __FUNCTION__, index, quads.size());
		}

		outTriangle = triangles[index];
	}

	void SetTextureScale(float uvScale) override
	{
		if (uvScale == 0.0f)
		{
			Throw(0, "%s: uv scale must not be zero", __FUNCTION__);
		}

		this->uvScale = uvScale;
	}
};

namespace Rococo::Graphics
{
	ISoftBoxBuilderSupervisor* CreateSoftboxBuilder()
	{
		return new SoftBoxBuilder();
	}
}
#include <rococo.mplat.h>
#include <vector>
#include <math.h>

using namespace Rococo;

class SoftBoxBuilder: public ISoftBoxBuilderSupervisor
{
	float innerWidth;
	float innerBreadth;
	float zTop;
	float uvScale = 1.0f;

	std::vector<Triangle> triangles;
	std::vector<SoftBoxQuad> quads;

	void AddNECorner(float x1, float y1, const SoftBoxTopSpec& spec)
	{
		// Theta is the angle from the horizontal, 0 is in the z plane. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

		// Phi of 0 points West (-1 0 0), Phi of 90 points North
		Degrees dPhi{ 90.0f / spec.northEdgeDivisions };

		Degrees theta0 = 0_degrees;

		float R = spec.northRadius;

		for (int32 i = 0; i < spec.northEdgeDivisions; i++)
		{
			Degrees phi0 = 0_degrees;
			Degrees theta1{ theta0.degrees + dTheta.degrees };

			for (int32 j = 0; j < spec.northEdgeDivisions; j++)
			{
				float R1 = R * Cos(theta1);
				float R0 = R * Cos(theta0);

				Degrees phi1 = { phi0.degrees + dPhi.degrees };
				float R0cosPhi0 = R0 * Cos(phi0);
				float R0cosPhi1 = R0 * Cos(phi1);
				float R1cosPhi0 = R1 * Cos(phi0);
				float R1cosPhi1 = R1 * Cos(phi1);
				float R0sinPhi0 = R0 * Sin(phi0);
				float R0sinPhi1 = R0 * Sin(phi1);
				float R1sinPhi0 = R1 * Sin(phi0);
				float R1sinPhi1 = R1 * Sin(phi1);

				float z1 = zTop - spec.southRadius + R * Sin(theta1);
				float z0 = zTop - spec.southRadius + R * Sin(theta0);

				SoftBoxQuad quad;
				quad.a.pos = { x1 + R1cosPhi0, y1 + R1sinPhi0, z1 };
				quad.b.pos = { x1 + R1cosPhi1, y1 + R1sinPhi1, z1 };
				quad.c.pos = { x1 + R0cosPhi0, y1 + R0sinPhi0, z0 };
				quad.d.pos = { x1 + R0cosPhi1, y1 + R0sinPhi1, z0 };
				
				quad.a.normal = { Cos(phi0) * Cos(theta1), Sin(phi0) * Cos(theta1), Sin(theta1) };
				quad.b.normal = { Cos(phi1) * Cos(theta1), Sin(phi1) * Cos(theta1), Sin(theta1) };
				quad.c.normal = { Cos(phi0) * Cos(theta0), Sin(phi0) * Cos(theta0), Sin(theta0) };
				quad.d.normal = { Cos(phi1) * Cos(theta0), Sin(phi1) * Cos(theta0), Sin(theta0) };

				quad.a.uv = { 0, 1 };
				quad.b.uv = { 1, 1 };
				quad.c.uv = { 1, 0 };
				quad.d.uv = { 1, 1 };

				quads.push_back(quad);

				phi0 = phi1;
			}


			theta0 = theta1;
		}
	}

	void AddNWCorner(float x0, float y1, const SoftBoxTopSpec& spec)
	{
		// Theta is the angle from the horizontal, 0 is in the z plane. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

		// Phi of 0 points West (-1 0 0), Phi of 90 points North
		Degrees dPhi{ 90.0f / spec.northEdgeDivisions };

		Degrees theta0 = 0_degrees;

		float R = spec.northRadius;

		for (int32 i = 0; i < spec.northEdgeDivisions; i++)
		{
			Degrees phi0 = 0_degrees;
			Degrees theta1{ theta0.degrees + dTheta.degrees };

			for (int32 j = 0; j < spec.northEdgeDivisions; j++)
			{
				float R1 = R * Cos(theta1);
				float R0 = R * Cos(theta0);

				Degrees phi1 = { phi0.degrees + dPhi.degrees };
				float R0cosPhi0 = R0 * Cos(phi0);
				float R0cosPhi1 = R0 * Cos(phi1);
				float R1cosPhi0 = R1 * Cos(phi0);
				float R1cosPhi1 = R1 * Cos(phi1);
				float R0sinPhi0 = R0 * Sin(phi0);
				float R0sinPhi1 = R0 * Sin(phi1);
				float R1sinPhi0 = R1 * Sin(phi0);
				float R1sinPhi1 = R1 * Sin(phi1);

				float z1 = zTop - spec.southRadius + R * Sin(theta1);
				float z0 = zTop - spec.southRadius + R * Sin(theta0);

				SoftBoxQuad quad;
				quad.a.pos = { x0 - R1cosPhi1, y1 + R1sinPhi1, z1 };
				quad.b.pos = { x0 - R1cosPhi0, y1 + R1sinPhi0, z1 };				
				quad.c.pos = { x0 - R0cosPhi1, y1 + R0sinPhi1, z0 };
				quad.d.pos = { x0 - R0cosPhi0, y1 + R0sinPhi0, z0 };
				
				quad.a.normal = { -Cos(phi1) * Cos(theta1), Sin(phi1) * Cos(theta1), Sin(theta1) };
				quad.b.normal = { -Cos(phi0) * Cos(theta1), Sin(phi0) * Cos(theta1), Sin(theta1) };
				quad.c.normal = { -Cos(phi1) * Cos(theta0), Sin(phi1) * Cos(theta0), Sin(theta0) };
				quad.d.normal = { -Cos(phi0) * Cos(theta0), Sin(phi0) * Cos(theta0), Sin(theta0) };

				quad.a.uv = { 0, 1 };
				quad.b.uv = { 1, 1 };
				quad.c.uv = { 1, 0 };
				quad.d.uv = { 1, 1 };

				quads.push_back(quad);

				phi0 = phi1;
			}


			theta0 = theta1;
		}
	}

	void AddSECorner(float x1, float y0, const SoftBoxTopSpec& spec)
	{
		// Theta is the angle from the horizontal, 0 is in the z plane. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

		// Phi of 0 points East (-1 0 0), Phi of 90 points South
		Degrees dPhi{ 90.0f / spec.northEdgeDivisions };

		Degrees theta0 = 0_degrees;

		float R = spec.northRadius;

		for (int32 i = 0; i < spec.northEdgeDivisions; i++)
		{
			Degrees phi0 = 0_degrees;
			Degrees theta1{ theta0.degrees + dTheta.degrees };

			for (int32 j = 0; j < spec.northEdgeDivisions; j++)
			{
				float R1 = R * Cos(theta1);
				float R0 = R * Cos(theta0);

				Degrees phi1 = { phi0.degrees + dPhi.degrees };
				float R0cosPhi0 = R0 * Cos(phi0);
				float R0cosPhi1 = R0 * Cos(phi1);
				float R1cosPhi0 = R1 * Cos(phi0);
				float R1cosPhi1 = R1 * Cos(phi1);
				float R0sinPhi0 = R0 * Sin(phi0);
				float R0sinPhi1 = R0 * Sin(phi1);
				float R1sinPhi0 = R1 * Sin(phi0);
				float R1sinPhi1 = R1 * Sin(phi1);

				float z1 = zTop - spec.southRadius + R * Sin(theta1);
				float z0 = zTop - spec.southRadius + R * Sin(theta0);

				SoftBoxQuad quad;
				quad.a.pos = { x1 + R1cosPhi1, y0 - R1sinPhi1, z1 };
				quad.b.pos = { x1 + R1cosPhi0, y0 - R1sinPhi0, z1 };				
				quad.c.pos = { x1 + R0cosPhi1, y0 - R0sinPhi1, z0 };
				quad.d.pos = { x1 + R0cosPhi0, y0 - R0sinPhi0, z0 };

				quad.a.normal = { Cos(phi1) * Cos(theta1), -Sin(phi1) * Cos(theta1), Sin(theta1) };
				quad.b.normal = { Cos(phi0) * Cos(theta1), -Sin(phi0) * Cos(theta1), Sin(theta1) };
				quad.c.normal = { Cos(phi1) * Cos(theta0), -Sin(phi1) * Cos(theta0), Sin(theta0) };
				quad.d.normal = { Cos(phi0) * Cos(theta0), -Sin(phi0) * Cos(theta0), Sin(theta0) };

				quad.a.uv = { 0, 1 };
				quad.b.uv = { 1, 1 };
				quad.c.uv = { 1, 0 };
				quad.d.uv = { 1, 1 };

				quads.push_back(quad);

				phi0 = phi1;
			}


			theta0 = theta1;
		}
	}


	void AddSWCorner(float x0, float y0, const SoftBoxTopSpec& spec)
	{
		// Theta is the angle from the horizontal, 0 is in the z plane. Theta of 90 points up ( 1 0 0)
		Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

		// Phi of 0 points West (-1 0 0), Phi of 90 points South
		Degrees dPhi{ 90.0f / spec.northEdgeDivisions };

		Degrees theta0 = 0_degrees;
		
		float R = spec.northRadius;

		for (int32 i = 0; i < spec.northEdgeDivisions; i++)
		{
			Degrees phi0 = 0_degrees;
			Degrees theta1{ theta0.degrees + dTheta.degrees };

			for (int32 j = 0; j < spec.northEdgeDivisions; j++)
			{
				float R1 = R * Cos(theta1);
				float R0 = R * Cos(theta0);

				Degrees phi1 = { phi0.degrees + dPhi.degrees };
				float R0cosPhi0 = R0 * Cos(phi0);
				float R0cosPhi1 = R0 * Cos(phi1);
				float R1cosPhi0 = R1 * Cos(phi0);
				float R1cosPhi1 = R1 * Cos(phi1);
				float R0sinPhi0 = R0 * Sin(phi0);
				float R0sinPhi1 = R0 * Sin(phi1);
				float R1sinPhi0 = R1 * Sin(phi0);
				float R1sinPhi1 = R1 * Sin(phi1);

				float z1 = zTop - spec.southRadius + R * Sin(theta1);
				float z0 = zTop - spec.southRadius + R * Sin(theta0);

				SoftBoxQuad quad;
				quad.a.pos = { x0 - R1cosPhi0, y0 - R1sinPhi0, z1 };
				quad.b.pos = { x0 - R1cosPhi1, y0 - R1sinPhi1, z1 };

				quad.c.pos = { x0 - R0cosPhi0, y0 - R0sinPhi0, z0 };
				quad.d.pos = { x0 - R0cosPhi1, y0 - R0sinPhi1, z0 };

				quad.a.normal = { -Cos(phi0) * Cos(theta1), -Sin(phi0) * Cos(theta1), Sin(theta1)};
				quad.b.normal = { -Cos(phi1) * Cos(theta1), -Sin(phi1) * Cos(theta1), Sin(theta1) };
				quad.c.normal = { -Cos(phi0) * Cos(theta0), -Sin(phi0) * Cos(theta0), Sin(theta0) };
				quad.d.normal = { -Cos(phi1) * Cos(theta0), -Sin(phi1) * Cos(theta0), Sin(theta0) };

				quad.a.uv = { 0, 1 };
				quad.b.uv = { 1, 1 };
				quad.c.uv = { 1, 0 };
				quad.d.uv = { 1, 1 };
			
				quads.push_back(quad);

				phi0 = phi1;
			}


			theta0 = theta1;
		}
	}

	void AddNorthEdge(float x0, float x1, float y1, const SoftBoxTopSpec& spec)
	{
		Degrees dTheta{ 90.0f / spec.northEdgeDivisions };

		Degrees theta0 = 0_degrees;

		for (int32 i = 0; i < spec.northEdgeDivisions; i++)
		{
			Degrees theta1 { theta0.degrees + dTheta.degrees };

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

			quads.push_back(northEdgeQuad);

			theta0 = theta1;
		}
	}

	void AddSouthEdge(float x0, float x1, float y0, const SoftBoxTopSpec& spec)
	{
		Degrees dTheta{ 90.0f / spec.southEdgeDivisions };

		Degrees theta0 = 0_degrees;

		for (int32 i = 0; i < spec.southEdgeDivisions; i++)
		{
			Degrees theta1 { theta0.degrees + dTheta.degrees };

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

			quads.push_back(southEdgeQuad);

			theta0 = theta1;
		}
	}

	void AddWestEdge(float x0, float y0, float y1, const SoftBoxTopSpec& spec)
	{
		Degrees dTheta{ 90.0f / spec.westEdgeDivisions };
		Degrees theta0 = 0_degrees;
		
		for (int32 i = 0; i < spec.westEdgeDivisions; i++)
		{
			Degrees theta1 { theta0.degrees + dTheta.degrees };

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

			quads.push_back(westEdgeQuad);

			theta0 = theta1;
		}
	}

	void AddEastEdge(float x1, float y0, float y1, const SoftBoxTopSpec& spec)
	{
		// Next we add four edges, each forms the top of a cylinder that takes the mesh from the horizontal to the vertical
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

			quads.push_back(eastEdgeQuad);

			theta0 = theta1;
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

		if (spec.northEdgeDivisions < 1 || spec.northEdgeDivisions > 100'000)
		{
			Throw(0, "%s: North edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.southEdgeDivisions < 1 || spec.southEdgeDivisions > 100'000)
		{
			Throw(0, "%s: South edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.westEdgeDivisions < 1 || spec.westEdgeDivisions > 100'000)
		{
			Throw(0, "%s: West edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		if (spec.eastEdgeDivisions < 1 || spec.eastEdgeDivisions > 100'000)
		{
			Throw(0, "%s: East edge divisions must lie between 1 and 100,000", __FUNCTION__);
		}

		zTop = spec.ztop;

		SoftBoxQuad topQuad;

		float x0 = spec.width   * -0.5f + spec.westRadius;
		float x1 = spec.width   *  0.5f - spec.eastRadius;
		float y0 = spec.breadth * -0.5f + spec.southRadius;
		float y1 = spec.breadth *  0.5f - spec.northRadius;

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

		quads.push_back(topQuad);

		AddNorthEdge(x0, x1, y1, spec);
		AddSouthEdge(x0, x1, y0, spec);
		AddWestEdge(x0, y0, y1, spec);
		AddEastEdge(x1, y0, y1, spec);

		AddSWCorner(x0, y0, spec);
		AddNWCorner(x0, y1, spec);
		AddNECorner(x1, y1, spec);
		AddSECorner(x1, y0, spec);

		// This may be pertinent around 2050
		if (quads.size() > 0x7FFFFFFFLL)
		{
			Clear();
			Throw(0, "Maximum quad array size exceeded. Clearning soft box builder");
		}
	}

	void Clear()
	{
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
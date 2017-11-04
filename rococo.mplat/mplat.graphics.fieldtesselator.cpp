#include <rococo.mplat.h>

#include <vector>
#include <random>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Graphics;

	std::default_random_engine rng;

	float AnyOf(float minValue, float maxValue)
	{
		float32 range = maxValue - minValue;
		float f = rng() / (float)rng.max();
		return range * f + minValue;
	}

	class FieldTesselator : public IFieldTesselator
	{
		enum { MAX_DIVS = 1024 };

		Quad macroQuad = { 0 };
		Vec2 uvA = { 0 }; // Texture co-ordinates at A
		Vec2 uvC = { 0 }; // Texture co-ordinates at C
		int32 rows = 0;
		int32 columns = 0;
		Matrix4x4 basis = { 0 }; // Converts from field co-ordinate system to world
		Vec3 rawTangent = { 0 }; // Points to the right (x-axis) from A to B
		Vec3 rawVertical = { 0 }; // Points upwards (y - axis) from D to A
		Vec3 normal = { 0 }; // Points towards the viewer (z - axis)
		Vec2 span = { 0 }; // Span of the macroQuad

		Vec2 delta = { 0 }; // Spatial difference between neighbouring cells along tangent and vertical
		Vec2 deltaUVs = { 0 }; // UV deltas between neighbouring cells

		Vec2 brickDelta = { 0 }; // Like delta, but accomodating half bricks at one end

		void InitBasis()
		{
			rawTangent = macroQuad.b - macroQuad.a;
			rawVertical = macroQuad.a - macroQuad.d;
			normal = Normalize(Cross(rawTangent, rawVertical));

			Vec3 tangent = Normalize(rawTangent);
			Vec3 vertical = Normalize(rawVertical);

			basis =
			{
				{ tangent.x, vertical.x, normal.x, 0 },
				{ tangent.y, vertical.y, normal.y, 0 },
				{ tangent.z, vertical.z, normal.z, 0 },
				{ 0        , 0         , 0       , 1 }
			};

			span.x = Length(rawTangent);
			span.y = Length(rawVertical);
		}

		std::vector<float> heights;

		void NormalizeAndZeroField()
		{
			columns = min(columns, (int32)MAX_DIVS);
			columns = max(columns, 1);

			rows = min(rows, (int32)MAX_DIVS);
			rows = max(rows, 1);

			delta = { 1.0f / columns, 1.0f / rows };

			brickDelta.y = delta.y;
			brickDelta.x = 1.0f / (columns - 0.5f);

			heights.resize((rows+3) * (columns+3));
			memset(&heights[0], 0, (rows + 3) * (columns + 3) * sizeof(float));
		}
	public:
		FieldTesselator()
		{
			static bool isInit = false;
			if (!isInit)
			{
				isInit = true;
				rng.seed((uint32)OS::UTCTime());
			}
		}

		void InitByFixedCellWidth(const Quad& positions, float maxCellWidth, float maxCellHeight) override
		{
			macroQuad = positions;
			InitBasis();

			if (maxCellWidth >= span.x)
			{
				columns = 1;
			}

			columns = ((int32) (span.x / maxCellWidth)) + 1;

			if (maxCellHeight >= span.y)
			{
				rows = 1;
			}

			rows = ((int32)(span.y / maxCellHeight)) + 1;

			NormalizeAndZeroField();
		}

		void InitByDivisions(const Quad& positions, int32 xDivs, int32 yDivs) override
		{
			macroQuad = positions;
			InitBasis();

			columns = xDivs;
			rows = yDivs;

			NormalizeAndZeroField();
		}

		void Destruct() override
		{
			delete this;
		}

		void SetUV(const Vec2& uvA, const Vec2& uvC) override
		{
			this->uvA = uvA;
			this->uvC = uvC;

			Vec2 uvSpan = uvC- uvA;
			deltaUVs = { uvSpan.x / columns, uvSpan.y / rows };
		}

		int32 NumberOfColumns() override
		{
			return columns;
		}

		int32 NumberOfRows() override
		{
			return rows;
		}

		void GetFlatSubQuad(int32 i, int32 j, QuadVertices& q) override
		{
			SetNoColours(q);
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::GetSubQuad - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::GetSubQuad - j (%d) out of bounds [0,%d) ", j, rows);
			}

			float DX0 = i * delta.x;
			float DY0 = j * delta.y;
			float DX1 = DX0 + delta.x;
			float DY1 = DY0 + delta.y;

			Vec3 DT0 = DX0 * rawTangent;
			Vec3 DT1 = DX1 * rawTangent;

			Vec3 DV0 = DY0 * rawVertical;
			Vec3 DV1 = DY1 * rawVertical;

			float hA = GetHeight(i, j);
			float hB = GetHeight(i + 1, j);
			float hC = GetHeight(i + 1, j + 1);
			float hD = GetHeight(i, j + 1);

			float h = 0.25f * (hA + hB + hC + hD);

			Vec3 DN = normal * h;

			q.positions.a = macroQuad.a + DT0 - DV0 + DN;
			q.positions.b = macroQuad.a + DT1 - DV0 + DN;
			q.positions.c = macroQuad.a + DT1 - DV1 + DN;
			q.positions.d = macroQuad.a + DT0 - DV1 + DN;

			q.uv.left   = uvA.x + deltaUVs.x * i;
			q.uv.top    = uvA.y + deltaUVs.y * j;
			q.uv.right  = uvA.x + deltaUVs.x * (i+1);
			q.uv.bottom = uvA.y + deltaUVs.y * (j+1);

			q.normals.a = normal;
			q.normals.b = normal;
			q.normals.c = normal;
			q.normals.d = normal;
		}

		void GetStackBondedBrick(int32 i, int32 j, QuadVertices& q, float cementThicknessRatio) override
		{
			RandomizeColours(q);
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::GetStackBondedBrick - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::GetStackBondedBrick - j (%d) out of bounds [0,%d) ", j, rows);
			}

			float DX0 = i * delta.x;
			float DY0 = j * delta.y;
			float DX1 = DX0 + delta.x;
			float DY1 = DY0 + delta.y;

			const float cementThickness = cementThicknessRatio * delta.x;

			float DX0_INNER = DX0 + cementThickness;
			float DX1_INNER = DX1 - cementThickness;
			float DY0_INNER = DY0 + cementThickness;
			float DY1_INNER = DY1 - cementThickness;

			Vec3 DT0 = DX0_INNER * rawTangent;
			Vec3 DT1 = DX1_INNER * rawTangent;

			Vec3 DV0 = DY0_INNER * rawVertical;
			Vec3 DV1 = DY1_INNER * rawVertical;

			Vec3 DN = normal * 0.0f;

			q.positions.a = macroQuad.a + DT0 - DV0 + DN;
			q.positions.b = macroQuad.a + DT1 - DV0 + DN;
			q.positions.c = macroQuad.a + DT1 - DV1 + DN;
			q.positions.d = macroQuad.a + DT0 - DV1 + DN;

			q.uv.left   = uvA.x + deltaUVs.x * DX0_INNER / delta.x;
			q.uv.top    = uvA.y + deltaUVs.y * DY0_INNER / delta.y;
			q.uv.right  = uvA.x + deltaUVs.x * DX1_INNER / delta.x;
			q.uv.bottom = uvA.y + deltaUVs.y * DY1_INNER / delta.y;

			q.normals.a = normal;
			q.normals.b = normal;
			q.normals.c = normal;
			q.normals.d = normal;
		}

		void GetBrickJoinRight(int32 i, int32 j, QuadVertices& q, float cementThicknessRatio)
		{
			SetNoColours(q);
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::GetBrickJoinRight - i (%d) out of bounds [0,%d) ", i, columns - 1);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::GetBrickJoinRight - j (%d) out of bounds [0,%d) ", j, rows);
			}

			if (i == columns - 1)
			{
				q.positions.a = q.positions.b = q.positions.b = q.positions.d = macroQuad.a;
				return;
			}

			const float cementThickness = cementThicknessRatio * brickDelta.y * 1.0f / span.y;

			float Y = j * delta.y + cementThickness;

			boolean32 isOdd = (j % 2);

			float DX0;

			if (i == 0)
			{
				DX0 = isOdd ? brickDelta.x : (0.5f  * brickDelta.x);
			}
			else
			{
				if (isOdd)
				{
					DX0 = (i + 1) * brickDelta.x;
				}
				else
				{
					DX0 = (i + 1 - 0.5f) * brickDelta.x;
				}
			}

			float DX1 = DX0 + 2.0f * cementThickness;

			float DY0 = Y + cementThickness;
			float DY1 = Y + delta.y - cementThickness;

			Vec3 DT0 = DX0 * rawTangent;
			Vec3 DT1 = DX1 * rawTangent;

			Vec3 DV0 = DY0 * rawVertical;
			Vec3 DV1 = DY1 * rawVertical;

			Vec3 DN = normal * 0.0f;

			q.positions.a = macroQuad.a + DT0 - DV0 + DN;
			q.positions.b = macroQuad.a + DT1 - DV0 + DN;
			q.positions.c = macroQuad.a + DT1 - DV1 + DN;
			q.positions.d = macroQuad.a + DT0 - DV1 + DN;

			q.uv.left = uvA.x + deltaUVs.x * DX0 * columns;
			q.uv.top = uvA.y + deltaUVs.y * DY0 * rows;
			q.uv.right = uvA.x + deltaUVs.x * DX1 * columns;
			q.uv.bottom = uvA.y + deltaUVs.y * DY1 * rows;

			q.normals.a = normal;
			q.normals.b = normal;
			q.normals.c = normal;
			q.normals.d = normal;
		}

		void GetBrickBedTop(int32 row, QuadVertices& q, float cementThicknessRatio)
		{
			SetNoColours(q);
			if (row < 0 || row >= rows)
			{
				Throw(0, "FieldTesselator::GetBrickBed - row (%d) out of bounds [0,%d) ", row, rows);
			}

			const float cementThickness = cementThicknessRatio * brickDelta.y * 1.0f / span.y;

			float Y = row * delta.y + cementThickness;

			float DY0 = Y - cementThickness;
			float DY1 = Y + cementThickness;

			Vec3 DT0 = 0.0f * rawTangent;
			Vec3 DT1 = 1.0f * rawTangent;

			Vec3 DV0 = DY0 * rawVertical;
			Vec3 DV1 = DY1 * rawVertical;

			Vec3 DN = normal * 0.0f;

			q.positions.a = macroQuad.a + DT0 - DV0 + DN;
			q.positions.b = macroQuad.a + DT1 - DV0 + DN;
			q.positions.c = macroQuad.a + DT1 - DV1 + DN;
			q.positions.d = macroQuad.a + DT0 - DV1 + DN;

			q.uv.left = uvA.x;
			q.uv.top = uvA.y + deltaUVs.y * DY0 * rows;
			q.uv.right = uvA.x + deltaUVs.x * columns;
			q.uv.bottom = uvA.y + deltaUVs.y * DY1 * rows;

			q.normals.a = normal;
			q.normals.b = normal;
			q.normals.c = normal;
			q.normals.d = normal;
		}

		void SetNoColours(QuadVertices& q)
		{
			int32 r = 0;
			int32 g = 0;
			int32 b = 0;
			q.colours.a = RGBAb(r, g, b, 255);
			q.colours.b = RGBAb(r, g, b, 255);
			q.colours.c = RGBAb(r, g, b, 255);
			q.colours.d = RGBAb(r, g, b, 255);
		}

		void RandomizeColours(QuadVertices& q)
		{
			int32 r = 64 + rand() % 192;
			int32 g = r;
			int32 b = r;

			int32 blend = 192 + rand() % 64;

			q.colours.a = RGBAb(r, g, b, blend);
			q.colours.b = RGBAb(r, g, b, blend);
			q.colours.c = RGBAb(r, g, b, blend);
			q.colours.d = RGBAb(r, g, b, blend);
		}

		void GetStretchBondedBrick(int32 i, int32 j, QuadVertices& q, QuadVertices& top, QuadVertices& left, QuadVertices& right, QuadVertices& bottom, float cementThicknessRatio)  override
		{
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::GetStretchBondedBrick - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::GetStretchBondedBrick - j (%d) out of bounds [0,%d) ", j, rows);
			}

			if (columns < 2)
			{
				Throw(0, "FieldTesselator::GetStretchBondedBrick - columns (%d) needs to be >= 2 for stretched bricks", columns);
			}

			RandomizeColours(q);
			RandomizeColours(top);
			RandomizeColours(left);
			RandomizeColours(right);
			RandomizeColours(bottom);

			boolean32 isOdd = (j % 2);

			const float cementThickness = cementThicknessRatio * brickDelta.y / span.y;

			float DX0;
			float DX1;

			if (i == 0)
			{
				DX0 = 0;
				DX1 = isOdd ? brickDelta.x : (0.5f * brickDelta.x);
			}
			else if (i == (columns - 1))
			{
				DX1 = 1.0f;
				DX0 = DX1 - ( !isOdd ? brickDelta.x : (0.5f * brickDelta.x) );
			}
			else
			{
				if (isOdd)
				{
					DX0 = i * brickDelta.x;
					DX1 = DX0 + brickDelta.x;
				}
				else
				{
					DX0 = (i - 0.5f) * brickDelta.x;
					DX1 = DX0 + brickDelta.x;
				}
			}
			

			float DY0 = j * delta.y + 1.0f * cementThickness;
			float DY1 = DY0 + delta.y;

			float DX0_INNER = (i == 0) ? 0.0f : (DX0 + 2.0f * cementThickness);
			float DX1_INNER = (i == columns - 1) ? 1.0f : DX1;
			float DY0_INNER = DY0 + cementThickness;
			float DY1_INNER = DY1 - cementThickness;

			Vec3 DT0 = DX0_INNER * rawTangent;
			Vec3 DT1 = DX1_INNER * rawTangent;

			Vec3 DV0 = DY0_INNER * rawVertical;
			Vec3 DV1 = DY1_INNER * rawVertical;

			float hA = GetHeight(i, j);
			float hB = GetHeight(i + 1, j);
			float hC = GetHeight(i + 1, j + 1);
			float hD = GetHeight(i, j + 1);

			Vec3 DN = normal * cementThickness * span.y;

			int index = rand() % 6;

			Quad flatQuad;
			flatQuad.a = macroQuad.a + DT0 - DV0;
			flatQuad.b = macroQuad.a + DT1 - DV0;
			flatQuad.c = macroQuad.a + DT1 - DV1;
			flatQuad.d = macroQuad.a + DT0 - DV1;

			q.positions.a = flatQuad.a + DN;
			q.positions.b = flatQuad.b + DN;
			q.positions.c = flatQuad.c + DN;
			q.positions.d = flatQuad.d + DN;

			Vec2 dz_ds = {0, 0};

			if (i >= 0 && i <= (columns - 1))
			{
				if (index == 0)
				{
					q.positions.a += normal * span.y * (hA + hB);
					q.positions.b += normal * span.y * (hA + hB);
					dz_ds.y = -span.y * (hA + hB) / (delta.y * span.y);
				}
				else if (index == 1)
				{
					q.positions.c += normal * span.y * (hC + hD);
					q.positions.d += normal * span.y * (hC + hD);
					dz_ds.y = span.y * (hC + hD) / (delta.y * span.y);
				}
				else if (index == 2)
				{
					q.positions.a += normal * span.y * (hA + hD);
					q.positions.d += normal * span.y * (hA + hD);
					dz_ds.x = -span.y * (hA + hD) / (delta.x * span.x);
				}
				else if (index == 3)
				{
					q.positions.b += normal * span.y * (hB + hC);
					q.positions.c += normal * span.y * (hB + hC);
					dz_ds.x = span.y * (hB + hC) / (delta.x * span.x);
				}
				else
				{
					float h = hA + hB + hC + hD;
					q.positions.a += normal * span.y * h;
					q.positions.b += normal * span.y * h;
					q.positions.c += normal * span.y * h;
					q.positions.d += normal * span.y * h;
				}
			}

			Vec3 brickspaceNormal = { 1.0f * -dz_ds.x, 1.0f * -dz_ds.y, 1 };

			Vec3 realNormal;
			TransformDirection(basis, Normalize(brickspaceNormal), realNormal);

			q.uv.left = uvA.x + deltaUVs.x * DX0_INNER / delta.x;
			q.uv.top = uvA.y + deltaUVs.y * DY0_INNER / delta.y;
			q.uv.right = uvA.x + deltaUVs.x * DX1_INNER / delta.x;
			q.uv.bottom = uvA.y + deltaUVs.y * DY1_INNER / delta.y;

			q.normals.a = realNormal;
			q.normals.b = realNormal;
			q.normals.c = realNormal;
			q.normals.d = realNormal;

			top.positions.d = q.positions.a;
			top.positions.c = q.positions.b;
			top.positions.a = flatQuad.a;
			top.positions.b = flatQuad.b;

			top.normals.a = top.normals.b = top.normals.c = top.normals.d = Normalize(rawVertical);

			top.uv.left = q.uv.left;
			top.uv.right = q.uv.right;
			top.uv.bottom = q.uv.top;
			top.uv.top = top.uv.bottom + cementThickness;

			bottom.positions.a = q.positions.d;
			bottom.positions.b = q.positions.c;
			bottom.positions.d = flatQuad.d;
			bottom.positions.c = flatQuad.c;

			bottom.normals.a = bottom.normals.b = bottom.normals.c = bottom.normals.d = -top.normals.a;

			bottom.uv.left = q.uv.left;
			bottom.uv.right = q.uv.right;
			bottom.uv.top = q.uv.bottom;
			bottom.uv.bottom = top.uv.top - cementThickness;

			left.positions.a = bottom.positions.d;
			left.positions.b = top.positions.a;
			left.positions.c = top.positions.d;
			left.positions.d = q.positions.d;

			left.normals.a = left.normals.b = left.normals.c = left.normals.d = -Normalize(rawTangent);

			left.uv.left = q.uv.bottom;
			left.uv.right = q.uv.top;
			left.uv.bottom = q.uv.left;
			left.uv.top = left.uv.bottom - cementThickness;

			right.positions.a = q.positions.b;
			right.positions.b = top.positions.b;
			right.positions.c = bottom.positions.c;
			right.positions.d = q.positions.c;

			right.normals.a = right.normals.b = right.normals.c = right.normals.d = -left.normals.a;

			right.uv.left = left.uv.top;
			right.uv.right = left.uv.bottom;
			right.uv.bottom = left.uv.left;
			right.uv.top = left.uv.right;
		}


		void GetPerturbedSubQuad(int32 i, int32 j, QuadVertices& q) override
		{
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::GetSubQuad - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::GetSubQuad - j (%d) out of bounds [0,%d) ", j, rows);
			}

			RandomizeColours(q);

			float DX0 = i * delta.x;
			float DY0 = j * delta.y;
			float DX1 = DX0 + delta.x;
			float DY1 = DY0 + delta.y;

			Vec3 DT0 = DX0 * rawTangent;
			Vec3 DT1 = DX1 * rawTangent;

			Vec3 DV0 = DY0 * rawVertical;
			Vec3 DV1 = DY1 * rawVertical;

			float hA = GetHeight(i, j);
			float hB = GetHeight(i + 1, j);
			float hC = GetHeight(i + 1, j + 1);
			float hD = GetHeight(i, j + 1);

			q.positions.a = macroQuad.a + DT0 - DV0 + hA * normal;
			q.positions.b = macroQuad.a + DT1 - DV0 + hB * normal;
			q.positions.c = macroQuad.a + DT1 - DV1 + hC * normal;
			q.positions.d = macroQuad.a + DT0 - DV1 + hD * normal;

			q.uv.left   = uvA.x + deltaUVs.x * i;
			q.uv.top    = uvA.y + deltaUVs.y * j;
			q.uv.right  = uvA.x + deltaUVs.x * (i + 1);
			q.uv.bottom = uvA.y + deltaUVs.y * (j + 1);

			Vec2 dh_dxyA = Get_DH_DXY(i,     j);
			Vec2 dh_dxyB = Get_DH_DXY(i+1,   j);
			Vec2 dh_dxyC = Get_DH_DXY(i+1, j+1);
			Vec2 dh_dxyD = Get_DH_DXY(i,   j+1);

			Vec3 Na = Normalize({ -dh_dxyA.x, dh_dxyA.y, 1.0f });
			Vec3 Nb = Normalize({ -dh_dxyB.x, dh_dxyB.y, 1.0f });
			Vec3 Nc = Normalize({ -dh_dxyC.x, dh_dxyC.y, 1.0f });
			Vec3 Nd = Normalize({ -dh_dxyD.x, dh_dxyD.y, 1.0f });

			TransformDirection(basis, Na, q.normals.a);
			TransformDirection(basis, Nb, q.normals.b);
			TransformDirection(basis, Nc, q.normals.c);
			TransformDirection(basis, Nd, q.normals.d);
		}

		float& GetHeight(int i, int j)
		{
			return heights[i + 1 + (j + 1) *  (columns + 3)];
		}

		float GetHeight(int i, int j) const
		{
			return heights[i + 1 + (j + 1) * (columns + 3)];
		}

		Vec2 Get_DH_DXY(int i, int j) const
		{
			float HI0 = GetHeight(i - 1, j);
			float HI2 = GetHeight(i + 1, j);
			float dh_dx = 0.5f * (HI2 - HI0);

			float HJ0 = GetHeight(i, j - 1);
			float HJ2 = GetHeight(i, j + 1);
			float dh_dy = 0.5f * (HJ2 - HJ0);

			return { dh_dx, dh_dy };
		}

		void PerturbField(int32 i, int32 j, float dH) override
		{
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::PerturbField - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::PerturbField - j (%d) out of bounds [0,%d) ", j, rows);
			}

			GetHeight(i, j) = dH;
		}

		void LevelField(int32 i0, int32 j0, int32 i1, int32 j1, float dH) override
		{
			if (i0 < 0 || i0 >= columns)
			{
				Throw(0, "FieldTesselator::LevelField - i0 (%d) out of bounds [0,%d) ", i0, columns);
			}

			if (j0 < 0 || j0 >= rows)
			{
				Throw(0, "FieldTesselator::LevelField - j0 (%d) out of bounds [0,%d) ", j0, rows);
			}

			if (i1 < 0 || i1 >= columns)
			{
				Throw(0, "FieldTesselator::LevelField - i1 (%d) out of bounds [0,%d) ", i1, columns);
			}

			if (j1 < 0 || j1 >= rows)
			{
				Throw(0, "FieldTesselator::LevelField - j1 (%d) out of bounds [0,%d) ", j1, rows);
			}

			float* pRow0 = &heights[i0 + 1 + (j0 + 1) *  (columns + 3)];

			for (int32 j = j0; j <= j1; ++j)
			{
				float* pRow = pRow0;

				for (int32 i = i0; i <= i1; ++i)
				{
					*pRow++ = dH;
				}

				pRow0 += (columns + 3);
			}
		}

		void RandomizeField(int32 i, int32 j, float minValue, float maxValue) override
		{
			if (i < 0 || i >= columns)
			{
				Throw(0, "FieldTesselator::RandomizeField - i (%d) out of bounds [0,%d) ", i, columns);
			}

			if (j < 0 || j >= rows)
			{
				Throw(0, "FieldTesselator::RandomizeField - j (%d) out of bounds [0,%d) ", j, rows);
			}

			auto& f = GetHeight(i, j);
			f = AnyOf(minValue, maxValue);
		}

		void GetBasis(Matrix4x4& transform) override
		{
			transform = basis;
		}
	};
}

namespace Rococo
{
	namespace Graphics
	{
		IFieldTesselator* CreateFieldTesselator()
		{
			return new FieldTesselator();
		}
	}
}
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

		void GetFlatSubQuad(int32 i, int32 j, Quad& subQuad, GuiRectf& subUV) override
		{
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

			subQuad.a = macroQuad.a + DT0 - DV0 + DN;
			subQuad.b = macroQuad.a + DT0 - DV0 + DN;
			subQuad.c = macroQuad.a + DT1 - DV1 + DN;
			subQuad.d = macroQuad.a + DT1 - DV1 + DN;

			subUV.left   = uvA.x + deltaUVs.x * DX0;
			subUV.top    = uvA.y + deltaUVs.y * DY0;
			subUV.right  = uvA.x * deltaUVs.x * DX1;
			subUV.bottom = uvA.y + deltaUVs.y * DY1;
		}

		void GetPerturbedSubQuad(int32 i, int32 j, Quad& subQuad, GuiRectf& subUV, Quad& subNormals) override
		{
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

			subQuad.a = macroQuad.a + DT0 - DV0 + hA * normal;
			subQuad.b = macroQuad.a + DT1 - DV0 + hB * normal;
			subQuad.c = macroQuad.a + DT1 - DV1 + hC * normal;
			subQuad.d = macroQuad.a + DT0 - DV1 + hD * normal;

			subUV.left   = uvA.x + deltaUVs.x * i;
			subUV.top    = uvA.y + deltaUVs.y * j;
			subUV.right  = uvA.x + deltaUVs.x * (i + 1);
			subUV.bottom = uvA.y + deltaUVs.y * (j + 1);	

			Vec2 dh_dxyA = Get_DH_DXY(i,     j);
			Vec2 dh_dxyB = Get_DH_DXY(i+1,   j);
			Vec2 dh_dxyC = Get_DH_DXY(i+1, j+1);
			Vec2 dh_dxyD = Get_DH_DXY(i,   j+1);

			Vec3 Na = Normalize({ -dh_dxyA.x, dh_dxyA.y, 1.0f });
			Vec3 Nb = Normalize({ -dh_dxyB.x, dh_dxyB.y, 1.0f });
			Vec3 Nc = Normalize({ -dh_dxyC.x, dh_dxyC.y, 1.0f });
			Vec3 Nd = Normalize({ -dh_dxyD.x, dh_dxyD.y, 1.0f });

			TransformNormal(basis, Na, subNormals.a);
			TransformNormal(basis, Nb, subNormals.b);
			TransformNormal(basis, Nc, subNormals.c);
			TransformNormal(basis, Nd, subNormals.d);
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
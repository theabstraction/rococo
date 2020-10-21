#include "rococo.mplat.h"
#include "mplat.landscapes.h"
#include <rococo.random.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace
{
	class LandscapeTesselator : public ILandscapeTesselatorSupervisor
	{
		int32 cellsPerAxis = 0;
		Metres span = 0_metres;
		std::vector<float> heightMap;
		Rococo::Random::RandomMT rng;
		int64 seedNumber = 777;
		Seconds generateDuration = 0.0_seconds;
		AABB bounds;
		Vec3 delta = { 0,0,0 };
		IMeshBuilderSupervisor& meshBuilder;
	public:
		LandscapeTesselator(IMeshBuilderSupervisor& refMeshBuilder):
			meshBuilder(refMeshBuilder)
		{

		}

		void AddQuadField(int32 base2exponentDivisions, Metres span) override
		{
			if (base2exponentDivisions < 5 || base2exponentDivisions > 13)
			{
				Throw(0, "%s: base2exponentDivisions range is 5 to 13", __FUNCTION__);
			}
			
			if (span <= 0)
			{
				Throw(0, "%s: span must be positive.", __FUNCTION__);
			}

			this->cellsPerAxis = 1 << base2exponentDivisions;
			this->span = span;	

			size_t pointsPerAxis = (size_t) this->cellsPerAxis + 1;
			heightMap.resize(pointsPerAxis * pointsPerAxis);
		}

		void Clear()
		{
			heightMap.clear();
			delta = { 0,0,0 };
		}

		Vec3 GetNormalAt(int i, int j) const
		{
			const float cellSpan_times_2 = span * 2.0f / cellsPerAxis;
			if (i > 0 && i < cellsPerAxis)
			{
				if (j > 0 && j < cellsPerAxis)
				{
					float hLeft  = HeightAt(i - 1, j);
					float hRight = HeightAt(i + 1, j);
					float hDown  = HeightAt(i, j - 1);
					float hUp    = HeightAt(i, j + 1);

					// df/dx = [f(x + h) - f(x - h)] / 2dh, but df = dz

					float dzdx = (hRight - hLeft) / cellSpan_times_2;
					float dzdy = (hUp - hDown) / cellSpan_times_2;

					return Normalize(Vec3{ -dzdx, -dzdy, 1.0f });
				}
			}

			return Vec3{ 0, 0, 1 }; // up
		}

		ID_MESH/* id */ CommitToMesh(const fstring& meshName) override
		{
			if (heightMap.empty())
			{
				Throw(0, "%s: the heightMap is empty", __FUNCTION__);
			}

			auto& mb = this->meshBuilder;
			mb.Clear();
			mb.Begin(meshName);

			const float* pHeights = heightMap.data();

			const float cellSpan = span / (float) cellsPerAxis;

			int pointsPerAxis = cellsPerAxis + 1;

			MaterialVertexData mvd;
			mvd.colour = RGBAb(255, 255, 255, 255);
			mvd.gloss = 0;
			mvd.materialId = 0;

			rng.Seed((uint32) seedNumber);

			ObjectVertex a, b, c, d;

			for (int j = 0; j < cellsPerAxis; ++j)
			{
				for (int i = 0; i < cellsPerAxis; ++i)
				{
					float height = *pHeights;
					a.position = { i * cellSpan,             j * cellSpan, height };
					b.position = { i * cellSpan,       (j + 1) * cellSpan, pHeights[pointsPerAxis] };
					c.position = { (i + 1) * cellSpan, (j + 1) * cellSpan, pHeights[pointsPerAxis + 1] };
					d.position = { (i + 1) * cellSpan,       j * cellSpan, pHeights[1] };
					a.material = b.material = c.material = d.material = mvd;

					float alpha = Rococo::Random::NextFloat(rng, 0, DEGREES_TO_RADIANS_QUOTIENT() * 360.0f);
					auto Rxy = Matrix2x2::RotateAnticlockwise(Radians{ alpha });

					a.uv = Rxy * Vec2 {-0.5f, -0.5f };
					b.uv = Rxy * Vec2 { 0.5f, -0.5f };
					c.uv = Rxy * Vec2 { 0.5f,  0.5f };
					d.uv = Rxy * Vec2 { 0.5f, -0.5f };

					a.normal = GetNormalAt(i, j);
					b.normal = GetNormalAt(i, j + 1);
					c.normal = GetNormalAt(i + 1, j + 1);
					d.normal = GetNormalAt(i + 1, j);

					mb.AddTriangle(a, b, c);
					mb.AddTriangle(c, d, a);

					pHeights++;
				}

				pHeights++;
			}

			mb.End(true, false);
			return ID_MESH::Invalid();
		}

		float NextRandomHeight(float minHeight, float maxHeight)
		{
			return Random::NextFloat(rng, minHeight, maxHeight);
		}

		void RecurseAndSubdivide(int x0, int x1, int y0, int y1, float maxRandom)
		{
			int DS = (x1 - x0);
			int halfDS = DS >> 1;
			int xH = x0 + halfDS;
			int yH = y0 + halfDS;

			float hX0Y0 = HeightAt(x0, y0);
			float hX0Y1 = HeightAt(x0, y1);
			float hX1Y0 = HeightAt(x1, y0);
			float hX1Y1 = HeightAt(x1, y1);

			float& height_m0 = GetRef(xH, y0);
			if (height_m0 <= -1.0f)
			{
				height_m0 = 0.5f * (hX0Y0 + hX1Y0);
				height_m0 += NextRandomHeight(0, maxRandom);
			}

			float& height_0m = GetRef(x0, yH);
			if (height_0m <= -1.0f)
			{
				height_0m = 0.5f * (hX0Y0 + hX0Y1);
				height_0m += NextRandomHeight(0, maxRandom);
			}

			float& height_m1 = GetRef(xH, y1);
			if (height_m1 <= -1.0f)
			{
				height_m1 = 0.5f * (hX0Y1 + hX1Y1);
				height_m1 += NextRandomHeight(0, maxRandom);
			}

			float& height_1m = GetRef(x1, yH);
			if (height_1m <= -1.0f)
			{
				height_1m = 0.5f * (hX1Y0 + hX1Y1);
				height_1m += NextRandomHeight(0, maxRandom);
			}

			float& height_mm = GetRef(xH, yH);
			if (height_mm <= -1.0f)
			{
				height_mm = 0.25f * (hX0Y0 + hX1Y1 + hX0Y1 + hX1Y0);
				const float CENTRE_DIMINUTION = 0.5f;
				height_mm += NextRandomHeight(0, CENTRE_DIMINUTION * maxRandom);
			}

			if (DS > 1)
			{
				float maxChildRandom = maxRandom * 0.5f;
				RecurseAndSubdivide(x0, xH, y0, yH, maxChildRandom);
				RecurseAndSubdivide(x0, xH, yH, y1, maxChildRandom);
				RecurseAndSubdivide(xH, x1, y0, yH, maxChildRandom);
				RecurseAndSubdivide(xH, x1, yH, y1, maxChildRandom);
			}
		}

		void GenerateByRecursiveSubdivision(Metres maxAltitude) override
		{
			if (heightMap.empty())
			{
				Throw(0, "%s: the height map was empty. Call AddQuadField(...) first.", __FUNCTION__);
			}

			rng.Seed((uint32)seedNumber);
			std::fill(heightMap.begin(), heightMap.end(), -1.0f);
			Set(0, 0, NextRandomHeight(0.0f, maxAltitude));
			Set(0, cellsPerAxis, NextRandomHeight(0.0f, maxAltitude));
			Set(cellsPerAxis, 0, NextRandomHeight(0.0f, maxAltitude));
			Set(cellsPerAxis, cellsPerAxis, NextRandomHeight(0.0f, maxAltitude));

			OS::ticks start = OS::CpuTicks();
			RecurseAndSubdivide(0, cellsPerAxis, 0, cellsPerAxis, maxAltitude * 0.5f);
			OS::ticks duration = OS::CpuTicks() - start;
			double dt = duration / (double) OS::CpuHz();
			generateDuration = Seconds{ (float) dt };
		}

		void Set(int32 x, int32 y, float altitude)
		{
			heightMap[x + y * (cellsPerAxis + 1)] = altitude;
		}

		float HeightAt(int32 x, int32 y) const
		{
			return heightMap[x + y * (cellsPerAxis + 1)];
		}

		float& operator()(int32 x, int32 y)
		{
			return GetRef(x, y);
		}

		float& GetRef(int32 x, int32 y)
		{
			return *(heightMap.data() + (x + y * (cellsPerAxis + 1)));
		}

		void GetBounds(Vec3& minPoint, Vec3& maxPoint) override
		{
			if (bounds.Span().x == 0)
			{
				float ds = span * 0.5f;
				bounds << Vec3{ -ds, -ds, 0 } << Vec3{ ds, ds, 0 };

				for (auto height : heightMap)
				{
					bounds << Vec3{ 0,0, height };
				}
			}

			minPoint = bounds.minXYZ;
			maxPoint = bounds.maxXYZ;
		}

		void RaiseMountain(const Vec3& atPosition, Metres deltaHeight, Metres spread) override
		{
			Throw(0, "Not implemented");
		}

		void SetHeights(const Vec2i& p0, const Vec2i& p1, Metres height) override
		{
			Throw(0, "Not implemented");
		}

		void SetSeed(int64 seedNumber) override
		{
			this->seedNumber = seedNumber == 0 ? OS::CpuTicks() : seedNumber;
		}

		void TranslateEachCell(const Vec3& delta) override
		{
			this->delta = delta;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Graphics
{
	ILandscapeTesselatorSupervisor* CreateLandscapeTesselator(IMeshBuilderSupervisor& meshes)
	{
		return new LandscapeTesselator(meshes);
	}
}
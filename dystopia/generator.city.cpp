#include "dystopia.h"
#include "rococo.renderer.h"
#include "meshes.h"
#include <random>
#include <algorithm>

namespace
{
	typedef std::mt19937  Randomizer;

	using namespace Rococo;
	using namespace Dystopia;

	float RandomQuotient(Randomizer& rng, float minValue, float maxValue)
	{
		float range = maxValue - minValue;

		uint32 r = rng();
		float value = (float)r / (float)0xFFFFFFFF;
		return min(value * range + minValue, maxValue);
	}
	
	Vec2 RandomVec2(Randomizer& rng)
	{
		return Vec2{ RandomQuotient(rng, -1.0f, 1.0f), RandomQuotient(rng, -1.0f, 1.0f) };
	}

	struct RoadContext
	{
		Environment* e;
		uint32 index;
		Randomizer* rng;
		std::vector<ObjectVertex>* cache;
	};

	void GenerateStraightRoadSegment(Vec2 left, Vec2 right, Vec2 leftGrad, Vec2 rightGrad, RoadContext& c, bool addWhite)
	{
		auto roadWidth = 2.0_metres;

		Vec2 leftNormal{ -leftGrad.y, leftGrad.x };
		Vec2 rightNormal{ -rightGrad.y, rightGrad.x };

		Vec3 vLeft = { left.x, left.y, 0 };
		Vec3 vRight = { right.x, right.y, 0 };

		Vec3 leftDelta  = roadWidth * Vec3{ leftNormal.x,   leftNormal.y, 0.0f };
		Vec3 rightDelta = roadWidth * Vec3{ rightNormal.x, rightNormal.y, 0.0f };

		

		Vec3 bottomLeft  = vLeft - leftDelta;
		Vec3 topLeft     = vLeft + leftDelta;
		Vec3 bottomRight = vRight - rightDelta;
		Vec3 topRight    = vRight + rightDelta;

		Vec3 up{ 0,0,1 };
		RGBAb roadColour(128, 128, 128);
		c.cache->push_back({ bottomLeft,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		c.cache->push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		c.cache->push_back({ bottomRight,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		c.cache->push_back({ bottomLeft,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		c.cache->push_back({ topLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		c.cache->push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });

		if (addWhite)
		{
			Vec3 leftDeltaWhite = 0.1f * Vec3{ leftNormal.x,   leftNormal.y, 0.01f };
			Vec3 rightDeltaWhite = 0.1f * Vec3{ rightNormal.x, rightNormal.y, 0.01f };
			bottomLeft = vLeft - leftDeltaWhite;
			topLeft = vLeft + leftDeltaWhite;
			bottomRight = vRight - rightDeltaWhite;
			topRight = vRight + rightDeltaWhite;

			RGBAb white(255, 255, 255);
			c.cache->push_back({ bottomLeft,	up, white, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, white, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight,	up, white, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft,	up, white, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft,		up, white, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, white, RGBAb(255,255,255, 0), 0, 1 });
		}
	}

	Vec2 SplineInterpolate(Vec2 P0, Vec2 P1, Vec2 m0, Vec2 m1, float t)
	{
		float a = 2.0f * Cube(t) - 3.0f * Square(t) + 1.0f;
		float b = Cube(t) - 2.0f * Square(t) + t;
		float c = -2.0f * Cube(t) + 3.0f * Square(t);
		float d = Cube(t) - Square(t);

		return a * P0 + b * m0 + c * P1 + d * m1;
	}

	Vec2 Lerp(Vec2 P0, Vec2 P1, float t)
	{
		return P0 + (P1 - P0) * t;
	}

	struct RoadNode
	{
		Vec2 gradient;
		Vec2 location;
	};

	typedef std::vector<RoadNode> TRoadVertices;

	void GenerateRoadSegment(Vec2 left, Vec2 right, Vec2 leftGrad, Vec2 rightGrad, RoadContext& c)
	{
		enum { nSegments = 5 };

		c.cache->clear();

		Vec2 midPoint = 0.5f * (left + right);

		float dt = 1.0f / nSegments;

		RoadNode positions[nSegments + 1];

		float t = 0;

		positions[0] = { leftGrad, left };

		for (int i = 1; i < nSegments; ++i)
		{
			t += dt;
			Vec2 v0 = SplineInterpolate(left, right, leftGrad, rightGrad, t);
			positions[i] = { { 0,0 }, v0 };
		}

		positions[nSegments] = { rightGrad,  right };

		for (int i = 1; i < nSegments; ++i)
		{
			positions[i].gradient = Normalize(positions[i + 1].location - positions[i - 1].location);
		}

		for (int i = 0; i < nSegments; ++i)
		{
			GenerateStraightRoadSegment(positions[i].location - midPoint, positions[i+1].location - midPoint, positions[i].gradient, positions[i+1].gradient, c, i > 0);
		}

		int bodyIndex = c.index + 0x20000001;
		c.index++;

		auto id = ID_MESH(bodyIndex);
		c.e->meshes.BuildMesh(&(c.cache->at(0)), c.cache->size(), id);
		Matrix4x4 loc = Matrix4x4::Translate(Vec3{ midPoint.x, midPoint.y, 0.0f });
		c.e->level.Builder().AddSolid(loc, id, SolidFlags_None);
	}

	void DivideRoad(Vec2 left, Vec2 right, TRoadVertices& v, Randomizer& rng)
	{
		Vec2 midPoint = 0.5f * (left + right);
		Vec2 randomDelta = { 0.0f, 0.0625f * RandomQuotient(rng, -1.0f, 1.0f) * Length(right - left) };
		Vec2 node = midPoint + randomDelta;

		auto meshLength = 10.0_metres;
		if (LengthSq(right - left) < Square(meshLength))
		{
			v.push_back({ {0,0}, node });
			return;
		}
		
		DivideRoad(left, node, v, rng);
		DivideRoad(node, right, v, rng);
	}

	void BuildRoads(TRoadVertices& v, Randomizer& rng)
	{
		Vec2 westPoint = { -1000.0_metres, 0.0f };
		Vec2 eastPoint = { 1000.0_metres, 0.0f };

		Vec2 midPoint = 0.5f * (westPoint + eastPoint);

		DivideRoad(westPoint, midPoint, v, rng);
		DivideRoad(midPoint, eastPoint, v, rng);

		std::sort(v.begin(), v.end(), [](RoadNode a, RoadNode b) {
			return a.location.x < b.location.x;
		});

		v[0].gradient = v[v.size() - 1].gradient = {1, 0};

		for (size_t i = 1; i < v.size() - 1; ++i)
		{
			Vec2 delta = v[i + 1].location - v[i - 1].location;
			v[i].gradient = Normalize(delta);
		}
	}

	void BuildHouses(TRoadVertices& road, Environment& e)
	{
		for (auto v : road)
		{
			Vec2 normal = { -v.gradient.y, v.gradient.x };

			Vec2 housePosition = v.location + 5.0f * normal;

			auto T = Matrix4x4::Translate({ housePosition.x, housePosition.y, 0.5f });
			e.level.Builder().AddSolid(T, GenerateRandomHouse(e), SolidFlags_Obstacle);

			housePosition = v.location - 5.0f * normal;

			T = Matrix4x4::Translate({ housePosition.x, housePosition.y, 0.5f });
			e.level.Builder().AddSolid(T, GenerateRandomHouse(e), SolidFlags_Obstacle);
		}
	}
}

namespace Dystopia
{
	void BuildRandomCity(const fstring& name, uint32 seedDelta, Environment& e)
	{
		uint32 hash = FastHash(name);
		Randomizer rng(hash + seedDelta);

		TRoadVertices mainRoad;	
		BuildRoads(mainRoad, rng);

		std::vector<ObjectVertex> cache;
		RoadContext c{ &e, 0, &rng, &cache };

		for (size_t i = 0; i < mainRoad.size() - 1; i++)
		{
			auto left = mainRoad[i];
			auto right = mainRoad[i + 1];

			GenerateRoadSegment(left.location, right.location, left.gradient, right.gradient, c);
		}

		BuildHouses(mainRoad, e);
	}
}
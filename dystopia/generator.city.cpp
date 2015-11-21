#include "dystopia.h"
#include "rococo.renderer.h"
#include "meshes.h"
#include <random>
#include <algorithm>
#include "rococo.strings.h"

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

		{
			Vec3 bottomLeft = vLeft + leftDelta;
			Vec3 topLeft = vLeft + 1.5f * leftDelta;
			Vec3 bottomRight = vRight + rightDelta;
			Vec3 topRight = vRight + 1.5f * rightDelta;

			RGBAb pavementColour(160, 160, 160);
			c.cache->push_back({ bottomLeft,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = vLeft - 1.5f * leftDelta;
			Vec3 topLeft = vLeft - leftDelta;
			Vec3 bottomRight = vRight - 1.5f * rightDelta;
			Vec3 topRight = vRight - rightDelta;

			RGBAb pavementColour(160, 160, 160);
			c.cache->push_back({ bottomLeft,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft,	up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight,		up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

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
		uint32 junctionIndex;
	};

	typedef std::vector<RoadNode> TRoadVertices;

	struct Junction
	{
		uint32 westEastIndex;
		uint32 northSouthIndex;
	};

	struct RoadNetwork
	{
		std::vector<TRoadVertices*> roads;
		std::vector<Junction> junctions;
	};

	struct JunctionData
	{
		Vec2 centre;
		Vec2 westOffset;
		Vec2 eastOffset;
		Vec2 southOffset;
		Vec2 northOffset;
		Vec2 westGrad;
		Vec2 eastGrad;
		Vec2 westNormal;
		Vec2 eastNormal;
		Vec2 southGrad;
		Vec2 northGrad;
		Vec2 southNormal;
		Vec2 northNormal;
		Vec2 westDelta;
		Vec2 eastDelta;
		Vec2 southDelta;
		Vec2 northDelta;
		Vec2 southWestCurb;
		Vec2 northWestCurb;
		Vec2 southEastCurb;
		Vec2 northEastCurb;

		Vec3 up{ 0,0,1 };
		RGBAb roadColour;
	};

	void JoinRoadsInJunction(const JunctionData& j, RoadContext& c)
	{
		{
			Vec3 topRight = Vec3::FromVec2(j.southWestCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.westOffset, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset - j.westDelta, 0);
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.southWestCurb, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.southOffset, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.southOffset + j.southDelta, 0);

			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = Vec3::FromVec2(j.southOffset, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.southOffset - j.southDelta, 0);
			Vec3 topRight = Vec3::FromVec2(j.southEastCurb, 0);
			c.cache->push_back({ topRight,	j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = Vec3::FromVec2(j.eastOffset - j.eastDelta, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southEastCurb, 0);
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}


		{
			Vec3 topLeft = Vec3::FromVec2(j.northOffset, 0);
			Vec3 topRight = Vec3::FromVec2(j.northOffset - j.northDelta, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.northEastCurb, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.eastOffset + j.eastDelta, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northEastCurb, 0);
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northOffset, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northOffset + j.northDelta, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.northWestCurb, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northWestCurb, 0);
			Vec3 topLeft = Vec3::FromVec2(j.westOffset + j.westDelta, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.westOffset, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northWestCurb, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.southWestCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.westOffset, 0);
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.southWestCurb, 0);
			Vec3 topRight = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.southOffset, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 topRight = Vec3::FromVec2(j.northEastCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.eastOffset, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.northOffset, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.northEastCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.northWestCurb, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.northWestCurb, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.southWestCurb, 0);
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northEastCurb, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northWestCurb, 0);
			c.cache->push_back({ topRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, j.roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}
	}

	void JoinPavementsInJunction(const JunctionData& j, RoadContext& c)
	{
		auto roadToPavementWidth = 3.0_metres;
		auto roadWidth = 2.0_metres;
		RGBAb pavementColour(160, 160, 160);
		
		{
			Vec3 topRight = Vec3::FromVec2(j.southOffset + j.southNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset -j.westNormal * roadToPavementWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southOffset + j.southNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}
		
		{
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset - j.westNormal * roadToPavementWidth, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.westOffset - j.westNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southOffset + j.southNormal * roadWidth, 0);
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.southOffset + j.southNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset - j.westNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southWestCurb, 0);
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northOffset + j.northNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset + j.westNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northOffset + j.northNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.northOffset + j.northNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset + j.westNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northWestCurb, 0);
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.northOffset + j.northNormal * roadToPavementWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.westOffset + j.westNormal * roadWidth, 0);
			Vec3 bottomLeft = Vec3::FromVec2(j.westOffset + j.westNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}


		{
			Vec3 topRight = Vec3::FromVec2(j.northOffset - j.northNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset + j.eastNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.northEastCurb, 0);
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = Vec3::FromVec2(j.northOffset - j.northNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset + j.eastNormal * roadWidth, 0);
			Vec3 topRight = Vec3::FromVec2(j.eastOffset + j.eastNormal * roadToPavementWidth, 0);
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topLeft = Vec3::FromVec2(j.northOffset - j.northNormal * roadWidth, 0);
			Vec3 topRight = Vec3::FromVec2(j.northOffset - j.northNormal * roadToPavementWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset + j.eastNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 topRight = Vec3::FromVec2(j.southOffset - j.southNormal * roadWidth, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southOffset - j.southNormal * roadToPavementWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset - j.eastNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 topLeft = Vec3::FromVec2(j.eastOffset - j.eastNormal * roadWidth, 0);
			Vec3 topRight = Vec3::FromVec2(j.eastOffset - j.eastNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ topRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		{
			Vec3 bottomLeft = Vec3::FromVec2(j.southEastCurb, 0);
			Vec3 topLeft = Vec3::FromVec2(j.southOffset - j.southNormal * roadWidth, 0);
			Vec3 bottomRight = Vec3::FromVec2(j.eastOffset - j.eastNormal * roadToPavementWidth, 0);
			c.cache->push_back({ topLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomLeft, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
			c.cache->push_back({ bottomRight, j.up, pavementColour, RGBAb(255,255,255, 0), 0, 1 });
		}
	}

	void GenerateJunction(const RoadNode& left, const RoadNode& right, const RoadNode& south, const RoadNode& north, RoadContext& c)
	{
		c.cache->clear();

		JunctionData j;

		j.roadColour = RGBAb(128, 128, 128);

		auto roadWidth = 2.0_metres;

		j.centre = 0.5f * (left.location + right.location);

		j.westOffset = left.location - j.centre;
		j.southOffset = south.location - j.centre;
		j.eastOffset = right.location - j.centre;
		j.northOffset = north.location - j.centre;

		j.westGrad = left.gradient;
		j.eastGrad = right.gradient;

		j.westNormal = { -j.westGrad.y, j.westGrad.x };
		j.eastNormal = { -j.eastGrad.y, j.eastGrad.x };

		j.southGrad = south.gradient;
		j.northGrad = north.gradient;

		j.southNormal = { -j.southGrad.y, j.southGrad.x };
		j.northNormal = { -j.northGrad.y, j.northGrad.x };

		j.westDelta = roadWidth * Vec2{ j.westNormal.x,  j.westNormal.y };
		j.eastDelta = roadWidth * Vec2{ j.eastNormal.x, j.eastNormal.y };

		j.southDelta = roadWidth * Vec2{ j.southNormal.x, j.southNormal.y };
		j.northDelta = roadWidth * Vec2{ j.northNormal.x, j.northNormal.y };
		
		j.southWestCurb = GetIntersect(j.westOffset - j.westDelta, j.westGrad, j.southOffset + j.southDelta, j.southGrad);
		j.southEastCurb = GetIntersect(j.eastOffset - j.eastDelta, j.eastGrad, j.southOffset - j.southDelta, j.southGrad);
		j.northWestCurb = GetIntersect(j.westOffset + j.westDelta, j.westGrad, j.northOffset + j.northDelta, j.northGrad);
		j.northEastCurb = GetIntersect(j.eastOffset + j.eastDelta, j.eastGrad, j.northOffset - j.northDelta, j.northGrad);

		JoinRoadsInJunction(j, c);
		JoinPavementsInJunction(j, c);

		int bodyIndex = c.index + 0x20000001;
		c.index++;

		auto id = ID_MESH(bodyIndex);
		c.e->meshes.BuildMesh(&(c.cache->at(0)), c.cache->size(), id, false);

		c.e->level.Builder().AddSolid(Vec3::FromVec2(j.centre, 0.0f), id, SolidFlags_None);
	}

	void GenerateJunction(const RoadNode& left, const RoadNode& right, RoadNetwork& network, const TRoadVertices& road, RoadContext& c)
	{
		int junctionIndex = left.junctionIndex;

		auto junction = network.junctions[junctionIndex - 1];

		if (road[0].location.x == road[road.size()-1].location.x)
		{
			// north south road. Skip
			return;
		}

		TRoadVertices& westEastRoad = *network.roads[junction.westEastIndex];
		TRoadVertices& northSouthRoad = *network.roads[junction.northSouthIndex];

		for (int i = 1; i < northSouthRoad.size()-1; ++i)
		{
			auto& nsSegment = northSouthRoad[i];
			if (nsSegment.junctionIndex == junctionIndex)
			{
				GenerateJunction(left, right, northSouthRoad[i], northSouthRoad[i + 1], c);
				break;
			}
		}
	}

	void GenerateRoadSegment(Vec2 left, Vec2 right, Vec2 leftGrad, Vec2 rightGrad, RoadContext& c, const wchar_t* name, size_t streetAddress)
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
		c.e->meshes.BuildMesh(&(c.cache->at(0)), c.cache->size(), id, false);

		ID_ENTITY roadId = c.e->level.Builder().AddSolid(Vec3{ midPoint.x, midPoint.y, 0.0f }, id, SolidFlags_RoadSection);

		wchar_t streetName[256];
		SafeFormat(streetName, _TRUNCATE, L"%I64u-%I64u %s", streetAddress, streetAddress + 1, name);
		c.e->level.Builder().Name(roadId, to_fstring(streetName));
	}

	void DivideRoadWE(Vec2 left, Vec2 right, TRoadVertices& v, Randomizer& rng)
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
		
		DivideRoadWE(left, node, v, rng);
		DivideRoadWE(node, right, v, rng);
	}

	void DivideRoadSN(Vec2 left, Vec2 right, TRoadVertices& v, Randomizer& rng)
	{
		Vec2 midPoint = 0.5f * (left + right);
		Vec2 randomDelta = { 0.0625f * RandomQuotient(rng, -1.0f, 1.0f) * Length(right - left), 0.0f };
		Vec2 node = midPoint + randomDelta;

		auto meshLength = 10.0_metres;
		if (LengthSq(right - left) < Square(meshLength))
		{
			v.push_back({ { 0,0 }, node });
			return;
		}

		DivideRoadSN(left, node, v, rng);
		DivideRoadSN(node, right, v, rng);
	}

	void BuildNorthSouthRoad(TRoadVertices& v, RoadNetwork& network, Randomizer& rng, uint32 index, Vec2 centre, uint32 junctionIndex)
	{
		Vec2 southPoint = { centre.x, -250.0_metres };
		Vec2 northPoint = { centre.x, 250.0_metres };

		v.push_back(RoadNode{ { 0, 1 }, southPoint, 0 });
		v.push_back(RoadNode{ { 0, 1 }, northPoint, 0 });

		//v.push_back({ { 0,0 }, midPoint });

		DivideRoadSN(southPoint, centre, v, rng);
		DivideRoadSN(centre, northPoint, v, rng);

		std::sort(v.begin(), v.end(), [](RoadNode a, RoadNode b) {
			return a.location.y < b.location.y;
		});

		v[0].gradient = v[v.size() - 1].gradient = { 0, 1 };

		for (size_t i = 1; i < v.size() - 1; ++i)
		{
			Vec2 delta = v[i + 1].location - v[i - 1].location;
			v[i].gradient = Normalize(delta);
			v[i].junctionIndex = 0;
		}

		for (size_t i = 1; i < v.size() - 1; ++i)
		{
			Vec2 southBit = 0.5f * (v[i].location + v[i + 1].location);
			if (LengthSq(southBit - centre) < Square(1.0))
			{
				v[i].junctionIndex = junctionIndex;
				v[i+1].junctionIndex = junctionIndex;
			}
		}
	}

	void BuildWestEastRoad(TRoadVertices& v, RoadNetwork& network, Randomizer& rng, uint32 index)
	{
		Vec2 westPoint = { -250.0_metres, 0.0f };
		Vec2 eastPoint = { 250.0_metres, 0.0f };

		Vec2 midPoint = 0.5f * (westPoint + eastPoint);

		DivideRoadWE(westPoint, midPoint, v, rng);
		DivideRoadWE(midPoint, eastPoint, v, rng);
		
		std::sort(v.begin(), v.end(), [](RoadNode a, RoadNode b) {
			return a.location.x < b.location.x;
		});

		v[0].gradient = v[v.size() - 1].gradient = {1, 0};

		for (size_t i = 1; i < v.size() - 1; ++i)
		{
			Vec2 delta = v[i + 1].location - v[i - 1].location;
			v[i].gradient = Normalize(delta);
			v[i].junctionIndex = 0;
		}

		for (size_t i = 1; i < v.size() - 1; ++i)
		{
			if ((rng() % 4) == 3)
			{
				uint32 junctionIndex = (uint32) network.junctions.size()+1;
				v[i].junctionIndex = junctionIndex;
				v[i+1].junctionIndex = junctionIndex;

				Vec2 centre = 0.5f * (v[i].location + v[i + 1].location);

				auto newRoad = new TRoadVertices();
				network.roads.push_back(newRoad);
				TRoadVertices& northSouthRoad = *newRoad;

				network.junctions.push_back(Junction{ index,  (uint32)network.roads.size()-1 });
				
				BuildNorthSouthRoad(northSouthRoad, network, rng, (uint32) network.roads.size(), centre, junctionIndex);
				i += 8;	
			}
		}
	}

	ID_MESH GetOneMeshOf(const std::vector<ID_MESH>& meshes, Randomizer& rng)
	{
		return meshes[rng() % meshes.size()];
	}

	void BuildHouses(TRoadVertices& road, Environment& e, Randomizer& rng)
	{
		std::vector<ID_MESH> meshes;

		for (int i = 0; i < 20; ++i)
		{
			meshes.push_back(GenerateRandomHouse(e, rng()));
		}

		for (int i = 1; i < road.size(); i++)
		{
			if (road[i-1].junctionIndex != 0 || road[i].junctionIndex != 0 || road[i+1].junctionIndex != 0)
			{
				continue;
			}

			Vec2 normal = { -road[i].gradient.y, road[i].gradient.x };

			Vec2 housePosition = road[i].location + 15.0f * normal;

			Radians theta{ acosf(Dot(normal, Vec2{ 1.0f, 0.0f })) };
			
			auto id = e.level.Builder().AddSolid({ housePosition.x, housePosition.y, 0.5f }, GetOneMeshOf(meshes, rng), SolidFlags_Obstacle);
			e.level.SetHeading(id, theta);

			housePosition = road[i].location - 15.0f * normal;

			id = e.level.Builder().AddSolid({ housePosition.x, housePosition.y, 0.5f }, GetOneMeshOf(meshes, rng), SolidFlags_Obstacle);
			e.level.SetHeading(id, theta);
		}
	}

	uint32 Next(Randomizer& rng, uint32 modulus)
	{
		return rng() % modulus;
	}

	void GenRandomStreetName(wchar_t randomName[256], Randomizer& rng)
	{
		const wchar_t* consonants[] = {
			L"b",
			L"c",
			L"e",
			L"f",
			L"g",
			L"h",
			L"j",
			L"k",
			L"l",
			L"m",
			L"n",
			L"p",
			L"qu",
			L"r",
			L"s",
			L"t",
			L"v",
			L"w",
			L"y",
			L"z",
			L"th",
			L"gh",
			L"ph",
			L"st",
			L"br",
			L"cr",
			L"dr",
			L"fr",
			L"gr",
			L"pr",
			L"wr",
			L"sc",
		};

		const wchar_t* vowels[] = {
			L"a",
			L"e",
			L"i",
			L"o",
			L"oo",
			L"u",
			L"ou",
			L"y",
		};

		const wchar_t* finalNames[] =
		{
			L"road",
			L"drive",
			L"street",
			L"lane",
			L"way"
		};

		randomName[0] = 0;

		uint32 lenOfName = 2 + Random::Next(4);
		for (uint32 i = 0; i < lenOfName; ++i)
		{
			const wchar_t* c = consonants[Next(rng, sizeof(consonants) / sizeof(const wchar_t*))];
			SafeCat(randomName, 256, c, _TRUNCATE);

			const wchar_t* v = vowels[Next(rng, sizeof(vowels) / sizeof(const wchar_t*))];
			SafeCat(randomName, 256, v, _TRUNCATE);
		}

		SafeCat(randomName, 256, L" ", _TRUNCATE);

		const wchar_t* fn = finalNames[Next(rng, sizeof(finalNames) / sizeof(const wchar_t*))];

		SafeCat(randomName, 256, fn, _TRUNCATE);

		randomName[0] = toupper(randomName[0]);
	}
}

namespace Dystopia
{
	void BuildRandomCity(const fstring& name, uint32 seedDelta, Environment& e, IEnumerable<const wchar_t*>& names)
	{
		uint32 hash = FastHash(name);
		Randomizer rng(hash + seedDelta);

		RoadNetwork network;
		TRoadVertices* mainRoad = new TRoadVertices();
		network.roads.push_back(mainRoad);

		BuildWestEastRoad(*mainRoad, network, rng, 0);

		std::vector<ObjectVertex> cache;
		RoadContext c{ &e, 0, &rng, &cache };

		struct : IEnumerator<const wchar_t*>
		{
			std::vector<std::wstring> names;

			virtual void operator()(const wchar_t* streetName)
			{
				names.push_back(streetName);
			}
		} streets;

		names.Enumerate(streets);

		if (network.roads.size() > streets.names.size())
		{
			size_t undefinedNames =  network.roads.size() - streets.names.size();
			while (undefinedNames > 0)
			{
				wchar_t randomName[256];
				GenRandomStreetName(randomName, rng);
				streets.names.push_back(randomName);
				undefinedNames--;
			}
		}
		
		for (size_t j = 0; j < network.roads.size(); ++j)
		{
			auto roadPtr = network.roads[j];
			auto& road = *roadPtr;

			for (size_t i = 0; i < road.size() - 1; i++)
			{
				auto left = road[i];
				auto right = road[i + 1];

				if (left.junctionIndex != 0 && right.junctionIndex != 0)
				{
					GenerateJunction(left, right, network, road, c);
				}
				else
				{
					GenerateRoadSegment(left.location, right.location, left.gradient, right.gradient, c, streets.names[j].c_str(), 2 * i + 1);
				}
			}

			BuildHouses(road, e, rng);
		}
	}
}
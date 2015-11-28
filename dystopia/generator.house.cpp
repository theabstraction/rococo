#include "dystopia.h"
#include "meshes.h"
#include <vector>

namespace
{
	using namespace Rococo;
	using namespace Dystopia;

	typedef std::vector<ObjectVertex> TObjectVertices;

	float RandomQuotient(IRandom& rng, float minValue, float maxValue)
	{
		float range = maxValue - minValue;

		uint32 r = rng();
		float value = (float)r / (float)0xFFFFFFFF;
		return min(value * range + minValue, maxValue);
	}

	/*
	struct ObjectVertex
	{
		Vec3 position;
		Vec3 normal;
		RGBAb emissiveColour;
		RGBAb diffuseColour;
		float u;
		float v;
	};
	*/

	const Vec3 north{ 0,  1, 0 };
	const Vec3 east { 1,  0, 0 };
	const Vec3 south{ 0, -1, 0 };
	const Vec3 west {-1,  0, 0 };
	const Vec3 up   { 0,  0, 1 };

	ID_MESH GenNextHouseId()
	{
		static ID_MESH nextId(0x21000000);
		nextId = ID_MESH(nextId.value + 1);
		return nextId;
	}

	void AddColourVertex(TObjectVertices& vertices, RGBAb colour, cr_vec3 position, cr_vec3 normal)
	{
		vertices.push_back({ position, normal, colour, RGBAb(0,0,0,0), 0, 0});
	}

	void AddQuad(TObjectVertices& vertices, Vec3 bottomLeft, Vec3 bottomRight, Vec3 topLeft, Vec3 topRight, Vec3 normal, RGBAb colour)
	{
		AddColourVertex(vertices, colour, bottomLeft, normal);
		AddColourVertex(vertices, colour, topLeft, normal);
		AddColourVertex(vertices, colour, bottomRight, normal);

		AddColourVertex(vertices, colour, topLeft, normal);
		AddColourVertex(vertices, colour, topRight, normal);
		AddColourVertex(vertices, colour, bottomRight, normal);
	}

	void AddPanel(TObjectVertices& vertices, Vec2 left, Vec2 right, float bottom, float top, Vec3 normal, RGBAb colour)
	{
		Vec3 bottomLeft{ left.x, left.y, bottom };
		Vec3 bottomRight{ right.x, right.y, bottom };
		Vec3 topLeft{ left.x, left.y, top };
		Vec3 topRight{ right.x, right.y, top };
		AddQuad(vertices, bottomLeft, bottomRight, topLeft, topRight, normal, colour);
	}

	void AddArchedRoof(TObjectVertices& vertices, Vec2 southWest, Vec2 northEast, float eaveSpan,  Metres height, Metres archHeight, RGBAb colour1, RGBAb colour2)
	{
		// West tri
		{
			Vec3 westLeft{ southWest.x, southWest.y, height };
			Vec3 westRight{ southWest.x, northEast.y, height };
			Vec3 westZennith{ southWest.x, 0, height + archHeight };
			AddColourVertex(vertices, colour1, westLeft, west);
			AddColourVertex(vertices, colour1, westRight, west);
			AddColourVertex(vertices, colour1, westZennith, west);
		}

		// East tri
		{
			Vec3 eastLeft{ northEast.x, southWest.y, height };
			Vec3 eastRight{ northEast.x, northEast.y, height };
			Vec3 eastZennith{ northEast.x, 0, height + archHeight };
			AddColourVertex(vertices, colour1, eastRight, east);
			AddColourVertex(vertices, colour1, eastLeft, east);
			AddColourVertex(vertices, colour1, eastZennith, east);
		}

		// North roof
		{
			float archY = height + archHeight;
			float eveY = height - eaveSpan;
			Vec3 topLeft{ southWest.x - eaveSpan, northEast.y, height };
			Vec3 topRight{ northEast.x + eaveSpan, northEast.y, height };
			Vec3 bottomLeft{ southWest.x - eaveSpan,0, archY };
			Vec3 bottomRight{ northEast.x + eaveSpan, 0, archY };

			float archSpan = topLeft.y - bottomLeft.y;
			AddQuad(vertices, bottomLeft, bottomRight, topLeft, topRight, Normalize({ 0, archHeight, archSpan }), colour2);

			Vec3 eveTopLeft = Vec3 { southWest.x - eaveSpan, northEast.y + eaveSpan, eveY };
			Vec3 eveTopRight = Vec3{ northEast.x + eaveSpan, northEast.y + eaveSpan, eveY };
			Vec3 eveBottomLeft = topLeft;
			Vec3 eveBottomRight = topRight;
			
			AddQuad(vertices, eveBottomLeft, eveBottomRight, eveTopLeft, eveTopRight, Normalize({ 0, archHeight, archSpan,  }), colour2);
		}

		// South roof
		{
			float archY = height + archHeight;
			float eveY = height - eaveSpan;
			Vec3 bottomLeft{ southWest.x - eaveSpan, southWest.y, height };
			Vec3 bottomRight{ northEast.x + eaveSpan, southWest.y, height };
			Vec3 topLeft{ southWest.x - eaveSpan,0, archY };
			Vec3 topRight{ northEast.x + eaveSpan, 0, archY };

			float archSpan = topLeft.y - bottomLeft.y;
			AddQuad(vertices, bottomLeft, bottomRight, topLeft, topRight, Normalize({ 0, -archHeight, archSpan }), colour2);

			Vec3 eveTopLeft = bottomLeft;
			Vec3 eveTopRight = bottomRight;
			Vec3 eveBottomLeft = Vec3{ southWest.x - eaveSpan, southWest.y - eaveSpan, eveY };
			Vec3 eveBottomRight = Vec3{ northEast.x + eaveSpan, southWest.y - eaveSpan, eveY };

			AddQuad(vertices, eveBottomLeft, eveBottomRight, eveTopLeft, eveTopRight, Normalize({ 0, -archHeight, archSpan }), colour2);
		}
	}

	void AddTetraRoof(TObjectVertices& vertices, Vec2 southWest, Vec2 northEast, float eaveSpan, Metres height, Metres archHeight, RGBAb colour)
	{
		Vec3 zennith{ 0, 0, height + archHeight };

		// West tri
		{
			Vec3 westNormal = Normalize(Vec3{ -archHeight, 0.0f, (northEast.x - southWest.x) * 0.5f });
			Vec3 westLeft{ southWest.x, northEast.y, height };
			Vec3 westRight{ southWest.x, southWest.y, height };

			AddColourVertex(vertices, colour, westLeft, westNormal);
			AddColourVertex(vertices, colour, zennith, westNormal);
			AddColourVertex(vertices, colour, westRight, westNormal);	
		}

		// East tri
		{
			Vec3 eastNormal = Normalize(Vec3{ archHeight, 0.0f, (northEast.x - southWest.x) * 0.5f });
			Vec3 eastLeft{ northEast.x, southWest.y, height };
			Vec3 easttRight{ northEast.x, northEast.y, height };
			AddColourVertex(vertices, colour, easttRight, eastNormal);
			AddColourVertex(vertices, colour, eastLeft, eastNormal);
			AddColourVertex(vertices, colour, zennith, eastNormal);
		}

		// North roof
		{
			Vec3 northNormal = Normalize(Vec3{ 0.0f, archHeight, 0.5f * (northEast.y - southWest.y) });
			Vec3 northLeft{ northEast.x, northEast.y, height };
			Vec3 northRight{ southWest.x, northEast.y, height };

			AddColourVertex(vertices, colour, northLeft, northNormal);
			AddColourVertex(vertices, colour, zennith, northNormal);
			AddColourVertex(vertices, colour, northRight, northNormal);
			
		}

		// South roof
		{
			Vec3 southNormal = Normalize(Vec3{ 0.0f, -archHeight, 0.5f * (northEast.y - southWest.y) });
			Vec3 southLeft{ southWest.x, southWest.y, height };
			Vec3 southRight{ northEast.x, southWest.y, height };

			AddColourVertex(vertices, colour, southLeft, southNormal);
			AddColourVertex(vertices, colour, zennith, southNormal);
			AddColourVertex(vertices, colour, southRight, southNormal);	
		}
	}

	const float minFeatureBorder = 0.2_metres;
	const float minWindowLength = 1.5_metres;
	const float doorWidth = 0.75_metres;

	struct BuildingSpec
	{
		float levelHeight;
		float windowHeightAboveGround;
		float windowBaseAboveGround;
	};

	Segment<Vec2> AddRandomDoor(TObjectVertices& vertices, Vec2 left, Vec2 right, cr_vec3 normal, IRandom& rng)
	{
		Vec2 proj = 0.01f * Vec2{ normal.x, normal.y };

		float len = Length(right - left);
		float t = RandomQuotient(rng, minFeatureBorder, len - doorWidth - minFeatureBorder) / len;

		Vec2 doorLeft = Lerp(left, right, t);
		Vec2 doorRight = Lerp(left, right, t + doorWidth / len);

		AddPanel(vertices, doorLeft + proj, doorRight + proj, 0, 2.0_metres, normal, RGBAb(255,200,255));

		if (t < 0.5f && (rng() % 4) == 0)
		{
			Vec2 door2Left = Lerp(left, right, t + doorWidth / len + 0.01f / len);
			Vec2 door2Right = Lerp(left, right, t + 2.0f * doorWidth / len + 0.01f / len);
			AddPanel(vertices, door2Left + proj, door2Right + proj, 0, 2.0_metres, normal, RGBAb(255, 200, 255));
			return{ doorLeft, door2Right };
		}
		else
		{
			return{ doorLeft, doorRight };
		}
	}

	void AddFace(TObjectVertices& vertices, BuildingSpec& spec, Vec2 left, Vec2 right, float roofHeight, cr_vec3 normal, RGBAb colour, IRandom& rng)
	{
		AddPanel(vertices, left, right, 0, roofHeight, normal, colour);
		if (rng() % 2 == 0)
		{
			auto door = AddRandomDoor(vertices, left, right, normal, rng);

			float len = Length(door.a - left);

			float groundWindowHeight = RandomQuotient(rng, 0.25f, 1.25f);

			if (len > 2.0f * minFeatureBorder + minWindowLength)
			{
				float t0 = minFeatureBorder / len;
				float t1 = (len - minFeatureBorder) / len;

				Vec2 proj{ normal.x, normal.y };
				
				Vec2 windowLeft = Lerp(left, door.a, t0) + proj * 0.01f;
				Vec2 windowRight = Lerp(left, door.a, t1) + proj * 0.01f;
				
				AddPanel(vertices, windowLeft, windowRight, groundWindowHeight, 2.15f, normal, RGBAb(255,255,255));
			}

			len = Length(right - door.b);

			if (len > 2.0f * minFeatureBorder + minWindowLength)
			{
				float t0 = minFeatureBorder / len;
				float t1 = (len - minFeatureBorder) / len;

				Vec2 proj{ normal.x, normal.y };

				Vec2 windowLeft = Lerp(door.b, right, t0) + proj * 0.01f;
				Vec2 windowRight = Lerp(door.b, right, t1) + proj * 0.01f;

				AddPanel(vertices, windowLeft, windowRight, groundWindowHeight, 2.15f, normal, RGBAb(255, 255, 255));
			}
		}

		float len = Length(right - left);

		float tStart = minFeatureBorder / len;
		float tEnd = 1.0f - tStart;

		uint32 maxWindows = (uint32) (len / 0.5f);

		uint32 nWindows = 1 + (rng() % maxWindows);

		float dt = 0.5f * (minFeatureBorder / len);
		float DT = (tEnd - tStart) / nWindows;
		
		Vec2 proj{ normal.x, normal.y };

		for (float z0 = spec.levelHeight; z0 + spec.levelHeight - 0.01f < roofHeight; z0 += spec.levelHeight)
		{
			float t = tStart;
			
			for (uint32 i = 0; i < nWindows; ++i, t += DT)
			{
				float t0 = t + dt;
				float t1 = t0 + DT - dt;

				Vec2 windowLeft = Lerp(left, right, t0) + proj * 0.01f;
				Vec2 windowRight = Lerp(left, right, t1) + proj * 0.01f;

				AddPanel(vertices, windowLeft, windowRight, z0 + spec.windowBaseAboveGround, z0 + spec.windowHeightAboveGround, normal, RGBAb(255, 255, 255));
			}
		}
	}

	void BuildHouse(TObjectVertices& vertices, Environment& e, uint32 seed)
	{
		Random::RandomMT rng(seed);

		float dx = RandomQuotient(rng, 2.0_metres, 4.0_metres);
		float dy = RandomQuotient(rng, 2.0_metres, 4.0_metres);

		float x0 = -dx;
		float x1 = dx;
		float y0 = -dy;
		float y1 = dy;

		Metres height{ RandomQuotient(rng, 4.0_metres, 20.0_metres) };
		Metres archheight{ RandomQuotient(rng, 0.0_metres, 4.0_metres) };

		if (height > 10.0f && (rng() % 2) == 0) archheight = 0.0_metres;

		uint32 colour = rng();
		RGBAb wallColour0 = RGBAb(colour, colour >> 8, colour >> 16);

		colour = rng();
		RGBAb wallColour1 = RGBAb(colour, colour >> 8, colour >> 16);
		RGBAb wallColour2 = RGBAb(wallColour0.red ^ wallColour1.green, wallColour0.blue ^ wallColour1.green, wallColour0.red ^ wallColour1.blue);
		
		BuildingSpec spec;
		spec.levelHeight = RandomQuotient(rng, 2.4f, 3.0f);
		spec.windowHeightAboveGround = RandomQuotient(rng, 2.0f, 2.35f);
		spec.windowBaseAboveGround = RandomQuotient(rng, 0.2f, 0.5f);

		AddFace(vertices, spec, { x0, y0 }, { x1, y0 }, height, south, wallColour0, rng);
		AddFace(vertices, spec, { x0, y1 }, { x0, y0 }, height, west, wallColour0, rng);
		AddFace(vertices, spec, { x1, y0 }, { x1, y1 }, height, east, wallColour0, rng);
		AddFace(vertices, spec, { x1, y1 }, { x0, y1 }, height, north, wallColour0, rng);

		if (archheight == 0)
		{
			AddArchedRoof(vertices, { x0, y0 }, { x1, y1 }, 0.1_metres, height, archheight, wallColour0, wallColour0);
		}
		else if (rng() % 2)
		{
			AddArchedRoof(vertices, { x0, y0 }, { x1, y1 }, 0.1_metres, height, archheight, wallColour0, RGBAb(128, 192, 192));
		}
		else
		{
			AddTetraRoof(vertices, { x0, y0 }, { x1, y1 }, 0.1_metres, height, archheight, RGBAb(192, 192, 128));
		}
	}
}

namespace Dystopia
{
	ID_MESH GenerateRandomHouse(Environment& e, uint32 seed)
	{
		static TObjectVertices cache;
		cache.clear();
		BuildHouse(cache, e, seed);

		auto id = GenNextHouseId();
		e.meshes.BuildMesh(&cache[0], cache.size(), id, true);
		return id;
	}
}
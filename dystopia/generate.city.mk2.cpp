#include "dystopia.h"
#include "rococo.maths.h"
#include "rococo.strings.h"
#include "rococo.renderer.h"
#include "meshes.h"
#include <algorithm>
#include <malloc.h>
#include <vector>

namespace
{
	using namespace Rococo;

	enum CompassDirection
	{
		CompassDirection_North,
		CompassDirection_East,
		CompassDirection_South,
		CompassDirection_West
	};

	enum Quadrant
	{
		NW,
		NE,
		SE,
		SW
	};

	struct CityCell
	{
		static const Metres cellWidth;
		bool isRoadConnected[4]; // One boolean for each of the 4 compass directions
		bool isConnectedToMainRoad;
		Vec2i index;

		CityCell()
		{
			isRoadConnected[0] = isRoadConnected[1] = isRoadConnected[2] = isRoadConnected[3] = false;
			isConnectedToMainRoad = false;
		}
	};

	const Metres CityCell::cellWidth = 100.0_metres;

	Vec3 GetQuadrantVectorOffset(Quadrant q)
	{
		static Vec3 offsets[4] =
		{
			{ -0.25f, 0.25f, 0 },
			{  0.25f, 0.25f, 0 },
			{  0.25f,-0.25f, 0 },
			{ -0.25f,-0.25f, 0 },
		};

		return offsets[q];
	}

	GuiRectf GetQuadrant(Quadrant q)
	{
		Vec3 centre = CityCell::cellWidth * GetQuadrantVectorOffset(q);
		return GuiRectf
		{
			centre.x - CityCell::cellWidth * 0.125f,
			centre.y + CityCell::cellWidth * 0.125f,
			centre.x + CityCell::cellWidth * 0.125f,
			centre.y - CityCell::cellWidth * 0.125f,
		};
	}

	Vec3 GetCompassDirectionAsVec3(CompassDirection direction)
	{
		static Vec3 centre[4] =
		{
			{ 0,  1, 0 },
			{ 1,  0, 0 },
			{ 0, -1, 0 },
			{-1,  0, 0 },
		};

		return centre[direction];
	}

	const Vec3 GetQuadrantCentreOffset(CompassDirection direction)
	{
		Vec3 v3 = GetCompassDirectionAsVec3(direction);
		return 0.25f * CityCell::cellWidth * v3;
	}

	typedef std::vector<Vec2i> TIndices;

	struct CityCells
	{
		const size_t cellsPerSide;
		std::vector<CityCell> cells;	

		CityCells(size_t _cellsPerSide) : 
			cellsPerSide(_cellsPerSide),
			cells(_cellsPerSide * _cellsPerSide)
		{
			for (int32 j = 0; j < _cellsPerSide; ++j)
			{
				for (int32 i = 0; i < _cellsPerSide; ++i)
				{
					cells[i + j * _cellsPerSide].index = { i, j };
				}
			}
		}

		CityCell& operator[](Vec2i index) { return cells[index.x + index.y * cellsPerSide]; }
		const CityCell& operator[](Vec2i index) const { return cells[index.x + index.y * cellsPerSide]; }
	};

	void AppendBoundedNeighbours(TIndices& neighbours, Vec2i node, int32 citySpan)
	{
		if (node.x > 0)
		{
			neighbours.push_back({ node.x - 1,node.y });
		}

		if (node.x < citySpan - 1)
		{
			neighbours.push_back({ node.x + 1,node.y });
		}

		if (node.y > 0)
		{
			neighbours.push_back({ node.x,node.y - 1 });
		}

		if (node.y < citySpan - 1)
		{
			neighbours.push_back({ node.x,node.y + 1 });
		}
	}

	void Join(CityCell& startCell, CityCell& neighbourCell)
	{
		if (neighbourCell.index.x < startCell.index.x)
		{
			startCell.isRoadConnected[CompassDirection_West] = true;
			neighbourCell.isRoadConnected[CompassDirection_East] = true;
		}
		else if (neighbourCell.index.x > startCell.index.x)
		{
			startCell.isRoadConnected[CompassDirection_East] = true;
			neighbourCell.isRoadConnected[CompassDirection_West] = true;
		}
		else if (neighbourCell.index.y > startCell.index.y)
		{
			startCell.isRoadConnected[CompassDirection_North] = true;
			neighbourCell.isRoadConnected[CompassDirection_South] = true;
		}
		else
		{
			startCell.isRoadConnected[CompassDirection_South] = true;
			neighbourCell.isRoadConnected[CompassDirection_North] = true;
		}

		if (neighbourCell.isConnectedToMainRoad)
		{
			startCell.isConnectedToMainRoad = true;
		}
	}

	void BuildStraightQuad(const CityCell& cell, std::vector<ObjectVertex>& vertexCache, Vec3 bottomLeft, Vec3 bottomRight, Vec3 topLeft, Vec3 topRight)
	{
		Vec3 up{ 0,0,1 };
		RGBAb roadColour(128, 128, 128);
		RGBAb noTexture(255, 255, 255, 0);

		vertexCache.push_back({ bottomLeft,		up, roadColour, noTexture, 0, 1 });
		vertexCache.push_back({ topRight,		up, roadColour, noTexture, 0, 1 });
		vertexCache.push_back({ bottomRight,	up, roadColour, noTexture, 0, 1 });
		vertexCache.push_back({ bottomLeft,		up, roadColour, noTexture, 0, 1 });
		vertexCache.push_back({ topLeft,		   up, roadColour, noTexture, 0, 1 });
		vertexCache.push_back({ topRight,		up, roadColour, noTexture, 0, 1 });
	}

	void BuildWhiteQuad(const CityCell& cell, std::vector<ObjectVertex>& vertexCache, Vec2 bottomLeft, Vec2 bottomRight, Vec2 topLeft, Vec2 topRight)
	{
		Vec3 up{ 0,0,1 };
		RGBAb white(255, 255, 255);
		RGBAb noTexture(255, 255, 255, 0);

		vertexCache.push_back({Vec3::FromVec2(bottomLeft, 0.02f),    up, white, noTexture, 0, 1 });
		vertexCache.push_back({ Vec3::FromVec2(topRight,  0.02f),	 up, white, noTexture, 0, 1 });
		vertexCache.push_back({ Vec3::FromVec2(bottomRight, 0.02f),  up, white, noTexture, 0, 1 });
		vertexCache.push_back({ Vec3::FromVec2(bottomLeft, 0.02f),   up, white, noTexture, 0, 1 });
		vertexCache.push_back({ Vec3::FromVec2(topLeft, 0.02f),	     up, white, noTexture, 0, 1 });
		vertexCache.push_back({ Vec3::FromVec2(topRight, 0.02f),	 up, white, noTexture, 0, 1 });
	}

	namespace Stripes
	{
		auto stripeWidth = 0.1_metres;
		float stripeBorder = 1.0_metres;
		float stripeLength = 4.0_metres;
	}

	void BuildStraightRoad(const CityCell& cell, std::vector<ObjectVertex>& vertexCache, Vec2 bottomLeftXY, Vec2 topRightXY, Vec2 directionXY)
	{
		Vec3 up{ 0,0,1 };
		RGBAb roadColour(128, 128, 128);
		RGBAb noTexture(255, 255, 255, 0);

		Vec3 bottomLeft = Vec3::FromVec2(bottomLeftXY, 0);
		Vec3 topRight = Vec3::FromVec2(topRightXY, 0);
		Vec3 bottomRight = { topRight.x, bottomLeft.y, 0 };
		Vec3 topLeft = { bottomLeft.x, topRight.y, 0 };

		BuildStraightQuad(cell, vertexCache, bottomLeft, bottomRight, topLeft, topRight);
		
		int numberOfStripesPerCell = int32(CityCell::cellWidth / (Stripes::stripeBorder + Stripes::stripeLength));
		float totalBorder = CityCell::cellWidth - numberOfStripesPerCell * Stripes::stripeLength;
		float averageBorder = totalBorder / numberOfStripesPerCell;

		if (directionXY.x != 0)
		{
			float x0 = 0.5f * averageBorder + bottomLeft.x;
			float y = 0.5f * (bottomLeftXY.y + topRightXY.y);

			while(true)
			{
				float x1 = x0 + Stripes::stripeLength;
				if (x1 >= bottomRight.x)
				{
					break;
				}

				Vec2 bottomLeftSection{ x0, y - Stripes::stripeWidth };
				Vec2 bottomRightSection{ x1, y - Stripes::stripeWidth };
				Vec2 topLeftSection { x0, y + Stripes::stripeWidth };
				Vec2 topRightSection{ x1, y + Stripes::stripeWidth };
				BuildWhiteQuad(cell, vertexCache, bottomLeftSection, bottomRightSection, topLeftSection, topRightSection);

				x0 += averageBorder + Stripes::stripeLength;
			}
		}
		else
		{
			float y0 = 0.5f * averageBorder + bottomLeft.y;
			float x = 0.5f * (bottomLeftXY.x + topRightXY.x);

			while (true)
			{
				float y1 = y0 + Stripes::stripeLength;
				if (y1 >= topRight.y)
				{
					break;
				}

				Vec2 bottomLeftSection{ x - Stripes::stripeWidth, y0 };
				Vec2 bottomRightSection{ x + Stripes::stripeWidth, y0 };
				Vec2 topLeftSection{ x - Stripes::stripeWidth, y1 };
				Vec2 topRightSection{ x + Stripes::stripeWidth, y1 };
				BuildWhiteQuad(cell, vertexCache, bottomLeftSection, bottomRightSection, topLeftSection, topRightSection);

				y0 += averageBorder + Stripes::stripeLength;
			}
		}
	}

	void BuildRoadTurn(const CityCell& cell, std::vector<ObjectVertex>& vertexCache, CompassDirection start, CompassDirection end)
	{
		const Metres roadSpan = 2.5_metres;

		Vec4 innerLoopStart;
		Vec4 outerLoopStart;
		Vec3 centre;
		Vec3 normal;

		if (start == CompassDirection_North)
		{
			Vec2 topRight = { roadSpan, cell.cellWidth * 0.5f };
			Vec2 bottomLeft = { -roadSpan, cell.cellWidth * 0.25f };
			Vec2 direction = { 0, 1 };
			BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

			if (end == CompassDirection_East)
			{
				Vec2 topRight = { cell.cellWidth * 0.5f, roadSpan, };
				Vec2 bottomLeft = { cell.cellWidth * 0.25f, -roadSpan };
				Vec2 direction = { 1, 0 };
				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

				innerLoopStart = Vec4{ 0.25f * CityCell::cellWidth, roadSpan, 0, 1 };
				outerLoopStart = Vec4{ 0.25f * CityCell::cellWidth,-roadSpan, 0, 1 };
				centre = Vec3{ 0.25f * CityCell::cellWidth, 0.25f * CityCell::cellWidth, 0 };
				normal = { 0 , -1, 0 };
			}
			else
			{
				Vec2 topRight = { -0.25f * cell.cellWidth, roadSpan, };
				Vec2 bottomLeft = { -0.5f * cell.cellWidth, -roadSpan };
				Vec2 direction = { 1, 0 };
				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

				innerLoopStart = Vec4{ -roadSpan, 0.25f * CityCell::cellWidth, 0, 1 };
				outerLoopStart = Vec4{  roadSpan, 0.25f * CityCell::cellWidth, 0, 1 };
				centre = Vec3{ -0.25f * CityCell::cellWidth, 0.25f * CityCell::cellWidth, 0};
				normal = { 1 , 0, 0 };
			}
		}
		else if(start == CompassDirection_East) // south to east
		{
			Vec2 topRight = { cell.cellWidth * 0.5f, roadSpan, };
			Vec2 bottomLeft = { cell.cellWidth * 0.25f, -roadSpan };
			Vec2 direction = { 1, 0 };
			BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

			topRight = { roadSpan, -0.25f * cell.cellWidth };
			bottomLeft = { -roadSpan, -0.5f * cell.cellWidth };
			direction = { 0, 1 };
			BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

			innerLoopStart = Vec4{  roadSpan, -0.25f * CityCell::cellWidth, 0, 1 };
			outerLoopStart = Vec4{ -roadSpan, -0.25f * CityCell::cellWidth, 0, 1 };
			centre = Vec3{ 0.25f * CityCell::cellWidth, -0.25f * CityCell::cellWidth, 0 };
			normal = { -1, 0, 0 };
		}
		else // south west
		{
			Vec2 topRight = { -0.25f * cell.cellWidth, roadSpan, };
			Vec2 bottomLeft = { -0.5f * cell.cellWidth, -roadSpan };
			Vec2 direction = { 1, 0 };
			BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

			topRight = { roadSpan, -0.25f * cell.cellWidth };
			bottomLeft = { -roadSpan, -0.5f * cell.cellWidth };
			direction = { 0, 1 };
			BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);

			innerLoopStart = Vec4{ -0.25f * CityCell::cellWidth, -roadSpan, 0, 1 };
			outerLoopStart = Vec4{ -0.25f * CityCell::cellWidth, roadSpan, 0, 1 };
			centre = Vec3{ -0.25f * CityCell::cellWidth, -0.25f * CityCell::cellWidth, 0 };
			normal = { 0 , 1, 0 };
		}

		Radians totalCurve = 90.0_degrees;

		auto moveOriginToCentre = Matrix4x4::Translate(-1.0f * centre);
		auto moveCentreToOrigin = Matrix4x4::Translate(centre);

		enum { CURVE_DIVISIONS = 40 };
		for (int i = 0; i < CURVE_DIVISIONS; ++i)
		{
			float thetaFirst = i * totalCurve / CURVE_DIVISIONS;
			float thetaSecond = (i + 1) * totalCurve / CURVE_DIVISIONS;

			auto Rfirst = Matrix4x4::RotateRHAnticlockwiseZ(Radians{ -thetaFirst });
			auto Rsecond = Matrix4x4::RotateRHAnticlockwiseZ(Radians{ -thetaSecond });

			Matrix4x4 T_RT1 = moveCentreToOrigin * Rfirst * moveOriginToCentre;
			Matrix4x4 T_RT2 = moveCentreToOrigin * Rsecond * moveOriginToCentre;

			Vec4 topLeft = T_RT1 * innerLoopStart;
			Vec4 topRight = T_RT1 * outerLoopStart;

			Vec4 bottomLeft = T_RT2 * innerLoopStart;
			Vec4 bottomRight = T_RT2 * outerLoopStart;

			BuildStraightQuad(cell, vertexCache, bottomLeft, bottomRight, topLeft, topRight);

			if (((i+1) & 3) != 0)
			{
				Vec3 loopCentre = 0.5f * (innerLoopStart + outerLoopStart);
				Vec4 innerStripe = Vec4::FromVec3(loopCentre - normal * Stripes::stripeWidth, 1.0f);
				Vec4 outerStripe = Vec4::FromVec3(loopCentre + normal * Stripes::stripeWidth, 1.0f);

				topLeft = T_RT1 * innerStripe;
				topRight = T_RT1 * outerStripe;

				bottomLeft = T_RT2 * innerStripe;
				bottomRight = T_RT2 * outerStripe;

				BuildWhiteQuad(cell, vertexCache, 
						{ bottomLeft.x, bottomLeft.y },
						{ bottomRight.x, bottomRight.y }, 
						{ topLeft.x, topLeft.y }, 
						{ topRight.x, topRight.y }
				);
			}
		}
	}

	void BuildRoadSegment(const CityCell& cell, std::vector<ObjectVertex>& vertexCache)
	{
		const Metres roadSpan = 2.5_metres;

		if (!cell.isRoadConnected[CompassDirection_West] && !cell.isRoadConnected[CompassDirection_East])
		{
			if (cell.isRoadConnected[CompassDirection_North] && cell.isRoadConnected[CompassDirection_South])
			{
				Vec2 bottomLeft = { -roadSpan, -cell.cellWidth * 0.5f };
				Vec2 topRight = { roadSpan, cell.cellWidth * 0.5f };
				Vec2 direction = { 0, 1 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
				return;
			}
		}

		if (cell.isRoadConnected[CompassDirection_West] && cell.isRoadConnected[CompassDirection_East])
		{
			if (!cell.isRoadConnected[CompassDirection_North] && !cell.isRoadConnected[CompassDirection_South])
			{
				Vec2 bottomLeft = { -cell.cellWidth * 0.5f, -roadSpan };
				Vec2 topRight = { cell.cellWidth * 0.5f, roadSpan };
				Vec2 direction = { 1, 0 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
				return;
			}
		}

		int count = 0;
		for (int i = 0; i < 4; ++i)
		{
			if (cell.isRoadConnected[i])
			{
				count++;
			}
		}

		if (count == 2)
		{
			// since we already have checked for straight roads, we are left with a bend

			int firstIndex = -1;
			int secondIndex = -1;

			for (int i = 0; i < 4; ++i)
			{
				if (cell.isRoadConnected[i])
				{
					if (firstIndex < 0)
					{
						firstIndex = i;
					}
					else
					{
						secondIndex = i;
						break;
					}
				}
			}

			BuildRoadTurn(cell, vertexCache, (CompassDirection) firstIndex, (CompassDirection)secondIndex);
		}
		else
		{
			if (cell.isRoadConnected[CompassDirection_West])
			{
				Vec2 bottomLeft = { -cell.cellWidth * 0.5f, -roadSpan };
				Vec2 topRight   = { 0, roadSpan };
				Vec2 direction = { 1, 0 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
			}
			if (cell.isRoadConnected[CompassDirection_East])
			{
				Vec2 topRight = { cell.cellWidth * 0.5f, roadSpan };
				Vec2 bottomLeft = { 0, -roadSpan };
				Vec2 direction = { 1, 0 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
			}
			if (cell.isRoadConnected[CompassDirection_North])
			{
				Vec2 topRight = { roadSpan, cell.cellWidth * 0.5f };
				Vec2 bottomLeft = { -roadSpan, 0 };
				Vec2 direction = { 0, 1 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
			}
			if (cell.isRoadConnected[CompassDirection_South])
			{
				Vec2 topRight = { roadSpan, 0 };
				Vec2 bottomLeft = { -roadSpan, -cell.cellWidth * 0.5f };
				Vec2 direction = { 0, 1 };

				BuildStraightRoad(cell, vertexCache, bottomLeft, topRight, direction);
			}
		}
	}
/*
		if (cell.isRoadConnected[CompassDirection_North])
		{
			Vec3 bottomLeft = { -roadSpan, 0, 0 };
			Vec3 bottomRight = { roadSpan, 0, 0 };
			Vec3 topLeft = { -roadSpan, CityCell::cellWidth * 0.5f, 0 };
			Vec3 topRight = { roadSpan, CityCell::cellWidth * 0.5f, 0 };

			Vec3 up{ 0,0,1 };
			RGBAb roadColour(128, 128, 128);
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomRight,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		if (cell.isRoadConnected[CompassDirection_South])
		{
			Vec3 topLeft = { -roadSpan, 0, 0 };
			Vec3 topRight = { roadSpan, 0, 0 };
			Vec3 bottomLeft = { -roadSpan, -CityCell::cellWidth * 0.5f, 0 };
			Vec3 bottomRight = { roadSpan, -CityCell::cellWidth * 0.5f, 0 };

			Vec3 up{ 0,0,1 };
			RGBAb roadColour(128, 128, 128);
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomRight,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		if (cell.isRoadConnected[CompassDirection_East])
		{
			Vec3 bottomLeft = { 0, -roadSpan, 0 };
			Vec3 topLeft = { 0, roadSpan, 0 };
			Vec3 bottomRight = { CityCell::cellWidth * 0.5f, -roadSpan, 0 };
			Vec3 topRight = { CityCell::cellWidth * 0.5f, roadSpan, 0 };

			Vec3 up{ 0,0,1 };
			RGBAb roadColour(128, 128, 128);
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomRight,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}

		if (cell.isRoadConnected[CompassDirection_West])
		{
			Vec3 bottomRight = { 0, -roadSpan, 0 };
			Vec3 topRight = { 0, roadSpan, 0 };
			Vec3 bottomLeft = { -CityCell::cellWidth * 0.5f, -roadSpan, 0 };
			Vec3 topLeft = { -CityCell::cellWidth * 0.5f, roadSpan, 0 };

			Vec3 up{ 0,0,1 };
			RGBAb roadColour(128, 128, 128);
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomRight,	up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ bottomLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topLeft,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
			vertexCache.push_back({ topRight,		up, roadColour, RGBAb(255,255,255, 0), 0, 1 });
		}
		*/

	uint32 GetNextInRecurringSequence(IRandom& rng, uint32 recurrenceRate)
	{
		static uint32 count = 0;
		static uint32 lastGen = 0;

		if (count == 0)
		{
			count = (rng() % recurrenceRate) + 1;
			lastGen = rng();
		}
		else
		{
			count--;
		}

		return lastGen;
	}

	bool IsAtLeastOneCellConnectedToMainRoad(const CityCells& city, const TIndices& indices)
	{
		for (auto& i : indices)
		{
			if (city[i].isConnectedToMainRoad)
			{
				return true;
			}
		}

		return false;
	}

	void GenerateMinorRoadFrom(Vec2i startNode, CityCells& city, IRandom& rng, TIndices& /* cache */ neighbours, TIndices& roadBuilder)
	{
		Vec2i currentNode = startNode;

		while(true)
		{
			auto& cell = city[currentNode];

			if (cell.isConnectedToMainRoad)
			{
				break;
			}

			neighbours.clear();
			AppendBoundedNeighbours(neighbours, currentNode, (int32) city.cellsPerSide);

			if (IsAtLeastOneCellConnectedToMainRoad(city, neighbours))
			{
				// remove neighbours that are not connected to the main road
				auto j = std::remove_if(neighbours.begin(), neighbours.end(),
					[&city](const Vec2i& index)
				{
					return !city[index].isConnectedToMainRoad;
				}
				);
				neighbours.erase(j, neighbours.end()); 
			}

			if (neighbours.empty())
			{
				break; // Nowhere to go
			}

			Vec2i neighbour = neighbours[GetNextInRecurringSequence(rng, 4) % neighbours.size()];
			roadBuilder.push_back(neighbour);
			Join(cell, city[neighbour]);
			currentNode = neighbour;
		};
	}

	Vec3 GetCellCentre(const CityCell& cell)
	{
		Vec3 cellCentreOffset{ 0.5f * CityCell::cellWidth, 0.5f * CityCell::cellWidth, 0 };
		Vec3 cellBottomLeft{ cell.index.x * CityCell::cellWidth, cell.index.y * CityCell::cellWidth, 0 };
		return cellBottomLeft + cellCentreOffset;
	}

	void RemoveIndex(TIndices& indices, Vec2i index)
	{
		auto i = std::remove_if(indices.begin(), indices.end(), 
			[index](const Vec2i& node)
			{
				return node == index;
			}
		);

		indices.erase(i, indices.end());
	}

	struct CityQuad
	{
		GuiRectf quad;
		Vec3 centre;
		const CityCell& cell;
	};

	GuiRectf Merge(const GuiRectf& a, const GuiRectf& b)
	{
		float left = min(a.left, b.left);
		float right = max(a.right, b.right);
		float top = max(a.top, b.top);
		float bottom = min(a.bottom, b.bottom);

		return{ left, top, right, bottom };
	}

	bool IsInCity(const CityCells& city, Vec2i p)
	{
		return p.x >= 0 && p.y >= 0 && p.x < city.cellsPerSide && p.y < city.cellsPerSide;
	}

	void ForEachQuadBetweenRoads(CityCells& city, Vec2i index, IEnumerator<CityQuad>& onQuad, IRandom& rng)
	{
		if (!IsInCity(city, index)) return;

		struct NeighBours
		{
			int32 ab[2];
			CompassDirection blocksAB[2];
		};

		static NeighBours neighbours[4] = {
			{ NE, SW, CompassDirection_North, CompassDirection_West },
			{ NW, SE, CompassDirection_North, CompassDirection_East },
			{ SW, NE, CompassDirection_South, CompassDirection_East },
			{ NW, SE, CompassDirection_West, CompassDirection_South },
		};

		const auto& i = city[index];
		{
			GuiRectf quad[4];
			for (int j = 0; j < 4; ++j)
			{
				quad[j] = GetQuadrant((Quadrant)j);
			}

			bool anyRoad = i.isRoadConnected[CompassDirection_North] | i.isRoadConnected[CompassDirection_South] | i.isRoadConnected[CompassDirection_West] | i.isRoadConnected[CompassDirection_East];
			if (!anyRoad)
			{
				GuiRectf q = Merge(quad[NW], quad[SE]);
				onQuad({ q, GetCellCentre(i), i });
			}
			else
			{
				int32 indexUsedCount = 4;
				bool indexUsed[4] = { false, false, false, false };

				while (indexUsedCount)
				{
					int32 unusedIndex = rng() % indexUsedCount;

					int32 index = 0;

					for (int i = 0; i < 4; ++i)
					{
						if (!indexUsed[i])
						{
							if (index == unusedIndex)
							{
								indexUsed[i] = true;
								index = i;
								break;
							}
							index++;
						}
					}

					indexUsedCount--;

					auto n = neighbours[index];

					int32 nOffset = rng() & 0x1;

					int neighbourIndex = n.ab[nOffset];
					int blockDirection = n.blocksAB[nOffset];

					if (!i.isRoadConnected[blockDirection] && !indexUsed[neighbourIndex])
					{
						indexUsed[neighbourIndex] = true;
						indexUsedCount--;
						GuiRectf q = Merge(quad[index], quad[neighbourIndex]);
						onQuad({ q, GetCellCentre(i), i });
					}
					else
					{
						int neighbourIndex = n.ab[!nOffset];
						int blockDirection = n.blocksAB[!nOffset];

						if (!i.isRoadConnected[blockDirection] && !indexUsed[neighbourIndex])
						{
							indexUsed[neighbourIndex] = true;
							indexUsedCount--;
							GuiRectf q = Merge(quad[index], quad[neighbourIndex]);
							onQuad({ q, GetCellCentre(i), i });
						}
						else
						{
							onQuad({ quad[index], GetCellCentre(i), i });
						}
					}
				}
			}
		}
	}

	using namespace Dystopia;

	void AddQuad(Environment& e, const GuiRectf& quad, cr_vec3 position, ID_MESH id)
	{
		ObjectVertex v[6];
		for (int i = 0; i < 6; ++i)
		{
			v[i].diffuseColour = RGBAb(255, 255, 255, 0);
			v[i].emissiveColour = RGBAb(255, 255, 255, 255);
			v[i].normal = { 0, 0, 1 };
			v[i].u = v[i].v = 0;
		}

		v[0].position = { quad.left, quad.bottom, 0 };
		v[1].position = { quad.left, quad.top, 0 };
		v[2].position = { quad.right, quad.top, 0 };
		v[3].position = { quad.right, quad.top, 0 };
		v[4].position = { quad.right, quad.bottom, 0 };
		v[5].position = { quad.left, quad.bottom, 0 };

		e.meshes.BuildMesh(v, 6, id, false);
		e.level.Builder().AddSolid(position, id, SolidFlags_None);
	}

	bool TryMerge(GuiRectf& mergedQuad, const GuiRectf& a, const GuiRectf& b)
	{
		if (a.left == b.left && a.right == b.right)
		{
			if ((fabsf(a.top - b.bottom) < CityCell::cellWidth * 0.35f) || (fabsf(a.bottom - b.top) < CityCell::cellWidth * 0.35f))
			{
				mergedQuad = Merge(a, b);
				return true;
			}
		}
		else if (a.top == b.top && a.bottom == b.bottom)
		{
			if ((fabsf(a.left - b.right) < CityCell::cellWidth * 0.55f) || (fabsf(a.right - b.left) < CityCell::cellWidth * 0.55f))
			{
				mergedQuad = Merge(a, b);
				return true;
			}
		}
		return false;
	}

	void MergeAndAppend(const std::vector<CityQuad>& a, const std::vector<CityQuad>& b, std::vector<CityQuad>& mergedQuads)
	{
		bool mergeResultI[4] = { false,false,false,false };
		bool mergeResultJ[4] = { false,false,false,false };

		for (size_t i = 0; i < a.size(); ++i)
		{
			for (size_t j = 0; j < b.size(); ++j)
			{
				if (!mergeResultJ[j])
				{
					Vec3 commonCentre = Lerp(a[i].centre, b[j].centre, 0.5f);
				
					Vec3 av3 = a[i].centre - commonCentre;
					Vec3 bv3 = b[j].centre - commonCentre;

					GuiRectf aq = a[i].quad + AsVec2(av3);
					GuiRectf bq = b[j].quad + AsVec2(bv3);

					GuiRectf mergedQuad;
					if (TryMerge(mergedQuad, aq, bq))
					{
						mergedQuads.push_back({ mergedQuad, commonCentre, a[i].cell });
						mergeResultI[i] = true;
						mergeResultJ[j] = true;
					}
				}
			}
		}

		for (size_t i = 0; i < a.size(); ++i)
		{
			if (!mergeResultI[i]) mergedQuads.push_back(a[i]);
		}

		for (size_t i = 0; i < b.size(); ++i)
		{
			if (!mergeResultJ[i]) mergedQuads.push_back(b[i]);
		}
	}
}

namespace Dystopia
{
	void BuildRandomCity_V2(const fstring& name, Metres cityRadius, uint32 seedDelta, Environment& e, IEnumerable<const wchar_t*>& names)
	{
		int cellSpan = int32(cityRadius / CityCell::cellWidth);

		CityCells city(cellSpan);

		uint32 hash = FastHash(name);
		Random::RandomMT rng(hash + seedDelta);

		// First thing to do is layout a main road that passes through one side of town to the other
		int32 mainRoadLine = rng() % city.cellsPerSide;
		if ((rng() & 1) == 0)
		{
			for (int32 i = 0; i < city.cellsPerSide; ++i)
			{
				Vec2i roadWestEast = { i, mainRoadLine };
				city[roadWestEast].isConnectedToMainRoad = true;
				city[roadWestEast].isRoadConnected[CompassDirection_West] = true;
				city[roadWestEast].isRoadConnected[CompassDirection_East] = true;
			}
		}
		else
		{
			for (int32 i = 0; i < city.cellsPerSide; ++i)
			{
				Vec2i roadNorthSouth = { mainRoadLine, i };
				city[roadNorthSouth].isConnectedToMainRoad = true;
				city[roadNorthSouth].isRoadConnected[CompassDirection_North] = true;
				city[roadNorthSouth].isRoadConnected[CompassDirection_South] = true;
			}
		}

		TIndices undefinedIndices;
		for (auto& c: city.cells)
		{
			undefinedIndices.push_back(c.index);
		}

		struct : IEnumerator<const wchar_t*>
		{
			std::vector<std::wstring> names;

			virtual void operator()(const wchar_t* streetName)
			{
				names.push_back(streetName);
			}
		} streets;

		names.Enumerate(streets);

		enum { MINOR_ROAD_COUNT = 16 };

		TIndices connectionsCache;
		TIndices road;

		for (int k = 0; k < MINOR_ROAD_COUNT && !undefinedIndices.empty(); ++k)
		{
			size_t index = rng() % undefinedIndices.size();

			auto i = undefinedIndices.begin();
			std::advance(i, index);

			Vec2i start = *i;

			road.clear();
			GenerateMinorRoadFrom(start, city, rng, connectionsCache, road);

			for (auto& node : road)
			{
				city[node].isConnectedToMainRoad = true;
			}

			undefinedIndices.erase(i);
		}
		
		std::vector<ObjectVertex> vertexCache;

		uint32 meshId = 0x20000000;

		for (auto& cell: city.cells)
		{
			meshId++;

			vertexCache.clear();
			BuildRoadSegment(cell, vertexCache);

			if (!vertexCache.empty())
			{
				e.meshes.BuildMesh(&vertexCache[0], vertexCache.size(), ID_MESH(meshId), false);
				e.level.Builder().AddSolid(GetCellCentre(cell), ID_MESH(meshId), SolidFlags_RoadSection);
			}
		}

		
		struct : public IVectorEnumerator<ID_MESH>
		{
			std::vector<ID_MESH> meshes;
			virtual ID_MESH* begin() { return &meshes[0]; }
			virtual ID_MESH* end() { return &meshes[0] + meshes.size(); }
			virtual const ID_MESH* begin() const { return &meshes[0]; }
			virtual const ID_MESH* end() const { return &meshes[0] + meshes.size(); }
			virtual size_t size() const { return meshes.size(); }
		} randomHouses;

		enum { RANDOM_HOUSE_MESH_COUNT = 100 };
		randomHouses.meshes.reserve(RANDOM_HOUSE_MESH_COUNT);
		for (int i = 0; i < RANDOM_HOUSE_MESH_COUNT; ++i)
		{
			randomHouses.meshes.push_back(GenerateRandomHouse(e, rng()));
		}

		struct: IEnumerator<CityQuad>
		{
			std::vector<CityQuad> quads;

			virtual void operator()(const CityQuad& cq)
			{
				quads.push_back(cq);
			}
		} quadsA, quadsB ;

		TIndices indices;
		for (auto i = city.cells.rbegin(); i != city.cells.rend(); ++i)
		{
			indices.push_back(i->index);
		}

		std::vector<CityQuad> mergedQuads;

		while (!indices.empty())
		{
			size_t indexCount = rng() % indices.size();
			Vec2i index = indices[indexCount];

			quadsA.quads.clear();
			quadsB.quads.clear();
			ForEachQuadBetweenRoads(city, index, quadsA, rng);

			static Vec2i offsets[4] =
			{
				{  1,  0 },
				{  0,  1 },
				{ -1,  0 },
				{  0, -1 },
			};

			bool hasMerged = false;

			for (int j = 0; j < 8; ++j)
			{
				Vec2i offset = offsets[rng() & 3];

				auto offsetIt = std::find(indices.begin(), indices.end(), index + offset);
				if (offsetIt != indices.end())
				{
					ForEachQuadBetweenRoads(city, index + offset, quadsB, rng);
					MergeAndAppend(quadsA.quads, quadsB.quads, mergedQuads);
					RemoveIndex(indices, index + offset);
					hasMerged = true;
					break;
				}
				else
				{
					continue;
				}
			}

			if (!hasMerged)
			{
				for (auto& cq : quadsA.quads)
				{
					mergedQuads.push_back(cq);
				}
			}

			RemoveIndex(indices, index);
		}

		uint32 quadMesh = 0x22000000;
		for (auto& mq : mergedQuads)
		{
			AddQuad(e, mq.quad, mq.centre, ID_MESH(quadMesh++));
			PopulateQuad(e, randomHouses, mq.quad, mq.centre, rng);	
		}

		for (auto& cell : city.cells)
		{
			Vec3 centre = GetCellCentre(cell);
			if (cell.isConnectedToMainRoad)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (!cell.isRoadConnected[i])
					{
						auto offset = GetQuadrantCentreOffset((CompassDirection)i);
					//	auto id = e.level.Builder().AddSolid(GetCellCentre(cell) + offset, meshes[rng() % RANDOM_HOUSE_MESH_COUNT], SolidFlags_Obstacle);
					}
				}
			}
		}
	}
}
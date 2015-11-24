#include "dystopia.h"
#include "rococo.maths.h"
#include "rococo.strings.h"
#include "rococo.renderer.h"
#include "meshes.h"
#include <random>
#include <algorithm>
#include <malloc.h>

namespace
{
	using namespace Rococo;
	typedef std::mt19937  Randomizer;

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

	Quad GetQuadrant(Quadrant q)
	{
		Vec3 centre = CityCell::cellWidth * GetQuadrantVectorOffset(q);
		return Quad
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

	const Metres CityCell::cellWidth = 5.0_metres;

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

	void BuildRoadSegment(const CityCell& cell, std::vector<ObjectVertex>& vertexCache)
	{
		const Metres roadSpan = 0.10_metres;

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
	}

	uint32 GetNextInRecurringSequence(Randomizer& rng, uint32 recurrenceRate)
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

	void GenerateMinorRoadFrom(Vec2i startNode, CityCells& city, Randomizer& rng, TIndices& /* cache */ neighbours, TIndices& roadBuilder)
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

		indices.erase(i);
	}

	struct CityQuad
	{
		Quad quad;
		Vec3 centre;
	};

	Quad Merge(const Quad& a, const Quad& b)
	{
		float left = min(a.left, b.left);
		float right = max(a.right, b.right);
		float top = max(a.top, b.top);
		float bottom = min(a.bottom, b.bottom);

		return{ left, top, right, bottom };
	}

	void ForEachQuadBetweenRoads(CityCells& city, IEnumerator<CityQuad>& onQuad, Randomizer& rng)
	{
		TIndices indices;

		std::vector<Quad> mergedQuads;

		// Put them into reverse order, because we will in general delete them close to reverse order
		for (auto i = city.cells.rbegin(); i != city.cells.rend(); ++i)
		{
			indices.push_back(i->index);
		}

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

		for (auto i : city.cells)
		{
			Quad quad[4];
			for (int j = 0; j < 4; ++j)
			{
				quad[j] = GetQuadrant((Quadrant)j);
			}

			bool anyRoad = i.isRoadConnected[CompassDirection_North] | i.isRoadConnected[CompassDirection_South] | i.isRoadConnected[CompassDirection_West] | i.isRoadConnected[CompassDirection_East];
			if (!anyRoad)
			{
				Quad q = Merge(quad[NW], quad[SE]);
				onQuad({ q, GetCellCentre(i) });
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
						Quad q = Merge(quad[index], quad[neighbourIndex]);
						onQuad({ q, GetCellCentre(i) });
					}
					else
					{
						int neighbourIndex = n.ab[!nOffset];
						int blockDirection = n.blocksAB[!nOffset];

						if (!i.isRoadConnected[blockDirection] && !indexUsed[neighbourIndex])
						{
							indexUsed[neighbourIndex] = true;
							indexUsedCount--;
							Quad q = Merge(quad[index], quad[neighbourIndex]);
							onQuad({ q, GetCellCentre(i) });
						}
						else
						{
							onQuad({ quad[index], GetCellCentre(i) });
						}
					}
				}
			}
		}
	}
}

namespace Dystopia
{
	void BuildRandomCity(const fstring& name, Metres cityRadius, uint32 seedDelta, Environment& e, IEnumerable<const wchar_t*>& names)
	{
		int cellSpan = int32(cityRadius / CityCell::cellWidth);

		CityCells city(cellSpan);

		uint32 hash = FastHash(name);
		Randomizer rng(hash + seedDelta);

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

		enum { MINOR_ROAD_COUNT = 8 };

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

		std::vector<ID_MESH> meshes;
		enum { RANDOM_HOUSE_MESH_COUNT = 100 };
		meshes.reserve(RANDOM_HOUSE_MESH_COUNT);
		for (int i = 0; i < RANDOM_HOUSE_MESH_COUNT; ++i)
		{
			meshes.push_back(GenerateRandomHouse(e, rng()));
		}

		struct: IEnumerator<CityQuad>
		{
			Environment* e;

			virtual void operator()(const CityQuad& cq)
			{
				const Quad& quad = cq.quad;

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

				auto mid = ID_MESH(meshId++);
				e->meshes.BuildMesh(v, 6, mid, false);
				e->level.Builder().AddSolid(cq.centre, mid, SolidFlags_None);
			}

			uint32 meshId = 0x20F00000;
		} addQuadToList ;

		addQuadToList.e = &e;
		ForEachQuadBetweenRoads(city, addQuadToList, rng);

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
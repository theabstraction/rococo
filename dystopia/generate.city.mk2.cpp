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

	struct CityCell
	{
		static const Metres cellWidth;
		bool isRoadConnected[4]; // One boolean for each of the 4 compass directions
		bool isConnectedToMainRoad;

		CityCell()
		{
			isRoadConnected[0] = isRoadConnected[1] = isRoadConnected[2] = isRoadConnected[3] = false;
			isConnectedToMainRoad = false;
		}
	};

	const Metres CityCell::cellWidth = 5.0_metres;

	struct CityCells
	{
		const size_t cellsPerSide;
		std::vector<CityCell> cells;	

		CityCells(size_t _cellsPerSide) : 
			cellsPerSide(_cellsPerSide),
			cells(_cellsPerSide * _cellsPerSide)
		{

		}

		CityCell& operator[](Vec2i index) { return cells[index.x + index.y * cellsPerSide]; }
		const CityCell& operator[](Vec2i index) const { return cells[index.x + index.y * cellsPerSide]; }
	};

	void AppendAvailableConnections(std::vector<Vec2i>& availableConnections, Vec2i node, int32 citySpan)
	{
		if (node.x > 0)
		{
			availableConnections.push_back({ node.x - 1,node.y });
		}

		if (node.x < citySpan - 1)
		{
			availableConnections.push_back({ node.x + 1,node.y });
		}

		if (node.y > 0)
		{
			availableConnections.push_back({ node.x,node.y - 1 });
		}

		if (node.y < citySpan - 1)
		{
			availableConnections.push_back({ node.x,node.y + 1 });
		}
	}

	void Join(CityCell& cell, Vec2i centre, Vec2i pathTo, CityCell& pathToCell)
	{
		if (pathTo.x < centre.x)
		{
			cell.isRoadConnected[CompassDirection_West] = true;
			pathToCell.isRoadConnected[CompassDirection_East] = true;
		}
		else if (pathTo.x > centre.x)
		{
			cell.isRoadConnected[CompassDirection_East] = true;
			pathToCell.isRoadConnected[CompassDirection_West] = true;
		}
		else if (pathTo.y > centre.y)
		{
			cell.isRoadConnected[CompassDirection_North] = true;
			pathToCell.isRoadConnected[CompassDirection_South] = true;
		}
		else
		{
			cell.isRoadConnected[CompassDirection_South] = true;
			pathToCell.isRoadConnected[CompassDirection_North] = true;
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
}

namespace Dystopia
{
	void BuildRandomCity(const fstring& name, Metres radius, uint32 seedDelta, Environment& e, IEnumerable<const wchar_t*>& names)
	{
		int cellSpan = (int32(radius / CityCell::cellWidth) & 0xFFFFFFFE) + 1;

		CityCells cells(cellSpan);

		uint32 hash = FastHash(name);
		Randomizer rng(hash + seedDelta);

		int32 mainRoadLine = rng() % cells.cellsPerSide;

		if ((rng() & 1) == 0)
		{
			for (int32 i = 0; i < cells.cellsPerSide; ++i)
			{
				Vec2i centreLineWestEast = { i, mainRoadLine };
				cells[centreLineWestEast].isConnectedToMainRoad = true;
				cells[centreLineWestEast].isRoadConnected[CompassDirection_West] = true;
				cells[centreLineWestEast].isRoadConnected[CompassDirection_East] = true;
			}
		}
		else
		{
			for (int32 i = 0; i < cells.cellsPerSide; ++i)
			{
				Vec2i centreLineWestEast = { mainRoadLine, i };
				cells[centreLineWestEast].isConnectedToMainRoad = true;
				cells[centreLineWestEast].isRoadConnected[CompassDirection_North] = true;
				cells[centreLineWestEast].isRoadConnected[CompassDirection_South] = true;
			}
		}

		std::vector<Vec2i> undefinedIndices(cellSpan * cellSpan);
		for (int j = 0; j < cellSpan; j++)
		{
			for (int i = 0; i < cellSpan; i++)
			{
				undefinedIndices[i + j * cellSpan] = Vec2i{ i, j };
			}
		}

		std::vector<Vec2i> availableConnections;

		struct : IEnumerator<const wchar_t*>
		{
			std::vector<std::wstring> names;

			virtual void operator()(const wchar_t* streetName)
			{
				names.push_back(streetName);
			}
		} streets;

		names.Enumerate(streets);

		for (int k = 0; k < 8 && !undefinedIndices.empty(); ++k)
		{
			size_t index = rng() % undefinedIndices.size();

			auto i = undefinedIndices.begin();
			std::advance(i, index);

			Vec2i target = *i;
			do
			{
				auto& cell = cells[target];

				if (!cell.isConnectedToMainRoad)
				{
					availableConnections.clear();
					AppendAvailableConnections(availableConnections, target, cellSpan);

					bool hasAtLeastOneConnectionToMainRoad = false;
					for (auto& i : availableConnections)
					{
						if (cells[i].isConnectedToMainRoad)
						{
							hasAtLeastOneConnectionToMainRoad = true;
							break;
						}
					}

					if (hasAtLeastOneConnectionToMainRoad)
					{
						auto j = std::remove_if(availableConnections.begin(), availableConnections.end(),
							[&cells](const Vec2i& index)
							{
								return !cells[index].isConnectedToMainRoad;
							}
						);
						availableConnections.erase(j, availableConnections.end());
					}

					Vec2i pathTo = availableConnections[GetNextInRecurringSequence(rng, 4) % availableConnections.size()];

					if (cells[pathTo].isConnectedToMainRoad)
					{
						cell.isConnectedToMainRoad = true;
					}

					Join(cell, target, pathTo, cells[pathTo]);
					target = pathTo;
				}
			} while (!cells[target].isConnectedToMainRoad);

			undefinedIndices.erase(i);
		}
		
		std::vector<ObjectVertex> vertexCache;

		uint32 meshId = 0x20000000;

		for (int j = 0; j < cellSpan; j++)
		{
			for (int i = 0; i < cellSpan; i++)
			{
				meshId++;
				const auto& cell = cells[{i, j}];
				vertexCache.clear();

				BuildRoadSegment(cell, vertexCache);

				if (!vertexCache.empty())
				{
					e.meshes.BuildMesh(&vertexCache[0], vertexCache.size(), ID_MESH(meshId), false);
					e.level.Builder().AddSolid({ i * CityCell::cellWidth, j * CityCell::cellWidth, 0 }, ID_MESH(meshId), SolidFlags_RoadSection);
				}
			}
		}
	}
}
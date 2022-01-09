#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>
#include <rococo.widgets.h>

#include <vector>
#include <string>

namespace
{
	using namespace HV;
}

namespace HV
{
	class EditMode_SectorBuilder : public ISectorBuilderEditor, private IEditMode
	{
		bool isLineBuilding{ false };
		std::vector<Vec2> lineList;
		IWorldMap& map;
		GuiMetrics metrics;
		IPublisher& publisher;

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			grc.Renderer().GetGuiMetrics(metrics);

			for (int i = 1; i < lineList.size(); ++i)
			{
				Vec2i start = map.GetScreenPosition(lineList[i - 1]);
				Vec2i end = map.GetScreenPosition(lineList[i]);
				Rococo::Graphics::DrawLine(grc, 2, start, end, RGBAb(255, 255, 0));
			}

			if (isLineBuilding && !lineList.empty())
			{
				Vec2i start = map.GetScreenPosition(lineList[lineList.size() - 1]);
				Rococo::Graphics::DrawLine(grc, 2, start, metrics.cursorPosition, RGBAb(255, 255, 0));
			}
		}

		std::string defaultTextures[3] =
		{
		   "!textures/hv/wall_1.jpg",
		   "!textures/hv/floor_1.jpg",
		   "!textures/hv/ceiling_1.jpg"
		};

		bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			return false;
		}

		void OnRawMouseEvent(const MouseEvent& key) override
		{
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
		{
			if (dWheel < 0)
			{
				map.ZoomIn(-dWheel);
			}
			else if (dWheel > 0)
			{
				map.ZoomOut(dWheel);
			}
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
		{
			if (clickedDown)
			{
				map.GrabAtCursor();
			}
			else
			{
				map.ReleaseGrab();
			}
		}

		void DestroyCrossedLines()
		{
			if (lineList.size() > 2)
			{
				Vec2 lastVertex = lineList[lineList.size() - 1];
				Vec2 startOfLastLine = lineList[lineList.size() - 2];

				for (size_t i = 1; i < lineList.size() - 2; ++i)
				{
					Vec2 a = lineList[i - 1];
					Vec2 b = lineList[i];

					float t, u;
					if (GetLineIntersect(a, b, startOfLastLine, lastVertex, t, u))
					{
						if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
						{
							Rococo::Widgets::SetStatus("Crossed lines: sector creation cancelled", publisher);
							lineList.clear();
							return;
						}
					}
					else if (DoParallelLinesIntersect(a, b, startOfLastLine, lastVertex))
					{
						lineList.clear();
						Rococo::Widgets::SetStatus("Sector creation cancelled", publisher);
						return;
					}
				}
			}
		}

		void TryAppendVertex(Vec2 worldPosition)
		{
			isLineBuilding = true;

			if (lineList.empty())
			{
				SectorAndSegment sns = GetFirstSectorWithVertex(worldPosition, map.Sectors());
				if (sns.sector == nullptr)
				{
					ISector* sector = GetFirstSectorContainingPoint(worldPosition, map.Sectors());
					if (sector != nullptr)
					{
						Rococo::Widgets::SetStatus("A new sector's first point must lie outside all other sectors", publisher);
						return;
					}
				}
			}
			else
			{
				Vec2 a = *lineList.rbegin();
				Vec2 b = worldPosition;

				SectorAndSegment sns = GetFirstSectorWithVertex(b, map.Sectors());
				if (sns.sector == nullptr)
				{
					auto* first = GetFirstSectorCrossingLine(a, b, map.Sectors());
					if (first)
					{
						Rococo::Widgets::SetStatus("Cannot place edge of a new sector within the vertices of another.", publisher);
						return;
					}
				}
			}

			if (!lineList.empty() && lineList[0] == worldPosition)
			{
				// Sector closed
				isLineBuilding = false;

				if (lineList.size() >= 3)
				{
					map.Sectors().AddSector(&lineList[0], lineList.size());
					Rococo::Widgets::SetStatus("Sector created", publisher);
				}
				lineList.clear();
				return;
			}
			else
			{
				lineList.push_back(worldPosition);
			}
			DestroyCrossedLines();
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
		{
			if (clickedDown)
			{
				Vec2 wp = map.SnapToGrid(map.GetWorldPosition(cursorPos));
				TryAppendVertex(wp);
			}
		}
	public:
		EditMode_SectorBuilder(IPublisher& _publisher, IWorldMap& _map) : publisher(_publisher), map(_map)
		{
		}

		void Free() override
		{
			delete this;
		}

		void SetTexture(int32 index, cstr name) override
		{
			defaultTextures[index] = name;
		}

		cstr GetTexture(int32 index) const  override
		{
			return defaultTextures[index].c_str();
		}

		cstr GetTexture(int32 state)  override
		{
			if (state < 0 || state >= 3) Throw(0, "Bad index to EditMode_SectorBuilder::GetTexture(%d)", state);
			return defaultTextures[state].c_str();
		}

		IEditMode& Mode()  override { return *this; }
		const ISector* GetHilight() const override { return nullptr; }
	};
}

namespace HV
{
	ISectorBuilderEditor* CreateSectorBuilder(IPublisher& publisher, IWorldMap& map)
	{
		return new EditMode_SectorBuilder(publisher, map);
	}
}
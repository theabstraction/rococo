#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>

using namespace Rococo::Strings;

namespace
{
	using namespace HV;
	using namespace Rococo;
	using namespace Rococo::Entities;

	class WorldMap: public IWorldMapSupervisor
	{
	private:
		IInstancesSupervisor& instances;
		IMobiles& mobiles;
		int32 gridlinePixelWidth{ 8 };
		Metres gridlineMetricWidth{ 2.0f };
		Vec2 gridCentre{ 0, 0 }; // Always uses integral co-ordinates
		bool isGrabbed{ false };
		Vec2i grabPosition;
		Vec2 grabbedCentre;
		Vec2i pixelOffset{ 0, 0 };
		Vec2i grabbedOffset{ 0,0 };
		GuiMetrics metrics{ 0 };

		ISectors& sectors;
	public:
		ISectors& Sectors() override { return sectors; }

		WorldMap(Platform& platform, ISectors& _sectors) :
			instances(platform.instances),
			mobiles(platform.mobiles),
			sectors(_sectors)
		{

		}

		void Free() override
		{
			delete this;
		}

		Vec2 GetWorldPosition(Vec2i screenPosition) override
		{
			Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };
			Vec2i cursorOffset = { screenPosition.x - centre.x , centre.y - screenPosition.y };
			Vec2i pixelDelta = cursorOffset - pixelOffset;

			Vec2 centreOffset = Vec2{ pixelDelta.x / (float)gridlinePixelWidth, pixelDelta.y / (float)gridlinePixelWidth };
			return gridCentre + centreOffset * gridlineMetricWidth;
		}

		Vec2i GetScreenPosition(Vec2 worldPosition) override
		{
			Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };

			Vec2 pixelDelta = (worldPosition - gridCentre) * (float)gridlinePixelWidth / gridlineMetricWidth;
			return{ ((int32)(pixelDelta.x)) + centre.x + pixelOffset.x, -(((int32)(pixelDelta.y)) - centre.y + pixelOffset.y) };
		}

		void DrawGridLines(IGuiRenderContext& grc)
		{
			Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
			int32 nGridLinesX = (metrics.screenSpan.x / gridlinePixelWidth);
			int32 nGridLinesY = (metrics.screenSpan.y / gridlinePixelWidth);
			int32 farLeft = centre.x - gridlinePixelWidth * (nGridLinesX >> 1) - gridlinePixelWidth;
			int32 farRight = centre.x + gridlinePixelWidth * (nGridLinesX >> 1) + gridlinePixelWidth;
			int32 farUp = centre.y - gridlinePixelWidth * (nGridLinesY >> 1) - gridlinePixelWidth;
			int32 farDown = centre.y + gridlinePixelWidth * (nGridLinesY >> 1) + gridlinePixelWidth;

			for (int j = farUp; j <= farDown; j += gridlinePixelWidth)
			{
				auto y = j - pixelOffset.y;
				Rococo::Graphics::DrawLine(grc, 1, { 0, y }, { metrics.screenSpan.x, y }, RGBAb(192, 192, 192, 255));
			}

			for (int i = farLeft; i <= farRight; i += gridlinePixelWidth)
			{
				auto x = i + pixelOffset.x;
				Rococo::Graphics::DrawLine(grc, 1, { x, 0 }, { x, metrics.screenSpan.y }, RGBAb(192, 192, 192, 255));
			}
		}

		void RenderTopGui(IGuiRenderContext& grc, ID_ENTITY cameraId) override
		{
			Vec2 worldCursor = GetWorldPosition(metrics.cursorPosition);

			char originText[24];
			SafeFormat(originText, sizeof(originText), "(%4.1f,%4.1f)", worldCursor.x, worldCursor.y);

			Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };

			GuiRect outRect{ centre.x - 70,0,centre.x + 70, 20 };
			Rococo::Graphics::DrawRectangle(grc, outRect, RGBAb(64, 64, 64, 224), RGBAb(64, 64, 64, 224));

			GuiRectf textRect{ (float)centre.x - 70, 0.0f,  (float)centre.x + 70, 20.0f };
			Rococo::Graphics::DrawText(grc, textRect, 0, to_fstring(originText), 0, RGBAb(255, 255, 255));

			auto camera = instances.ECS().GetBodyComponent(cameraId);
			Vec3 entityPos = camera->Model().GetPosition();
			auto labelPos = GetScreenPosition({ entityPos.x, entityPos.y });

			FPSAngles angles;
			mobiles.GetAngles(cameraId, angles);

			HV::GraphicsEx::DrawPointer(grc, labelPos, angles.heading, RGBAb(0, 0, 0), RGBAb(255, 255, 0));
		}

		void Render(IGuiRenderContext& grc, const ISector* litSector, bool isTransparent) override
		{
			grc.Renderer().GetGuiMetrics(metrics);

			if (isGrabbed)
			{
				Vec2i delta = metrics.cursorPosition - grabPosition;
				// If we drag centre to upperleft, delta.x and delta.y -ve
				// But we are moving map centre in +ve world direction and -ve y direction

				int32 nCellsXDelta = -(delta.x + grabbedOffset.x) / gridlinePixelWidth;
				int32 nCellsYDelta = (delta.y + grabbedOffset.y) / gridlinePixelWidth;

				gridCentre.x = grabbedCentre.x + gridlineMetricWidth * (float)nCellsXDelta;
				gridCentre.y = grabbedCentre.y + gridlineMetricWidth * (float)nCellsYDelta;

				pixelOffset.x = (delta.x + grabbedOffset.x) % gridlinePixelWidth;
				pixelOffset.y = (-delta.y - grabbedOffset.y) % gridlinePixelWidth;
			}

			if (!isTransparent)
			{
				Rococo::Graphics::DrawRectangle(grc, { 0,0,metrics.screenSpan.x, metrics.screenSpan.y }, RGBAb(0, 0, 0, 224), RGBAb(0, 0, 0, 224));
				DrawGridLines(grc);
			}

			Vec2i centreOffseti = GetScreenPosition(Vec2{ 0, 0 });
			Vec2 centreOffset{ (float)centreOffseti.x, (float)centreOffseti.y };

			const float scale = gridlinePixelWidth / gridlineMetricWidth;

			for (ISector* sector : sectors)
			{
				float dim = !litSector ? 0.9f : 0.7f;
				RGBAb colour = sector->GetGuiColour(sector == litSector ? 1.0f : dim);
				ObjectVertexBuffer vertices = sector->FloorVertices();
				for (size_t i = 0; i < vertices.VertexCount; i += 3)
				{
					GuiVertex v[3];
					for (int j = 0; j < 3; ++j)
					{
						Vec2 worldPos{ vertices.v[i + j].position.x, vertices.v[i + j].position.y };
						auto pos = GetScreenPosition(worldPos);
						v[j].pos.x = (float)pos.x;
						v[j].pos.y = (float)pos.y;
						v[j].vd.uv = vertices.v[i + j].uv;
						v[j].vd.fontBlend = 0;
						v[j].colour = colour;
						v[j].sd = { 1.0f, 0.0f, 0.0f, 0.0f };
					}

					grc.AddTriangle(v);
				}
			}

			if (litSector)
			{
				size_t nVertices;
				const Vec2* v = litSector->WallVertices(nVertices);
				Ring<Vec2> ring(v, nVertices);

				for (size_t i = 0; i < nVertices; ++i)
				{
					Vec2 p = ring[i];
					Vec2 q = ring[i + 1];

					Vec2i pos = GetScreenPosition(p);
					Rococo::Graphics::DrawLine(grc, 2, pos, GetScreenPosition(q), RGBAb(255, 255, 255));

					GuiRect pixelRect = GuiRect{ -6, -6, 6, 6 } + pos;
					Rococo::Graphics::DrawRectangle(grc, pixelRect, RGBAb(255, 255, 255, 64), RGBAb(255, 255, 255, 64));
				};
			}
		}

		void ZoomIn(int32 degrees) override
		{
			pixelOffset = { 0,0 };

			for (int i = 0; i < degrees; ++i)
			{
				if (gridlinePixelWidth == 256)
				{
					return;
				}
				else if (gridlinePixelWidth < 24)
				{
					gridlinePixelWidth += 2;
				}
				else
				{
					gridlinePixelWidth += 8;
				}
			}
		}

		void ZoomOut(int32 degrees) override
		{
			pixelOffset = { 0,0 };

			for (int i = 0; i < degrees; ++i)
			{
				if (gridlinePixelWidth == 2)
				{
					return;
				}
				else if (gridlinePixelWidth < 24)
				{
					gridlinePixelWidth -= 2;
				}
				else
				{
					gridlinePixelWidth -= 8;
				}
			}
		}

		Vec2 SnapToGrid(Vec2 worldPosition) override
		{
			float x = gridlineMetricWidth * roundf(worldPosition.x / gridlineMetricWidth);
			float y = gridlineMetricWidth * roundf(worldPosition.y / gridlineMetricWidth);
			return{ x, y };
		}

		void GrabAtCursor() override
		{
			isGrabbed = true;
			grabPosition = metrics.cursorPosition;
			grabbedCentre = gridCentre;
			grabbedOffset = { pixelOffset.x, -pixelOffset.y };
		}

		void ReleaseGrab() override
		{
			isGrabbed = false;
		}
	};
}

namespace HV
{
	IWorldMapSupervisor* CreateWorldMap(Platform& platform, ISectors& sectors)
	{
		return new WorldMap(platform, sectors);
	}
}
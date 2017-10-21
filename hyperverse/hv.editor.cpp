#include "hv.h"
#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.widgets.h>
#include <rococo.mplat.h>
#include <rococo.textures.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.rings.inl>

namespace
{
	using namespace HV;
	using namespace Rococo;
	using namespace Rococo::Widgets;
	using namespace Rococo::Entities;

	class WorldMap
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
		ISectors& Sectors() { return sectors; }

		WorldMap(Platform& platform, ISectors& _sectors) :
			instances(platform.instances),
			mobiles(platform.mobiles),
			sectors(_sectors)
		{

		}

		Vec2 GetWorldPosition(Vec2i screenPosition)
		{
			Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };
			Vec2i cursorOffset = { screenPosition.x - centre.x , centre.y - screenPosition.y };
			Vec2i pixelDelta = cursorOffset - pixelOffset;

			Vec2 centreOffset = Vec2{ pixelDelta.x / (float)gridlinePixelWidth, pixelDelta.y / (float)gridlinePixelWidth };
			return gridCentre + centreOffset * gridlineMetricWidth;
		}

		Vec2i GetScreenPosition(Vec2 worldPosition)
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
			int32 farUp = centre.y - gridlinePixelWidth *  (nGridLinesY >> 1) - gridlinePixelWidth;
			int32 farDown = centre.y + gridlinePixelWidth *  (nGridLinesY >> 1) + gridlinePixelWidth;

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

		void RenderTopGui(IGuiRenderContext& grc, ID_ENTITY cameraId)
		{
			Vec2 worldCursor = GetWorldPosition(metrics.cursorPosition);

			rchar originText[24];
			SafeFormat(originText, sizeof(originText), "(%4.1f,%4.1f)", worldCursor.x, worldCursor.y);

			Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
			Rococo::Graphics::DrawRectangle(grc, { centre.x - 70,0,centre.x + 70, 20 }, RGBAb(64, 64, 64, 224), RGBAb(64, 64, 64, 224));
			Rococo::Graphics::RenderCentredText(grc, originText, RGBAb(255, 255, 255), 9, { metrics.screenSpan.x >> 1, 8 });

			auto* entity = instances.GetEntity(cameraId);
			Vec3 entityPos = entity->Position();
			auto labelPos = GetScreenPosition({ entityPos.x, entityPos.y });

			FPSAngles angles;
			mobiles.GetAngles(cameraId, angles);

			HV::Graphics::DrawPointer(grc, labelPos, angles.heading, RGBAb(0, 0, 0), RGBAb(255, 255, 0));
		}

		void Render(IGuiRenderContext& grc, const ISector* litSector)
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

			Rococo::Graphics::DrawRectangle(grc, { 0,0,metrics.screenSpan.x, metrics.screenSpan.y }, RGBAb(0, 0, 0, 224), RGBAb(0, 0, 0, 224));

			DrawGridLines(grc);

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
						v[j].x = (float)pos.x;
						v[j].y = (float)pos.y;
						v[j].u = vertices.v[i + j].u;
						v[j].v = vertices.v[i + j].v;
						v[j].colour = colour;
						v[j].fontBlend = 0;
						v[j].saturation = 1;
						v[j].textureIndex = 1;
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

					GuiRect pixelRect = GuiRect{ -6, -6, 6, 6 } +pos;
					Rococo::Graphics::DrawRectangle(grc, pixelRect, RGBAb(255, 255, 255, 64), RGBAb(255, 255, 255, 64));
				};
			}
		}

		void ZoomIn(int32 degrees)
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

		void ZoomOut(int32 degrees)
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

		Vec2 SnapToGrid(Vec2 worldPosition)
		{
			float x = gridlineMetricWidth * roundf(worldPosition.x / gridlineMetricWidth);
			float y = gridlineMetricWidth * roundf(worldPosition.y / gridlineMetricWidth);
			return{ x, y };
		}

		void GrabAtCursor()
		{
			isGrabbed = true;
			grabPosition = metrics.cursorPosition;
			grabbedCentre = gridCentre;
			grabbedOffset = { pixelOffset.x, -pixelOffset.y };
		}

		void ReleaseGrab()
		{
			isGrabbed = false;
		}
	};

	ROCOCOAPI IEditMode : public IUIElement
	{
	   virtual const ISector* GetHilight() const = 0;
	};

	class EditMode_SectorEditor : private IEditMode
	{
		WorldMap& map;
		GuiMetrics metrics;
		Platform& platform;
		IEditorState* editor;
		Windows::IWindow& parent;
		ISector* lit{ nullptr };

		void Render(IGuiRenderContext& grc, const GuiRect& rect) override
		{

		}

		bool OnKeyboardEvent(const KeyboardEvent& k) override
		{
			Key key = platform.keyboard.GetKeyFromEvent(k);

			auto* action = platform.keyboard.GetAction(key.KeyName);
			if (action && Eq(action, "gui.editor.sector.delete"))
			{
				if (!key.isPressed)
				{
					if (lit)
					{
						map.Sectors().Delete(lit);
						lit = nullptr;
					}
				}
				return true;
			}

			return false;
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

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
		{
			if (clickedDown)
			{
				Vec2 wp = map.GetWorldPosition(cursorPos);
				for (auto* s : map.Sectors())
				{
					int32 index = s->GetFloorTriangleIndexContainingPoint(wp);
					if (index >= 0)
					{
						if (lit == s)
						{
							lit->InvokeSectorDialog(parent, *editor);
						}
						lit = s;
					}
				}
			}
		}
	public:
		EditMode_SectorEditor(Platform& _platform, WorldMap& _map, Windows::IWindow& _parent) :
			platform(_platform),
			map(_map),
			parent(_parent),
			editor(nullptr)
		{ }
		IEditMode& Mode() { return *this; }
		const ISector* GetHilight() const override { return lit; }
		void SetEditor(IEditorState* editor) { this->editor = editor; }
	};

	class EditMode_SectorBuilder : private IEditMode
	{
		bool isLineBuilding{ false };
		std::vector<Vec2> lineList;
		WorldMap& map;
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
							SetStatus("Crossed lines: sector creation cancelled", publisher);
							lineList.clear();
							return;
						}
					}
					else if (DoParallelLinesIntersect(a, b, startOfLastLine, lastVertex))
					{
						lineList.clear();
						SetStatus("Sector creation cancelled", publisher);
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
				SectorAndSegment sns = map.Sectors().GetFirstSectorWithVertex(worldPosition);
				if (sns.sector == nullptr)
				{
					ISector* sector = map.Sectors().GetFirstSectorContainingPoint(worldPosition);
					if (sector != nullptr)
					{
						SetStatus("A new sector's first point must lie outside all other sectors", publisher);
						return;
					}
				}
			}
			else
			{
				Vec2 a = *lineList.rbegin();
				Vec2 b = worldPosition;

				SectorAndSegment sns = map.Sectors().GetFirstSectorWithVertex(b);
				if (sns.sector == nullptr)
				{
					auto* first = map.Sectors().GetFirstSectorCrossingLine(a, b);
					if (first)
					{
						SetStatus("Cannot place edge of a new sector within the vertices of another.", publisher);
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
					SectorPalette palette{ defaultTextures[0].c_str(), defaultTextures[1].c_str(), defaultTextures[2].c_str() };
					map.Sectors().AddSector(palette, &lineList[0], lineList.size());
					SetStatus("Sector created", publisher);
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
		EditMode_SectorBuilder(IPublisher& _publisher, WorldMap& _map) : publisher(_publisher), map(_map)
		{
		}

		void SetTexture(int32 index, cstr name)
		{
			defaultTextures[index] = name;
		}

		cstr GetTexture(int32 index) const
		{
			return defaultTextures[index].c_str();
		}

		cstr GetTexture(int32 state)
		{
			if (state < 0 || state >= 3) Throw(0, "Bad index to EditMode_SectorBuilder::GetTexture(%d)", state);
			return defaultTextures[state].c_str();
		}

		IEditMode& Mode() { return *this; }
		const ISector* GetHilight() const override { return nullptr; }
	};

	EventId vScrollChanged = "editor.tools.vscroll_ui"_event;
	EventId vScrollSet = "editor.tools.vscroll_set"_event;
	EventId vScrollGet = "editor.tools.vscroll_get"_event;

	class TextureList : public IUIElement, public IObserver
	{
		Platform& platform;

		std::unordered_map<std::string, ID_TEXTURE> files;
		std::vector<std::string> q;
		std::vector<std::string> readyList;

		std::string selectedItem;

		int32 scrollPosition = 0;
	public:
		TextureList(Platform& _platform) : platform(_platform)
		{
			platform.gui.RegisterPopulator("editor.tools.imagelist", this);

			struct : IEventCallback<cstr>
			{
				std::unordered_map<std::string, ID_TEXTURE>* files;
				virtual void OnEvent(cstr filename)
				{
					files->insert(std::make_pair(filename, ID_TEXTURE::Invalid()));
				}
			} anon;
			anon.files = &files;

			platform.utilities.EnumerateFiles(anon, "!textures/hv/");

			if (anon.files->empty())
			{
				Throw(0, "No textures found under !textures/hv/");
			}

			PushAllOnQueue();

			platform.publisher.Attach(this, vScrollChanged);
			platform.publisher.Attach(this, vScrollGet);
		}

		~TextureList()
		{
			platform.publisher.Detach(this);
			platform.gui.UnregisterPopulator(this);
		}

		bool IsLoading() const
		{
			return !q.empty();
		}

		int32 GetIndexOf(cstr name)
		{
			for (int32 i = 0; i < (int32)readyList.size(); ++i)
			{
				if (Eq(readyList[i].c_str(), name))
				{
					return i;
				}
			}

			return -1;
		}

		cstr GetNeighbour(cstr name, bool forward)
		{
			int32 index = GetIndexOf(name);
			if (forward) index++; else index--;

			if (index >= (int32)readyList.size())
			{
				index = (int32)readyList.size() - 1;
			}

			if (index < 0)
			{
				index = 0;
			}

			return readyList[index].c_str();
		}

		void ScrollTo(cstr filename)
		{
			int index = -1;
			for (int32 i = 0; i < (int32)readyList.size(); i++)
			{
				if (Eq(readyList[i].c_str(), filename))
				{
					index = i;
					break;
				}
			}

			if (index < 0) return;

			scrollPosition = index * lastDy;
			selectedItem = filename;

			ScrollEvent se(vScrollSet);
			se.fromScrollbar = false;
			se.logicalMaxValue = (int32)readyList.size() * lastDy;

			scrollPosition = min(scrollPosition, se.logicalMaxValue - lastPageSize);

			se.logicalMinValue = 0;
			se.logicalPageSize = lastPageSize;
			se.rowSize = lastDy / 4;
			se.logicalValue = scrollPosition;

			platform.publisher.Publish(se);
		}

		void PushAllOnQueue()
		{
			for (auto& f : files)
			{
				q.push_back(f.first);
			}
		}

		cstr GetSelectedTexture() const
		{
			return selectedItem.empty() ? nullptr : selectedItem.c_str();
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			return false;
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
		{
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
		{
			if (clickedDown)
			{
				struct : IEventCallback<ImageCallbackArgs>
				{
					Vec2i cursorPos;
					std::string target;
					virtual void OnEvent(ImageCallbackArgs& args)
					{
						GuiRect itemRect{ (int32)args.target.left, (int32)args.target.top, (int32)args.target.right, (int32)args.target.bottom };
						if (IsPointInRect(cursorPos, itemRect))
						{
							target = args.filename;
						}
					}
				} selectItem;

				selectItem.cursorPos = cursorPos;
				selectItem.target = selectedItem;

				EnumerateVisibleImages(selectItem, lastRect);

				selectedItem = selectItem.target;

				HV::Events::ChangeDefaultTextureEvent ev;
				ev.wallName = selectedItem.c_str();
				platform.publisher.Publish(ev);
			}
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
		{
		}

		bool DequeSome()
		{
			auto start = OS::CpuTicks();

			bool updated = false;

			while (!q.empty())
			{
				updated = true;

				auto top = q.back();

				auto id = platform.instances.ReadyTexture(top.c_str());

				files[top] = id;

				q.pop_back();

				readyList.push_back(top);

				auto now = OS::CpuTicks();
				auto dt = now - start;

				if (dt > (OS::CpuHz() >> 5))
				{
					// We've capped texture loading so that frame rate does not fall far below 32 framea per second
					break;
				}
			}

			return updated;
		}

		int32 lastDy = 0;
		int32 lastPageSize = 0;
		GuiRect lastRect = { 0,0,0,0 };

		void OnEvent(Event& ev) override
		{
			if (ev.id == vScrollChanged)
			{
				auto& se = As<ScrollEvent>(ev);
				if (!se.fromScrollbar)
				{
					se.fromScrollbar = false;
					se.logicalMaxValue = (int32)readyList.size() * lastDy;
					se.logicalMinValue = 0;
					se.logicalPageSize = lastPageSize;
					se.rowSize = lastDy / 4;
					se.logicalValue = scrollPosition;
				}
				else
				{
					scrollPosition = se.logicalValue;
				}
			}
		}

		struct ImageCallbackArgs
		{
			GuiRectf target;
			cstr filename;
			float txUVtop;
			float txUVbottom;
			ID_TEXTURE textureId;
		};

		void EnumerateVisibleImages(IEventCallback<ImageCallbackArgs>& cb, const GuiRect& absRect)
		{
			float x0 = absRect.left + 1.0f;
			float x1 = absRect.right - 1.0f;

			int32 width = Width(absRect) - 2;

			float y = absRect.top + 1.0f - scrollPosition;
			float dy = (float)width;

			lastDy = (int32)dy;
			lastPageSize = (int32)Height(absRect) - 2;

			for (auto& f : readyList)
			{
				auto id = files[f];

				float y1 = y + dy;

				float h = 1.0f;

				float b = y1;

				if (y1 >= absRect.bottom)
				{
					h = (absRect.bottom - y) / dy;
					b = (float)absRect.bottom - 1;
				}

				if (y1 > absRect.top)
				{
					float g = 0;
					float t = y;

					if (y <= absRect.top)
					{
						g = (absRect.top + 1 - y) / dy;
						t = (float)(absRect.top + 1);
					}

					ImageCallbackArgs args;
					args.filename = f.c_str();
					args.target = GuiRectf{ x0, t, x1, b };
					args.textureId = id;
					args.txUVbottom = h;
					args.txUVtop = g;

					cb.OnEvent(args);
				}

				if (h < 1.0f)
				{
					break;
				}

				y = y1;
			}
		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			bool updated = DequeSome();

			if (updated)
			{
				ScrollEvent se("editor.tools.vscroll_set"_event);
				se.fromScrollbar = false;
				se.logicalMaxValue = (int32)readyList.size() * lastDy;
				se.logicalMinValue = 0;
				se.logicalPageSize = lastPageSize;
				se.rowSize = lastDy / 4;
				se.logicalValue = scrollPosition;
				platform.publisher.Publish(se);
			}

			lastRect = absRect;

			struct : IEventCallback<ImageCallbackArgs>
			{
				IGuiRenderContext* grc;
				std::string* target;

				virtual void OnEvent(ImageCallbackArgs& args)
				{
					grc->SelectTexture(args.textureId);

					float x0 = args.target.left;
					float x1 = args.target.right;
					float t = args.target.top;
					float b = args.target.bottom;

					float g = args.txUVtop;
					float h = args.txUVbottom;

					GuiVertex v[6] =
					{
					   { x0, t, 0, 0, RGBAb(255,255,255), 0, g, 0 },
					   { x1, t, 0, 0, RGBAb(255,255,255), 1, g, 0 },
					   { x1, b, 0, 0, RGBAb(255,255,255), 1, h, 0 },
					   { x0, t, 0, 0, RGBAb(255,255,255), 0, g, 0 },
					   { x0, b, 0, 0, RGBAb(255,255,255), 0, h, 0 },
					   { x1, b, 0, 0, RGBAb(255,255,255), 1, h, 0 }
					};

					grc->AddTriangle(v);
					grc->AddTriangle(v + 3);

					if (args.filename == *target)
					{
						GuiRect hilight{ (int32)args.target.left + 1, (int32)args.target.top + 1, (int32)args.target.right - 1, (int32)args.target.bottom - 1 };
						Rococo::Graphics::DrawBorderAround(*grc, hilight, Vec2i{ 2,2 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
					}
				}
			} renderImages;

			renderImages.grc = &grc;
			renderImages.target = &selectedItem;

			EnumerateVisibleImages(renderImages, absRect);
		}
	};

	class ToggleEventHandler;
	struct ToggleStateChanged
	{
		ToggleEventHandler* handler;
	};

	class ToggleEventHandler : public IObserver
	{
		std::vector<cstr> names;
		IPublisher& publisher;
		EventId id;
		int state = 0;
		IEventCallback<ToggleStateChanged>* eventHandler;

	public:
		ToggleEventHandler(cstr handlerName, IPublisher& _publisher, std::vector<cstr> _names) :
			id(handlerName, FastHash(handlerName)), publisher(_publisher), names(_names)
		{
			publisher.Attach(this, id);
		}

		~ToggleEventHandler()
		{
			publisher.Detach(this);
		}

		void AddHandler(IEventCallback<ToggleStateChanged>* _eventHandler)
		{
			eventHandler = _eventHandler;
		}

		int State() const
		{
			return state;
		}

		void SetState(int index)
		{
			if (index < 0 || index >= (int32)names.size())
			{
				Throw(0, "ToggleEventHandler::SetState. Index out of bounds: %d. [0,%d]", index, names.size());
			}

			this->state = index;
		}

		void OnEvent(Event& ev) override
		{
			if (ev.id == id)
			{
				auto& toe = As<TextOutputEvent>(ev);
				if (toe.isGetting)
				{
					SafeFormat(toe.text, sizeof(toe.text), "%s", names[state]);
				}
				else
				{
					for (int32 i = 0; i < names.size(); ++i)
					{
						if (Eq(names[i], toe.text))
						{
							this->state = i;

							if (eventHandler)
							{
								eventHandler->OnEvent(ToggleStateChanged{ this });
							}
							break;
						}
					}
				}
			}
		}
	};

	class Editor : public IEditor, public IUIElement, private IObserver, IEventCallback<ToggleStateChanged>, public IEditorState
	{
		WorldMap map;
		EditMode_SectorBuilder editMode_SectorBuilder;
		EditMode_SectorEditor editMode_SectorEditor;
		GuiMetrics metrics;
		AutoFree<IStatusBar> statusbar;
		Platform& platform;
		IPlayerSupervisor& players;
		TextureList textureList;

		ToggleEventHandler editModeHandler;
		ToggleEventHandler textureTargetHandler;
		ToggleEventHandler scrollLock;

		virtual bool IsScrollLocked() const
		{
			return scrollLock.State() == 1;
		}

		virtual void SetNeighbourTextureAt(Vec2 pos, bool forward)
		{
			auto* sector = map.Sectors().GetFirstSectorContainingPoint(pos);
			if (sector)
			{
				int state = textureTargetHandler.State();
				cstr textureName = sector->GetTexture(state);
				cstr nextName = textureList.GetNeighbour(textureName, forward);
				sector->SetTexture(state, nextName);
			}
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			return EditMode().OnKeyboardEvent(key);
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
		{
			EditMode().OnMouseMove(cursorPos, delta, dWheel);
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
		{
			EditMode().OnMouseLClick(cursorPos, clickedDown);
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
		{
			EditMode().OnMouseRClick(cursorPos, clickedDown);
		}

		IEditMode& EditMode()
		{
			return editModeHandler.State() == 0 ? editMode_SectorBuilder.Mode() : editMode_SectorEditor.Mode();
		}

		void OnEvent(ToggleStateChanged& ev)
		{
			if (ev.handler == &textureTargetHandler)
			{
				int state = textureTargetHandler.State();
				cstr textureName = editMode_SectorBuilder.GetTexture(state);
				textureList.ScrollTo(textureName);
			}
		}

		void OnEvent(Event& ev) override
		{
			if (ev.id == HV::Events::changeDefaultTextureId)
			{
				auto& cdt = As<HV::Events::ChangeDefaultTextureEvent>(ev);

				int32 textureTargetIndex = textureTargetHandler.State();
				editMode_SectorBuilder.SetTexture(textureTargetIndex, cdt.wallName);
			}
		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			grc.Renderer().GetGuiMetrics(metrics);
			map.Render(grc, EditMode().GetHilight());

			EditMode().Render(grc, absRect);

			map.RenderTopGui(grc, players.GetPlayer(0)->GetPlayerEntity());

			GuiRect statusRect{ absRect.left, absRect.bottom - 24, absRect.right, absRect.bottom };
			statusbar->Render(grc, statusRect);
		}

		void Free() override
		{
			delete this;
		}

		void OnEditorNew(cstr command)
		{

		}

		void OnEditorLoad(cstr command)
		{

		}

	public:
		Editor(Platform& _platform, IPlayerSupervisor& _players, ISectors& sectors) :
			platform(_platform),
			players(_players),
			map(_platform, sectors),
			textureList(_platform),
			editMode_SectorBuilder(_platform.publisher, map),
			editMode_SectorEditor(_platform, map, _platform.renderer.Window()),
			statusbar(CreateStatusBar(_platform.publisher)),
			editModeHandler("editor.edit_mode", _platform.publisher, { "v", "s" }),
			textureTargetHandler("editor.texture.target", _platform.publisher, { "w", "f", "c" }),
			scrollLock("editor.texture.lock", _platform.publisher, { "U", "L" })
		{
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorNew, "editor.new", nullptr);
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorLoad, "editor.load", nullptr);

			platform.publisher.Attach(this, HV::Events::changeDefaultTextureId);

			editModeHandler.SetState(0);
			textureTargetHandler.SetState(0);
			scrollLock.SetState(0);

			textureTargetHandler.AddHandler(this);

			platform.gui.RegisterPopulator("sector_editor", this);

			editMode_SectorEditor.SetEditor(this);
		}

		~Editor()
		{
			platform.gui.UnregisterPopulator(this);
			platform.publisher.Detach(this);
		}

		virtual cstr TextureName(int index) const
		{
			return editMode_SectorBuilder.GetTexture(index);
		}

		bool IsLoading() const override
		{
			return textureList.IsLoading();
		}
	};
}

namespace HV
{
	IEditor* CreateEditor(Platform& platform, IPlayerSupervisor& players, ISectors& sectors)
	{
		return new Editor(platform, players, sectors);
	}

	namespace Events
	{
		EventId changeDefaultTextureId = "editor.set.default.texture"_event;
	}
}
#include "hv.h"
#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.widgets.h>
#include <rococo.mplat.h>
#include <rococo.textures.h>
#include <rococo.ui.h>

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
		size_t litIndex = -1;

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
					auto& s = map.Sectors();

					size_t nSectors = s.end() - s.begin();
					if (litIndex < nSectors)
					{
						s.Delete(s.begin()[litIndex]);
						litIndex = -1;
					}
				}
				return true;
			}

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
						auto& secs = map.Sectors();

						size_t nSectors = secs.end() - secs.begin();
						if (litIndex < nSectors)
						{
							if (secs.begin()[litIndex] == s)
							{
								// secs.begin()[litIndex]->InvokeSectorDialog(parent, *editor);
							}
						}

						for (size_t i = 0; i < nSectors; ++i)
						{
							if (secs.begin()[i] == s)
							{
								litIndex = i;
								editor->SetPropertyTarget(secs.begin()[litIndex]);
								return;
							}
						}
					}
				}

				litIndex = -1;
				editor->SetPropertyTarget(nullptr);
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
		const ISector* GetHilight() const override
		{
			auto& secs = map.Sectors();
			size_t nSectors = secs.end() - secs.begin();
			return (litIndex < nSectors) ? secs.begin()[litIndex] : nullptr;
		}

		void SetEditor(IEditorState* editor) { this->editor = editor; }
		void CancelHilight() { litIndex = -1; }
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
	EventId vScrollSendKey = "editor.tools.vscroll_sendkey"_event;
	EventId vScrollSendMouse = "editor.tools.vscroll_sendmouse"_event;

	class TextureList : public IUIElement, public IObserver
	{
		Platform& platform;
		int32 scrollPosition = 0;
		Vec2i absTopLeft = { 0,0 };
		std::string target;
	public:
		TextureList(Platform& _platform) : platform(_platform)
		{
			platform.gui.RegisterPopulator("editor.tools.imagelist", this);
			platform.publisher.Attach(this, vScrollChanged);
			platform.publisher.Attach(this, vScrollGet);
		}

		~TextureList()
		{
			platform.publisher.Detach(this);
			platform.gui.UnregisterPopulator(this);
		}

		int32 GetIndexOf(cstr name)
		{
			return (int32)platform.renderer.GetMaterialId(name);
		}

		cstr GetNeighbour(cstr name, bool forward)
		{
			int32 i = GetIndexOf(name);

			MaterialArrayMetrics mam;
			platform.renderer.GetMaterialArrayMetrics(mam);

			if (forward) i++;
			else i--;

			if (i < 0) i = 0;
			if (i >= mam.NumberOfElements) i = mam.NumberOfElements - 1;

			return platform.renderer.GetMaterialTextureName((float)i);
		}

		void ScrollTo(cstr filename)
		{
			int index = -1;

			if (index < 0) return;

			scrollPosition = index * lastDy;

			ScrollEvent se(vScrollSet);
			se.fromScrollbar = false;
			se.logicalMaxValue = 0 * lastDy;

			scrollPosition = min(scrollPosition, se.logicalMaxValue - lastPageSize);

			se.logicalMinValue = 0;
			se.logicalPageSize = lastPageSize;
			se.rowSize = lastDy / 4;
			se.logicalValue = scrollPosition;

			platform.publisher.Publish(se);
		}

		cstr GetSelectedTexture() const
		{
			return "none";
		}

		bool OnKeyboardEvent(const KeyboardEvent& key) override
		{
			RouteKeyboard rk(vScrollSendKey, key);
			rk.consume = false;
			platform.publisher.Publish(rk);
			return rk.consume;
		}

		void OnRawMouseEvent(const MouseEvent& me) override
		{
			if (me.HasFlag(MouseEvent::LDown) || me.HasFlag(MouseEvent::LUp))
			{
				// We handle Lbutton ourselves as 'left-click to select'
				return;
			}
			RouteMouse rm(vScrollSendMouse, me);
			rm.absTopleft = absTopLeft;
			platform.publisher.Publish(rm);
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
				selectItem.target = "none";

				EnumerateVisibleImages(selectItem, lastRect);

				target = selectItem.target;

				HV::Events::ChangeDefaultTextureEvent ev;
				ev.wallName = selectItem.target.c_str();
				platform.publisher.Publish(ev);
			}
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
		{
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
					se.logicalMaxValue = 0 * lastDy;
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
			MaterialId matid;
		};

		void EnumerateVisibleImages(IEventCallback<ImageCallbackArgs>& cb, const GuiRect& absRect)
		{
			float x0 = absRect.left + 1.0f;
			float x1 = absRect.right - 1.0f;

			int32 width = Width(absRect) - 2;

			float y = absRect.top + 1.0f - scrollPosition;
			float dy = (float)width;

			lastPageSize = (int32)Height(absRect) - 2;

			MaterialArrayMetrics metrics;
			platform.renderer.GetMaterialArrayMetrics(metrics);

			for (MaterialId i = 0; i < metrics.NumberOfElements; i = i + 1)
			{
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
					args.filename = platform.renderer.GetMaterialTextureName(i);
					args.target = GuiRectf{ x0, t, x1, b };
					args.matid = i;
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
			int32 width = Width(absRect);

			absTopLeft = TopLeft(absRect);

			if (lastDy == 0 || lastDy != width)
			{
				lastDy = width;

				MaterialArrayMetrics mam;
				grc.Renderer().GetMaterialArrayMetrics(mam);

				int maxValue = mam.NumberOfElements * lastDy;
				lastPageSize = min(Height(absRect), maxValue);

				ScrollEvent se("editor.tools.vscroll_set"_event);
				se.fromScrollbar = false;
				se.logicalMaxValue = maxValue;
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
				cstr target;

				virtual void OnEvent(ImageCallbackArgs& args)
				{
					float x0 = args.target.left;
					float x1 = args.target.right;
					float t = args.target.top;
					float b = args.target.bottom;

					float g = args.txUVtop;
					float h = args.txUVbottom;

					SpriteVertexData material{ 0, 0, args.matid, 1 };
					RGBAb unused(0, 0, 0);
					GuiVertex v[6] =
					{
					   { {x0, t}, {{0, g}, 0}, material, unused },
					   { {x1, t}, {{1, g}, 0}, material, unused },
					   { {x1, b}, {{1, h}, 0}, material, unused },
					   { {x0, t}, {{0, g}, 0}, material, unused },
					   { {x0, b}, {{0, h}, 0}, material, unused },
					   { {x1, b}, {{1, h}, 0}, material, unused }
					};

					grc->AddTriangle(v);
					grc->AddTriangle(v + 3);

					if (target && Eq(args.filename, target))
					{
						GuiRect hilight{ (int32)args.target.left + 1, (int32)args.target.top + 1, (int32)args.target.right - 1, (int32)args.target.bottom - 1 };
						Rococo::Graphics::DrawBorderAround(*grc, hilight, Vec2i{ 2,2 }, RGBAb(224, 224, 224), RGBAb(255, 255, 255));
					}
				}
			} renderImages;

			renderImages.grc = &grc;
			renderImages.target = target.empty() ? nullptr : target.c_str();

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

	ROCOCOAPI IBloodyPropertyType
	{
		virtual void Click(bool clickedDown, Vec2i pos) = 0;
		virtual void Free() = 0;
		virtual cstr Name() const = 0;
		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour) = 0;
		virtual RGBAb NameColour() const = 0;
	};

	class BloodyFloatBinding : public IBloodyPropertyType
	{
		float value;
	public:
		BloodyFloatBinding(float _value) : value(_value)
		{

		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Float32";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			char buffer[16];
			SafeFormat(buffer, 16, "%f", value);
			Rococo::Graphics::RenderVerticalCentredText(rc, buffer, colour, 9, { rect.left + 4, Centre(rect).y }, &rect);
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 0, 0, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{

		}
	};

	class BloodyBoolBinding : public IBloodyPropertyType, public IKeyboardSink
	{
		bool value;
		Platform& platform;
	public:
		BloodyBoolBinding(Platform& _platform, bool _value): platform(_platform), value(_value)
		{

		}

		~BloodyBoolBinding()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					platform.gui.DetachKeyboardSink(this);
					break;
				case IO::VKCode_HOME:
					value = true;
					return true;
				case IO::VKCode_END:
					value = false;
					return true;
				case IO::VKCode_LEFT:
				case IO::VKCode_RIGHT:
				case IO::VKCode_SPACEBAR:
					value = !value;
					return true;
				}
			}
			return true;
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "bool";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle(rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			char buffer[16];
			SafeFormat(buffer, 16, "%s", value ? "true" : "false");
			Rococo::Graphics::RenderVerticalCentredText(rc, buffer, colour, 9, { rect.left + 4, Centre(rect).y }, &rect);

			int32 ds = Height(rect) - 4;
			GuiRect tickRect{ rect.right - ds, rect.top + 2, rect.right - 2, rect.bottom - 2 };
			Rococo::Graphics::DrawBorderAround(rc, tickRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));

			if (value)
			{
				Vec2i centre = Centre(tickRect);
				Rococo::Graphics::DrawLine(rc, 2, centre + Vec2i{ -6, -4 }, centre + Vec2i{ -2,+4 }, RGBAb(255, 128, 128, 255));
				Rococo::Graphics::DrawLine(rc, 2, centre + Vec2i{ -2,4 }, centre + Vec2i{ 4, -12 }, RGBAb(255, 128, 128, 255));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 64, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (clickedDown)
			{
				value = !value;
				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}
			}
		}
	};

	class BloodyEnumInt32Binding : public IBloodyPropertyType, public IKeyboardSink
	{
		int32 value = 0;
		std::string name;
		std::unordered_map<std::string, int> constantsNameToValue;
		std::unordered_map<int, std::string> constantsValueToName;
		std::vector<std::string> orderedByName;
		Platform& platform;
	public:
		BloodyEnumInt32Binding(Platform& _platform, cstr _name) : name(_name), platform(_platform)
		{

		}

		~BloodyEnumInt32Binding()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		virtual void AddEnumConstant(cstr key, int32 value)
		{
			constantsNameToValue[key] = value;
			constantsValueToName[value] = key;
			orderedByName.clear();
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return name.c_str();
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					platform.gui.DetachKeyboardSink(this);
					break;
				case IO::VKCode_HOME:
					if (!orderedByName.empty())
					{
						auto i = constantsNameToValue.find(orderedByName[0]);
						value = i != constantsNameToValue.end() ? i->second : value;
					}
					return true;
				case IO::VKCode_END:
					if (!orderedByName.empty())
					{
						auto i = constantsNameToValue.find(*orderedByName.rbegin());
						value = i != constantsNameToValue.end() ? i->second : value;
					}
					return true;
				case IO::VKCode_LEFT:
					if (!orderedByName.empty())
					{
						value = GetLeftValue(value);
					}
					return true;
				case IO::VKCode_RIGHT:
					if (!orderedByName.empty())
					{
						value = GetRightValue(value);
					}
					return true;
				case IO::VKCode_PGDOWN:
				{
					size_t delta = max(1ULL, orderedByName.size() / 10);
					for (size_t i = 0; i < delta; ++i)
					{
						value = GetLeftValue(value);
					}
				}
				return true;
				case IO::VKCode_PGUP:
				{
					size_t delta = max(1ULL, orderedByName.size() / 10);
					for (size_t i = 0; i < delta; ++i)
					{
						value = GetRightValue(value);
					}
				}
				return true;
				}

				char c = platform.keyboard.TryGetAscii(key);
				if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
				{
					for (size_t i = 0; i < orderedByName.size(); ++i)
					{
						char d = orderedByName[i][0];
						if ((d >= 'A' && d <= 'Z') || (d >= 'A' && d <= 'Z'))
						{
							if ((c & ~32) == (d & ~32))
							{
								// case independent match
								auto k = constantsNameToValue.find(orderedByName[i]);
								value = (k != constantsNameToValue.end()) ? k->second : value;
								return true;
							}
						}
					}
				}
				else if (c > 0)
				{
					for (size_t i = 0; i < orderedByName.size(); ++i)
					{
						char d = orderedByName[i][0];
						if (c == d)
						{
							auto k = constantsNameToValue.find(orderedByName[i]);
							value = (k != constantsNameToValue.end()) ? k->second : value;
							return true;
						}
					}
				}
			}
			return true;
		}

		void RenderValue(IGuiRenderContext& rc, int32 v, const GuiRect& rect, RGBAb colour)
		{
			char buffer[64];
			auto i = constantsValueToName.find(v);
			if (i == constantsValueToName.end())
			{
				SafeFormat(buffer, sizeof(buffer), "uknown(%d)", v);
			}
			else
			{
				SafeFormat(buffer, sizeof(buffer), "%s", i->second);
			}

			Rococo::Graphics::RenderCentredText(rc, buffer, colour, 9, Centre(rect), &rect);
		}

		int GetLeftValue(int v)
		{
			if (orderedByName.empty()) return v;

			auto i = constantsValueToName.find(v);
			if (i != constantsValueToName.end())
			{
				auto& name = i->second;

				if (orderedByName[0] == name)
				{
					return v;
				}

				for (size_t j = 1; j < orderedByName.size(); ++j)
				{
					if (orderedByName[j] == name)
					{
						auto k = constantsNameToValue.find(orderedByName[j - 1]);
						if (k == constantsNameToValue.end())
						{
							break;
						}
						else
						{
							return k->second;
						}
					}
				}
			}

			return max(v - 1, 0);
		}

		int GetRightValue(int v)
		{
			if (orderedByName.empty()) return v;

			auto i = constantsValueToName.find(v);
			if (i != constantsValueToName.end())
			{
				auto& name = i->second;

				if (*orderedByName.rbegin() == name)
				{
					return v;
				}

				for (size_t j = 0; j < orderedByName.size() - 1; ++j)
				{
					if (orderedByName[j] == name)
					{
						auto k = constantsNameToValue.find(orderedByName[j + 1]);
						if (k == constantsNameToValue.end())
						{
							break;
						}
						else
						{
							return k->second;
						}
					}
				}
			}

			return  min(v + 1, 0x7FFFFFFF);
		}

		GuiRect absRect{ 0,0,0,0 };

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			if (orderedByName.empty())
			{
				for (auto& i : constantsNameToValue)
				{
					orderedByName.push_back(i.first.c_str());
				}

				std::sort(orderedByName.begin(), orderedByName.end());
			}

			absRect = rect;

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle(rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			int32 spinnerSpan = Height(rect);

			int32 textSpan = Width(rect) - 2 * spinnerSpan;

			int32 nVisibleValues = 3;

			const int32 cellSpan = textSpan / nVisibleValues;

			Vec2i centre = Centre(rect);

			GuiRect currentValueRect{ centre.x - (cellSpan >> 1), rect.top, centre.x + (cellSpan >> 1), rect.bottom };

			RenderValue(rc, value, currentValueRect, colour);

			if (IsPointInRect(metrics.cursorPosition, rect))
			{
				GuiRect leftValueRect{ currentValueRect.left - cellSpan, rect.top, currentValueRect.left - 1, rect.bottom };
				GuiRect rightValueRect{ currentValueRect.right + 1, rect.top,  currentValueRect.right + cellSpan, rect.bottom };

				RGBAb dullColour(colour.red >> 1, colour.green >> 1, colour.blue >> 1);

				int leftValue = GetLeftValue(value);
				if (leftValue != value) RenderValue(rc, leftValue, leftValueRect, dullColour);

				int rightValue = GetRightValue(value);
				if (rightValue != value) RenderValue(rc, rightValue, rightValueRect, dullColour);

				int32 clickBorder = 4;
				GuiRect leftClickRect{ rect.left + clickBorder, rect.top + clickBorder, rect.left + spinnerSpan - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingLeft(rc, leftClickRect,  (leftValue != value) ? RGBAb(255, 255, 128) : RGBAb(64, 64, 64));

				GuiRect rightClickRect{ rect.right - spinnerSpan + clickBorder, rect.top + clickBorder, rect.right - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingRight(rc, rightClickRect, (rightValue != value) ? RGBAb(255, 255, 128) : RGBAb(64, 64, 64));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(16, 64, 16, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (!clickedDown)
			{
				auto centre = Centre(absRect);
				if (pos.x > centre.x + 20) value = GetRightValue(value);
				else if (pos.x < centre.x - 20) value = GetLeftValue(value);

				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}
			}
		}
	};

	struct IValidator
	{
		virtual bool IsLegal(char c, int charPos) const = 0;
		virtual void OnDetached(char* buffer) = 0;
	};

	struct PositiveIntegerValidator : public IValidator
	{
		bool IsLegal(char c, int ccursorPos) const override
		{
			return c >= '0' && c <= '9';
		}

		void OnDetached(char* buffer) override
		{
			if (*buffer == 0)
			{
				SafeFormat(buffer, 2, "0");
			}
		}
	};

	class TextEditorBox: public IKeyboardSink
	{
		char* buffer;
		int32 capacity;
		int32 cursorPos = 0;
		Platform& platform;
		bool defaultToEnd;
		IValidator& validator;
	public:
		TextEditorBox(Platform& _platform, char* _buffer, size_t _capacity, bool _defaultToEnd, IValidator& _validator) :
			platform(_platform), buffer(_buffer), capacity((int32)_capacity), defaultToEnd(_defaultToEnd), validator(_validator) {}

		~TextEditorBox()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		void PushBufferRightOne()
		{
			int32 len = (int32) strlen(buffer);
			if (len >= capacity - 1)
			{
				return;
			}

			for (int32 i = len; i > cursorPos; --i)
			{
				buffer[i] = buffer[i - 1];
			}
		}


		void DeleteRight(int pos)
		{
			int32 len = (int32) strlen(buffer);

			for (int32 i = pos; i < len; ++i)
			{
				buffer[i] = buffer[i+1];
			}
		}

		void AddCharOverwrite(char c)
		{
			int len = (int) strlen(buffer);
			if (cursorPos < len)
			{
				buffer[cursorPos++] = c;
			}
			else if (len < capacity - 1)
			{
				buffer[cursorPos++] = c;
			}
		}

		void AddCharInsert(char c)
		{
			int len = (int)strlen(buffer);
			if (len < capacity - 1)
			{
				PushBufferRightOne();
				buffer[cursorPos++] = c;
			}
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					platform.gui.DetachKeyboardSink(this);
					validator.OnDetached(buffer);
					return true;
				case IO::VKCode_BACKSPACE:
					if (cursorPos > 0)
					{
						DeleteRight(cursorPos-1);
						cursorPos--;
					}
					return true;
				case IO::VKCode_DELETE:
					DeleteRight(cursorPos);
					return true;
				case IO::VKCode_HOME:
					cursorPos = 0;
					return true;
				case IO::VKCode_END:
					cursorPos = (int32) strlen(buffer);
					return true;
				case IO::VKCode_LEFT:
					if (cursorPos > 0)
					{
						cursorPos--;
					}
					return true;
				case IO::VKCode_RIGHT:
					if (cursorPos < (int32)strlen(buffer))
					{
						cursorPos++;
					}
					return true;
				}

				char c = platform.keyboard.TryGetAscii(key);

				if (!validator.IsLegal(c, cursorPos))
				{
					OS::BeepWarning();
				}
				else
				{
					if (c >= 32)
					{
						if (platform.gui.IsOverwriting())
						{
							AddCharOverwrite(c);
						}
						else
						{
							AddCharInsert(c);
						}
						return true;
					}
				}
			}
			return false;
		}

		void Click(bool clickedDown)
		{
			if (clickedDown)
			{
				if (platform.gui.CurrentKeyboardSink() == this)
				{
					platform.gui.DetachKeyboardSink(this);
					validator.OnDetached(buffer);
				}
				else
				{
					platform.gui.AttachKeyboardSink(this);
				}
			}
		}

		void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			int32 len = (int32)strlen(buffer);
			if (cursorPos > len)
			{
				cursorPos = len;
			}

			int x = rect.left + 4;
			int y = Centre(rect).y;

			if (platform.gui.CurrentKeyboardSink() == this)
			{
				Rococo::Graphics::DrawRectangle (rc, rect, RGBAb(64, 0, 0, 128), RGBAb(64, 0, 0, 255));
				Rococo::Graphics::DrawBorderAround(rc, rect, { 1,1 }, RGBAb(255, 255, 255, 255), RGBAb(224, 224, 224, 255));
			}

			struct : IEventCallback<Rococo::Graphics::GlyphCallbackArgs>
			{
				int targetPos;
				GuiRect cursorRect{ 0,0,0,0 };
				GuiRect lastRect;
				virtual void OnEvent(Rococo::Graphics::GlyphCallbackArgs& args)
				{
					if (args.index == targetPos)
					{
						cursorRect = args.rect;
					}

					lastRect = args.rect;
				}
			} cb;

			cb.lastRect = { x, rect.top, x, rect.bottom };
			cb.targetPos = cursorPos;

			GuiRect clipRect = { x, rect.top, rect.right - 10, rect.bottom };

			int pos = platform.gui.CurrentKeyboardSink() == this ? cursorPos : (defaultToEnd ? len : 0);

			Rococo::Graphics::RenderVerticalCentredTextWithCallback(rc, pos, cb, buffer, colour, 10, { x, Centre(rect).y }, clipRect);
			
			if (platform.gui.CurrentKeyboardSink() == this)
			{
				if (cb.cursorRect.left == cb.cursorRect.right)
				{
					cb.cursorRect = { cb.lastRect.right,  y - 5, cb.lastRect.right + 8,  y + 5 };
				}

				cb.cursorRect.right = cb.cursorRect.left + 8;

				OS::ticks t = OS::CpuTicks();
				OS::ticks hz = OS::CpuHz();
				uint8 alpha = ((512 * t) / hz) % 255;

				if (platform.gui.IsOverwriting())
				{
					Rococo::Graphics::DrawRectangle(rc, cb.cursorRect, RGBAb(255, 255, 255, alpha >> 1), RGBAb(255, 255, 255, alpha >> 1));
				}
				else
				{
					Rococo::Graphics::DrawLine(rc, 2, BottomLeft(cb.cursorRect), BottomRight(cb.cursorRect), RGBAb(255, 255, 255, alpha));
				}
			}
		}
	};

	class BloodyIntBinding : public IBloodyPropertyType, public IValidator
	{
		int32 value;
		TextEditorBox teb;
		Platform& platform;
		char buffer[12];
		bool addHexView;
	public:
		BloodyIntBinding(Platform& _platform, bool _addHexView, int _value) :
			platform(_platform),
			teb(_platform, buffer, 12, false, *this),
			addHexView(_addHexView),
			value(_value)
		{
			SafeFormat(buffer, 12, "%d", _value);
		}

		virtual bool IsLegal(char c, int charPos) const
		{
			if (charPos == 0)
			{
				if (buffer[0] != '-' && c == '-')
				{
					return true;
				}

				if (buffer[0] == '-' && platform.gui.IsOverwriting())
				{
					return false;
				}
			}
			return c >= '0' && c <= '9';
		}

		virtual void OnDetached(char* buffer)
		{
			value = atoi(buffer);
			SafeFormat(buffer, 10, "%d", value);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Int32";
		}

		GuiRect tebRect;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			tebRect = rect;
			teb.Render(rc, tebRect, colour);
			 
			if (addHexView && platform.gui.CurrentKeyboardSink() != &teb)
			{
				char hex[12];
				SafeFormat(hex, 12, "0x%8.8X", value);

				RGBAb dullColour(colour.red, colour.green, colour.blue, colour.alpha >> 1);
				GuiRect hexRect{ rect.left, rect.top, rect.right - 8, rect.bottom };
				Rococo::Graphics::RenderRightAlignedText(rc, hex, dullColour, 9, hexRect);
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(48, 16, 0, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, tebRect))
			{
				teb.Click(clickedDown);
			}
		}
	};

	class BloodyColour : public IBloodyPropertyType, public IKeyboardSink
	{
		RGBAb value;
		std::string name;

		Platform& platform;

		PositiveIntegerValidator piv;

		char rbuffer[4] = "255";
		char gbuffer[4] = "0";
		char bbuffer[4] = "0";
		char abuffer[4] = "255";

		TextEditorBox rbox;
		TextEditorBox gbox;
		TextEditorBox bbox;
		TextEditorBox abox;

		TextEditorBox* teb[4] = { &rbox, &gbox, &bbox, &abox };
	public:
		BloodyColour(Platform& _platform, cstr _name, RGBAb _colour) :
			name(_name),
			platform(_platform),
			rbox(_platform, rbuffer, 4, false, piv),
			gbox(_platform, gbuffer, 4, false, piv),
			bbox(_platform, bbuffer, 4, false, piv),
			abox(_platform, abuffer, 4, false, piv),
			value(_colour)
		{

		}

		~BloodyColour()
		{
			platform.gui.DetachKeyboardSink(this);
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return name.c_str();
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			if (!key.IsUp())
			{
				switch (key.VKey)
				{
				case IO::VKCode_ENTER:
					return true;
				case IO::VKCode_HOME:
					return true;
				case IO::VKCode_END:
					return true;
				case IO::VKCode_LEFT:
					return true;
				case IO::VKCode_RIGHT:
					return true;
				case IO::VKCode_PGDOWN:
					return true;
				case IO::VKCode_PGUP:
					return true;
				}
			}
			return true;
		}

		GuiRect absRect{ 0,0,0,0 };

		GuiRect tebRect[4];

		void ParseValue()
		{
			int32 red = atoi(rbuffer);
			int32 green = atoi(gbuffer);
			int32 blue = atoi(bbuffer);
			int32 alpha = atoi(abuffer);

			value.red = (uint8)red;
			value.green = (uint8)green;
			value.blue = (uint8)blue;
			value.alpha = (uint8)alpha;
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			ParseValue();

			absRect = rect;

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			int span = Height(rect);

			GuiRect buttonRect{ rect.left + 2, rect.top + 2, rect.left + span - 2, rect.bottom - 2 };

			RGBAb fullColour{ value.red, value.green, value.blue, 255 };
			Rococo::Graphics::DrawRectangle(rc, buttonRect, fullColour, value);

			if (IsPointInRect(metrics.cursorPosition, buttonRect))
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));
			}
			else
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(192, 192, 192), RGBAb(160, 160, 160));
			}

			int32 cellSpan = (Width(rect) - Width(buttonRect)) / 4;

			TextEditorBox* teb[] = { &rbox, &gbox, &bbox, &abox };
			RGBAb fontColours[4] = { RGBAb(255,0,0,255), RGBAb(0,255,0,255), RGBAb(0,0,255,255), RGBAb(128,128,128,255) };

			int32 x = buttonRect.right + 1;
			for (int i = 0; i < 4; ++i)
			{
				tebRect[i] = GuiRect{ x, rect.top, x + cellSpan-2, rect.bottom };
				if (i == 3) tebRect[i].right = rect.right - 1;
				teb[i]->Render(rc, tebRect[i], fontColours[i]);

				Rococo::Graphics::DrawBorderAround(rc, tebRect[i], { 1,1 }, RGBAb(192, 192, 192, 32), RGBAb(160, 160, 160, 32));

				x += cellSpan;
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 32, 32, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{	
			for (int i = 0; i < 4; ++i)
			{
				if (IsPointInRect(pos, tebRect[i]))
				{
					teb[i]->Click(clickedDown);
					break;
				}
			}
		}
	};

	class BloodyMaterialBinding : public IBloodyPropertyType, public IValidator
	{
		char value[IO::MAX_PATHLEN] = { "random" };
		Platform& platform;
		TextEditorBox teb;
		MaterialId id = -1;

	public:
		BloodyMaterialBinding(Platform& _platform, cstr matString) : 
			platform(_platform),
			teb(_platform, value, IO::MAX_PATHLEN, true, *this)
		{
			SafeFormat(value, IO::MAX_PATHLEN, "%s", matString);
		}

		virtual bool IsLegal(char c, int charPos) const
		{
			return true;
		}

		void OnDetached(char* buffer) override
		{
			char sysPath[IO::MAX_PATHLEN];
			try
			{
				platform.installation.ConvertPingPathToSysPath(value, sysPath, IO::MAX_PATHLEN);
				id = platform.renderer.GetMaterialId(sysPath);
			}
			catch (IException&)
			{
				id = -1;
			}

			if (*buffer == 0)
			{
				SafeFormat(buffer, IO::MAX_PATHLEN, "random");
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "MaterialId";
		}

		MaterialId GetLeftValue(MaterialId v)
		{
			if (v >= 0) v -= 1;
			return v;
		}

		MaterialId GetRightValue(MaterialId v)
		{
			MaterialArrayMetrics metrics;
			platform.renderer.GetMaterialArrayMetrics(metrics);
			if (v < metrics.NumberOfElements - 1)
			{
				v += 1;
			}
			return v;
		}

		GuiRect leftClickRect;
		GuiRect rightClickRect;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			int32 spinnerSpan = Height(rect);

			GuiRect editorRect{ rect.left + spinnerSpan, rect.top, rect.right - spinnerSpan, rect.bottom };

			if (Eq("random", value) && platform.gui.CurrentKeyboardSink() != &teb)
			{
				Rococo::Graphics::RenderCentredText(rc, value, RGBAb(128, 0, 0, 255), 9, Centre(editorRect), &editorRect);
			}
			else
			{
				teb.Render(rc, editorRect, id < 0 ? RGBAb(255, 64, 64, 255) : colour);
			}

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			MaterialId leftValue = GetLeftValue(id);
			MaterialId rightValue = GetRightValue(id);

			if (IsPointInRect(metrics.cursorPosition, rect))
			{
				RGBAb spinColour1 = IsPointInRect(metrics.cursorPosition, leftClickRect) ? RGBAb(255, 255, 128) : RGBAb(192, 192, 64);
				int32 clickBorder = 4;
				leftClickRect = GuiRect{ rect.left + clickBorder, rect.top + clickBorder, rect.left + spinnerSpan - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingLeft(rc, leftClickRect, (leftValue != id) ? spinColour1 : RGBAb(64, 64, 64));

				RGBAb spinColour2 = IsPointInRect(metrics.cursorPosition, rightClickRect) ? RGBAb(255, 255, 128) : RGBAb(192, 192, 64);
				rightClickRect = GuiRect{ rect.right - spinnerSpan + clickBorder, rect.top + clickBorder, rect.right - clickBorder, rect.bottom - clickBorder };
				Rococo::Graphics::DrawTriangleFacingRight(rc, rightClickRect, (rightValue != id) ? spinColour2 : RGBAb(64, 64, 64));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(96, 0, 96, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			bool consumed = false;

			if (clickedDown)
			{
				auto oldId = id;

				if (IsPointInRect(pos, leftClickRect))
				{
					id = GetLeftValue(id);
					if (id == -1)
					{
						SafeFormat(value, sizeof(value), "random");
					}
					consumed = true;
				}
				else if (IsPointInRect(pos, rightClickRect))
				{
					id = GetRightValue(id);
					consumed = true;
				}

				if (id == -1)
				{
					if (*value == 0)
					{
						SafeFormat(value, sizeof(value), "random");
					}
				}
				else if (oldId != id)
				{
					auto mat = platform.renderer.GetMaterialTextureName(id);
					if (mat)
					{
						char sysName[IO::MAX_PATHLEN];
						SafeFormat(sysName, sizeof(sysName), "%s", mat);

						auto root = platform.installation.Content();

						if (strstr(sysName, root))
						{
							cstr subPath = sysName + root.length;
							SafeFormat(value, sizeof(value), "!%s", subPath);
							OS::ToUnixPath(value);
						}
					}
				}
			}

			if (!consumed) teb.Click(clickedDown);
		}
	};


	class BloodySpacer : public IBloodyPropertyType
	{
	public:
		BloodySpacer() 
		{
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Spacer";
		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
		}

		RGBAb NameColour() const override
		{
			return RGBAb(0, 0, 0, 0);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
		}
	};

	class BloodyPingPathBinding : public IBloodyPropertyType, public IValidator
	{
		char value[IO::MAX_PATHLEN] = { 0 };
		Platform& platform;
		TextEditorBox teb;
		bool validated = false;
	public:
		BloodyPingPathBinding(Platform& _platform, cstr pingPath) : platform(_platform), 
			teb(_platform, value, IO::MAX_PATHLEN, true, *this)
		{
			SafeFormat(value, IO::MAX_PATHLEN, "%s", pingPath);
		}

		virtual bool IsLegal(char c, int cursorPos) const
		{
			return true;
		}

		void OnDetached(char* buffer)
		{
			char sysPath[IO::MAX_PATHLEN];
			try
			{
				platform.installation.ConvertPingPathToSysPath(buffer, sysPath, IO::MAX_PATHLEN);
				validated = OS::IsFileExistant(sysPath);
			}
			catch (IException&)
			{
				validated = false;
			}
		}

		virtual void Free()
		{
			delete this;
		}

		virtual cstr Name() const
		{
			return "Ping Path";
		}

		GuiRect editorRect;
		GuiRect buttonRect;

		const fstring loadImage = "!textures/toolbars/load.tif"_fstring;

		virtual void Render(IGuiRenderContext& rc, const GuiRect& rect, RGBAb colour)
		{
			int span = Height(rect);
			editorRect = GuiRect{ rect.left + span, rect.top, rect.right, rect.bottom };

			RGBAb colour2 = validated ? colour : RGBAb(255, 64, 64);
			teb.Render(rc, editorRect, colour2);

			buttonRect = GuiRect{ rect.left, rect.top + 2, editorRect.left - 1, rect.bottom - 2 };

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);

			Textures::BitmapLocation bml;
			if (platform.renderer.SpriteBuilder().TryGetBitmapLocation(loadImage, bml))
			{
				Rococo::Graphics::DrawSpriteCentred(buttonRect, bml, rc);
			}
			else
			{
				Rococo::Graphics::RenderCentredText(rc, "~", RGBAb(255, 255, 255), 3, Centre(buttonRect), &buttonRect);
			}

			if (IsPointInRect(metrics.cursorPosition, buttonRect))
			{
				Rococo::Graphics::DrawBorderAround(rc, buttonRect, { 1,1 }, RGBAb(255, 255, 255), RGBAb(224, 224, 224));
			}
		}

		RGBAb NameColour() const override
		{
			return RGBAb(64, 64, 64, 128);
		}

		void Click(bool clickedDown, Vec2i pos) override
		{
			if (IsPointInRect(pos, editorRect))
			{
				teb.Click(clickedDown);
			}
			else if (IsPointInRect(pos, buttonRect))
			{
				SaveDesc sd;
				sd.caption = "Select or Create file";
				sd.ext = "*.sxy";
				sd.extDesc = "Sexy script (.sxy)";
				sd.shortName = nullptr;

				bool changed = false;

				auto content = platform.installation.Content();

				if (*value == '!')
				{
					char buffer[IO::MAX_PATHLEN];
					platform.os.ConvertUnixPathToSysPath(value, buffer, IO::MAX_PATHLEN);
					SafeFormat(sd.path, sizeof(sd.path), "%s%s", (cstr) content, buffer);

					if (OS::IsFileExistant(sd.path))
					{
						changed = platform.utilities.GetSaveLocation(platform.renderer.Window(), sd);
					}
					else
					{
						if (platform.utilities.QueryYesNo(platform, platform.renderer.Window(), "Cannot locate file. Navigate to content?"))
						{
							SafeFormat(sd.path, sizeof(sd.path), "%s", (cstr)content);
							changed = platform.utilities.GetSaveLocation(platform.renderer.Window(), sd);
						}
					}
				}
				else
				{
					if (platform.utilities.QueryYesNo(platform, platform.renderer.Window(), "Path did not begin with ping. Navigate to content?"))
					{
						SafeFormat(sd.path, sizeof(sd.path), "%s", (cstr)content);
						changed = platform.utilities.GetSaveLocation(platform.renderer.Window(), sd);
					}
				}

				if (changed)
				{
					if (strstr(sd.path, content) != sd.path)
					{
						try
						{
							Throw(0, "Path was not a subdirectory of the content folder\nChoose a secure ping path");
						}
						catch (IException& ex)
						{
							platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, platform.title);
						}
						return;
					}

					OS::ToUnixPath(sd.path + content.length);

					SecureFormat(value, IO::MAX_PATHLEN, "!%s", sd.path + content.length);

					OnDetached(value);
				}
			}
		}
	};

	class BloodyProperty
	{
		AutoFree<IBloodyPropertyType> prop;
		std::string name;
		GuiRect lastRect{ 0,0,0,0 };
	public:
		BloodyProperty(IBloodyPropertyType* _prop, cstr _name) :
			name(_name), prop(_prop)
		{

		}

		cstr Name() const { return name.c_str(); }
		IBloodyPropertyType& Prop() { return *prop; }
		void SetRect(const GuiRect& rect) { lastRect = rect;  }
		bool IsInRect(Vec2i p) const { return IsPointInRect(p, lastRect); }
	};

	class BloodyPropertySetEditor: public IUIElement, private IEventCallback<Rococo::Events::ScrollEvent>, public IBloodyPropertySetEditorSupervisor
	{
		std::vector<BloodyProperty*> properties;
		Platform& platform;
		AutoFree<IScrollbar> vscroll;

		bool ValidateUnique(cstr name) const
		{
			for (auto i : properties)
			{
				if (Eq(i->Name(), name))
				{
					return true;
				}
			}

			return false;
		}

		void ValidateNotFound(cstr name) const
		{
			if (ValidateUnique(name)) Throw(0, "Duplicate property with name %s", name);
		}

		void Add(BloodyProperty* bp)
		{
			ValidateNotFound(bp->Name());
			properties.push_back(bp);
		}
	public:
		BloodyPropertySetEditor(Platform& _platform): 
			platform(_platform),
			vscroll(platform.utilities.CreateScrollbar(true))
		{
		}

		~BloodyPropertySetEditor()
		{
			Clear();
		}

		void Clear()
		{
			for (auto i : properties)
			{
				delete i;
			}
			properties.clear();
		}

		virtual void Free()
		{
			delete this;
		}

		void AddBool(cstr name, bool value) override
		{
			Add( new BloodyProperty(new BloodyBoolBinding(platform, value), name) );
		}

		void AddSpacer() override
		{
			properties.push_back(new BloodyProperty(new BloodySpacer(), ""));
		}

		void AddFloat(cstr name, float value) override
		{
			Add( new BloodyProperty(new BloodyFloatBinding(value), name) );
		}

		void AddInt(cstr name, bool addHexView, int value) override
		{
			Add( new BloodyProperty(new BloodyIntBinding(platform, addHexView, value), name) );
		}

		void AddMaterialCategory(cstr name, Rococo::Graphics::MaterialCategory cat) override
		{
			auto* b = new BloodyEnumInt32Binding(platform, "MaterialCategory");
			b->AddEnumConstant("Rock",   Rococo::Graphics::MaterialCategory_Rock);
			b->AddEnumConstant("Stone",  Rococo::Graphics::MaterialCategory_Stone);
			b->AddEnumConstant("Marble", Rococo::Graphics::MaterialCategory_Marble);
			b->AddEnumConstant("Metal",  Rococo::Graphics::MaterialCategory_Metal);
			b->AddEnumConstant("Wood",   Rococo::Graphics::MaterialCategory_Wood);
			Add( new BloodyProperty(b, name));
		}

		void AddColour(cstr name, RGBAb colour) override
		{
			auto* b = new BloodyColour(platform, "Colour", colour);
			Add(new BloodyProperty(b, name));
		}

		void AddMaterialString(cstr name, cstr matString) override
		{
			auto* b = new BloodyMaterialBinding(platform, matString);
			Add(new BloodyProperty(b, name));
		}

		void AddPingPath(cstr name, cstr pingPath) override
		{
			Add(new BloodyProperty(new BloodyPingPathBinding(platform, pingPath), name));
		}
		
		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			return false;
		}

		virtual void OnRawMouseEvent(const MouseEvent& ev)
		{
		}

		virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
		{

		}

		virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown)
		{
			for (auto i : properties)
			{
				if (i->IsInRect(cursorPos))
				{
					i->Prop().Click(clickedDown, cursorPos);
					return;
				}
			}
		}

		virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown)
		{

		}

		virtual void OnEvent(Rococo::Events::ScrollEvent& se)
		{

		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& absRect)
		{
			GuiRect scrollRect{ absRect.right - 20, absRect.top, absRect.right, absRect.bottom};

			Modality modality;
			modality.isModal = true;
			modality.isTop = true;
			modality.isUnderModal = false;
			vscroll->Render(rc, scrollRect, modality, RGBAb(48, 48, 48, 240), RGBAb(32, 32, 32, 240), RGBAb(255, 255, 255), RGBAb(192, 192, 192), *this, ""_event);

			GuiRect mainRect{ absRect.left, absRect.top, scrollRect.left-1, absRect.bottom };

			GuiMetrics metrics;
			rc.Renderer().GetGuiMetrics(metrics);
			if (IsPointInRect(metrics.cursorPosition, mainRect))
			{
				Rococo::Graphics::DrawRectangle(rc, mainRect, RGBAb(0, 0, 24, 255), RGBAb(0, 0, 24, 192));
			}
			else
			{
				Rococo::Graphics::DrawRectangle(rc, mainRect, RGBAb(0, 0, 0, 255), RGBAb(0, 0, 0, 192));
			}

			int y = absRect.top + 2;

			int odd = true;

			for (auto p : properties)
			{
				int y1 = y + 20;
				GuiRect rowRect{ absRect.left + 2, y, mainRect.right - 2, y1 };
				RGBAb edge1 = IsPointInRect(metrics.cursorPosition, rowRect) ? RGBAb(255, 255, 255) : RGBAb(64, 64, 64, 64);
				RGBAb edge2 = IsPointInRect(metrics.cursorPosition, rowRect) ?  RGBAb(224, 224, 224) : RGBAb(32, 32, 32, 64);

				RGBAb fontColour = IsPointInRect(metrics.cursorPosition, rowRect) ? RGBAb(255, 255, 255) : RGBAb(224, 224, 224, 224);

				if (*p->Name())
				{
					GuiRect nameRect{ rowRect.left, y, rowRect.left + 130, y1 };
					Rococo::Graphics::DrawRectangle(rc, nameRect, p->Prop().NameColour(), p->Prop().NameColour());
					Rococo::Graphics::RenderVerticalCentredText(rc, p->Name(), fontColour, 9, { rowRect.left + 4, Centre(rowRect).y }, &nameRect);

					GuiRect valueRect{ nameRect.right + 1, y, rowRect.right, y1 };
					p->Prop().Render(rc, valueRect, fontColour);
					p->SetRect(valueRect);
					Rococo::Graphics::DrawBorderAround(rc, valueRect, { 1,1 }, edge1, edge2);
				}

				y = y1 + 3;
				odd = !odd;
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

		BloodyPropertySetEditor wallEditor;
		BloodyPropertySetEditor floorEditor;
		BloodyPropertySetEditor ceilingEditor;
		BloodyPropertySetEditor doorEditor;

		char levelpath[IO::MAX_PATHLEN] = { 0 };

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

		void OnRawMouseEvent(const MouseEvent& key) override
		{
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

		IPropertyTarget* target = nullptr;

		virtual void SetPropertyTargetToSuccessor()
		{
			target = nullptr;
		}

		virtual void SetPropertyTarget(IPropertyTarget* target)
		{
			wallEditor.Clear();
			floorEditor.Clear();
			ceilingEditor.Clear();
			doorEditor.Clear();

			if (target)
			{
				target->Assign(this);
				target->GetProperties("walls", wallEditor);
				target->GetProperties("floor", floorEditor);
				target->GetProperties("ceiling", ceilingEditor);
				target->GetProperties("door", doorEditor);
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
			map.Sectors().Builder()->Clear();
			editMode_SectorEditor.CancelHilight();
		}

		void OnEditorLoad(cstr command)
		{
			LoadDesc ld;
			ld.caption = "Select a level file to load";
			ld.ext = "*.level.sxy";
			ld.extDesc = "Sexy script level-file (.level.sxy)";
			ld.shortName = nullptr;
			*ld.path = 0;

			if (platform.utilities.GetLoadLocation(platform.renderer.Window(), ld))
			{
				Load(ld.path);
				SafeFormat(levelpath, sizeof(levelpath), "%s", ld.path);
				platform.utilities.AddSubtitle(platform, ld.shortName);
			}
		}

		void OnEditorSave(cstr command)
		{
			SaveDesc sd;
			sd.caption = "Select a level file to save";
			sd.ext = "*.level.sxy";
			sd.extDesc = "Sexy script level-file (.level.sxy)";
			sd.shortName = nullptr;

			SafeFormat(sd.path, sizeof(sd.path), "%s", levelpath);

			if (platform.utilities.GetSaveLocation(platform.renderer.Window(), sd))
			{
				Save(sd.path);
				SafeFormat(levelpath, sizeof(levelpath), "%s", sd.path);
				platform.utilities.AddSubtitle(platform, sd.shortName);
			}
		}

		void Load(cstr filename)
		{
			HV::Events::SetNextLevelEvent setNextLevelEvent;
			setNextLevelEvent.name = filename;

			platform.publisher.Publish(setNextLevelEvent);
		}

		void Save(cstr filename)
		{
			std::vector<char> buffer;
			buffer.resize(1024_kilobytes);

			char* buffer0 = &buffer[0];
			memset(buffer0, 0, buffer.size());

			StackStringBuilder sb(buffer0, buffer.size());

			sb.AppendFormat("(' #file.type hv.level)\n\n");

			sb.AppendFormat("(' #include\n\t\"!scripts/mplat.sxh.sxy\""
				"\n\t\"!scripts/hv.sxh.sxy\""
				"\n\t\"!scripts/types.sxy\""
				"\n\t\"!scripts/hv/hv.types.sxy\""
				")\n\n");

			sb.AppendFormat("(namespace EntryPoint)\n\t(alias Main EntryPoint.Main)\n\n");
			sb.AppendFormat("(function Main(Int32 id)->(Int32 exitCode) :\n");
			sb.AppendFormat("\t(AddSectorsToLevel)\n");
			sb.AppendFormat(")\n\n");

			map.Sectors().SaveAsFunction(sb);

			try
			{
				platform.utilities.SaveBinary(filename, buffer0, sb.Length());
			}
			catch (IException& ex)
			{
				platform.utilities.ShowErrorBox(platform.renderer.Window(), ex, ex.Message());
			};
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
			scrollLock("editor.texture.lock", _platform.publisher, { "U", "L" }),
			wallEditor(_platform),
			floorEditor(_platform),
			ceilingEditor(_platform),
			doorEditor(_platform)
		{
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorNew, "editor.new", nullptr);
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorLoad, "editor.load", nullptr);
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorSave, "editor.save", nullptr);

			platform.publisher.Attach(this, HV::Events::changeDefaultTextureId);

			editModeHandler.SetState(0);
			textureTargetHandler.SetState(0);
			scrollLock.SetState(0);

			textureTargetHandler.AddHandler(this);

			platform.gui.RegisterPopulator("sector_editor", this);

			editMode_SectorEditor.SetEditor(this);

			platform.gui.RegisterPopulator("editor.tab.walls", &wallEditor);
			platform.gui.RegisterPopulator("editor.tab.floor", &floorEditor);
			platform.gui.RegisterPopulator("editor.tab.ceiling", &ceilingEditor);
			platform.gui.RegisterPopulator("editor.tab.doors", &doorEditor);
		}

		~Editor()
		{
			platform.gui.UnregisterPopulator(&wallEditor);
			platform.gui.UnregisterPopulator(&floorEditor);
			platform.gui.UnregisterPopulator(&ceilingEditor);
			platform.gui.UnregisterPopulator(&doorEditor);
			platform.gui.UnregisterPopulator(this);
			platform.publisher.Detach(this);
		}

		virtual cstr TextureName(int index) const
		{
			return editMode_SectorBuilder.GetTexture(index);
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
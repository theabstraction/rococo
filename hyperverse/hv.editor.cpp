#include "hv.h"
#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.widgets.h>

#include <vector>
#include <deque>
#include<algorithm>

#include <rococo.rings.inl>

namespace
{
   using namespace HV;
   using namespace Rococo;
   using namespace Rococo::Widgets;


   class WorldMap
   {
   private:
      HV::Entities::IInstancesSupervisor& instances;
      int32 gridlinePixelWidth{ 8 };
      Metres gridlineMetricWidth{ 2.0f };
      Vec2 gridCentre{ 0, 0 }; // Always uses integral co-ordinates
      bool isGrabbed{ false };
      Vec2i grabPosition;
      Vec2 grabbedCentre;
      Vec2i pixelOffset{ 0, 0 };
      Vec2i grabbedOffset{ 0,0 };
      GuiMetrics metrics { 0 };   

      AutoFree<ISectors> sectors;
   public:
      ISectors& Sectors() { return *sectors; }

      WorldMap(HV::Entities::IInstancesSupervisor& _instances) : instances(_instances), sectors(CreateSectors(_instances))
      {
         
      }

      Vec2 GetWorldPosition(Vec2i screenPosition)
      {
         Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };
         Vec2i cursorOffset = { screenPosition.x - centre.x , centre.y - screenPosition.y };
         Vec2i pixelDelta = cursorOffset - pixelOffset;

         Vec2 centreOffset =  Vec2{ pixelDelta.x / (float) gridlinePixelWidth, pixelDelta.y / (float) gridlinePixelWidth };
         return gridCentre + centreOffset * gridlineMetricWidth;
      }

      Vec2i GetScreenPosition(Vec2 worldPosition)
      {
         Vec2i centre = { metrics.screenSpan.x >> 1,metrics.screenSpan.y >> 1 };

         Vec2 pixelDelta = (worldPosition - gridCentre) * (float) gridlinePixelWidth / gridlineMetricWidth;
         return{ ((int32)(pixelDelta.x)) + centre.x + pixelOffset.x, - (  ((int32)(pixelDelta.y)) - centre.y + pixelOffset.y )};
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

      void RenderTopGui(IGuiRenderContext& grc)
      {
         Vec2 worldCursor = GetWorldPosition(metrics.cursorPosition);

         rchar originText[24];
         SafeFormat(originText, sizeof(originText), "(%4.1f,%4.1f)", worldCursor.x, worldCursor.y);

         Vec2i centre{ metrics.screenSpan.x >> 1, metrics.screenSpan.y >> 1 };
         Rococo::Graphics::DrawRectangle(grc, { centre.x - 70,0,centre.x + 70, 20 }, RGBAb(64, 64, 64, 224), RGBAb(64, 64, 64, 224));
         Rococo::Graphics::RenderCentredText(grc, originText, RGBAb(255, 255, 255), 9, { metrics.screenSpan.x >> 1, 8 });
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

         Vec2i centreOffseti = GetScreenPosition(Vec2{0, 0});
         Vec2 centreOffset{ (float)centreOffseti.x, (float)centreOffseti.y };

         const float scale = gridlinePixelWidth / gridlineMetricWidth;

         for (ISector* sector : *sectors)
         {
            float dim = !litSector ? 0.9f : 0.7f;
            RGBAb colour = sector->GetGuiColour(sector == litSector ? 1.0f : dim);
            ObjectVertexBuffer vertices = sector->FloorVertices();
            for(size_t i = 0; i < vertices.VertexCount; i += 3 )
            {
               GuiVertex v[3];
               for (int j = 0; j < 3; ++j)
               {
                  Vec2 worldPos{ vertices.v[i + j].position.x, vertices.v[i + j].position.y };
                  auto pos = GetScreenPosition(worldPos);
                  v[j].x = (float) pos.x;
                  v[j].y = (float) pos.y;
                  v[j].u = vertices.v[i + j].uv.x;
                  v[j].v = vertices.v[i + j].uv.y;
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

               Rococo::Graphics::DrawLine(grc, 2, GetScreenPosition(p), GetScreenPosition(q), RGBAb(255, 255, 255));
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

   ROCOCOAPI IEditMode: public IUITarget
   {
      virtual void Render(IGuiRenderContext& grc) = 0; 
      virtual const ISector* GetHilight() const = 0;
   };

   class EditMode_SectorEditor : private IEditMode
   {
      WorldMap& map;
      GuiMetrics metrics;
      IPublisher& publisher;
      Windows::IWindow& parent;
      ISector* lit{ nullptr };

      void GetRect(GuiRect& rect) const override
      {
         rect = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
      }

      void Render(IGuiRenderContext& grc) override
      {
         grc.Renderer().GetGuiMetrics(metrics);
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
                     lit->InvokeSectorDialog(parent);
                  }
                  lit = s;
               }
            }
         }
      }
   public:
      EditMode_SectorEditor(IPublisher& _publisher, WorldMap& _map, Windows::IWindow& _parent) : 
         publisher(_publisher), 
         map(_map),
         parent(_parent)
      { }
      IEditMode& Mode() { return *this; }
      const ISector* GetHilight() const override { return lit; }
   };

   class EditMode_SectorBuilder: private IEditMode
   {
      bool isLineBuilding{ false };
      std::vector<Vec2> lineList;
      WorldMap& map;
      GuiMetrics metrics;
      IPublisher& publisher;

      void GetRect(GuiRect& rect) const override
      {
         rect = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
      }

      void Render(IGuiRenderContext& grc) override
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
            Vec2 lastVertex = lineList[lineList.size()-1];
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
            // First point is not allowed to be in or on any sector
            SectorAndSegment sns = map.Sectors().GetFirstSectorWithPoint(worldPosition);
            if (sns.sector != nullptr)
            {
               SetStatus("A new sector's first point must lie outside all other sectors", publisher);
               return;
            }

            ISector* sector = map.Sectors().GetFirstSectorContainingPoint(worldPosition);
            if (sector != nullptr)
            {
               SetStatus("A new sector's first point must lie outside all other sectors", publisher);
               return;
            }   
         }

         if (!lineList.empty())
         {
            Vec2 a = *lineList.rbegin();
            Vec2 b = worldPosition;

            SectorAndSegment sns = map.Sectors().GetFirstSectorWithPoint(b);
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
               map.Sectors().AddSector(&lineList[0], lineList.size());
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
      IEditMode& Mode() { return *this; }
      const ISector* GetHilight() const override { return nullptr; }
   };

   class Editor : public IEditor, public IUIOverlay, public IObserver
   {
      IPublisher& publisher;
      HV::Entities::IInstancesSupervisor& instances;
      WorldMap map;
      EditMode_SectorBuilder editMode_SectorBuilder;
      EditMode_SectorEditor editMode_SectorEditor;
      AutoFree<IWindowTree> windowTree;
      IEditMode* editMode;
      GuiMetrics metrics;
      bool isActive{ false };
      AutoFree<IToolbar> toolbar;
      AutoFree<IStatusBar> statusbar;

      void Render(IGuiRenderContext& grc) override
      {
         if (!isActive) return;

         grc.Renderer().GetGuiMetrics(metrics);
         map.Render(grc, editMode->GetHilight());

         editMode->Render(grc);

         map.RenderTopGui(grc);

         toolbar->Render(grc, true, 4, RGBAb(224, 224, 224), RGBAb(128, 128, 128));

         GuiRect statusRect{ 0, metrics.screenSpan.y - 24, metrics.screenSpan.x, metrics.screenSpan.y };
         statusbar->Render(grc, statusRect);
      }

      IUIOverlay& Overlay() override
      {
         return *this;
      }

      virtual void Activate(bool isActive)
      {
         this->isActive = isActive;
      }

      void Free() override
      {
         delete this;
      }

      enum Mode
      {
         Mode_Vertex,
         Mode_Sector
      };

      void SetMode(Mode mode)
      {
         if (mode == Mode_Vertex)
         {
            editMode = &editMode_SectorBuilder.Mode();
            toolbar->SetToggleOn("vertices");
            toolbar->SetToggleOff("sectors");
         }
         else
         {
            editMode = &editMode_SectorEditor.Mode();
            toolbar->SetToggleOff("vertices");
            toolbar->SetToggleOn("sectors");
         }

         windowTree->Clear();
         windowTree->AddTarget(editMode, 0);
         windowTree->AddTarget(toolbar, 1);
      }

      void OnEvent(Event& ev) override
      {
         if (isActive)
         {
            if (ev == "editor.ui.vertices"_event)
            {
               SetMode(Mode_Vertex);
            }
            else if (ev == "editor.ui.sectors"_event)
            {
               SetMode(Mode_Sector);
            }
            RouteEventToUI(ev, metrics.cursorPosition, *windowTree);
         }
      }
   public:
      Editor(IPublisher& _publisher, HV::Entities::IInstancesSupervisor& _instances, IRenderer& renderer, Windows::IWindow& parent) :
         publisher(_publisher),
         instances(_instances),
         map(_instances),
         editMode_SectorBuilder(publisher, map),
         editMode_SectorEditor(publisher, map, parent),
         toolbar(Widgets::CreateToolbar(publisher, renderer)),
         windowTree(CreateWindowTree()),
         statusbar(CreateStatusBar(_publisher))
      {      
         publisher.Attach(this);
         toolbar->AddButton("load", "editor.ui.load"_event, "!textures/toolbars/load.tif");
         toolbar->AddButton("save", "editor.ui.save"_event, "!textures/toolbars/save.tif");
         toolbar->AddButton("vertices", "editor.ui.vertices"_event, "!textures/toolbars/builder.tif");
         toolbar->AddButton("sectors", "editor.ui.sectors"_event, "!textures/toolbars/sectors.tif");
         toolbar->SetToggleColours(RGBAb(255, 255, 255), RGBAb(128, 128, 128));
         SetMode(Mode_Vertex);
      }

      ~Editor()
      {
         publisher.Detach(this);
      }
   };
}

namespace HV
{
   IEditor* CreateEditor(IPublisher& publisher, HV::Entities::IInstancesSupervisor& instances, IRenderer& renderer, Windows::IWindow& parent)
   {
      return new Editor(publisher, instances, renderer, parent);
   }
}
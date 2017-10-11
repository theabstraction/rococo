#ifndef ROCOCO_HV
#define ROCOCO_HV

#include <rococo.mplat.h>

using namespace Rococo;
using namespace Rococo::Events;

namespace HV
{
   struct IPlayerSupervisor;
}

#include "hv.script.types.h"

#define NO_VTABLE __declspec(novtable)

#include "hv.sxh.h"

namespace HV
{
   struct ConfigText
   {
      const fstring key;
      const fstring value;

      operator const fstring() { return key; }
   };

   struct ConfigInt
   {
      const fstring key;
      int value;

      operator const fstring() { return key; }
   };

   struct ConfigFloat
   {
      const fstring key;
      float value;

      operator const fstring() { return key; }
   };

   struct ConfigBool
   {
      const fstring key;
      const boolean32 value;
      operator const fstring() { return key; }
   };

   namespace Events
   {
      namespace Player
      {
         struct OnPlayerActionEvent;
         struct OnPlayerViewChangeEvent;
      }
   }

   ROCOCOAPI IPlayerSupervisor
   {
      virtual void Free() = 0;
      virtual IPlayer* GetPlayer(int32 index) = 0;
   };

   IPlayerSupervisor* CreatePlayerSupervisor(Platform& platform);
  
   bool QueryYesNo(Windows::IWindow& ownerWindow, cstr message);

   ROCOCOAPI IEditor
   {
      virtual bool IsScrollLocked() const = 0;
      virtual void SetNeighbourTextureAt(Vec2 pos, bool forward) = 0;
      virtual void Free() = 0;
   };

   IEditor* CreateEditor(Platform& platform, IPlayerSupervisor& players);

   struct ObjectVertexBuffer
   {
      const ObjectVertex* v;
      const size_t VertexCount;
   };

   struct ISectors;
   struct ISector;

   struct Segment
   {
      int32 perimeterIndexStart;
      int32 perimeterIndexEnd;
   };

   struct SectorAndSegment
   {
      ISector* sector;
      Segment segment;
   };

   struct SectorPalette
   {
      cstr wallTextureName;
      cstr floorTextureName;
      cstr ceilingTextureName;
   };

   ROCOCOAPI IEditorState
   {
      virtual cstr TextureName(int index) const = 0;
   };

   ROCOCOAPI ISector
   {
      virtual void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) = 0;
      virtual bool DoesLineCrossSector(Vec2 a, Vec2 b) = 0;
      virtual ObjectVertexBuffer FloorVertices() const = 0;
      virtual void Free() = 0;
      virtual float Z0() const = 0;
      virtual float Z1() const = 0;  
      virtual Segment GetSegment(Vec2 p, Vec2 q) = 0;
      virtual int32 GetFloorTriangleIndexContainingPoint(Vec2 p) = 0;
      virtual RGBAb GetGuiColour(float intensity) const = 0;
      virtual int32 GetPerimeterIndex(Vec2 a) = 0;
      virtual void InvokeSectorDialog(Rococo::Windows::IWindow& parent, IEditorState& state) = 0;
      virtual const Vec2* WallVertices(size_t& nVertices) const = 0;
      virtual void Rebuild() = 0;
      virtual void RemoveWallSegment(const Segment& segment, const Vec2& a, const Vec2& b, float oppositeElevation, float oppositeHeight, ISector* other) = 0;
      virtual void SetPalette(const SectorPalette& palette) = 0;
      virtual cstr GetTexture(int32 state) const = 0;
      virtual void SetTexture(int32 state, cstr texture) = 0;
      virtual bool Is4PointRectangular() const = 0; // The sector has four points and its perimeter in 2D space is a rectangle or square
      virtual bool IsCorridor() const = 0; // The sector Is4PointRectangular & two opposing edges are portals to other sectors and neither is itself a 4PtRect
   };

   ISector* CreateSector(Platform& platform, ISectors& co_sectors);

   ROCOCOAPI ISectors
   {
      virtual void AddSector(const SectorPalette& palette, const Vec2* perimeter, size_t nVertices) = 0;
      virtual void Free() = 0;

      virtual void Delete(ISector* sector) = 0;

      virtual ISector* GetFirstSectorCrossingLine(Vec2 a, Vec2 b) = 0;
      virtual SectorAndSegment GetFirstSectorWithPoint(Vec2 a) = 0;
      virtual ISector* GetFirstSectorContainingPoint(Vec2 a) = 0;
      virtual ISector** begin() = 0;
      virtual ISector** end() = 0;
   };

   ISectors* CreateSectors(Platform& platform);

   ROCOCOAPI IGameMode
   {    
      virtual void Activate() = 0;
      virtual void Deactivate() = 0;
      virtual void UpdateAI(const IUltraClock& clock) = 0;
   };

   ROCOCOAPI IGameModeSupervisor: public IGameMode
   {
      virtual void Free() = 0;
   };

   struct Cosmos
   {
      Platform& platform;
      IPlayerSupervisor& players;
      IEditor& editor;
   };

   IGameModeSupervisor* CreateFPSGameLogic(Cosmos& e);

   IApp* CreateHVApp(Cosmos& e);
   void RunEnvironmentScript(Cosmos& e, cstr name);

   namespace Graphics
   {
      void DrawPointer(IGuiRenderContext& grc, Vec2i pos, Degrees heading, RGBAb shadowColour, RGBAb bodyColour);
   }
}


#endif
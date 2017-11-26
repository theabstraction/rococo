#ifndef ROCOCO_HV
#define ROCOCO_HV

#include <rococo.mplat.h>

using namespace Rococo;
using namespace Rococo::Events;

namespace HV
{
   struct IPlayerSupervisor;
   struct ISectors;

   struct IPlayerBase
   {
      virtual float& JumpSpeed() = 0;
      virtual float& DuckFactor() = 0;
      virtual float Height() const = 0;
   };
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
      virtual void Free() = 0;
   };

   struct ISectors;
   struct ISector;

   IEditor* CreateEditor(Platform& platform, IPlayerSupervisor& players, ISectors& sectors);

   struct ObjectVertexBuffer
   {
      const ObjectVertex* v;
      const size_t VertexCount;
   };

   struct Segment
   {
      int32 perimeterIndexStart;
      int32 perimeterIndexEnd;
   };

   struct Gap
   {
      Vec2 a;
      Vec2 b;
      float z0;
      float z1;
      ISector* other;
	  Sphere bounds;
	  mutable int64 iterationFrame;
   };

   struct SectorAndSegment
   {
      ISector* sector;
      Segment segment;
   };

   struct IPropertyHost;

   ROCOCOAPI IPropertyTarget
   {
	   virtual void Assign(IPropertyHost* host) = 0; // N.B a property target must never by Next to itself
	   virtual void GetProperties(cstr category, IBloodyPropertySetEditor& editor) = 0;
	   virtual void NotifyChanged() = 0;
   };

   ROCOCOAPI IPropertyHost
   {
	   virtual void SetPropertyTarget(IPropertyTarget* target) = 0;
	   virtual void SetPropertyTargetToSuccessor() = 0;
   };

   ROCOCOAPI IEditorState: public IPropertyHost
   {

      virtual cstr TextureName(int index) const = 0;
   };

   struct Barrier
   {
	   Vec2 p; // barrier LHS
	   Vec2 q; // barrier RHS 
	   float z0; // Bottom level above ground
	   float z1; // Top level above ground. z1 > z0
   };

   struct VisibleSector
   {
	   ISector& sector;
   };

   struct Material
   {
	   MaterialVertexData mvd;
	   char persistentName[IO::MAX_PATHLEN];
	   Rococo::Graphics::MaterialCategory category;
   };

   struct MaterialArgs
   {
	   Material* mat;
	   BodyComponentMatClass bcmc;
   };

   ROCOCOAPI MatEnumerator
   {
	   virtual void Enumerate(IEventCallback<MaterialArgs>& cb) = 0;
   };

   struct ISectorLayout;

   ROCOCOAPI ISector: public IPropertyTarget
   {
	  virtual const AABB2d& GetAABB() const = 0;
	  virtual uint32 Id() const = 0;

	  // Iteration frames are used by some iteration functions to mark sectors as having been enumrerated
	  // Generally the frame count is incremented each function call
	  // 0x81000000000 to 0x82000000000 are from calls to ForEverySectorVisibleAt
	  virtual int64 IterationFrame() const = 0;
	  virtual void SetIterationFrame(int64 value) = 0;

	  virtual const Gap* GetGapAtSegment(const Vec2& a, const Vec2& b) const = 0;

	  virtual const Barrier* Barriers(size_t& barrierCount) const = 0;
      virtual void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) = 0;
      virtual bool DoesLineCrossSector(Vec2 a, Vec2 b) = 0;
      virtual ObjectVertexBuffer FloorVertices() const = 0;
      virtual const Gap* Gaps(size_t& count) const = 0;

	  virtual void Decouple() = 0; // Called to take the sector out of the world, prior to deletion
      virtual void Free() = 0;
      virtual float Z0() const = 0;
      virtual float Z1() const = 0;  
      virtual Segment GetSegment(Vec2 p, Vec2 q) = 0;
      virtual int32 GetFloorTriangleIndexContainingPoint(Vec2 p) = 0;
      virtual RGBAb GetGuiColour(float intensity) const = 0;
      virtual int32 GetPerimeterIndex(Vec2 a) const = 0;
      virtual void InvokeSectorRebuild(bool force) = 0;
      virtual const Vec2* WallVertices(size_t& nVertices) const = 0;
      virtual void Rebuild() = 0;
      virtual bool Is4PointRectangular() const = 0; // The sector has four points and its perimeter in 2D space is a rectangle or square
      virtual bool IsCorridor() const = 0; // The sector Is4PointRectangular & two opposing edges are portals to other sectors and neither is itself a 4PtRect
      virtual const Segment* GetWallSegments(size_t& count) const = 0;
	  virtual void OnSectorScriptChanged(const FileModifiedArgs& args) = 0;

	  virtual void ForEveryObjectInSector(IEventCallback<const ID_ENTITY>& cb) = 0;

	  virtual void SaveTemplate(StringBuilder& sb) = 0;
	  virtual void SetTemplate(MatEnumerator& enumerator) = 0;

	  virtual const LightSpec* Lights(size_t& numberOfLights) const = 0;

	  virtual void SyncEnvironmentMapToSector() = 0;

	  virtual ISectorLayout* Layout() = 0;
   };

   float GetHeightAtPointInSector(cr_vec3 p, ISector& sector);

   ISector* CreateSector(Platform& platform, ISectors& co_sectors);

   ROCOCOAPI ISectors
   {
	  virtual ISectorBuilder* Builder() = 0;
	  virtual void Free() = 0;

	  virtual void AddSector(const Vec2* perimeter, size_t nVertices) = 0;
	  virtual void Delete(ISector* sector) = 0;

	  virtual ISector* GetFirstSectorCrossingLine(Vec2 a, Vec2 b) = 0;
	  virtual SectorAndSegment GetFirstSectorWithVertex(Vec2 a) = 0;
	  virtual ISector* GetFirstSectorContainingPoint(Vec2 a) = 0;
	  virtual ISector** begin() = 0;
	  virtual ISector** end() = 0;

	  virtual void OnSectorScriptChanged(const FileModifiedArgs& args) = 0;
	  virtual size_t ForEverySectorVisibleBy(cr_m4x4 worldToScreen, cr_vec3 eye, cr_vec3 forward, IEventCallback<VisibleSector>& cb) = 0;
	  virtual void ResetConfig() = 0;

	  virtual void SaveAsFunction(StringBuilder& sb) = 0;

	  virtual cstr GetTemplateDoorScript(bool& hasDoor) const = 0;
	  virtual cstr GetTemplateWallScript(bool& usesScript) const = 0;

	  virtual void BindProperties(IBloodyPropertySetEditor& editor) = 0;
	  virtual void NotifyChanged() = 0;
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
      ISectors& sectors;
   };

   IGameModeSupervisor* CreateFPSGameLogic(Cosmos& e);

   IApp* CreateHVApp(Cosmos& e);
   void RunEnvironmentScript(Cosmos& e, cstr name, bool releaseSource = false);

   namespace GraphicsEx
   {
      void DrawPointer(IGuiRenderContext& grc, Vec2i pos, Degrees heading, RGBAb shadowColour, RGBAb bodyColour);

	  extern BodyComponentMatClass BodyComponentMatClass_Physics_Hull;
	  extern BodyComponentMatClass BodyComponentMatClass_Brickwork;
	  extern BodyComponentMatClass BodyComponentMatClass_Cement;
	  extern BodyComponentMatClass BodyComponentMatClass_Floor;
	  extern BodyComponentMatClass BodyComponentMatClass_Ceiling;
	  extern BodyComponentMatClass BodyComponentMatClass_Door_Mullions;
	  extern BodyComponentMatClass BodyComponentMatClass_Door_Panels;
	  extern BodyComponentMatClass BodyComponentMatClass_Door_Casing;
	  extern BodyComponentMatClass BodyComponentMatClass_Door_Rails;
   }
}


#endif
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

   typedef cstr VisitorName;

   ROCOCOAPI IMathsVisitor
   {
      virtual void Clear() = 0;
      virtual void Show(VisitorName name, const Matrix4x4& m) = 0;
      virtual void ShowRow(VisitorName name, const float* vector, const size_t nComponents) = 0;
      virtual void ShowColumn(VisitorName name, const float* vector, const size_t nComponents) = 0;
      virtual void ShowDecimal(VisitorName name, const int32 value) = 0;
      virtual void Show(VisitorName name, const float value) = 0;
      virtual void ShowHex(VisitorName name, const int32 value) = 0;
      virtual void ShowBool(VisitorName name, const bool value) = 0;
      virtual void ShowDecimal(VisitorName name, const int64 value) = 0;
      virtual void ShowHex(VisitorName name, const int64 value) = 0;
      virtual void ShowPointer(VisitorName name, const void* ptr) = 0;
      virtual void ShowString(VisitorName name, cstr format, ...) = 0;
   };

   ROCOCOAPI IMathsVenue
   {
      virtual void ShowVenue(IMathsVisitor& visitor) = 0;
   };

   ROCOCOAPI IMathsVisitorSupervisor: public IMathsVisitor
   {
      virtual IUIOverlay& Overlay() = 0;
      virtual void Free() = 0;
   };

   IMathsVisitorSupervisor* CreateMathsVisitor();

   namespace Events
   {
      namespace Player
      {
         struct OnPlayerActionEvent;
         struct OnPlayerViewChangeEvent;
      }
   }

   namespace Graphics
   {
      ROCOCOAPI ISceneBuilderSupervisor: public ISceneBuilder
      {
         virtual void Free() = 0;
      };

      ROCOCOAPI ICameraSupervisor : public ICamera
      {
         virtual void Append(Events::Player::OnPlayerViewChangeEvent& pvce) = 0;
         virtual void Free() = 0;
         virtual void Update(const IUltraClock& clock) = 0;
         virtual IMathsVenue& Venue() = 0;
      };

      ROCOCOAPI ISceneSupervisor : public IScene
      {
         virtual void Free() = 0;
         virtual ISceneBuilderSupervisor&  Builder() = 0;
      };

      ROCOCOAPI ISpriteSupervisor : public ISprites
      {
         virtual void Free() = 0;
      };

      ISpriteSupervisor* CreateSpriteSupervisor(IRenderer & renderer);

      ISceneSupervisor* CreateScene(Rococo::Entities::IInstancesSupervisor& instances, ICameraSupervisor& camera, Platform& platform);
      ICameraSupervisor* CreateCamera(Rococo::Entities::IInstancesSupervisor& instances, Entities::IMobiles& mobiles, IRenderer& render);
   }

   ROCOCOAPI IPlayerSupervisor
   {
      virtual void Free() = 0;
      virtual IPlayer* GetPlayer(int32 index) = 0;
   };

   IPlayerSupervisor* CreatePlayerSupervisor(Platform& platform);

   struct Key
   {
      cstr KeyName;
      bool isPressed;
   };

   ROCOCOAPI IKeyboardSupervisor: public IKeyboard
   {
      virtual cstr GetAction(cstr keyName) = 0;
      virtual Key GetKeyFromEvent(const KeyboardEvent& ke) = 0;
      virtual void Free() = 0;
   };

   IKeyboardSupervisor* CreateKeyboardSupervisor();

   ROCOCOAPI IConfigSupervisor: public IConfig
   {
      virtual cstr GetText(cstr name) const = 0;
      virtual void Free() = 0;
   };

   IConfigSupervisor* CreateConfig();

   namespace Events
   {
      namespace Entities
      {
         struct OnTryMoveMobileEvent;
      }
   }

   ROCOCOAPI IMobilesSupervisor: public Entities::IMobiles
   {
      virtual void Append(Events::Entities::OnTryMoveMobileEvent& tmm) = 0;
      virtual void Free() = 0;
   };

   IMobilesSupervisor* CreateMobilesSupervisor(Rococo::Entities::IInstancesSupervisor& instances, IPublisher& publisher);
  
   bool QueryYesNo(Windows::IWindow& ownerWindow, cstr message);

   ROCOCOAPI IEditor
   {
      virtual void Free() = 0;
   };

   IEditor* CreateEditor(Platform& platform);

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

   ROCOCOAPI ISector
   {
      virtual void Build(const Vec2* positionArray, size_t nVertices, float z0, float z1) = 0;
      virtual bool DoesLineCrossSector(Vec2 a, Vec2 b) = 0;
      virtual ObjectVertexBuffer FloorVertices() const = 0;
      virtual void Free() = 0;
     
      virtual Segment GetSegment(Vec2 p, Vec2 q) = 0;
      virtual int32 GetFloorTriangleIndexContainingPoint(Vec2 p) = 0;
      virtual RGBAb GetGuiColour(float intensity) const = 0;
      virtual int32 GetPerimeterIndex(Vec2 a) = 0;
      virtual void InvokeSectorDialog(Rococo::Windows::IWindow& parent) = 0; 
      virtual const Vec2* WallVertices(size_t& nVertices) const = 0;
      virtual void RemoveWallSegment(Segment segment) = 0;
   };

   ISector* CreateSector(Rococo::Entities::IInstancesSupervisor&  instances, ISectors& co_sectors);

   ROCOCOAPI ISectors
   {
      virtual void AddSector(const Vec2* perimeter, size_t nVertices) = 0;
      virtual void Free() = 0;

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
      virtual void Append(Events::Player::OnPlayerActionEvent& ev) = 0;
      virtual void UpdateAI(const IUltraClock& clock) = 0;
   };

   ROCOCOAPI IGameModeSupervisor: public IGameMode
   {
      virtual void Free() = 0;
   };

   struct Cosmos
   {
      Platform& platform;
      IConfigSupervisor& config;
      Graphics::ISceneSupervisor& scene;
      IMobilesSupervisor& mobiles;
      Graphics::ICameraSupervisor& camera;
      Graphics::ISpriteSupervisor& sprites;
      IPlayerSupervisor& players;
      IKeyboardSupervisor& keyboard;
      IMathsVisitorSupervisor& mathsDebugger;
      IEditor& editor;
   };

   IGameModeSupervisor* CreateFPSGameLogic(Cosmos& e);

   IApp* CreateHVApp(Cosmos& e);
   void RunEnvironmentScript(Cosmos& e, cstr name);
}


#endif
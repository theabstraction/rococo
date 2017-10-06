#include "hv.h"
#include "hv.events.h"
#include <unordered_map>

using namespace HV;
using namespace HV::Events;
using namespace HV::Events::Player;

using namespace Rococo::Entities;

class FPSControl
{
private:
   void OnForward(bool start)
   {
      isMovingForward = start;
      isAutoRun = false;
   }

   void OnBackward(bool start)
   {
      isMovingBackward = start;
      isAutoRun = false;
   }


   void OnStraffeLeft(bool start)
   {
      isMovingLeft = start;
   }


   void OnStraffeRight(bool start)
   {
      isMovingRight = start;
   }

   void OnJump(bool start)
   {

   }

   void OnAutoRun(bool start)
   {
      isAutoRun = true;
   }

public:
   typedef void (FPSControl::*ACTION_FUNCTION)(bool start);
   static std::unordered_map<std::string, ACTION_FUNCTION> nameToAction;

   bool isMovingForward{ false };
   bool isMovingBackward{ false };
   bool isMovingLeft{ false };
   bool isMovingRight{ false };
   bool isAutoRun{ false };

   float headingDelta{ 0 };
   float elevationDelta{ 0 };

   Vec3 speeds{ 0,0,0 };

   FPSControl()
   {
   }

   void Clear()
   {
      isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
      headingDelta = elevationDelta = 0;
   }

   void Append(OnPlayerActionEvent& ev)
   {
      auto i = nameToAction.find(ev.Name);
      if (i != nameToAction.end())
      {
         ((*this).*(i->second))(ev.start);
      }
   }

   void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
   {
      headingDelta += 0.25f * delta.x;
      elevationDelta -= delta.y;
   }

   void GetPlayerDelta(ID_ENTITY playerId, float dt, Entities::MoveMobileArgs& mm, Degrees& viewElevationDelta)
   {
      float forwardDelta = 0;
      if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
      if (isMovingBackward) forwardDelta -= 1.0f;

      float straffeDelta = 0;
      if (isMovingLeft) straffeDelta -= 1.0f;
      if (isMovingRight) straffeDelta += 1.0f;

      if (forwardDelta != 0 || straffeDelta != 0 || headingDelta != 0)
      {
         mm.fowardDelta = dt * ((forwardDelta > 0) ? forwardDelta * speeds.x : forwardDelta * speeds.z);
         mm.straffeDelta = dt * straffeDelta * speeds.y;
         mm.entityId = playerId;
         mm.delta = { Degrees{ headingDelta }, Degrees{ 0 }, Degrees{ 0 } };
         headingDelta = 0;
      }

      viewElevationDelta = Degrees{ -elevationDelta };
      elevationDelta = 0;
   }
};

std::unordered_map<std::string, FPSControl::ACTION_FUNCTION> FPSControl::nameToAction =
{
   { "move.fps.forward",         &FPSControl::OnForward },
   { "move.fps.backward",        &FPSControl::OnBackward },
   { "move.fps.straffeleft",     &FPSControl::OnStraffeLeft },
   { "move.fps.strafferight",    &FPSControl::OnStraffeRight },
   { "move.fps.jump",            &FPSControl::OnJump },
   { "move.fps.autorun",         &FPSControl::OnAutoRun }
};

struct FPSGameLogic : public IGameModeSupervisor, public IUIElement
{
   Cosmos& e;

   FPSControl fpsControl;

   FPSGameLogic(Cosmos& _e): e(_e)
   {
      fpsControl.speeds = Vec3{ 10.0f, 5.0f, 5.0f };
   }

   ~FPSGameLogic()
   {
      e.platform.gui.UnregisterPopulator(this);
   }

   void Activate()
   {
      fpsControl.Clear();
   }

   void Deactivate()
   {
      fpsControl.Clear();
   }

   void Append(OnPlayerActionEvent& ev) override
   {
      fpsControl.Append(ev);
   }

   void Free() override
   {
      delete this;
   }

   void PopulateScene()
   {
      e.platform.scene.Builder().Clear();

      struct ANON : public IEntityCallback
      {
         Rococo::Graphics::ISceneBuilderSupervisor& builder;

         virtual void OnEntity(int64 index, IEntity& entity, ID_ENTITY id)
         {
            builder.AddStatics(id);
         }

         ANON(Rococo::Graphics::ISceneBuilderSupervisor& _builder) : builder(_builder) {}
      } addToScene(e.platform.scene.Builder());

      e.platform.instances.ForAll(addToScene);
   }

   void UpdateAI(const IUltraClock& clock) override
   {
      e.platform.gui.RegisterPopulator("fps", this);

      auto* player = e.players.GetPlayer(0);

      float dt = clock.DT();

      Degrees viewElevationDelta;

      Entities::MoveMobileArgs mm;
      fpsControl.GetPlayerDelta(player->GetPlayerEntity(), dt, mm, viewElevationDelta);

      e.platform.mobiles.TryMoveMobile(mm);

      if (viewElevationDelta != 0)
      {
         e.platform.camera.ElevateView(player->GetPlayerEntity(), viewElevationDelta);
      }

      PopulateScene();
   }

   void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
   {
      auto* player = e.players.GetPlayer(0);
      fpsControl.OnMouseMove(cursorPos, delta, dWheel);
   }

   void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
   {

   }

   void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
   {

   }

   void Render(IGuiRenderContext& rc, const GuiRect& absRect) override
   {

   }
};

namespace HV
{
   IGameModeSupervisor* CreateFPSGameLogic(Cosmos& e)
   {
      return new FPSGameLogic(e);
   }
}
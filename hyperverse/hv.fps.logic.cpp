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

   bool Append(OnPlayerActionEvent& ev)
   {
      auto i = nameToAction.find(ev.Name);
      if (i != nameToAction.end())
      {
         ((*this).*(i->second))(ev.start);
         return true;
      }
      else
      {
         return false;
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

   bool Append(OnPlayerActionEvent& ev)
   {
      return fpsControl.Append(ev);
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

   struct Triangle
   {
      Vec3 a;
      Vec3 b;
      Vec3 c;
   };

   Vec3 ComputeCollisionAgainstTriangle(cr_vec3 start, cr_vec3 end, ID_ENTITY id, float dt, ISector& sector)
   {
   }

   struct CollisionParameters
   {
      Vec3 start;
      Vec3 end;
      ID_ENTITY playerId;
      float dt;
      float jumpSpeed;
   };

   bool GetTriangleHeight(const Triangle& t, cr_vec2 P, float& result)
   {
      // Triangle is in a plane (P - A).N = 0 where A is a point in plane, N is normal and P is any other point in plane
      // Expands to P.N = A.N: 
      // Px.Nx + Py.Ny + Pz.Nz = A.N
      // Pz = [A.N - (Px.Nx + Py.Ny)] / Nz

      Vec3 N = Cross(t.b - t.a, t.c - t.b);

      if (fabs(N.z) <= 0.001f) return false;

      result = (Dot(t.a, N) - (P.x * N.x + P.y * N.y)) / N.z;
      return true;
   }

   Vec3 ComputeCollisionInSector(CollisionParameters& cp, ISector& sector)
   {
      int32 index = sector.GetFloorTriangleIndexContainingPoint({ cp.end.x, cp.end.y });
      if (index >= 0)
      {
         auto* v = sector.FloorVertices().v;
         Triangle t;
         t.a = v[3 * index].position;
         t.b = v[3 * index + 1].position;
         t.c = v[3 * index + 2].position;

         float h;   
         if (GetTriangleHeight(t, { cp.end.x, cp.end.y }, h))
         {
            h += 1.65f; // Player's eye level

            if (cp.end.z < h)
            {
               cp.end.z = h;
               cp.jumpSpeed = 0.0f;
            }
            else if (cp.end.z > h )
            {
               Vec3 g = { 0, 0, -9.81f };
               Vec3 afterGravity = cp.end + (0.5f * cp.dt * cp.dt * g) + (cp.dt * Vec3{ 0, 0, cp.jumpSpeed });

               if (afterGravity.z < h)
               {
                  cp.end.z = h;
                  cp.jumpSpeed = 0.0f;
               }
               else
               {
                  cp.jumpSpeed += g.z * cp.dt;
                  cp.end = afterGravity;
               }
            }
         }
      }
     
      return cp.end;
   }

   Vec3 CorrectPosition(CollisionParameters& cp)
   {
      auto* toSector = e.sectors.GetFirstSectorContainingPoint({ cp.end.x, cp.end.y });
      if (!toSector) return cp.end;
      return ComputeCollisionInSector(cp, *toSector);
   }

   void UpdatePlayer(float dt)
   {
      auto* player = e.players.GetPlayer(0);
      auto id = player->GetPlayerEntity();

      Degrees viewElevationDelta;

      Entities::MoveMobileArgs mm;
      fpsControl.GetPlayerDelta(id, dt, mm, viewElevationDelta);

      auto pe = e.platform.instances.GetEntity(id);

      Vec3 before = pe->Position();

      e.platform.mobiles.TryMoveMobile(mm);

      Vec3 after = pe->Position();

      CollisionParameters cp{ before, after, id, dt, player->JumpSpeed() };
      Vec3 final = CorrectPosition(cp);

      player->JumpSpeed() = cp.jumpSpeed;

      pe->Model().SetPosition(final);

      if (viewElevationDelta != 0)
      {
         e.platform.camera.ElevateView(id, viewElevationDelta);
      }

      PopulateScene();

      Matrix4x4 m;
      e.platform.camera.GetWorld(m);
      Vec3 dir{ -m.row2.x, -m.row2.y, -m.row2.z };
      e.platform.scene.Builder().SetLight(dir, final, 0);
   }

   void UpdateAI(const IUltraClock& clock) override
   {
      e.platform.gui.RegisterPopulator("fps", this);
      UpdatePlayer(clock.DT());
   }

   bool OnKeyboardEvent(const KeyboardEvent& k)
   {
      Key key = e.platform.keyboard.GetKeyFromEvent(k);
      auto* action = e.platform.keyboard.GetAction(key.KeyName);
      if (action)
      {
          HV::Events::Player::OnPlayerActionEvent pae;
          pae.Name = action;
          pae.start = key.isPressed;
          return Append(pae);
      }

      return false;
   }

   void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
   {
      auto* player = e.players.GetPlayer(0);

      if (dWheel != 0)
      {
         if (e.editor.IsScrollLocked())
         {
            auto id = player->GetPlayerEntity();
            auto pos = e.platform.instances.GetEntity(id)->Position();
            e.editor.SetNeighbourTextureAt({ pos.x, pos.y }, dWheel > 0);
         }
      }

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
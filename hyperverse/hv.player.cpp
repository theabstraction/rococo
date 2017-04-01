#include "hv.events.h"
#include <unordered_map>

namespace
{
   using namespace HV;
   using namespace HV::Events;
   using namespace Rococo::Events;

   ROCOCOAPI IControlMethod
   {
      virtual void Clear() = 0;
      virtual void Free() = 0;
      virtual void OnEvent(Event& ev) = 0;
      virtual void Update(ID_ENTITY playerId, const IUltraClock& clock, IPublisher& publisher) = 0;
   };

   struct NullControl : public IControlMethod
   {
      virtual void Clear()
      {

      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnEvent(Event& ev)
      {

      }

      virtual void Update(ID_ENTITY playerId, const IUltraClock& clock, IPublisher& publisher)
      {
      }
   };

   struct FourWayScroller : public IControlMethod
   {
   public:
      typedef void (FourWayScroller::*ACTION_FUNCTION)(bool start);
      static std::unordered_map<std::wstring, ACTION_FUNCTION> nameToAction;

      bool isMovingForward{ false };
      bool isMovingBackward{ false };
      bool isMovingLeft{ false };
      bool isMovingRight{ false };
      bool isAutoRun{ false };

      Vec3 speeds;

      FourWayScroller(cr_vec3 _speeds) :
         speeds(_speeds)
      {
      }

      virtual void Clear() override
      {
         isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnEvent(Event& ev)
      {
         if (ev == HV::Events::Player::OnPlayerAction)
         {
            auto& pae = As<HV::Events::Player::OnPlayerActionEvent>(ev);

            auto i = nameToAction.find(pae.Name);
            if (i != nameToAction.end())
            {
               ((*this).*(i->second))(pae.start);
            }
         }
      }

      virtual void Update(ID_ENTITY playerId, const IUltraClock& clock, IPublisher& publisher)
      {
         HV::Events::Entities::OnTryMoveMobileEvent tmm;

         float forwardDelta = 0;
         if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
         if (isMovingBackward) forwardDelta -= 1.0f;

         float straffeDelta = 0;
         if (isMovingLeft) straffeDelta -= 1.0f;
         if (isMovingRight) straffeDelta += 1.0f;

         if (forwardDelta != 0 || straffeDelta != 0)
         {
            tmm.fowardDelta = clock.DT() * ((forwardDelta > 0) ? forwardDelta * speeds.x : forwardDelta * speeds.z);
            tmm.straffeDelta = clock.DT() * straffeDelta * speeds.y;
            tmm.entityId = playerId;
            tmm.delta = { 0,0,0 };
            Rococo::Events::Publish(publisher, tmm);
         }
      }

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
   };

   std::unordered_map<std::wstring, FourWayScroller::ACTION_FUNCTION> FourWayScroller::nameToAction =
   {
      { L"move.fps.forward",         &FourWayScroller::OnForward },
      { L"move.fps.backward",        &FourWayScroller::OnBackward },
      { L"move.fps.straffeleft",     &FourWayScroller::OnStraffeLeft },
      { L"move.fps.strafferight",    &FourWayScroller::OnStraffeRight },
      { L"move.fps.jump",            &FourWayScroller::OnJump },
      { L"move.fps.autorun",         &FourWayScroller::OnAutoRun }
   };

   struct FPSControl: public IControlMethod
   {
   public:
      typedef void (FPSControl::*ACTION_FUNCTION)(bool start);
      static std::unordered_map<std::wstring, ACTION_FUNCTION> nameToAction;
 
      bool isMovingForward{ false };
      bool isMovingBackward{ false };
      bool isMovingLeft{ false };
      bool isMovingRight{ false };
      bool isAutoRun{ false };

      float headingDelta{ 0 };
      float elevationDelta{ 0 };

      Vec3 speeds;

      FPSControl(cr_vec3 _speeds):
         speeds(_speeds)
      {
      }

      virtual void Clear() override
      {
         isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
         headingDelta = elevationDelta = 0;
      }

      virtual void Free()
      {
         delete this;
      }

      virtual void OnEvent(Event& ev)
      {
         if (ev == HV::Events::Player::OnPlayerAction)
         {
            auto& pae = As<HV::Events::Player::OnPlayerActionEvent>(ev);

            auto i = nameToAction.find(pae.Name);
            if (i != nameToAction.end())
            {
               ((*this).*(i->second))(pae.start);
            }
         }
         else if (ev == Input::OnMouseMoveRelative)
         {
            auto& mmr = As<Input::OnMouseMoveRelativeEvent>(ev);
            headingDelta += 0.25f * mmr.dx;
            elevationDelta -= mmr.dy;
         }
      }

      virtual void Update(ID_ENTITY playerId, const IUltraClock& clock, IPublisher& publisher)
      {
         HV::Events::Entities::OnTryMoveMobileEvent tmm;

         float forwardDelta = 0;
         if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
         if (isMovingBackward) forwardDelta -= 1.0f;

         float straffeDelta = 0;
         if (isMovingLeft) straffeDelta -= 1.0f;
         if (isMovingRight) straffeDelta += 1.0f;

         if (forwardDelta != 0 || straffeDelta != 0 || headingDelta != 0)
         {
            tmm.fowardDelta = clock.DT() * ((forwardDelta > 0) ? forwardDelta * speeds.x : forwardDelta * speeds.z);
            tmm.straffeDelta = clock.DT() * straffeDelta * speeds.y;
            tmm.entityId = playerId;
            tmm.delta = { Degrees { headingDelta }, Degrees { 0 }, Degrees { 0 } };
            Rococo::Events::Publish(publisher, tmm);
            headingDelta = 0;
         }

         if (elevationDelta)
         {
            HV::Events::Player::OnPlayerViewChangeEvent pvce;
            pvce.playerEntityId = playerId;
            pvce.elevationDelta = elevationDelta;
            Rococo::Events::Publish(publisher, pvce);
            elevationDelta = 0;
         }
      }
     
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
   };

   std::unordered_map<std::wstring, FPSControl::ACTION_FUNCTION> FPSControl::nameToAction =
   {
      { L"move.fps.forward",         &FPSControl::OnForward },
      { L"move.fps.backward",        &FPSControl::OnBackward },
      { L"move.fps.straffeleft",     &FPSControl::OnStraffeLeft },
      { L"move.fps.strafferight",    &FPSControl::OnStraffeRight },
      { L"move.fps.jump",            &FPSControl::OnJump },
      { L"move.fps.autorun",         &FPSControl::OnAutoRun }
   };

   class Player : public IPlayer, public IObserver
   {
      ID_ENTITY playerId;
      AutoFree<IControlMethod> control;

      Vec3 speeds{ 10.0f, 10.0f, 5.0f };
   public:
      Player() : control{ new NullControl }
      {

      }

      virtual void SetSpeed(float forward, float backward, float straffe)
      {
         speeds.x = forward;
         speeds.y = straffe;
         speeds.z = backward;
      }

      virtual void Clear()
      {
      }

      virtual void OnEvent(Event& ev)
      {
         control->OnEvent(ev);
      }

      virtual void Update(const IUltraClock& clock, IPublisher& publisher)
      {
         control->Update(playerId, clock, publisher);
      }

      virtual void SetPlayerEntity(ID_ENTITY id)
      {
         playerId = id;
      }

      virtual ID_ENTITY GetPlayerEntity()
      {
         return playerId;
      }

      virtual void SetControl4WayScroller()
      {
         control = new FourWayScroller(speeds);
      }

      virtual void SetControlFPS()
      {
         control = new FPSControl(speeds);
      }

      virtual void SetControlNone()
      {
         control = new NullControl();
      }
   };

   class PlayerSupervisor : public IPlayerSupervisor, public IObserver
   {
      ID_ENTITY playerId;
      Rococo::Events::IPublisher& publisher;

      Player player;
   public:
      PlayerSupervisor(Rococo::Events::IPublisher& _publisher):
         publisher(_publisher)
      {
         publisher.Attach(this);
      }

      ~PlayerSupervisor()
      {
         publisher.Detach(this);
      }

      virtual void OnEvent(Event& ev)
      {
         player.OnEvent(ev);
      }

      virtual void Update(const IUltraClock& clock)
      {
         player.Update(clock, publisher);
      }

      virtual IPlayer* GetPlayer(int32 index)
      {
         if (index != 0)
         {
            Rococo::Throw(0, L"Bad index. Only player #0 is recognized");
         }

         return &player;
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace HV
{
   IPlayerSupervisor* CreatePlayerSupervisor(Rococo::Events::IPublisher& publisher)
   {
      return new PlayerSupervisor(publisher);
   }
}

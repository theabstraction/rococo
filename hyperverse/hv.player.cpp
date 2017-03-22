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

      int32 headingDelta{ 0 };
      int32 elevationDelta{ 0 };

      FPSControl()
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
            headingDelta += mmr.dx;
            elevationDelta += mmr.dy;
         }
      }

      virtual void Update(ID_ENTITY playerId, const IUltraClock& clock, IPublisher& publisher)
      {
         HV::Events::Player::OnPlayerTryMoveEvent ptme;

         float forwardDelta = 0;
         if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
         if (isMovingBackward) forwardDelta -= 1.0f;

         float straffeDelta = 0;
         if (isMovingLeft) straffeDelta -= 1.0f;
         if (isMovingRight) straffeDelta += 1.0f;

         if (forwardDelta != 0 || straffeDelta != 0 || headingDelta != 0)
         {
            ptme.fowardDelta = clock.DT() * forwardDelta;
            ptme.straffeDelta = clock.DT() * straffeDelta;
            ptme.playerEntityId = playerId;
            ptme.headingDelta = headingDelta;

            Rococo::Events::Publish(publisher, ptme);
            headingDelta = 0;
         }

         if (elevationDelta)
         {
            HV::Events::Player::OnPlayerViewChangeEvent pvce;
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
   public:
      Player() : control{ new NullControl }
      {

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

      virtual void SetControlFPS()
      {
         control = new FPSControl();
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

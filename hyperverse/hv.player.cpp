#include "hv.h"
#include <unordered_map>

namespace
{
   using namespace HV;

   class Player;
   typedef void (Player::*ACTION_FUNCTION)(bool start);

   class Player : public IPlayer, public IObserver
   {
      ID_ENTITY playerId;
      bool fpsControl{ false };

      std::unordered_map<std::wstring, ACTION_FUNCTION> nameToAction
      {
         {L"move.fps.forward",         &Player::OnForward},
         {L"move.fps.backward",        &Player::OnBackward},
         {L"move.fps.straffeleft",     &Player::OnStraffeLeft },
         {L"move.fps.strafferight",    &Player::OnStraffeRight},
         {L"move.fps.jump",            &Player::OnJump },
         {L"move.fps.autorun",         &Player::OnAutoRun }
      };
    
      bool isMovingForward{ false };
      void OnForward(bool start)
      {
         isMovingForward = start;
         isAutoRun = false;
      }

      bool isMovingBackward{ false };
      void OnBackward(bool start)
      {
         isMovingBackward = start;
         isAutoRun = false;
      }

      bool isMovingLeft{ false };
      void OnStraffeLeft(bool start)
      {
         isMovingLeft = start;
      }

      bool isMovingRight{ false };
      void OnStraffeRight(bool start)
      {
         isMovingRight = start;
      }

      void OnJump(bool start)
      {

      }

      bool isAutoRun{ false };
      void OnAutoRun(bool start)
      {
         isAutoRun = true;
      }
   public:
      virtual void Clear()
      {
         isMovingBackward = isMovingForward = isMovingLeft = isMovingRight = false;
      }

      virtual void OnEvent(Event& ev)
      {
         if (ev == HV::Events::OnPlayerAction)
         {
            auto& pae = Rococo::Events::As<HV::Events::OnPlayerActionEvent>(ev);
            if (fpsControl)
            {
               auto i = nameToAction.find(pae.Name);
               if (i != nameToAction.end())
               {
                  ((*this).*(i->second))(pae.start);
               }
            }
         }
      }

      virtual void Update(const IUltraClock& clock, IPublisher& publisher)
      {
         if (fpsControl)
         {
            HV::Events::OnPlayerTryMoveEvent ptme;

            float forwardDelta = 0;
            if (isMovingForward || isAutoRun) forwardDelta += 1.0f;
            if (isMovingBackward) forwardDelta -= 1.0f;

            float straffeDelta = 0;
            if (isMovingLeft) straffeDelta -= 1.0f;
            if (isMovingRight) straffeDelta += 1.0f;
               
            if (forwardDelta != 0 || straffeDelta != 0)
            {
               ptme.fowardDelta = clock.DT() * forwardDelta;
               ptme.straffeDelta = clock.DT() * straffeDelta;
               ptme.playerEntityId = playerId;

               Rococo::Events::Publish(publisher, ptme);
            }        
         }
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
         fpsControl = true;
         Clear();
      }

      virtual void SetControlNone()
      {
         fpsControl = false;
         Clear();
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

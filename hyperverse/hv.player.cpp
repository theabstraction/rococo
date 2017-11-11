#include "hv.events.h"
#include <unordered_map>

namespace
{
   using namespace HV;
   using namespace HV::Events;
   using namespace Rococo::Events;


   class Player : public IPlayer
   {
      ID_ENTITY playerId;
      float jumpSpeed = 0;
      const float height = 1.65_metres;
      float duckFactor = 1.0f;
   public:
      Player()
      {

      }

      virtual void Clear()
      {
      }


      virtual void SetPlayerEntity(ID_ENTITY id)
      {
         playerId = id;
      }

      virtual ID_ENTITY GetPlayerEntity()
      {
         return playerId;
      }

      virtual float& JumpSpeed()
      {
         return jumpSpeed;
      }

      virtual float Height() const
      {
         return height;
      }

      virtual float& DuckFactor()
      {
         return duckFactor;
      }
   };

   class PlayerSupervisor : public IPlayerSupervisor
   {
      Player player;
   public:
      PlayerSupervisor(Platform& _platform)
      {
      }

      ~PlayerSupervisor()
      {
      }

      virtual IPlayer* GetPlayer(int32 index)
      {
         if (index != 0)
         {
            Rococo::Throw(0, "Bad index. Only player #0 is recognized");
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
   IPlayerSupervisor* CreatePlayerSupervisor(Platform& platform)
   {
      return new PlayerSupervisor(platform);
   }
}

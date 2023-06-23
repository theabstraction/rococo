#include "hv.events.h"
#include <unordered_map>

namespace
{
   using namespace HV;
   using namespace HV::Events;
   using namespace Rococo::Events;


   class Player : public IPlayer
   {
      friend class PlayerSupervisor;

      ID_ENTITY playerId;
      float jumpSpeed = 0;
      const float height = 1.65_metres;
      float duckFactor = 1.0f;

      AutoFree<IInventoryArraySupervisor> inventory;
   public:
      Player()
      {

      }

      void Clear()
      {
      }

      IInventoryArray* GetInventory() override
      {
          return inventory;
      }

      void SetPlayerEntity(ID_ENTITY id) override
      {
         playerId = id;
      }

      ID_ENTITY GetPlayerEntity() override
      {
         return playerId;
      }

      float& JumpSpeed() override
      {
         return jumpSpeed;
      }

      float Height() const override
      {
         return height;
      }

      float& DuckFactor() override
      {
         return duckFactor;
      }
   };

   class PlayerSupervisor : public IPlayerSupervisor
   {
      Player player;

      enum { PLAYER_INVENTORY_SLOTS = 64 };
   public:
      PlayerSupervisor(Platform& _platform)
      {
          player.inventory = _platform.plumbing.utilities.CreateInventoryArray(PLAYER_INVENTORY_SLOTS);
      }

      ~PlayerSupervisor()
      {
      }

      IPlayer* GetPlayer(int32 index) override
      {
         if (index != 0)
         {
            Rococo::Throw(0, "Bad index. Only player #0 is recognized");
         }

         return &player;
      }

      void Free() override
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

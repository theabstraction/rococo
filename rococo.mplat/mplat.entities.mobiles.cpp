#include <rococo.mplat.h>
#include <unordered_map>

using namespace Rococo;
using namespace Rococo::Entities;

namespace
{
   struct Mobiles : public IMobilesSupervisor
   {
      IInstancesSupervisor& instances;

      std::unordered_map<ID_ENTITY, FPSAngles, ID_ENTITY> mapIdToAngles;

      Mobiles(IInstancesSupervisor& _instances) : instances(_instances)
      {
      }

      ~Mobiles()
      {
      }

      virtual bool TryMoveMobile(const MoveMobileArgs& tmm)
      {
         auto i = mapIdToAngles.find(tmm.entityId);
         if (i != mapIdToAngles.end())
         {
            auto& angles = i->second;
            angles.elevation.degrees += fmodf(tmm.delta.elevation, 360.0f);
            angles.heading.degrees += fmodf(tmm.delta.heading, 360.0f);
            angles.tilt.degrees += tmm.delta.tilt;

            if (angles.tilt.degrees < -90.0f) angles.tilt.degrees = -90.0f;
            if (angles.tilt.degrees > 90.0f) angles.tilt.degrees = 90.0f;

            auto* entity = instances.GetEntity(tmm.entityId);
            if (!entity)
            {
               Throw(0, "Mobile with id # %llx did not match an entity", tmm.entityId.value);
            }

            auto& modelRef = entity->Model();
            Vec3 pos = modelRef.GetPosition();

            Matrix4x4 model = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ - angles.heading.degrees });

            auto forward = model.GetForwardDirection();
            auto right = model.GetRightDirection();

            Vec3 df = forward * tmm.fowardDelta;
            Vec3 dr = right * tmm.straffeDelta;

            model.SetPosition(pos + df + dr);

            modelRef = model;
            return true;
         }

         return false;
      }
         
      virtual void Link(ID_ENTITY id)
      {
         if (!instances.GetEntity(id))
         {
            Throw(0, "No such entity");
         }

         auto i = mapIdToAngles.find(id);
         if (i == mapIdToAngles.end())
         {
            mapIdToAngles.insert(std::make_pair(id, FPSAngles{ 0,0,0 }));
         }
      }

      virtual void GetAngles(ID_ENTITY id, FPSAngles& angles)
      {
         auto i = mapIdToAngles.find(id);
         if (i == mapIdToAngles.end())
         {
            Throw(0, "No such entity");
         }

         angles = i->second;
      }

      virtual void SetAngles(ID_ENTITY id, const FPSAngles& angles)
      {
         auto i = mapIdToAngles.find(id);
         if (i == mapIdToAngles.end())
         {
            Throw(0, "No such entity");
         }

         i->second = angles;
      }

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Entities
   {
      IMobilesSupervisor* CreateMobilesSupervisor(IInstancesSupervisor& instances)
      {
         return new Mobiles(instances);
      }
   }
}
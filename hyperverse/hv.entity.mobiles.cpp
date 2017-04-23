#include "hv.events.h"
#include <unordered_map>

using namespace HV;
using namespace HV::Entities;
using namespace HV::Events::Entities;
using namespace Rococo;
using namespace Rococo::Events;

namespace
{
   struct Mobiles : public IMobilesSupervisor, public IObserver
   {
      IInstancesSupervisor& instances;
      IPublisher& publisher;

      std::unordered_map<ID_ENTITY, FPSAngles, ID_ENTITY> mapIdToAngles;

      Mobiles(IInstancesSupervisor& _instances, IPublisher& _publisher) : instances(_instances), publisher(_publisher)
      {
         publisher.Attach(this);
      }

      ~Mobiles()
      {
         publisher.Detach(this);
      }

      virtual void OnEvent(Event& ev)
      {
         if (ev == OnTryMoveMobile)
         {
            auto& tmm = As<OnTryMoveMobileEvent>(ev);
            auto i = mapIdToAngles.find(tmm.entityId);
            if (i != mapIdToAngles.end())
            {
               auto& angles = i->second;
               angles.elevation.quantity += fmodf(tmm.delta.elevation, 360.0f);
               angles.heading.quantity += fmodf(tmm.delta.heading, 360.0f);
               angles.tilt.quantity += tmm.delta.tilt;

               if (angles.tilt.quantity < -90.0f) angles.tilt.quantity = -90.0f;
               if (angles.tilt.quantity > 90.0f) angles.tilt.quantity = 90.0f;

               auto* entity = instances.GetEntity(tmm.entityId);

               auto& modelRef = entity->Model();
               Vec3 pos = modelRef.GetPosition();

               Matrix4x4 model = Matrix4x4::RotateRHAnticlockwiseZ(Degrees{ - angles.heading.quantity });

               auto forward = model.GetForwardDirection();
               auto right = model.GetRightDirection();

               Vec3 df = forward * tmm.fowardDelta;
               Vec3 dr = right * tmm.straffeDelta;

               model.SetPosition(pos + df + dr);

               modelRef = model;
            }
         }
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

namespace HV
{
   IMobilesSupervisor* CreateMobilesSupervisor(IInstancesSupervisor& instances, IPublisher& publisher)
   {
      return new Mobiles(instances, publisher);
   }
}